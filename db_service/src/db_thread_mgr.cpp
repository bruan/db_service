#include "db_thread_mgr.h"

using namespace std;
using namespace db;

CDbThreadMgr::CDbThreadMgr()
{
}

CDbThreadMgr::~CDbThreadMgr()
{
	this->exit();
}

bool CDbThreadMgr::init(const string& szHost, uint16_t nPort, const string& szDb, const string& szUser, const string& szPassword, const string& szCharacterset, uint32_t nDbThreadCount, uint64_t nMaxCacheSize)
{
	DebugAstEx(nDbThreadCount > 0, false);

	this->m_sDbConnectionInfo.szHost = szHost;
	this->m_sDbConnectionInfo.nPort = nPort;
	this->m_sDbConnectionInfo.szDb = szDb;
	this->m_sDbConnectionInfo.szUser = szUser;
	this->m_sDbConnectionInfo.szPassword = szPassword;
	this->m_sDbConnectionInfo.szCharacterset = szCharacterset;

	nMaxCacheSize = nMaxCacheSize / nDbThreadCount;
	this->m_vecDbThread.resize(nDbThreadCount);
	for (uint32_t i = 0; i < nDbThreadCount; ++i)
	{
		this->m_vecDbThread[i] = new CDbThread();
		if (!this->m_vecDbThread[i]->init(this, nMaxCacheSize))
			return false;
	}

	return true;
}

void CDbThreadMgr::exit()
{
	for (uint32_t i = 0; i < this->m_vecDbThread.size(); ++i)
	{
		this->m_vecDbThread[i]->join();
		delete this->m_vecDbThread[i];
	}
	this->m_vecDbThread.clear();
}

void CDbThreadMgr::query(uint32_t nThreadIndex, const SDbCommand& sDbCommand)
{
	nThreadIndex = nThreadIndex % this->m_vecDbThread.size();

	this->m_vecDbThread[nThreadIndex]->query(sDbCommand);
}

uint32_t CDbThreadMgr::getThreadCount() const
{
	return (uint32_t)this->m_vecDbThread.size();
}

const SDbConnectionInfo& CDbThreadMgr::getDbConnectionInfo() const
{
	return this->m_sDbConnectionInfo;
}

void CDbThreadMgr::addResultInfo(const SDbResultInfo& sResultInfo)
{
	unique_lock<mutex> lock(this->m_tResultLock);
	this->m_listResultInfo.push_back(sResultInfo);
}

void CDbThreadMgr::getResultInfo(list<SDbResultInfo>& listResultInfo)
{
	unique_lock<mutex> lock(this->m_tResultLock);
	listResultInfo.splice(listResultInfo.end(), this->m_listResultInfo);
}

void CDbThreadMgr::getQPS(vector<uint32_t>& vecQPS)
{
	vecQPS.resize(this->m_vecDbThread.size());
	for (size_t i = 0; i < this->m_vecDbThread.size(); ++i)
	{
		vecQPS[i] = this->m_vecDbThread[i]->getQPS();
	}
}

void CDbThreadMgr::getQueueSize(vector<uint32_t>& vecSize)
{
	vecSize.resize(this->m_vecDbThread.size());
	for (size_t i = 0; i < this->m_vecDbThread.size(); ++i)
	{
		vecSize[i] = this->m_vecDbThread[i]->getQueueSize();
	}
}