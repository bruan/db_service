#include "db_connection.h"
#include "db_record_set.h"
#include "db_common.h"
#include <sstream>

namespace db
{
	CDbConnection::CDbConnection()
		: m_pMysql(nullptr)
	{
	}

	bool CDbConnection::connect(const std::string& szHost, uint16_t nPort, const std::string& szUser, const std::string& szPassword, const std::string& szDbname, const std::string& szCharacterset)
	{
		DebugAstEx(!this->isConnect(), false);

		this->m_pMysql = mysql_init(nullptr);
		if (nullptr == this->m_pMysql)
		{
			PrintWarning("mysql_init error");
			return false;
		}

		/*
		CLIENT_FOUND_ROWS 为了使mysql_affected_rows返回的数量完全有where语句控制，而不是因为数据没有变动就返回0
		CLIENT_MULTI_STATEMENTS 为了可以在一次执行mysql_real_query中可以执行多条sql（用;隔开），但是这样会导致SQL注入非常容易（通过union操作可以用一个语句做到SQL注入），所以这里禁用了
		*/
		if (this->m_pMysql != mysql_real_connect(this->m_pMysql, szHost.c_str(), szUser.c_str(), szPassword.c_str(), szDbname.c_str(), nPort, nullptr, CLIENT_FOUND_ROWS))
		{
			PrintWarning("mysql_real_connect error: %s", mysql_error(this->m_pMysql));
			mysql_close(this->m_pMysql);
			this->m_pMysql = nullptr;
			return false;
		}
		if (0 != mysql_set_character_set(this->m_pMysql, szCharacterset.c_str()))
		{
			PrintWarning("mysql_set_character_set error: %s", mysql_error(this->m_pMysql));
			mysql_close(this->m_pMysql);
			this->m_pMysql = nullptr;
			return false;
		}
		return true;
	}

	bool CDbConnection::isConnect() const
	{
		return this->m_pMysql != nullptr;
	}

	CDbConnection::~CDbConnection()
	{
		this->close();
	}

	void CDbConnection::close()
	{
		if (!this->isConnect())
			return;

		mysql_close(this->m_pMysql);
		this->m_pMysql = nullptr;
	}

	uint32_t CDbConnection::execute(const std::string& szSQL, CDbRecordset** pDbRecordset)
	{
		DebugAstEx(this->isConnect(), kMET_Unknwon);

		int32_t nError = mysql_real_query(this->m_pMysql, szSQL.c_str(), (unsigned long)szSQL.size());
		if (0 != nError)
		{
			PrintWarning("mysql_real_query error: %d, error: %s sql: %s", nError, mysql_error(this->m_pMysql), szSQL.c_str());
			return nError;
		}
		MYSQL_RES* pRes = mysql_store_result(this->m_pMysql);
		if (nullptr == pRes)
		{
			nError = mysql_errno(this->m_pMysql);
			if (0 != nError)
				PrintWarning("mysql_store_result error: %s sql: %s", mysql_error(this->m_pMysql), szSQL.c_str());
			
			return nError;
		}
		DebugAstEx(pDbRecordset != nullptr, kMET_Unknwon);

		*pDbRecordset = new CDbRecordset();
		if (!(*pDbRecordset)->init(this, pRes, szSQL))
		{
			delete *pDbRecordset;
			(*pDbRecordset) = nullptr;
			return kMET_Unknwon;
		}
		return kMET_OK;
	}

	bool CDbConnection::ping()
	{
		DebugAstEx(this->isConnect(), false);

		return 0 == mysql_ping(this->m_pMysql);
	}

	uint64_t CDbConnection::getAffectedRow() const
	{
		DebugAstEx(this->isConnect(), 0);

		return (uint64_t)mysql_affected_rows(this->m_pMysql);
	}

	std::string CDbConnection::escape(const std::string& szSQL)
	{
		DebugAstEx(this->m_pMysql != nullptr, "");
		if (szSQL.empty())
			return "";

		std::string szBuf;
		szBuf.resize(szSQL.size() * 2);

		unsigned long nSize = mysql_real_escape_string(this->m_pMysql, &szBuf[0], szSQL.c_str(), (unsigned long)szSQL.size());
		szBuf.resize(nSize);

		return szBuf;
	}

	void CDbConnection::enableAutoCommit(bool enable)
	{
		std::ostringstream oss;
		oss << "set autocommit = " << enable ? 1 : 0;
		this->execute(oss.str(), nullptr);
	}

	void CDbConnection::begintrans()
	{
		this->execute("start transaction", nullptr);
	}

	void CDbConnection::endtrans()
	{
		this->execute("commit", nullptr);
	}

	void CDbConnection::rollback()
	{
		this->execute("rollback", nullptr);
	}
}