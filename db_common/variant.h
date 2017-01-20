#pragma once
#include <stdint.h>
#include <string>

namespace db
{
	enum EVariantValueType
	{
		eVVT_None,
		eVVT_Int32,
		eVVT_UInt32,
		eVVT_Int64,
		eVVT_UInt64,
		eVVT_Double,
		eVVT_String,
	};

	class CVariant
	{
	public:
		CVariant();
		CVariant(int8_t value);
		CVariant(uint8_t value);
		CVariant(int16_t value);
		CVariant(uint16_t value);
		CVariant(int32_t value);
		CVariant(uint32_t value);
		CVariant(int64_t value);
		CVariant(uint64_t value);
		CVariant(double value);
		CVariant(const char* value, uint32_t nLen);
		CVariant(const std::string& value);

		CVariant(const CVariant& rhs);

		CVariant(CVariant&& rhs);

		~CVariant();

		operator int8_t() const;
		operator uint8_t() const;
		operator int16_t() const;
		operator uint16_t() const;
		operator int32_t() const;
		operator uint32_t() const;
		operator int64_t() const;
		operator uint64_t() const;
		operator double() const;
		operator const char*() const;
		operator std::string() const;

		CVariant& operator = (const CVariant& rhs);
		CVariant& operator = (CVariant&& rhs);
		CVariant& operator = (int8_t value);
		CVariant& operator = (uint8_t value);
		CVariant& operator = (int16_t value);
		CVariant& operator = (uint16_t value);
		CVariant& operator = (int32_t value);
		CVariant& operator = (uint32_t value);
		CVariant& operator = (int64_t value);
		CVariant& operator = (uint64_t value);
		CVariant& operator = (double value);
		CVariant& operator = (const std::string& value);

		size_t				getSize() const;
		EVariantValueType	getType(void) const;
		bool				isVaild() const;
		std::string			toString() const;

	private:
		void				setString(const char* szStr, uint32_t nLen);
		void				clear();

	private:
		EVariantValueType m_eType;
		union
		{
			int64_t	m_nValue;
			double	m_fValue;
			struct SText
			{
				char*		szStr;
				uint32_t	nLen;
			} m_sText;
#define		m_szStr m_sText.szStr
#define		m_nLen  m_sText.nLen
		};
	};
}