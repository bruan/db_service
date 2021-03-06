#pragma once
#include "db_service_base.h"
#include "db_thread.h"
#include "db_service.h"

#include <map>
#include <vector>

namespace db
{
	struct SDbConnectionInfo
	{
		std::string	szHost;
		uint16_t	nPort;
		std::string	szDb;
		std::string	szUser;
		std::string	szPassword;
		std::string	szCharacterset;
	};

	class CDbThreadMgr
	{
	public:
		CDbThreadMgr();
		~CDbThreadMgr();

		bool		init(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, uint32_t nDbThreadCount, uint64_t nMaxCacheSize, uint32_t nWritebackTime);

		uint32_t	getThreadCount() const;
		void		query(uint32_t nThreadIndex, const SDbCommand& sDbCommand);
		const SDbConnectionInfo&
					getDbConnectionInfo() const;

		void		addResultInfo(const SDbResultInfo& sResultInfo);

		void		exit();

		void		getResultInfo(std::list<SDbResultInfo>& listResultInfo);

		void		getQPS(std::vector<uint32_t>& vecQPS);
		void		getQueueSize(std::vector<uint32_t>& vecSize);

		void		setMaxCacheSize(uint64_t nSize);
		
	private:
		std::vector<CDbThread*>		m_vecDbThread;
		SDbConnectionInfo			m_sDbConnectionInfo;
		std::mutex					m_tResultLock;
		std::list<SDbResultInfo>	m_listResultInfo;
	};
}