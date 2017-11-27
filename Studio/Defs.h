/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#ifndef _asm_h_
#define _asm_h_


namespace Defs
{
	enum Breakpoint
	{
		BPT_NONE	= 0x00,		// no breakpoint
		BPT_EXECUTE	= 0x01,		// stop at execution
		BPT_READ	= 0x02,		// stop at location read
		BPT_WRITE	= 0x04,		// stop at location write
		BPT_MASK	= 0x07,
		BPT_NO_CODE	= 0x08,		// no code in a given line of source file
		BPT_TEMP_EXEC=0x10,		// temporary exec. breakpoint used by simulator
		BPT_DISABLED= 0x80		// breakpoint disabled
	};

	//............................... debug info ................................

	typedef UINT16 FileUID;
};


#endif
