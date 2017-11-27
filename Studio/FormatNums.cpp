/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "FormatNums.h"


void FormatNums::IncEditField(CWnd *ctrl, int iDelta, int iMin, int iMax)
{
  int num,old;
  NumFmt fmt;

  old = num = ReadNumber(ctrl,fmt);

  num += iDelta;
  if (num > iMax)
    num = iMax;
  else if (num < iMin)
    num = iMin;
  if (num != old)
    SetNumber(ctrl,num,fmt);
}


int FormatNums::ReadNumber(CWnd *ctrl, NumFmt &fmt)
{
  TCHAR buf[32];
  int num= 0;
  if (ctrl==nullptr)
    return num;

  ctrl->GetWindowText(buf,sizeof(buf)/sizeof(buf[0]));
  ASSERT(false);
/*
  if (buf[0]==_T('$'))
  {
    fmt = NUM_HEX_DOL;
    if (sscanf(buf+1, _T("%X"),&num) <= 0)
      ;
  }
  else if (buf[0]==_T('0') && (buf[1]==_T('x') || buf[1]==_T('X')))
  {
    fmt = NUM_HEX_0X;
    if (sscanf(buf+2, _T("%X"),&num) <= 0)
      ;
  }
  else if (buf[0]>=_T('0') && buf[0]<=_T('9'))
  {
    fmt = NUM_DEC;
    if (sscanf(buf, _T("%d"),&num) <= 0)
      ;
  }
  else
    fmt = NUM_ERR;
*/
  return num;
}


void FormatNums::SetNumber(CWnd *ctrl, int num, NumFmt fmt)
{
  TCHAR buf[32];

  buf[0] = 0;

  switch (fmt)
  {
    case NUM_ERR:
    case NUM_HEX_0X:
      wsprintf(buf,_T("0x%04X"),num);
      break;
    case NUM_HEX_DOL:
      wsprintf(buf,_T("$%04X"),num);
      break;
    case NUM_DEC:
      wsprintf(buf,_T("%d"),num);
      break;
    default:
      ASSERT(FALSE);
  }

  if (ctrl)
  {
    ctrl->SetWindowText(buf);
    ctrl->UpdateWindow();
  }
}


#include <locale>
#include <iostream>
#include <iomanip>

class comma_numpunct : public std::numpunct<wchar_t>
{
protected:
	virtual wchar_t do_thousands_sep() const
	{
		return L',';
	}

	virtual std::string do_grouping() const
	{
		return "\03";
	}
};


std::wstring FormatWithDecimalSep(unsigned int n)
{
	// create a new locale based on the current application default
	// then extend it with an extra facet that controls numeric output
	static std::locale comma_locale(std::locale(), new comma_numpunct());

	std::wostringstream ost;

	// use comma separation
	ost.imbue(comma_locale);

	ost << std::fixed << n;

	return ost.str();
}
