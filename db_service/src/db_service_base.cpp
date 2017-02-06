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
	char* encodeVarint(char* szBuf, uint64_t nValue)
	{
		static const int32_t B = 128;
		unsigned char* ptr = reinterpret_cast<unsigned char*>(szBuf);
		while (nValue >= B)
		{
			*(ptr++) = (nValue&(B - 1)) | B;
			nValue >>= 7;
		}

		*(ptr++) = static_cast<unsigned char>(nValue);
		return reinterpret_cast<char*>(ptr);
	}

	const char* decodeVarint(const char* szBuf, uint64_t& nValue)
	{
		static const int32_t B = 128;
		const unsigned char* ptr = reinterpret_cast<const unsigned char*>(szBuf);
		for (uint32_t nShift = 0; nShift < 64; nShift += 7)
		{
			uint64_t nByte = *(ptr);
			++ptr;
			if (nByte&B)
			{
				nValue |= ((nByte&(B - 1)) << nShift);
			}
			else
			{
				nValue |= (nByte << nShift);

				return reinterpret_cast<const char*>(ptr);
			}
		}

		return nullptr;
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif