#include "db_client.h"

namespace db
{

	CDbClient::CDbClient(CDbProxy* pDbProxy)
		: m_pDbProxy(pDbProxy)
	{
	}

	CDbClient::~CDbClient()
	{
		this->m_pDbProxy->removePendingResponseInfo(this);
	}

	bool CDbClient::select(uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->select(this, nID, szTableName, nContext, callback);
	}

	bool CDbClient::update(const google::protobuf::Message* pMessage)
	{
		return this->m_pDbProxy->update(pMessage);
	}

	bool CDbClient::update_r(const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->update_r(this, pMessage, nContext, callback);
	}

	bool CDbClient::remove(uint64_t nID, const std::string& szTableName)
	{
		return this->m_pDbProxy->remove(nID, szTableName);
	}

	bool CDbClient::remove_r(uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->remove_r(this, nID, szTableName, nContext, callback);
	}

	bool CDbClient::insert(const google::protobuf::Message* pMessage)
	{
		return this->m_pDbProxy->insert(pMessage);
	}

	bool CDbClient::insert_r(const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->insert_r(this, pMessage, nContext, callback);
	}

	bool CDbClient::query(uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CVariant>& vecArg, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->query(this, nAssociateID, szTableName, szWhereClause, vecArg, nContext, callback);
	}

	bool CDbClient::call(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CVariant>& vecArg)
	{
		return this->m_pDbProxy->call(nAssociateID, szSQL, vecArg);
	}

	bool CDbClient::call_r(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CVariant>& vecArg, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->call_r(this, nAssociateID, szSQL, vecArg, nContext, callback);
	}

	bool CDbClient::nop(uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback)
	{
		return this->m_pDbProxy->nop(this, nAssociateID, nContext, callback);
	}

}