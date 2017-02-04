#include "db_thread.h"
#include "db_thread_mgr.h"
#include "db_command_handler.h"
#include "db_common.h"

namespace db
{

	CDbThread::CDbThread()
		: m_pDbThreadMgr(nullptr)
		, m_pThread(nullptr)
		, m_quit(0)
		, m_nQPS(0)
	{
	}

	CDbThread::~CDbThread()
	{
		delete this->m_pThread;
	}

	bool CDbThread::connectDb(bool bInit)
	{
		do
		{
			if( !this->m_dbConnection.connect( 
				this->m_pDbThreadMgr->getDbConnectionInfo().szHost,
				this->m_pDbThreadMgr->getDbConnectionInfo().nPort,
				this->m_pDbThreadMgr->getDbConnectionInfo().szUser,
				this->m_pDbThreadMgr->getDbConnectionInfo().szPassword,
				this->m_pDbThreadMgr->getDbConnectionInfo().szDb,
				this->m_pDbThreadMgr->getDbConnectionInfo().szCharacterset))
			{
				if( bInit )
					return false;

				//Sleep(1000);
				continue;
			}
			break;
		} while (1);

		this->m_dbCommandHandlerProxy.onConnect(&this->m_dbConnection);
		return true;
	}

	void CDbThread::join()
	{
		this->m_quit.store(1, std::memory_order_release);
		this->m_pThread->join();
	}

	bool CDbThread::init(CDbThreadMgr* pDbThreadMgr, uint64_t nMaxCacheSize)
	{
		DebugAstEx(pDbThreadMgr != nullptr, false);
		this->m_pDbThreadMgr = pDbThreadMgr;
		if (!this->m_dbCommandHandlerProxy.init(nMaxCacheSize))
			return false;

		if (!this->connectDb(true))
			return false;

		this->m_pThread = new std::thread([this]()
		{
			while (true)
			{
				this->onProcess();
				if (this->m_quit.load(std::memory_order_acquire))
				{
					std::unique_lock<std::mutex> lock(this->m_tCommandLock);
					if (this->m_listCommand.empty())
						break;
				}
			}

			this->m_dbCommandHandlerProxy.flushAllCache();

			this->m_dbConnection.close();
			this->m_dbCommandHandlerProxy.onDisconnect();
		});

		return true;
	}

	bool CDbThread::onProcess()
	{
		if (!this->m_dbConnection.isConnect())
		{
			this->m_dbConnection.close();
			this->m_dbCommandHandlerProxy.onDisconnect();
			this->connectDb(false);
		}

		std::list<SDbCommand> listCommand;
		{
			std::unique_lock<std::mutex> lock(this->m_tCommandLock);
			while (this->m_listCommand.empty())
			{
				this->m_condition.wait(lock);
			}
			listCommand.splice(listCommand.end(), this->m_listCommand);
		}

		for (auto iter = listCommand.begin(); iter != listCommand.end(); ++iter)
		{
			SDbCommand sDbCommand = *iter;

			std::shared_ptr<google::protobuf::Message> pMessage;
			uint32_t nErrorCode = this->m_dbCommandHandlerProxy.onDbCommand(sDbCommand.nType, sDbCommand.pMessage, pMessage);
			if (nErrorCode == kRC_LOST_CONNECTION)
			{
				std::unique_lock<std::mutex> lock(this->m_tCommandLock);
				this->m_listCommand.splice(this->m_listCommand.begin(), listCommand, iter, listCommand.end());
				break;
			}
			if (sDbCommand.nSessionID == 0)
				continue;

			SDbResultInfo sDbResultInfo;
			sDbResultInfo.pResponse = std::make_shared<proto::db::response>();
			sDbResultInfo.pResponse->set_session_id(sDbCommand.nSessionID);
			sDbResultInfo.pResponse->set_err_code(nErrorCode);
			if (pMessage != nullptr)
			{
				std::string* szContent = new std::string;
				pMessage->SerializeToString(szContent);
				sDbResultInfo.pResponse->set_name(pMessage->GetTypeName());
				sDbResultInfo.pResponse->set_allocated_content(szContent);
			}

			this->m_pDbThreadMgr->addResultInfo(sDbResultInfo);
		}

		return true;
	}

	void CDbThread::query(const SDbCommand& sDbCommand)
	{
		std::unique_lock<std::mutex> lock(this->m_tCommandLock);
		this->m_listCommand.push_back(sDbCommand);

		this->m_condition.notify_all();
	}

	uint32_t CDbThread::getQueueSize()
	{
		std::unique_lock<std::mutex> lock(this->m_tCommandLock);
		return (uint32_t)this->m_listCommand.size();
	}

	uint32_t CDbThread::getQPS()
	{
		return this->m_nQPS;
	}
}