#include "db_proxy.h"
#include "proto_src\select_command.pb.h"
#include "proto_src\delete_command.pb.h"
#include "proto_src\db_option.pb.h"
#include "proto_src\response.pb.h"
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
			proto::db::result_set command5;
		}
	};

	static SOnce s_Once;

	static google::protobuf::Message* createMessage(const std::string& szMessageName)
	{
		const google::protobuf::Descriptor* pDescriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(szMessageName);
		if (pDescriptor == nullptr)
			return nullptr;

		const google::protobuf::Message* pProtoType = google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
		if (pProtoType == nullptr)
			return nullptr;

		return pProtoType->New();
	}

	static bool getPrimaryValue(const google::protobuf::Message* pMessage, uint64_t& nValue)
	{
		if (pMessage == nullptr)
			return false;

		const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
		if (pReflection == nullptr)
			return false;

		const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
		if (pDescriptor == nullptr)
			return false;

		std::string szPrimaryFieldName = pDescriptor->options().GetExtension(primary_key);
		if (szPrimaryFieldName.empty())
			return false;
		
		const google::protobuf::FieldDescriptor* pFieldDescriptor = pMessage->GetDescriptor()->FindFieldByName(szPrimaryFieldName);
		if (pFieldDescriptor == nullptr)
			return false;
		
		switch (pFieldDescriptor->type())
		{
		case google::protobuf::FieldDescriptor::TYPE_INT32:
			nValue = pReflection->GetInt32(*pMessage, pFieldDescriptor);
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT32:
			nValue = pReflection->GetUInt32(*pMessage, pFieldDescriptor);
			break;

		case google::protobuf::FieldDescriptor::TYPE_INT64:
			nValue = pReflection->GetInt64(*pMessage, pFieldDescriptor);
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT64:
			nValue = pReflection->GetUInt64(*pMessage, pFieldDescriptor);
			break;

		default:
			return false;
		}
		
		return true;
	}

	CDbProxy::CDbProxy()
		: m_nNextSessionID(1)
	{
	}

	CDbProxy::~CDbProxy()
	{
	}

	bool CDbProxy::select(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		proto::db::select_command command;
		command.set_id(nID);
		command.set_table_name(szTableName);

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Select);
		request.mutable_content()->PackFrom(command);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
		
		return this->sendRequest(&request);
	}

	bool CDbProxy::update(const google::protobuf::Message* pMessage)
	{
		uint64_t nID = 0;
		if (!getPrimaryValue(pMessage, nID))
			return false;

		proto::db::request request;
		request.set_session_id(0);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Update);
		request.mutable_content()->PackFrom(*pMessage);

		return this->sendRequest(&request);
	}

	bool CDbProxy::update_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		uint64_t nID = 0;
		if (!getPrimaryValue(pMessage, nID))
			return false;

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Update);
		request.mutable_content()->PackFrom(*pMessage);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	bool CDbProxy::remove(uint64_t nID, const std::string& szTableName)
	{
		proto::db::delete_command command;
		command.set_id(nID);
		command.set_table_name(szTableName);

		proto::db::request request;
		request.set_session_id(0);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Delete);
		request.mutable_content()->PackFrom(command);

		return this->sendRequest(&request);
	}

	bool CDbProxy::remove_r(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		proto::db::delete_command command;
		command.set_id(nID);
		command.set_table_name(szTableName);

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Delete);
		request.mutable_content()->PackFrom(command);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	bool CDbProxy::insert(const google::protobuf::Message* pMessage)
	{
		uint64_t nID = 0;
		if (!getPrimaryValue(pMessage, nID))
			return false;

		proto::db::request request;
		request.set_session_id(0);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Insert);
		request.mutable_content()->PackFrom(*pMessage);

		return this->sendRequest(&request);
	}

	bool CDbProxy::insert_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		uint64_t nID = 0;
		if (!getPrimaryValue(pMessage, nID))
			return false;

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id((uint32_t)nID);
		request.set_type(kOT_Insert);
		request.mutable_content()->PackFrom(*pMessage);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	bool CDbProxy::query(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CVariant>& vecArg, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		proto::db::query_command command;
		command.set_table_name(szTableName);
		command.set_where_clause(szWhereClause);
		for (size_t i = 0; i < vecArg.size(); ++i)
		{
			std::string szArg = vecArg[i].toString();
			command.add_args(szArg);
		}

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id(nAssociateID);
		request.set_type(kOT_Query);
		request.mutable_content()->PackFrom(command);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	bool CDbProxy::call(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CVariant>& vecArg)
	{
		proto::db::call_command command;
		command.set_sql(szSQL);
		for (size_t i = 0; i < vecArg.size(); ++i)
		{
			std::string szArg = vecArg[i].toString();
			command.add_args(szArg);
		}

		proto::db::request request;
		request.set_session_id(0);
		request.set_associate_id(nAssociateID);
		request.set_type(kOT_Call);
		request.mutable_content()->PackFrom(command);

		return this->sendRequest(&request);
	}

	bool CDbProxy::call_r(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szSQL, const std::vector<CVariant>& vecArg, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		proto::db::call_command command;
		command.set_sql(szSQL);
		for (size_t i = 0; i < vecArg.size(); ++i)
		{
			std::string szArg = vecArg[i].toString();
			command.add_args(szArg);
		}

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id(nAssociateID);
		request.set_type(kOT_Call);
		request.mutable_content()->PackFrom(command);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	bool CDbProxy::nop(CDbClient* pDbClient, uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback)
	{
		if (pDbClient == nullptr)
			return false;

		proto::db::nop_command command;

		uint64_t nSessionID = this->genSessionID();
		proto::db::request request;
		request.set_session_id(nSessionID);
		request.set_associate_id(nAssociateID);
		request.set_type(kOT_Nop);
		request.mutable_content()->PackFrom(command);

		SPendingResponseInfo sPendingResponseInfo;
		sPendingResponseInfo.nSessionID = nSessionID;
		sPendingResponseInfo.pDbClient = pDbClient;
		sPendingResponseInfo.callback = callback;
		sPendingResponseInfo.nContext = nContext;

		this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;

		return this->sendRequest(&request);
	}

	void CDbProxy::onMessage(const google::protobuf::Message* pMessage)
	{
		if (nullptr == pMessage)
			return;

		const proto::db::response* pResponse = dynamic_cast<const proto::db::response*>(pMessage);
		if (nullptr == pResponse)
			return;

		auto iter = this->m_mapPendingResponseInfo.find(pResponse->session_id());
		if (iter == this->m_mapPendingResponseInfo.end())
			return;

		SPendingResponseInfo& sPendingResponseInfo = iter->second;

		google::protobuf::Message* pContent = createMessage(pResponse->name());
		if (pContent == nullptr)
		{
			sPendingResponseInfo.callback(kRC_PROTO_ERROR, nullptr, sPendingResponseInfo.nContext);
			this->m_mapPendingResponseInfo.erase(iter);
			return;
		}

		if (!pContent->ParseFromString(pResponse->content()))
		{
			delete pContent;
			sPendingResponseInfo.callback(kRC_PROTO_ERROR, nullptr, sPendingResponseInfo.nContext);
			this->m_mapPendingResponseInfo.erase(iter);
			return;
		}

		sPendingResponseInfo.callback(pResponse->err_code(), pContent, sPendingResponseInfo.nContext);
		this->m_mapPendingResponseInfo.erase(iter);
		delete pContent;
	}

	void CDbProxy::removePendingResponseInfo(CDbClient* pDbClient)
	{
		if (nullptr == pDbClient)
			return;

		for (auto iter = this->m_mapPendingResponseInfo.begin(); iter != m_mapPendingResponseInfo.end();)
		{
			const SPendingResponseInfo& session = iter->second;
			if (session.pDbClient == pDbClient)
				iter = this->m_mapPendingResponseInfo.erase(iter);
			else
				++iter;
		}
	}

	uint64_t CDbProxy::genSessionID()
	{
		return this->m_nNextSessionID++;
	}
}