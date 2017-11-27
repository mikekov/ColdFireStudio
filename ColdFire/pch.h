/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// precompiled header files

#pragma once

// xutility(2176): warning C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct.
// boost::algorithm::to_lower_copy() triggers this warning
#pragma warning (disable: 4996)
#include <xutility>
#pragma warning (default: 4996)

#include <vector>
#include <set>
#include <map>
#include <deque>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <iomanip>
#include <functional>
#include <utility>

#include <assert.h>
#include <ctype.h>

// ASSERT and TRACE
#include "debug_macros.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

// using filesystem v3 from boost, and single path class
#define BOOST_FILESYSTEM_NO_LIB 1
#define BOOST_ALL_NO_LIB 1
#include <boost/filesystem.hpp>

typedef boost::filesystem::path Path;

#include <boost/optional.hpp>


#include "../const_string/const_string.hpp"
#include "../const_string/concatenation.hpp"

template<class Arr>
inline int array_count(const Arr& array)	{ return sizeof array / sizeof array[0]; }

#undef OVERFLOW		// undef offensive definition from math.h
