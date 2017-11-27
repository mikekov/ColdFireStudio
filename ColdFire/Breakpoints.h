/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

namespace cf {


enum BreakpointType
{
	BPT_NONE	= 0x00,		// none
	BPT_EXECUTE	= 0x01,		// break program execution
	BPT_READ	= 0x02,		// break at memory read
	BPT_WRITE	= 0x04,		// break at memory write
	BPT_MASK	= 0x07,
//	BPT_NO_CODE	= 0x08,		// wiersz nie zawiera kodu - przerwanie nie mo¿e byæ ustawione
	BPT_TEMP_EXEC=0x10,		// przerwanie tymczasowe do zatrzymania programu
	BPT_DISABLED= 0x80		// breakpoint disabled
};


}
