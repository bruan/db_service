#include "db_thread.h"
#include "db_thread_mgr.h"
#include "db_command_handler.h"
#include "db_common.h"
#include "db_protobuf.h"

#include "proto_src\select_command.pb.h"
#include "proto_src\delete_command.pb.h"

using namespace std;
using namespace google::protobuf;
using namespace db;
using namespace proto::db;

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
	this->m_quit.store(1, memory_order_release);
	this->m_pThread->join();
}

bool CDbThread::init(CDbThreadMgr* pDbThreadMgr, uint64_t nMaxCacheSize, uint32_t nWritebackTime)
{
	DebugAstEx(pDbThreadMgr != nullptr, false);
	this->m_pDbThreadMgr = pDbThreadMgr;
	if (!this->m_dbCommandHandlerProxy.init())
		return false;

	if (!this->m_dbCacheMgr.init(this, nMaxCacheSize, nWritebackTime))
		return false;

	if (!this->connectDb(true))
		return false;

	this->m_pThread = new thread([this]()
	{
		while (true)
		{
			this->onProcess();
			if (this->m_quit.load(memory_order_acquire))
			{
				unique_lock<mutex> lock(this->m_tCommandLock);
				if (this->m_listCommand.empty())
					break;
			}
		}

		this->flushCache(0, true);

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

	this->m_dbCacheMgr.update();

	list<SDbCommand> listCommand;
	{
		unique_lock<mutex> lock(this->m_tCommandLock);
		while (this->m_listCommand.empty())
		{
			this->m_condition.wait(lock);
		}
		listCommand.splice(listCommand.end(), this->m_listCommand);
	}

	for (auto iter = listCommand.begin(); iter != listCommand.end(); ++iter)
	{
		SDbCommand sDbCommand = *iter;

		if (sDbCommand.nType == kOT_Flush)
		{
			this->flushCache(sDbCommand.nSessionID, sDbCommand.nServiceID != 0);
			return true;
		}

		shared_ptr<Message> pMessage;
		uint32_t nErrorCode = kRC_OK;
		if (!this->onPreCache(sDbCommand.nType, sDbCommand.pMessage, pMessage))
		{
			nErrorCode = this->m_dbCommandHandlerProxy.onDbCommand(sDbCommand.nType, sDbCommand.pMessage, &pMessage);
			if (nErrorCode == kRC_LOST_CONNECTION)
			{
				unique_lock<mutex> lock(this->m_tCommandLock);
				this->m_listCommand.splice(this->m_listCommand.begin(), listCommand, iter, listCommand.end());
				break;
			}
		}
		this->onPostCache(sDbCommand.nType, sDbCommand.pMessage, pMessage);

		if (sDbCommand.nSessionID == 0)
			continue;

		SDbResultInfo sDbResultInfo;
		sDbResultInfo.pResponse = make_shared<response>();
		sDbResultInfo.pResponse->set_session_id(sDbCommand.nSessionID);
		sDbResultInfo.pResponse->set_err_code(nErrorCode);
		if (pMessage != nullptr)
		{
			string* szContent = new string;
			pMessage->SerializeToString(szContent);
			sDbResultInfo.pResponse->set_name(pMessage->GetTypeName());
			sDbResultInfo.pResponse->set_allocated_content(szContent);
		}

		this->m_pDbThreadMgr->addResultInfo(sDbResultInfo);
	}

	return true;
}

bool CDbThread::onPreCache(uint32_t nType, Message* pRequest, shared_ptr<Message>& pResponse)
{
	if (this->m_dbCacheMgr.getMaxCacheSize() <= 0)
		return false;

	switch (nType)
	{
	case kOT_Select:
		{
			const proto::db::select_command* pCommand = dynamic_cast<const proto::db::select_command*>(pRequest);
			DebugAstEx(pCommand != nullptr, false);

			string szDataName = getMessageNameByTableName(pCommand->table_name());
			Message* pMessage = this->m_dbCacheMgr.getData(pCommand->id(), szDataName);
			if (pMessage != nullptr)
			{
				pResponse = shared_ptr<Message>(pMessage);
				return true;
			}
		}
		break;

	case kOT_Update:
		{
			uint64_t nID = 0;
			if (!getPrimaryValue(pRequest, nID))
				return false;

			std::string szData;
			if (!pRequest->SerializeToString(&szData))
				return false;

			if (this->m_dbCacheMgr.setData(nID, pRequest))
				return true;
		}
		break;

	case kOT_Insert:
		{
			uint64_t nID = 0;
			if (!getPrimaryValue(pRequest, nID))
				return false;

			std::string szData;
			if (!pRequest->SerializeToString(&szData))
				return false;

			this->m_dbCacheMgr.addData(nID, pRequest);
		}
		break;

	case kOT_Delete:
		{
			const delete_command* pCommand = dynamic_cast<const delete_command*>(pRequest);
			DebugAstEx(pCommand != nullptr, false);

			this->m_dbCacheMgr.delData(pCommand->id(), getMessageNameByTableName(pCommand->table_name()));
		}
		break;
	}

	return false;
}

void CDbThread::onPostCache(uint32_t nType, Message* pRequest, shared_ptr<Message>& pResponse)
{
	if (this->m_dbCacheMgr.getMaxCacheSize() <= 0)
		return;

	if (nType == kOT_Select)
	{
		DebugAst(pResponse != nullptr);

		const select_command* pCommand = dynamic_cast<const select_command*>(pRequest);
		DebugAst(pCommand != nullptr);

		std::string szData;
		if (!pResponse->SerializeToString(&szData))
			return;

		this->m_dbCacheMgr.setData(pCommand->id(), pResponse.get());
	}
}

void CDbThread::query(const SDbCommand& sDbCommand)
{
	unique_lock<mutex> lock(this->m_tCommandLock);
	this->m_listCommand.push_back(sDbCommand);

	this->m_condition.notify_all();
}

uint32_t CDbThread::getQueueSize()
{
	unique_lock<mutex> lock(this->m_tCommandLock);
	return (uint32_t)this->m_listCommand.size();
}

uint32_t CDbThread::getQPS()
{
	return this->m_nQPS;
}

CDbCommandHandlerProxy& CDbThread::getDbCommandHandlerProxy()
{
	return this->m_dbCommandHandlerProxy;
}

void CDbThread::setMaxCahceSize(uint64_t nSize)
{
	this->m_dbCacheMgr.setMaxCacheSize(nSize);
}

void CDbThread::flushCache(uint64_t nKey, bool bDel)
{
	this->m_dbCacheMgr.flushCache(nKey, bDel);
}