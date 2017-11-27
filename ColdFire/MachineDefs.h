/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once


typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned __int64	uint64;

typedef signed char			int8;
typedef short				int16;
typedef int					int32;
typedef __int64				int64;

#define BIT_FIELDS_LSB_TO_MSB 1		// this is bit field layout in VC; from least significant bit to the most significant bit
