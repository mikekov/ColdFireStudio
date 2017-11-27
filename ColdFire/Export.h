/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#ifdef BUILDING_CF_ASM
	#define CF_DECL		__declspec(dllexport)
#else
	#define CF_DECL		__declspec(dllimport)
#endif
