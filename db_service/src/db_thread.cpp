#include "db_thread.h"
#include "db_thread_mgr.h"
#include "db_command_handler.h"
#include "db_common.h"

namespace db
{

	CDbThread::CDbThread(CDbThreadMgr* pDbThreadMgr)
		: m_pDbThreadMgr(pDbThreadMgr)
		, m_pThread(nullptr)
		, m_quit(0)
	{
	}

	CDbThread::~CDbThread()
	{
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

		this->m_pDbCommandHandlerProxy->onConnect(&this->m_dbConnection);
		return true;
	}

	void CDbThread::join()
	{
		this->m_quit.store(1, std::memory_order_release);
		this->m_pThread->join();
	}

	bool CDbThread::init()
	{
		this->m_pDbCommandHandlerProxy = new CDbCommandHandlerProxy();
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

			this->m_dbConnection.close();
			this->m_pDbCommandHandlerProxy->onDisconnect();
			delete this->m_pDbCommandHandlerProxy;

			while (!this->m_listCommand.empty())
			{
				SDbCommand sDbCommand = this->m_listCommand.back();
				delete sDbCommand.pMessage;
				this->m_listCommand.pop_back();
			};
		});

		return true;
	}

	bool CDbThread::onProcess()
	{
		if (!this->m_dbConnection.isConnect())
		{
			this->m_dbConnection.close();
			this->m_pDbCommandHandlerProxy->onDisconnect();
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
			uint32_t nErrorCode = this->m_pDbCommandHandlerProxy->onDbCommand(sDbCommand.nType, sDbCommand.pMessage, pMessage);
			if (nErrorCode == kRC_LOST_CONNECTION)
			{
				std::unique_lock<std::mutex> lock(this->m_tCommandLock);
				this->m_listCommand.splice(this->m_listCommand.begin(), listCommand, iter, listCommand.end());
				break;
			}
			delete sDbCommand.pMessage;
			if (sDbCommand.nSessionID == 0)
				continue;

			proto::db::response* pResponse = new proto::db::response();
			pResponse->set_session_id(sDbCommand.nSessionID);
			pResponse->set_err_code(nErrorCode);
			if (pMessage != nullptr)
			{
				std::string* szContent = new std::string;
				pMessage->SerializeToString(szContent);
				pResponse->set_name(pMessage->GetTypeName());
				pResponse->set_allocated_content(szContent);
			}

			this->m_pDbThreadMgr->addResult(sDbCommand.nServiceID, pResponse);
		}

		return true;
	}

	void CDbThread::query(const SDbCommand& sDbCommand)
	{
		std::unique_lock<std::mutex> lock(this->m_tCommandLock);
		this->m_listCommand.push_back(sDbCommand);

		this->m_condition.notify_all();
	}
}