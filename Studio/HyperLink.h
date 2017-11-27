////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CHyperlink implements a simple text hyperlink
//
#ifndef _HYPRILNK_H
#define _HYPRILNK_H

//////////////////
// Simple text hyperlink derived from CString
//
class CHyperlink : public CString
{
public:
  CHyperlink(LPCTSTR link = NULL) : CString(link)
  {}
  ~CHyperlink()
  {}

  const CHyperlink& operator = (LPCTSTR lpsz)
  {
    CString::operator=(lpsz);
    return *this;
  }

  operator const TCHAR*()
  {
	  return this->GetString();
  }

  HINSTANCE Navigate();
};

#endif
