#include "db_client.h"

using namespace std;
using namespace google::protobuf;
using namespace db;

CDbClient::CDbClient(CDbProxy* pDbProxy)
	: m_pDbProxy(pDbProxy)
{
}

CDbClient::~CDbClient()
{
	this->m_pDbProxy->removePendingResponseInfo(this);
}

bool CDbClient::select(uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->select(this, nID, szTableName, nTimeout, nContext, callback);
}

bool CDbClient::update(const Message* pMessage)
{
	return this->m_pDbProxy->update(pMessage);
}

bool CDbClient::update_r(const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->update_r(this, pMessage, nTimeout, nContext, callback);
}

bool CDbClient::remove(uint64_t nID, const string& szTableName)
{
	return this->m_pDbProxy->remove(nID, szTableName);
}

bool CDbClient::remove_r(uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->remove_r(this, nID, szTableName, nTimeout, nContext, callback);
}

bool CDbClient::insert(const Message* pMessage)
{
	return this->m_pDbProxy->insert(pMessage);
}

bool CDbClient::insert_r(const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->insert_r(this, pMessage, nTimeout, nContext, callback);
}

bool CDbClient::query(uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->query(this, nAssociateID, szTableName, szWhereClause, vecArg, nTimeout, nContext, callback);
}

bool CDbClient::call(uint32_t nAssociateID, const string& szSQL, const vector<CDbVariant>& vecArg)
{
	return this->m_pDbProxy->call(nAssociateID, szSQL, vecArg);
}

bool CDbClient::call_r(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->call_r(this, nAssociateID, szSQL, vecArg, nTimeout, nContext, callback);
}

bool CDbClient::nop(uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback)
{
	return this->m_pDbProxy->nop(this, nAssociateID, nContext, callback);
}

bool CDbClient::flush(uint64_t nID, EFlushCacheType eType)
{
	return this->m_pDbProxy->flush(this, nID, eType);
}
