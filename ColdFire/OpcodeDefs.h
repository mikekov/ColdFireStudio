/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

enum EA_ModeBits
{
	EA_Dx=			0,
	EA_Ax=			1,
	EA_Ax_IND=		2,
	EA_Ax_INC=		3,
	EA_DEC_Ax=		4,
	EA_DISP_Ax=		5,
	EA_DISP_Ax_Ix=	6,
	EA_EXT=			7,

	EA_EXT_ABS_W=		0,
	EA_EXT_ABS_L=		1,
	EA_EXT_DISP_PC=		2,
	EA_EXT_DISP_PC_Ix=	3,
	EA_EXT_IMMEDIATE=	4,
};
