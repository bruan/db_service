#pragma once

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <list>
#include <functional>

#include "db_common.h"
#include "db_variant.h"
#include "google/protobuf/message.h"
#include "proto_src/request.pb.h"


namespace db
{
	typedef std::function<void(uint32_t, const google::protobuf::Message*, uint64_t)> DbCallback;

	class CDbClient;
	class CDbProxy
	{
		friend class CDbClient;

	public:
		CDbProxy();
		virtual ~CDbProxy();

		bool			select(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			update(const google::protobuf::Message* pMessage);
		bool			update_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			remove(uint64_t nID, const std::string& szTableName);
		bool			remove_r(CDbClient* pDbClient, uint64_t nID, const std::string& szTableName, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			insert(const google::protobuf::Message* pMessage);
		bool			insert_r(CDbClient* pDbClient, const google::protobuf::Message* pMessage, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			query(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szTableName, const std::string& szWhereClause, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			call(uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg);
		bool			call_r(CDbClient* pDbClient, uint32_t nAssociateID, const std::string& szSQL, const std::vector<CDbVariant>& vecArg, uint32_t nTimeout, uint64_t nContext, const DbCallback& callback);
		bool			nop(CDbClient* pDbClient, uint32_t nAssociateID, uint64_t nContext, const DbCallback& callback);
		bool			flush(CDbClient* pDbClient, uint64_t nID, EFlushCacheType eType);

		void			onMessage(const google::protobuf::Message* pMessage);

		void			onTimer();

	protected:
		virtual bool	sendRequest(const proto::db::request* pRequest) = 0;

	private:
		void			removePendingResponseInfo(CDbClient* pDbClient);

		uint64_t		genSessionID();

	private:
		struct STimeoutInfo
		{
			uint64_t	nSessionID;
			int64_t		nTimeout;
		};

		struct STimeoutInfoComp
		{
			bool operator()(const STimeoutInfo& lhs, const STimeoutInfo& rhs)
			{
				return lhs.nTimeout < rhs.nTimeout;
			}
		};

		struct SPendingResponseInfo
		{
			uint64_t	nSessionID;
			uint64_t	nContext;
			DbCallback	callback;
			CDbClient*	pDbClient;
			std::list<STimeoutInfo>::iterator
						iterTimeout;
		};

		uint64_t									m_nNextSessionID;
		std::map<uint64_t, SPendingResponseInfo>	m_mapPendingResponseInfo;
		std::map<CDbClient*, std::set<uint64_t>>	m_mapPendingResponseInfoByDbClient;
		std::list<STimeoutInfo>						m_listTimeout;
	};
}