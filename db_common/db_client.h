#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "google/protobuf/message.h"

#include "db_proxy.h"
#include "db_variant.h"

namespace db
{
	class CDbClient
	{
	public:
		CDbClient(CDbProxy* pDbProxy);
		virtual ~CDbClient();

		bool select(uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback);
		bool update(const google::protobuf::Message* pMessage);
		bool update_r(const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback);
		bool remove(uint64_t nID, const std::string& szTableName);
		bool remove_r(uint64_t nID, const std::string& szTableName, uint64_t nContext, const DbCallback& callback);
		bool insert(const google::protobuf::Message* pMessage);
		bool insert_r(const google::protobuf::Message* pMessage, uint64_t nContext, const DbCallback& callback);

		bool query(uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CDbVariant>& vecArg, uint64_t nContext, const DbCallback& callback);
		bool call(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg);
		bool call_r(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg, uint64_t nContext, const DbCallback& callback);

		bool nop(uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback);

	private:
		CDbProxy* m_pDbProxy;
	};
}