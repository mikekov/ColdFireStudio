/******************************************************************************
 * Common debugging macros.
 *
 * ASSERT
 * TRACE
 *****************************************************************************/

#if !defined(_DEBUG_H_0DBFB267_B244_11D3_A459_000629B2F85_INCLUDED_)
#define _DEBUG_H_0DBFB267_B244_11D3_A459_000629B2F85_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif	// _MSC_VER >= 1000

#ifndef	_MFC_VER

#include <crtdbg.h>
#define	ASSERT	assert

#include <stdarg.h>
//#include <debugapi.h>
// this is a hack: introduce this single function without including all the baggage of Windows defs
extern "C" {
__declspec(dllimport) void __stdcall OutputDebugStringA(const char* arg);
}

#ifdef	_DEBUG

inline void _cdecl Trace0DBFB266_B244_11D3_A459_000629B2F85(char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[512];

	auto buf= _vsnprintf_s(buffer, sizeof(buffer) / sizeof(*buffer), format, args);
	ASSERT(buf < sizeof(buffer));	// Output truncated as it was > sizeof(buffer)

	OutputDebugStringA(buffer);
	va_end(args);
}

#define	TRACE	Trace0DBFB266_B244_11D3_A459_000629B2F85

#else	// !_DEBUG

#define TRACE		__noop

#endif	// _DEBUG

#endif	// _MFC_VER

#endif	// !defined(_DEBUG_H_0DBFB267_B244_11D3_A459_000629B2F85_INCLUDED_)
