/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// SaveCode.h : Select file to save binary code

#include "resource.h"
#include "LoadCode.h"


class SaveCode : public CFileDialog
{
	CString title_;
	BinaryFormat format_;
	CString file_;
	const cf::BinaryProgram& code_;

	DECLARE_DYNAMIC(SaveCode)

public:
	SaveCode(const TCHAR* file_name, CWnd* parent, const cf::BinaryProgram& code);

	void Save();

protected:
	DECLARE_MESSAGE_MAP()
};
