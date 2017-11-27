/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once


class EditBox : boost::noncopyable
{
public:
	EditBox();
	~EditBox();

	void Subclass(CWnd* parent, int id);

	enum Entry { Empty, HexNum, DecNum, DollarNum, Symbol, Expression, Other };
	// read input from the edit box
	Entry GetEntry(unsigned int* number, CString* text) const;

	//bool HasNumber() const;
	//bool HasSymbol() const;
	//bool HasExpression() const;

	unsigned int GetValue() const;
	void SetValue(unsigned int value);

	CString GetText() const;
	void SetText(const wchar_t* text);

	void ChangeCallback(const std::function<void (EditBox& box)>& fn);

	void AddSymbol(CString name, int value);
	void RemoveSymbol(CString name);

	void SetFont(CFont& font);

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
