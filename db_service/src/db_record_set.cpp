#include "db_record_set.h"
#include "db_connection.h"
#include "db_common.h"

#include <sstream>

namespace db
{
	CDbRecordset::CDbRecordset()
		: m_pConnection(nullptr)
		, m_pRes(nullptr)
		, m_curRow(nullptr)
		, m_nRowCount(0)
		, m_nFieldCount(0)
	{
	}

	CDbRecordset::~CDbRecordset()
	{
		mysql_free_result(this->m_pRes);
	}

	bool CDbRecordset::init(CDbConnection* pDbConnection, MYSQL_RES* pRes, const std::string& szSQL)
	{
		DebugAstEx(pDbConnection != nullptr && pRes != nullptr, false);

		this->m_pConnection = pDbConnection;
		this->m_pRes = pRes;
		this->m_curRow = nullptr;
		this->m_nFieldCount = mysql_num_fields(pRes);
		this->m_nRowCount = (uint64_t)mysql_num_rows(pRes);
		this->m_pField = mysql_fetch_fields(pRes);

		this->m_szSQL = szSQL;

		return true;
	}

	uint32_t CDbRecordset::getFieldCount() const
	{
		return this->m_nFieldCount;
	}

	uint64_t CDbRecordset::getRowCount() const
	{
		return this->m_nRowCount;
	}

	bool CDbRecordset::fatchNextRow()
	{
		this->m_curRow = mysql_fetch_row(this->m_pRes);

		return nullptr != this->m_curRow;
	}

	bool CDbRecordset::locate(uint64_t nRow)
	{
		DebugAstEx(nRow < this->m_nRowCount, false);

		mysql_data_seek(this->m_pRes, nRow);
		this->m_curRow = mysql_fetch_row(this->m_pRes);

		return nullptr != this->m_curRow;
	}

	CVariant CDbRecordset::getVariantData(uint32_t nCol) const
	{
		DebugAstEx(nCol < this->m_nFieldCount, CVariant());

		if (nullptr == this->m_curRow || nullptr == this->m_pField)
			return CVariant();

		switch (this->m_pField[nCol].type)
		{
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
			{
				if (this->m_curRow[nCol] != nullptr)
				{
					int32_t nVal = 0;
					std::istringstream iss(this->m_curRow[nCol]);
					iss >> nVal;
					return CVariant(nVal);
				}
				else
				{
					return CVariant(0);
				}
			}
			break;

		case MYSQL_TYPE_LONGLONG:
			{
				if (this->m_curRow[nCol] != nullptr)
				{
					int64_t nVal = 0;
					std::istringstream iss(this->m_curRow[nCol]);
					iss >> nVal;
					return CVariant(nVal);
				}
				else
				{
					return CVariant(0);
				}
			}
			break;

		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_VAR_STRING:
			{
				return CVariant(this->m_curRow[nCol]);
			}
			break;

		default:
			{
				PrintWarning("CDbRecordset::getData invaild type %d", (int32_t)this->m_pFieldType[nCol]);
			}
		}
		return CVariant();
	}

	const char* CDbRecordset::getData(uint32_t nCol) const
	{
		DebugAstEx(nCol < this->m_nFieldCount, nullptr);

		if (nullptr == this->m_curRow)
			return nullptr;

		const char* szData = this->m_curRow[nCol];
		if (nullptr == szData)
			return "";

		return szData;
	}

	const std::string& CDbRecordset::getSQL() const
	{
		return this->m_szSQL;
	}

	const char* CDbRecordset::getFieldName(uint32_t nCol) const
	{
		DebugAstEx(nCol < this->m_nFieldCount, nullptr);

		if (nullptr == this->m_pField)
			return nullptr;
		
		return this->m_pField[nCol].name;
	}

}