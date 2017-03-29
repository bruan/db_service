#include "db_proxy.h"
#include "proto_src/select_command.pb.h"
#include "proto_src/delete_command.pb.h"
#include "proto_src/db_option.pb.h"
#include "proto_src/response.pb.h"
#include "proto_src/query_command.pb.h"
#include "proto_src/call_command.pb.h"
#include "proto_src/result_set.pb.h"
#include "proto_src/nop_command.pb.h"
#include "proto_src/flush_command.pb.h"

#include <time.h>

using namespace std;
using namespace google::protobuf;
using namespace db;
using namespace proto::db;

struct SOnce
{
	SOnce()
	{
		select_command	command1;
		delete_command	command2;
		query_command	command3;
		call_command	command4;
		flush_command	command5;
		result_set		command6;
	}
};

static SOnce s_Once;

static Message* createMessage(const string& szMessageName)
{
	const Descriptor* pDescriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(szMessageName);
	if (pDescriptor == nullptr)
		return nullptr;

	const Message* pProtoType = MessageFactory::generated_factory()->GetPrototype(pDescriptor);
	if (pProtoType == nullptr)
		return nullptr;

	return pProtoType->New();
}

static bool getPrimaryValue(const Message* pMessage, uint64_t& nValue)
{
	if (pMessage == nullptr)
		return false;

	const Reflection* pReflection = pMessage->GetReflection();
	if (pReflection == nullptr)
		return false;

	const Descriptor* pDescriptor = pMessage->GetDescriptor();
	if (pDescriptor == nullptr)
		return false;

	string szPrimaryFieldName = pDescriptor->options().GetExtension(primary_key);
	if (szPrimaryFieldName.empty())
		return false;

	const FieldDescriptor* pFieldDescriptor = pMessage->GetDescriptor()->FindFieldByName(szPrimaryFieldName);
	if (pFieldDescriptor == nullptr)
		return false;

	switch (pFieldDescriptor->type())
	{
	case FieldDescriptor::TYPE_INT32:
	case FieldDescriptor::TYPE_SINT32:
	case FieldDescriptor::TYPE_SFIXED32:
		nValue = pReflection->GetInt32(*pMessage, pFieldDescriptor);
		break;

	case FieldDescriptor::TYPE_UINT32:
	case FieldDescriptor::TYPE_FIXED32:
		nValue = pReflection->GetUInt32(*pMessage, pFieldDescriptor);
		break;

	case FieldDescriptor::TYPE_INT64:
	case FieldDescriptor::TYPE_SINT64:
	case FieldDescriptor::TYPE_SFIXED64:
		nValue = pReflection->GetInt64(*pMessage, pFieldDescriptor);
		break;

	case FieldDescriptor::TYPE_UINT64:
	case FieldDescriptor::TYPE_FIXED64:
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

bool CDbProxy::select(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	select_command command;
	command.set_id(nID);
	command.set_table_name(szTableName);

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id((uint32_t)nID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_SELECT);
	request.mutable_content()->PackFrom(command);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;
	
	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::update(const Message* pMessage)
{
	uint64_t nID = 0;
	if (!getPrimaryValue(pMessage, nID))
		return false;

	request request;
	request.set_session_id(0);
	request.set_associate_id((uint32_t)nID);
	request.set_type(kOT_UPDATE);
	request.mutable_content()->PackFrom(*pMessage);

	return this->sendRequest(&request);
}

bool CDbProxy::update_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	uint64_t nID = 0;
	if (!getPrimaryValue(pMessage, nID))
		return false;

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id((uint32_t)nID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_UPDATE);
	request.mutable_content()->PackFrom(*pMessage);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;

	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::remove(uint64_t nID, const string& szTableName)
{
	delete_command command;
	command.set_id(nID);
	command.set_table_name(szTableName);

	request request;
	request.set_session_id(0);
	request.set_associate_id((uint32_t)nID);
	request.set_type(kOT_DELETE);
	request.mutable_content()->PackFrom(command);

	return this->sendRequest(&request);
}

bool CDbProxy::remove_r(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	delete_command command;
	command.set_id(nID);
	command.set_table_name(szTableName);

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id((uint32_t)nID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_DELETE);
	request.mutable_content()->PackFrom(command);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;

	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::insert(const Message* pMessage)
{
	uint64_t nID = 0;
	if (!getPrimaryValue(pMessage, nID))
		return false;

	request request;
	request.set_session_id(0);
	request.set_associate_id((uint32_t)nID);
	request.set_type(kOT_INSERT);
	request.mutable_content()->PackFrom(*pMessage);

	return this->sendRequest(&request);
}

bool CDbProxy::insert_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	uint64_t nID = 0;
	if (!getPrimaryValue(pMessage, nID))
		return false;

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id((uint32_t)nID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_INSERT);
	request.mutable_content()->PackFrom(*pMessage);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;

	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::query(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	query_command command;
	command.set_table_name(szTableName);
	command.set_where_clause(szWhereClause);
	for (size_t i = 0; i < vecArg.size(); ++i)
	{
		string szArg = vecArg[i].toString();
		command.add_args(szArg);
	}

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id(nAssociateID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_QUERY);
	request.mutable_content()->PackFrom(command);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;

	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::call(uint32_t nAssociateID, const string& szSQL, const vector<CDbVariant>& vecArg)
{
	call_command command;
	command.set_sql(szSQL);
	for (size_t i = 0; i < vecArg.size(); ++i)
	{
		string szArg = vecArg[i].toString();
		command.add_args(szArg);
	}

	request request;
	request.set_session_id(0);
	request.set_associate_id(nAssociateID);
	request.set_type(kOT_CALL);
	request.mutable_content()->PackFrom(command);

	return this->sendRequest(&request);
}

bool CDbProxy::call_r(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	call_command command;
	command.set_sql(szSQL);
	for (size_t i = 0; i < vecArg.size(); ++i)
	{
		string szArg = vecArg[i].toString();
		command.add_args(szArg);
	}

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id(nAssociateID);
	request.set_timeout(nTimeout);
	request.set_type(kOT_CALL);
	request.mutable_content()->PackFrom(command);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;

	STimeoutInfo sTimeoutInfo;
	sTimeoutInfo.nSessionID = nSessionID;
	sTimeoutInfo.nTimeout = (int64_t)time(nullptr) + nTimeout;
	auto iter = lower_bound(this->m_listTimeout.begin(), this->m_listTimeout.end(), sTimeoutInfo, STimeoutInfoComp());
	iter = this->m_listTimeout.insert(iter, sTimeoutInfo);
	if (iter == this->m_listTimeout.end())
		return false;

	sPendingResponseInfo.iterTimeout = iter;
	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::nop(CDbClient* pDbClient, uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback)
{
	if (pDbClient == nullptr)
		return false;

	nop_command command;

	uint64_t nSessionID = this->genSessionID();
	request request;
	request.set_session_id(nSessionID);
	request.set_associate_id(nAssociateID);
	request.set_type(kOT_NOP);
	request.mutable_content()->PackFrom(command);

	SPendingResponseInfo sPendingResponseInfo;
	sPendingResponseInfo.nSessionID = nSessionID;
	sPendingResponseInfo.pDbClient = pDbClient;
	sPendingResponseInfo.callback = callback;
	sPendingResponseInfo.nContext = nContext;
	sPendingResponseInfo.iterTimeout = this->m_listTimeout.end();

	this->m_mapPendingResponseInfo[nSessionID] = sPendingResponseInfo;
	this->m_mapPendingResponseInfoByDbClient[pDbClient].insert(nSessionID);

	return this->sendRequest(&request);
}

bool CDbProxy::flush(CDbClient* pDbClient, uint64_t nID, EFlushCacheType eType)
{
	if (pDbClient == nullptr)
		return false;

	flush_command command;
	command.set_id(nID);
	command.set_type(eType);

	request request;
	request.set_session_id(0);
	request.set_associate_id((uint32_t)nID);
	request.set_type(kOT_FLUSH);
	request.mutable_content()->PackFrom(command);

	return this->sendRequest(&request);
}

void CDbProxy::onMessage(const Message* pMessage)
{
	if (nullptr == pMessage)
		return;

	const response* pResponse = dynamic_cast<const response*>(pMessage);
	if (nullptr == pResponse)
		return;

	auto iter = this->m_mapPendingResponseInfo.find(pResponse->session_id());
	if (iter == this->m_mapPendingResponseInfo.end())
		return;

	SPendingResponseInfo sPendingResponseInfo = iter->second;
	this->m_mapPendingResponseInfo.erase(iter);
	auto iterDbClient = this->m_mapPendingResponseInfoByDbClient.find(sPendingResponseInfo.pDbClient);
	if (iterDbClient != this->m_mapPendingResponseInfoByDbClient.end())
		iterDbClient->second.erase(pResponse->session_id());
	if (sPendingResponseInfo.iterTimeout != this->m_listTimeout.end())
		this->m_listTimeout.erase(sPendingResponseInfo.iterTimeout);

	unique_ptr<Message> pContent = unique_ptr<Message>(createMessage(pResponse->name()));
	if (pContent == nullptr)
	{
		sPendingResponseInfo.callback(kRC_PROTO_ERROR, nullptr, sPendingResponseInfo.nContext);
		return;
	}

	if (!pContent->ParseFromString(pResponse->content()))
	{
		sPendingResponseInfo.callback(kRC_PROTO_ERROR, nullptr, sPendingResponseInfo.nContext);
		return;
	}

	sPendingResponseInfo.callback(pResponse->err_code(), pContent.get(), sPendingResponseInfo.nContext);
}

void CDbProxy::removePendingResponseInfo(CDbClient* pDbClient)
{
	if (nullptr == pDbClient)
		return;

	auto iter = this->m_mapPendingResponseInfoByDbClient.find(pDbClient);
	if (iter == this->m_mapPendingResponseInfoByDbClient.end())
		return;

	std::set<uint64_t>& setSessionID = iter->second;
	for (auto iterSessionID = setSessionID.begin(); iterSessionID != setSessionID.end(); ++iterSessionID)
	{
		this->m_mapPendingResponseInfo.erase(*iterSessionID);
	}

	this->m_mapPendingResponseInfoByDbClient.erase(iter);
}

uint64_t CDbProxy::genSessionID()
{
	return this->m_nNextSessionID++;
}

void db::CDbProxy::onTimer()
{
	int64_t nCurTime = time(nullptr);
	for (auto iter = this->m_listTimeout.begin(); iter != this->m_listTimeout.end();)
	{
		if (nCurTime < iter->nTimeout)
			break;

		uint64_t nSessionID = iter->nSessionID;
		iter = this->m_listTimeout.erase(iter);
		
		auto iterResponseInfo = this->m_mapPendingResponseInfo.find(nSessionID);
		if (iterResponseInfo == this->m_mapPendingResponseInfo.end())
			continue;

		SPendingResponseInfo sPendingResponseInfo = iterResponseInfo->second;
		this->m_mapPendingResponseInfo.erase(iterResponseInfo);
		auto iterDbClient = this->m_mapPendingResponseInfoByDbClient.find(sPendingResponseInfo.pDbClient);
		if (iterDbClient != this->m_mapPendingResponseInfoByDbClient.end())
			iterDbClient->second.erase(nSessionID);

		sPendingResponseInfo.callback(kRC_TIME_OUT, nullptr, sPendingResponseInfo.nContext);
	}
}
