#pragma once
#include "db_service_base.h"
#include "db_thread.h"

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

	class CDbThread;
	class CDbThreadMgr
	{
	public:
		CDbThreadMgr();
		~CDbThreadMgr();

		bool		init(const std::string& szHost, uint16_t nPort, const std::string& szDb, const std::string& szUser, const std::string& szPassword, const std::string& szCharacterset, uint32_t nDbThreadCount);

		uint32_t	getThreadCount() const;
		void		query(uint32_t nThreadIndex, const SDbCommand& sDbCommand);
		const SDbConnectionInfo&
					getDbConnectionInfo() const;

		void		addResult(uint32_t nServiceID, proto::db::response* pResponse);

		void		exit();

		void		getResult(std::list<std::pair<uint32_t, proto::db::response*>>& listResult);

	private:
		std::vector<CDbThread*>	m_vecDbThread;
		SDbConnectionInfo		m_sDbConnectionInfo;
		std::mutex				m_tResultLock;
		std::list<std::pair<uint32_t, proto::db::response*>>
								m_listResult;
	};
}