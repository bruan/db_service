#include "db_service_base.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#endif
#include <vector>
#include <string>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif

namespace db
{
	bool itoa(int32_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix)
	{
#ifdef _WIN32
		return (::_itoa_s(nValue, szBuf, nBufSize, nRadix) == 0);
#else
		return true;
#endif
	}

	bool uitoa(uint32_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix)
	{
#ifdef _WIN32
		int64_t n64Value = nValue;
		return (::_ui64toa_s(n64Value, szBuf, nBufSize, nRadix) == 0);
#else
		return true;
#endif
	}

	bool i64toa(int64_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix)
	{
#ifdef _WIN32
		return (::_i64toa_s(nValue, szBuf, nBufSize, nRadix) == 0);
#else
		return true;
#endif
	}

	bool ui64toa(uint64_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix)
	{
#ifdef _WIN32
		return (::_ui64toa_s(nValue, szBuf, nBufSize, nRadix) == 0);
#else
		return true;
#endif
	}

	bool atoi(const char* szBuf, int32_t& nVal)
	{
		errno = 0;
		nVal = ::strtol(szBuf, NULL, 10);

		return errno == 0;
	}

	bool atoui(const char* szBuf, uint32_t& nVal)
	{
		errno = 0;
		nVal = ::strtoul(szBuf, NULL, 10);
		return errno == 0;
	}

	bool atoi64(const char* szBuf, int64_t& nVal)
	{
#ifdef _WIN32
		errno = 0;
		nVal = ::_strtoi64(szBuf, NULL, 10);
		return errno == 0;
#else
		return true;
#endif
	}

	bool atoui64(const char* szBuf, uint64_t& nVal)
	{
#ifdef _WIN32
		errno = 0;
		nVal = ::_strtoui64(szBuf, NULL, 10);
		return errno == 0;
#else
		return true;
#endif
	}
}

#ifdef _WIN32
#pragma warning(pop)
#endif