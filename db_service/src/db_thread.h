#pragma once
#include "db_service_base.h"
#include "db_command_handler_proxy.h"
#include "db_connection.h"
#include "db_cache_mgr.h"

#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>

#include "proto_src/response.pb.h"

namespace db
{
	class CDbThreadMgr;
	class CDbThread
	{
	public:
		CDbThread();
		~CDbThread();

		bool		init(CDbThreadMgr* pDbThreadMgr, uint64_t nMaxCacheSize, uint32_t nWritebackTime);
		void		query(const SDbCommand& sDbCommand);
		void		join();
		uint32_t	getQueueSize();
		uint32_t	getQPS();
		CDbCommandHandlerProxy&
					getDbCommandHandlerProxy();

		void		setMaxCacheSize(uint64_t nSize);

	private:
		bool		connectDb(bool bInit);
		void		onProcess();
		void		onDestroy();

		bool		onPreCache(uint32_t nType, google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse);
		void		onPostCache(uint32_t nType, google::protobuf::Message* pRequest, std::shared_ptr<google::protobuf::Message>& pResponse);
		void		flushCache(uint64_t nKey, bool bDel);

	private:
		volatile uint32_t		m_quit;
		std::condition_variable	m_condition;
		std::mutex				m_tCommandLock;
		std::thread				m_thread;
		std::list<SDbCommand>	m_listCommand;
		CDbConnection			m_dbConnection;
		CDbCommandHandlerProxy	m_dbCommandHandlerProxy;
		CDbThreadMgr*			m_pDbThreadMgr;
		CDbCacheMgr				m_dbCacheMgr;
		uint32_t				m_nQPS;
	};
}