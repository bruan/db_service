#include "db_service.h"
#include "db_protobuf.h"
#include "db_thread_mgr.h"

#include <vector>
#include <string>
#ifdef _WIN32
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#endif
#include "db_common.h"

#include "proto_src/select_command.pb.h"
#include "proto_src/delete_command.pb.h"
#include "proto_src/query_command.pb.h"
#include "proto_src/call_command.pb.h"
#include "proto_src/result_set.pb.h"
#include "proto_src/nop_command.pb.h"

using namespace std;
using namespace google::protobuf;
using namespace proto::db;

struct SOnce
{
	SOnce()
	{
		select_command	command1;
		delete_command	command2;
		query_command	command3;
		call_command	command4;
		nop_command		command5;
		result_set		command6;
	}
};

static SOnce s_Once;
static map<uint32_t, db::CDbThreadMgr*>	s_mapDbThreadMgr;

namespace db
{

	uint32_t create(const string& szHost, uint16_t nPort, const string& szDb, const string& szUser, const string& szPassword, const string& szCharacterset, const string& szProtoDir, uint32_t nDbThreadCount, uint64_t nMaxCacheSize)
	{
		vector<string> vecProto;

#ifdef _WIN32
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = ::FindFirstFileA((szProtoDir + "/*.proto").c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return 0;

		do
		{
			vecProto.push_back(FindFileData.cFileName);
		} while (::FindNextFileA(hFind, &FindFileData) != 0);
		::FindClose(hFind);
#else
		DIR* pDir = nullptr;
		struct dirent* pFile = nullptr;
		struct stat sb;

		if ((pDir = opendir(szProtoDir.c_str())) == nullptr)
			return 0;

		while ((pFile = readdir(pDir)) != nullptr)
		{
			if (strncmp(pFile->d_name, ".", 1) == 0)
				continue;

			string szName = pFile->d_name;
			size_t pos = szName.find_last_of(".proto");
			if (pos + 1 != szName.size())
				continue;

			vecProto.push_back(szName);
		}
		closedir(pDir);
#endif
		if (!importProtobuf("proto", vecProto))
			return 0;

		CDbThreadMgr* pDbThreadMgr = new CDbThreadMgr();
		if (!pDbThreadMgr->init(szHost, nPort, szDb, szUser, szPassword, szCharacterset, nDbThreadCount, nMaxCacheSize))
		{
			delete pDbThreadMgr;
			return 0;
		}

		static uint32_t nID = 1;
		s_mapDbThreadMgr[nID] = pDbThreadMgr;

		return nID++;
	}

	void release(uint32_t nID)
	{
		auto iter = s_mapDbThreadMgr.find(nID);
		DebugAst(iter != s_mapDbThreadMgr.end());
		CDbThreadMgr* pDbThreadMgr = iter->second;
		s_mapDbThreadMgr.erase(iter);
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->exit();

		delete pDbThreadMgr;
	}

	void getResultInfo(uint32_t nID, list<SDbResultInfo>& listResultInfo)
	{
		auto iter = s_mapDbThreadMgr.find(nID);
		DebugAst(iter != s_mapDbThreadMgr.end());
		CDbThreadMgr* pDbThreadMgr = iter->second;
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getResultInfo(listResultInfo);
	}

	void query(uint32_t nID, uint32_t nServiceID, const request* pRequest)
	{
		DebugAst(pRequest != nullptr);

		auto iter = s_mapDbThreadMgr.find(nID);
		DebugAst(iter != s_mapDbThreadMgr.end());
		CDbThreadMgr* pDbThreadMgr = iter->second;
		DebugAst(pDbThreadMgr != nullptr);

		static const char* szPrefix = "type.googleapis.com/";
		static const size_t nPrefixLen = strlen(szPrefix);

		const string& szURL = pRequest->content().type_url();
		size_t pos = szURL.find(szPrefix);
		DebugAst(pos != string::npos);
		string szMessageName = szURL.substr(pos + nPrefixLen);

		Message* pMessage = createMessage(szMessageName);
		if (pMessage == nullptr)
		{
			PrintWarning("create message error %s", szMessageName.c_str());
			return;
		}
		if (!pRequest->content().UnpackTo(pMessage))
		{
			PrintWarning("unpack message error %s", szMessageName.c_str());
			delete pMessage;
			return;
		}

		SDbCommand sDbCommand;
		sDbCommand.nServiceID = nServiceID;
		sDbCommand.nType = pRequest->type();
		sDbCommand.nSessionID = pRequest->session_id();
		sDbCommand.pMessage = pMessage;

		pDbThreadMgr->query(pRequest->associate_id(), sDbCommand);
	}

	void getQPS(uint32_t nID, vector<uint32_t>& vecQPS)
	{
		auto iter = s_mapDbThreadMgr.find(nID);
		DebugAst(iter != s_mapDbThreadMgr.end());
		CDbThreadMgr* pDbThreadMgr = iter->second;
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getQPS(vecQPS);
	}

	void getQueueSize(uint32_t nID, vector<uint32_t>& vecSize)
	{
		auto iter = s_mapDbThreadMgr.find(nID);
		DebugAst(iter != s_mapDbThreadMgr.end());
		CDbThreadMgr* pDbThreadMgr = iter->second;
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getQueueSize(vecSize);
	}
}