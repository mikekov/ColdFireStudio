/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

inline uint32 SignExtendByte(uint8 v)
{
	return int32(int16(int8(v)));	// sign extended byte to uint32
}

inline uint32 SignExtendWord(uint16 v)
{
	return int32(int16(v));			// sign extended word to uint32
}

inline uint16 SignExtendByte2Word(uint8 v)
{
	return int16(int8(v));			// sign extended byte to uint16
}


inline uint16 ByteReverse(uint16 src)
{
	return (src & 0xff) << 8 | (src >> 8);
}

inline uint32 ByteReverse(uint32 src)
{
	// 11 22 33 44
	// 44 33 22 11
	return src << 24 | (src & 0xff00) << 8 | (src & 0xff0000) >> 8 | src >> 24;
}
