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

namespace db
{
	struct SOnce
	{
		SOnce()
		{
			proto::db::select_command command1;
			proto::db::delete_command command2;
			proto::db::query_command command3;
			proto::db::call_command	command4;
			proto::db::nop_command command5;
			proto::db::result_set command6;
		}
	};

	static SOnce s_Once;

	CDbThreadMgr* create(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, const std::string& szProtoDir, uint32_t nDbThreadCount)
	{
		std::vector<std::string> vecProto;

#ifdef _WIN32
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = ::FindFirstFileA((szProtoDir + "/*.proto").c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return false;

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
			return false;

		while ((pFile = readdir(pDir)) != nullptr)
		{
			if (strncmp(pFile->d_name, ".", 1) == 0)
				continue;

			std::string szName = pFile->d_name;
			size_t pos = szName.find_last_of(".proto");
			if (pos + 1 != szName.size())
				continue;

			vecProto.push_back(szName);
		}
		closedir(pDir);
#endif
		if (!importProtobuf("proto", vecProto))
			return nullptr;

		CDbThreadMgr* pDbThreadMgr = new CDbThreadMgr();
		if (!pDbThreadMgr->init(szHost, nPort, szDb, szUser, szPassword, szCharacterset, nDbThreadCount))
		{
			delete pDbThreadMgr;
			return nullptr;
		}

		return pDbThreadMgr;
	}

	void release(CDbThreadMgr* pDbThreadMgr)
	{
		DebugAst(pDbThreadMgr != nullptr);
		pDbThreadMgr->exit();

		delete pDbThreadMgr;
	}

	void getResultInfo(CDbThreadMgr* pDbThreadMgr, std::list<SDbResultInfo>& listResultInfo)
	{
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getResultInfo(listResultInfo);
	}

	void query(CDbThreadMgr* pDbThreadMgr, uint32_t nServiceID, const proto::db::request* pRequest)
	{
		DebugAst(pDbThreadMgr != nullptr);
		DebugAst(pRequest != nullptr);

		static const char* szPrefix = "type.googleapis.com/";
		static const size_t nPrefixLen = strlen(szPrefix);

		const std::string& szURL = pRequest->content().type_url();
		size_t pos = szURL.find(szPrefix);
		DebugAst(pos != std::string::npos);
		std::string szMessageName = szURL.substr(pos + nPrefixLen);

		google::protobuf::Message* pMessage = createMessage(szMessageName);
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

	void getQPS(CDbThreadMgr* pDbThreadMgr, std::vector<uint32_t>& vecQPS)
	{
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getQPS(vecQPS);
	}

	void getQueueSize(CDbThreadMgr* pDbThreadMgr, std::vector<uint32_t>& vecSize)
	{
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getQueueSize(vecSize);
	}
}