/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _asm_h_
#define _asm_h_

#include "MachineDefs.h"
#include "BasicTypes.h"
#include "Stat.h"

namespace masm
{
	enum InstrType	// rodzaj dyrektywy asemblera
	{
		I_ORG,		// origin
		I_DCB,		// declare block
		I_DS,		// define space
		I_END,		// end assembly
		I_ERROR,	// user error
		I_INCLUDE,	// include source file
		I_INCLUDE_BIN,	// insert binary file
		I_IF,		// conditional assembly
		I_ELSE,
		I_ELSEIF,
		I_ENDIF,
		I_MACRO,	// macrodefinition
		I_EXITM,
		I_ENDM,
		I_SET,		// assign value
		I_REPEAT,	// repeat
		I_ENDR,
		I_OPT,		// options
		//I_ROM_AREA,	// protected memory area
		//I_IO_WND,	// size of I/O window (columns, rows)
		I_DC_B,		// dc.b
		I_DC_W,		// dc.w
		I_DC_L,		// dc.l
		I_DC,		// dc.?
		I_ALIGN,	// align ORG to given boundary (by default modulo 2)
		I_SECTION,	// todo: sections
	};

	enum OperType		// operator type (lexer)
	{
		O_HI, O_LO,
		O_B_AND, O_B_OR, O_B_XOR, O_B_NOT,
		O_PLUS, O_MINUS, O_DIV, O_MUL, O_MOD,
		O_AND, O_OR, O_NOT,
		O_EQ, O_NE, O_GT, O_LT, O_GTE, O_LTE,
		O_SHL, O_SHR
	};

	static const char LOCAL_LABEL_CHAR= '.';	// first character of local labels
	static const char* MULTIPARAM= "...";		// ellipsis - variable number of macro params

	enum Breakpoint
	{
		BPT_NONE	= 0x00,		// nie ma przerwania
		BPT_EXECUTE	= 0x01,		// przerwanie przy wykonaniu
		BPT_READ	= 0x02,		// przerwanie przy odczycie
		BPT_WRITE	= 0x04,		// przerwanie przy zapisie
		BPT_MASK	= 0x07,
		BPT_NO_CODE	= 0x08,		// wiersz nie zawiera kodu - przerwanie nie mo¿e byæ ustawione
		BPT_TEMP_EXEC=0x10,		// przerwanie tymczasowe do zatrzymania programu
		BPT_DISABLED= 0x80		// przerwanie wy³¹czone
	};

	//............................... debug info ................................

	typedef uint16 FileUID;

	enum DbgFlag
	{
		DBG_EMPTY	= 0,
		DBG_CODE	= 1,
		DBG_DATA	= 2,
		DBG_MACRO	= 4,
	};

}

#endif
