/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define WINVER 0x0601

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxtempl.h>
#include <AFXCVIEW.H>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#include <afxmaskededit.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
//#include <afxcontrolbars.h>

#include <afxpriv.h>
#include <afxole.h>

#include <AFXMT.H>

#include <afxmdiframewndex.h>
#include <afxmdichildwndex.h>
#include <afxwinappex.h>

#pragma warning(disable : 4800)

#include <Shlwapi.h>


// xutility(2176): warning C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct.
// boost::signal2 triggers this warning
#pragma warning (disable: 4996)
#include <xutility>
//#pragma warning (default: 4996)

#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <mutex>
#include <functional>
#include <iomanip>
#include <atomic>


//#define _SCB_REPLACE_MINIFRAME	1
#define _SCB_MINIFRAME_CAPTION		1

template<class Arr>
inline size_t array_count(const Arr& array)	{ return sizeof array / sizeof array[0]; }


#include <boost/optional.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

//#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

// filesystem v3
#define BOOST_FILESYSTEM_NO_LIB 1
#define BOOST_ALL_NO_LIB 1
#include <boost/filesystem.hpp>

typedef boost::filesystem::path Path;

#include <boost/algorithm/string/predicate.hpp>

#include <boost/signals2.hpp>

#undef min
#undef max

#undef ZeroMemory
