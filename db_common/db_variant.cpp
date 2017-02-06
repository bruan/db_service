#include "db_variant.h"
#include <string.h>
#include <sstream>

#include "db_common.h"

using namespace std;
using namespace db;

CDbVariant::CDbVariant()
	: m_eType(eVVT_None)
{
}

CDbVariant::CDbVariant(int8_t value)
{
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(uint8_t value)
{
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(int16_t value)
{
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(uint16_t value)
{
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(int32_t value)
{
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(uint32_t value)
{
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(int64_t value)
{
	this->m_eType = eVVT_Int64;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(uint64_t value)
{
	this->m_eType = eVVT_UInt64;
	this->m_nValue = value;
}

CDbVariant::CDbVariant(double value)
{
	this->m_eType = eVVT_Double;
	this->m_fValue = value;
}

CDbVariant::CDbVariant(const char* value, uint32_t nLen)
{
	// 必须设置成None，不然clear函数中会删除一个无效的指针
	this->m_eType = eVVT_None;
	this->setString(value, nLen);
}

CDbVariant::CDbVariant(const string& value)
{
	// 必须设置成None，不然Clear函数中会删除一个无效的指针
	this->m_eType = eVVT_None;
	this->setString(value.c_str(), (uint32_t)value.size());
}

CDbVariant::CDbVariant(const CDbVariant& rhs)
{
	this->m_eType = eVVT_None;
	*this = rhs;
}

CDbVariant::CDbVariant(CDbVariant&& rhs)
{
	*this = rhs;
}

CDbVariant::~CDbVariant()
{
	this->clear();
}

EVariantValueType CDbVariant::getType() const
{
	return m_eType;
}

CDbVariant& CDbVariant::operator = (const CDbVariant& rhs)
{
	if (this == &rhs)
		return *this;

	this->clear();
	this->m_eType = rhs.m_eType;
	switch (rhs.m_eType)
	{
	case eVVT_Int32:
	case eVVT_UInt32:
	case eVVT_Int64:
	case eVVT_UInt64:
		this->m_nValue = rhs.m_nValue;
		break;

	case eVVT_Double:
		this->m_fValue = rhs.m_fValue;
		break;

	case eVVT_String:
		this->setString(rhs.m_sText.szStr, rhs.m_sText.nLen);
		break;

	default:
		PrintWarning("CVariant::operator = & invalid type %d", (int32_t)rhs.m_eType);
	}

	return *this;
}

CDbVariant& CDbVariant::operator = (CDbVariant&& rhs)
{
	if (this == &rhs)
		return *this;

	this->clear();
	this->m_eType = rhs.m_eType;
	switch (rhs.m_eType)
	{
	case eVVT_Int32:
	case eVVT_UInt32:
	case eVVT_Int64:
	case eVVT_UInt64:
		{
			this->m_nValue = rhs.m_nValue;
			rhs.m_nValue = 0;
		}
		break;

	case eVVT_Double:
		{
			this->m_fValue = rhs.m_fValue;
			rhs.m_fValue = 0.0;
		}
		break;

	case eVVT_String:
		{
			this->m_sText.szStr = rhs.m_sText.szStr;
			this->m_sText.nLen = rhs.m_sText.nLen;
			rhs.m_sText.szStr = nullptr;
			rhs.m_sText.nLen = 0;
		}
		break;

	default:
		PrintWarning("CVariant::operator = && invalid type %d", (int32_t)rhs.m_eType);
	}

	rhs.m_eType = eVVT_None;

	return *this;
}

void CDbVariant::clear()
{
	switch (this->m_eType)
	{
	case eVVT_String:
		delete[] this->m_sText.szStr;
		this->m_sText.nLen = 0;
		break;

	default:
		break;
	}

	this->m_eType = eVVT_None;
	this->m_fValue = 0.0;
	this->m_nValue = 0;
	this->m_sText.szStr = nullptr;
}

CDbVariant::operator int8_t() const
{
	return (int8_t)this->operator int64_t();
}

CDbVariant::operator uint8_t() const
{
	return (uint8_t)this->operator int64_t();
}

CDbVariant::operator int16_t() const
{
	return (int16_t)this->operator int64_t();
}

CDbVariant::operator uint16_t() const
{
	return (uint16_t)this->operator int64_t();
}

CDbVariant::operator int32_t() const
{
	return (int32_t)this->operator int64_t();
}

CDbVariant::operator uint32_t() const
{
	return (uint32_t)this->operator int64_t();
}

CDbVariant::operator int64_t() const
{
	switch (this->m_eType)
	{
	case eVVT_Int32:
	case eVVT_UInt32:
	case eVVT_Int64:
	case eVVT_UInt64:
		return this->m_nValue;

	case eVVT_Double:
		return (int64_t)this->m_fValue;

	default:
		PrintWarning("operator int64_t invalid type %d", (int32_t)this->m_eType);
	}
	return 0;
}

CDbVariant::operator uint64_t() const
{
	return (uint64_t)this->operator int64_t();
}

CDbVariant::operator double() const
{
	switch (this->m_eType)
	{
	case eVVT_Int32:
	case eVVT_UInt32:
	case eVVT_Int64:
	case eVVT_UInt64:
		return (double)this->m_nValue;

	case eVVT_Double:
		return this->m_fValue;

	default:
		PrintWarning("operator double invalid type %d", (int32_t)this->m_eType);
	}
	return 0.0;
}

CDbVariant::operator const char*() const
{
	switch (this->m_eType)
	{
	case eVVT_String:
		return this->m_sText.szStr;

	default:
		PrintWarning("operator const char* invalid type %d", (int32_t)this->m_eType);
	}
	return nullptr;
}

CDbVariant::operator string() const
{
	switch (this->m_eType)
	{
	case eVVT_String:
		return string(this->m_sText.szStr, this->m_sText.nLen);

	default:
		PrintWarning("operator const char* invalid type %d", (int32_t)this->m_eType);
	}
	return string();
}

size_t CDbVariant::getSize() const
{
	switch (this->m_eType)
	{
	case eVVT_String:
		return this->m_sText.nLen;

	case eVVT_Int32:
	case eVVT_UInt32:
		return sizeof(uint32_t);

	case eVVT_Int64:
	case eVVT_UInt64:
		return sizeof(int64_t);

	case eVVT_Double:
		return sizeof(double);

	default:
		break;
	}
	return 0;
}

CDbVariant& CDbVariant::operator = (int8_t value)
{
	this->clear();
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (uint8_t value)
{
	this->clear();
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (int16_t value)
{
	this->clear();
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (uint16_t value)
{
	this->clear();
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (int32_t value)
{
	this->clear();
	this->m_eType = eVVT_Int32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (uint32_t value)
{
	this->clear();
	this->m_eType = eVVT_UInt32;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (int64_t value)
{
	this->clear();
	this->m_eType = eVVT_Int64;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (uint64_t value)
{
	this->clear();
	this->m_eType = eVVT_UInt64;
	this->m_nValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (double value)
{
	this->clear();
	this->m_eType = eVVT_Double;
	this->m_fValue = value;
	return *this;
}

CDbVariant& CDbVariant::operator = (const string& value)
{
	this->setString(value.c_str(), (uint32_t)value.size());

	return *this;
}

void CDbVariant::setString(const char* szStr, uint32_t nLen)
{
	this->clear();

	if (szStr == nullptr)
		return;

	this->m_eType = eVVT_String;

	this->m_sText.szStr = new char[nLen];
	memcpy(this->m_sText.szStr, szStr, nLen);
	this->m_sText.nLen = nLen;
}

bool CDbVariant::isVaild() const
{
	return this->m_eType != eVVT_None;
}

string CDbVariant::toString() const
{
	switch (this->m_eType)
	{
	case eVVT_String:
		{
			if (this->m_sText.szStr == nullptr)
				return "";

			return string(this->m_sText.szStr, this->m_sText.nLen);
		}
		break;

	case eVVT_Int32:
	case eVVT_Int64:
	case eVVT_UInt32:
	case eVVT_UInt64:
		{
			ostringstream oss;
			oss << this->m_nValue;
			return oss.str();
		}
		break;

	case eVVT_Double:
		{
			ostringstream oss;
			oss << this->m_fValue;
			return oss.str();
		}
		break;

	default:
		break;
	}

	return "";
}