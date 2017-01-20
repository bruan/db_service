#include "db_service.h"
#include "db_protobuf.h"
#include "db_thread_mgr.h"

#include <vector>
#include <string>

#include "db_common.h"

#include "proto_src\select_command.pb.h"
#include "proto_src\delete_command.pb.h"
#include "proto_src\query_command.pb.h"
#include "proto_src\call_command.pb.h"
#include "proto_src\result_set.pb.h"
#include "proto_src\nop_command.pb.h"

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

	CDbThreadMgr* create(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, uint32_t nDbThreadCount)
	{	
		char szDir[MAX_PATH + 32] = { 0 };
		int32_t ret = ::GetCurrentDirectoryA(MAX_PATH, szDir);
		if (ret == 0)
			return false;

		szDir[ret] = '\0';

		strcat_s(szDir, "\\proto\\*.proto");

		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = ::FindFirstFileA(szDir, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return false;

		std::vector<std::string> vecProto;
		do
		{
			vecProto.push_back(FindFileData.cFileName);
		} while (::FindNextFileA(hFind, &FindFileData) != 0);
		::FindClose(hFind);

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
		delete pDbThreadMgr;
	}

	void getResult(CDbThreadMgr* pDbThreadMgr, std::list<std::pair<uint32_t, proto::db::response*>>& listResult)
	{
		DebugAst(pDbThreadMgr != nullptr);

		pDbThreadMgr->getResult(listResult);
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
}