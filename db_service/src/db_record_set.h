#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "db_variant.h"
#include "mysql/mysql.h"

namespace db
{
	class CDbConnection;
	class CDbRecordset
	{
	public:
		CDbRecordset();
		virtual ~CDbRecordset();

		bool		init(CDbConnection* pDbConnection, MYSQL_RES* pRes, const std::string& szSQL);
		uint64_t	getRowCount() const;
		uint32_t	getFieldCount() const;
		bool		fatchNextRow();
		bool		locate(uint64_t nRow);
		CDbVariant	getVariantData(uint32_t nCol) const;
		const char*	getData(uint32_t nCol) const;
		const char*	getFieldName(uint32_t nCol) const;

		const std::string&
					getSQL() const;

	private:
		CDbConnection*		m_pConnection;
		MYSQL_RES*			m_pRes;
		MYSQL_ROW			m_curRow;
		MYSQL_FIELD*		m_pField;
		uint64_t			m_nRowCount;
		uint32_t			m_nFieldCount;
		std::string			m_szSQL;
	};
}