#pragma once
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

namespace db
{

	char*		encodeVarint(char* szBuf, uint64_t nValue);
	const char*	decodeVarint(const char* szBuf, uint64_t& nValue);

#define	DebugAst(Exp)			do{ if(!(Exp)) { assert(0); return; } } while(0)
#define	DebugAstEx(Exp, Ret)	do{ if(!(Exp)) { assert(0); return Ret; } } while(0)

#ifndef _countof
#define _countof(arg) sizeof(arg)/sizeof(arg[0])
#endif

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