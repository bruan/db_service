#pragma once
#include "db_service_base.h"
#include "db_command_handler_proxy.h"
#include "db_connection.h"
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "proto_src\response.pb.h"

namespace db
{
	class CDbThreadMgr;
	class CDbThread
	{
	public:
		CDbThread(CDbThreadMgr* pDbThreadMgr);
		~CDbThread();

		bool	init();
		void	query(const SDbCommand& sDbCommand);
		void	join();

	private:
		bool	connectDb(bool bInit);
		bool	onProcess();
		void	onDestroy();

	private:
		std::atomic<uint32_t>	m_quit;
		std::condition_variable	m_condition;
		std::mutex				m_tCommandLock;
		std::thread*			m_pThread;
		std::list<SDbCommand>	m_listCommand;
		CDbConnection			m_dbConnection;
		CDbCommandHandlerProxy*	m_pDbCommandHandlerProxy;
		CDbThreadMgr*			m_pDbThreadMgr;
	};
}