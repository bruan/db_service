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

	class CDbVariant
	{
	public:
		CDbVariant();
		CDbVariant(int8_t value);
		CDbVariant(uint8_t value);
		CDbVariant(int16_t value);
		CDbVariant(uint16_t value);
		CDbVariant(int32_t value);
		CDbVariant(uint32_t value);
		CDbVariant(int64_t value);
		CDbVariant(uint64_t value);
		CDbVariant(double value);
		CDbVariant(const char* value, uint32_t nLen);
		CDbVariant(const std::string& value);

		CDbVariant(const CDbVariant& rhs);

		CDbVariant(CDbVariant&& rhs);

		~CDbVariant();

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

		CDbVariant& operator = (const CDbVariant& rhs);
		CDbVariant& operator = (CDbVariant&& rhs);
		CDbVariant& operator = (int8_t value);
		CDbVariant& operator = (uint8_t value);
		CDbVariant& operator = (int16_t value);
		CDbVariant& operator = (uint16_t value);
		CDbVariant& operator = (int32_t value);
		CDbVariant& operator = (uint32_t value);
		CDbVariant& operator = (int64_t value);
		CDbVariant& operator = (uint64_t value);
		CDbVariant& operator = (double value);
		CDbVariant& operator = (const std::string& value);

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
			struct
			{
				char*		szStr;
				uint32_t	nLen;
			} m_sText;
			int64_t	m_nValue;
			double	m_fValue;
		};
	};
}