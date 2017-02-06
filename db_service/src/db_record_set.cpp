#include "db_record_set.h"
#include "db_connection.h"
#include "db_common.h"

#include <sstream>

using namespace std;
using namespace db;

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

bool CDbRecordset::init(CDbConnection* pDbConnection, MYSQL_RES* pRes, const string& szSQL)
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

CDbVariant CDbRecordset::getVariantData(uint32_t nCol) const
{
	DebugAstEx(nCol < this->m_nFieldCount, CDbVariant());

	if (nullptr == this->m_curRow || nullptr == this->m_pField)
		return CDbVariant();

	switch (this->m_pField[nCol].type)
	{
	case MYSQL_TYPE_TINY:
	case MYSQL_TYPE_SHORT:
	case MYSQL_TYPE_LONG:
		{
			if (this->m_curRow[nCol] != nullptr)
			{
				int32_t nVal = 0;
				istringstream iss(this->m_curRow[nCol]);
				iss >> nVal;
				return CDbVariant(nVal);
			}
			else
			{
				return CDbVariant(0);
			}
		}
		break;

	case MYSQL_TYPE_LONGLONG:
		{
			if (this->m_curRow[nCol] != nullptr)
			{
				int64_t nVal = 0;
				istringstream iss(this->m_curRow[nCol]);
				iss >> nVal;
				return CDbVariant(nVal);
			}
			else
			{
				return CDbVariant(0);
			}
		}
		break;

	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_VARCHAR:
		{
			return CDbVariant(this->m_curRow[nCol]);
		}
		break;

	case MYSQL_TYPE_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_TINY_BLOB:
		{
			return CDbVariant(this->m_curRow[nCol], this->m_pField[nCol].max_length);
		}
		break;

	default:
		{
			PrintWarning("CDbRecordset::getData invaild type %d", (int32_t)this->m_pField[nCol].type);
		}
	}
	return CDbVariant();
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

const string& CDbRecordset::getSQL() const
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