/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#ifndef _format_nums_
#define _format_nums_

#include <iosfwd>
#include <iomanip>


class FormatNums
{
public:
  enum NumFmt { NUM_ERR, NUM_DEC, NUM_HEX_0X, NUM_HEX_DOL };

  int ReadNumber(CWnd *ctrl, NumFmt &fmt);
  void SetNumber(CWnd *ctrl, int num, NumFmt fmt);
  void IncEditField(CWnd *ctrl, int iDelta, int iMin, int iMax);
};


extern std::wstring FormatWithDecimalSep(unsigned int n);


#endif
