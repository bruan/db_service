#pragma once
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

namespace db
{
	bool	itoa(int32_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix);

	bool	uitoa(uint32_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix);

	bool	i64toa(int64_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix);
	bool	ui64toa(uint64_t nValue, char* szBuf, size_t nBufSize, uint32_t nRadix);

	bool	atoi(const char* szBuf, int32_t& nVal);
	bool	atoui(const char* szBuf, uint32_t& nVal);

	bool	atoi64(const char* szBuf, int64_t& nVal);
	bool	atoui64(const char* szBuf, uint64_t& nVal);

#define	DebugAst(Exp)			do{ if(!(Exp)) { assert(0); return; } } while(0)
#define	DebugAstEx(Exp, Ret)	do{ if(!(Exp)) { assert(0); return Ret; } } while(0)


#define _DEBUG_SQL 1

	template<class F>
	class CDeferWrapper
	{
	public:
		CDeferWrapper(F f) : m_f(f)
		{

		}

		~CDeferWrapper()
		{
			this->m_f();
		}

	private:
		F m_f;
	};

	template<class F>
	CDeferWrapper<F> makeDeferWrapper(F f)
	{
		return CDeferWrapper<F>(f);
	};

#define STRING_JOIN(arg1, arg2) DO_STRING_JOIN(arg1, arg2)
#define DO_STRING_JOIN(arg1, arg2) arg1 ## arg2
#define DEFER(code) auto STRING_JOIN(__Defer, __LINE__) = db::makeDeferWrapper( [=]() { code; } )
}