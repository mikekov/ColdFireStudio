// Number edit based upon MFC MaskedEdit

#include "pch.h"
#include "NumberEdit.h"
#include "Block.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// NumberEdit

IMPLEMENT_DYNAMIC(NumberEdit, CEdit)
static const int CMD_CHANGE_FORMAT= 1234;
static const int TOOLBAR_ID= 10;

BEGIN_MESSAGE_MAP(NumberEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_MESSAGE(WM_CUT, &NumberEdit::OnCut)
	ON_MESSAGE(WM_CLEAR, &NumberEdit::OnClear)
	ON_MESSAGE(WM_PASTE, &NumberEdit::OnPaste)
	ON_MESSAGE(WM_SETTEXT, &NumberEdit::OnSetText)
	ON_MESSAGE(WM_GETTEXT, &NumberEdit::OnGetText)
	ON_MESSAGE(WM_GETTEXTLENGTH, &NumberEdit::OnGetTextLength)
	ON_CONTROL_REFLECT(EN_CHANGE, &NumberEdit::OnChange)
	ON_WM_ERASEBKGND()
	ON_NOTIFY(NM_CUSTOMDRAW, TOOLBAR_ID, &NumberEdit::OnCustomDraw)
	ON_COMMAND(CMD_CHANGE_FORMAT, &NumberEdit::OnChangeFormat)
END_MESSAGE_MAP()


void NumberEdit::OnChangeFormat()
{
	// toggle format
	SetFormat(hex_ ? Decimal : Hexadecimal);

	TBBUTTONINFO b;
	memset(&b, 0, sizeof b);
	b.cbSize = sizeof b;
	b.dwMask = TBIF_TEXT;
	wchar_t text[2];
	b.pszText = text;
	text[0] = hex_ ? '$' : 'd';
	text[1] = 0;
	format_switch_.SetButtonInfo(CMD_CHANGE_FORMAT, &b);
}


void NumberEdit::OnCustomDraw(NMHDR* nm_hdr, LRESULT* result)
{
	NMTBCUSTOMDRAW* custom_draw= reinterpret_cast<NMTBCUSTOMDRAW*>(nm_hdr);
	*result = CDRF_DODEFAULT;

	switch (custom_draw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*result = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		custom_draw->clrTextHighlight = custom_draw->clrText = ::GetSysColor(COLOR_GRAYTEXT);
		break;
	}
}


NumberEdit::NumberEdit()
{
	get_masked_chars_only_ = true;
	set_masked_chars_only_ = false;
	paste_processing_ = false;
	set_text_processing_ = false;
	step_ = 0;
	hex_ = true;
	in_update_ = false;
	long_word_ = true;
	initialized_ = false;
	margin_ = 0;
	enable_switch_ = true;
}

NumberEdit::~NumberEdit()
{}

void NumberEdit::EnableMask(LPCTSTR mask, LPCTSTR input_template, TCHAR mask_input_template, LPCTSTR valid)
{
	ENSURE(mask != nullptr);
	ENSURE(input_template != nullptr);
	ASSERT(_istprint(mask_input_template));
	mask_ = mask;
	input_template_ = input_template;
	mask_input_template_ = mask_input_template;
	str_ = input_template;
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	if (valid != nullptr)
		valid_ = valid;
	else
		valid_.Empty();
}

void NumberEdit::DisableMask()
{
	mask_.Empty();
	input_template_.Empty();
}

void NumberEdit::SetValidChars(LPCTSTR valid)
{
	if (valid != nullptr)
		valid_ = valid;
	else
		valid_.Empty();
}


bool NumberEdit::IsMaskedChar(TCHAR chr, TCHAR mask_char) const
{
	// Check the key against the mask
	switch (mask_char)
	{
	case _T('D'): // digit only
		if (_istdigit(chr))
			return true;
		break;

	case _T('d'): // digit or space
		if (_istdigit(chr))
			return true;
		if (_istspace(chr))
			return true;
		break;

	case _T('+'): // '+' or '-' or space
		if (chr == _T('+') || chr == _T('-'))
			return true;
		if (_istspace(chr))
			return true;
		break;

	case _T('C'): // alpha only
		if (_istalpha(chr))
			return true;
		break;

	case _T('c'): // alpha or space
		if (_istalpha(chr))
			return true;
		if (_istspace(chr))
			return true;
		break;

	case _T('A'): // alpha numeric only
		if (_istalnum(chr))
			return true;
		break;

	case _T('a'): // alpha numeric or space
		if (_istalnum(chr))
			return true;
		if (_istspace(chr))
			return true;
		break;

	case _T('*'): // a printable character
		if (_istprint(chr))
			return true;
		break;
	}

	return false; // not allowed symbol
}


BOOL NumberEdit::SetValue(LPCTSTR string, bool with_delimiters)
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());
	ENSURE(string != nullptr);

	// Make sure the string is not longer than the mask
	CString source = string;
	if (!mask_.IsEmpty())
	{
		if (with_delimiters)
		{
			if (source.GetLength() > mask_.GetLength())
				return false;
		}
		else
		{
			// Count _T('_') in input_template_
			int count = 0;
			for (int i = 0; i < input_template_.GetLength(); i++)
				if (input_template_[i] == _T('_'))
					count++;

			if (source.GetLength() > count)
				return false;
		}
	}

	// Make sure the value has only valid string characters
	if (!valid_.IsEmpty())
	{
		bool ok= true;
		for (int pos= 0; ok && pos < source.GetLength(); pos++)
			if (input_template_.IsEmpty() || input_template_[pos] == _T('_'))
				if (input_template_.IsEmpty() || source[pos] != mask_input_template_) // allow mask_input_template_
					ok = valid_.Find(source[pos]) != -1;

		if (!ok)
			return false;
	}

	// Use mask, validate against the mask
	if (!mask_.IsEmpty())
	{
		ASSERT(str_.GetLength() == mask_.GetLength());

		CString result = input_template_;

		// Replace '_' with default char
		for (int i= 0; i < result.GetLength(); ++i)
			if (input_template_[i] == _T('_'))
				result.SetAt(i, mask_input_template_);

		int src_chr= 0;
		int dst_chr= 0;
		while (src_chr<source.GetLength() && dst_chr < input_template_.GetLength())
		{
			// dst_chr - character entry position("_" char)
			if (input_template_[dst_chr] == _T('_'))
			{
				TCHAR chr = source[src_chr];
				if (chr != mask_input_template_) // allow mask_input_template_
					if (!IsMaskedChar(chr, mask_[dst_chr]))
						return false;

				result.SetAt(dst_chr, chr);
				src_chr++;
				dst_chr++;
			}
			else	// dst_chr - delimeter
			{
				if (with_delimiters)
				{
					if (input_template_[dst_chr] != source[src_chr])
						return false;

					src_chr++;
					dst_chr++;
				}
				else
					dst_chr++;
			}
		}
		str_ = result;
	}
	else // Don't use mask
	{
		str_ = source;
	}

	return true;
}

const CString NumberEdit::GetMaskedValue(bool with_spaces) const
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	// Don't use mask
	if (mask_.IsEmpty())
		return str_;

	// Use mask
	ASSERT(str_.GetLength() == mask_.GetLength());

	CString result;
	for (int chr= 0; chr < input_template_.GetLength(); chr++)
	{
		if (input_template_[chr] == _T('_'))
		{
			TCHAR ch = str_[chr];
			if (ch == mask_input_template_)
			{
				if (with_spaces)
					result += ch;
			}
			else
			{
				ASSERT((!valid_.IsEmpty()) ?(valid_.Find(ch) != -1) : true);
				ASSERT(IsMaskedChar(ch, mask_[chr]));
				result += ch;
			}
		}
	}
	return result;
}

///////////////////////////////////
// Replace standard CWnd operations

void NumberEdit::SetWindowText(LPCTSTR string)
{
	CEdit::SetWindowText(string);
}

int NumberEdit::GetWindowText(_Out_writes_to_(max_count, return + 1) LPTSTR string_buf, _In_ int max_count) const
{
	return CEdit::GetWindowText(string_buf, max_count);
}

void NumberEdit::GetWindowText(CString& string) const
{
	CEdit::GetWindowText(string);
}

///////////////////////////////////
// Handlers

int NumberEdit::OnCreate(LPCREATESTRUCT create_struct)
{
	if (CEdit::OnCreate(create_struct) == -1)
		return -1;

	CWnd::SetWindowText(str_);
	return 0;
}


void NumberEdit::OnKeyDown(UINT chr, UINT rep_cnt, UINT flags)
{
	// Make sure the mask has entry positions
	int start_bound, end_bound;
	GetGroupBounds(start_bound, end_bound);
	if (start_bound == -1)
	{
		// mask has no entry positions
		MessageBeep((UINT)-1);
		return;
	}

	switch (chr)
	{
	case VK_END:
		{
			// Calc last group bounds
			int group_start, group_end;
			CEdit::GetSel(group_start, group_end);
			ASSERT(group_start != -1);
			GetGroupBounds(group_start, group_end, group_end, true);
			if (group_start == -1)
				GetGroupBounds(group_start, group_end, str_.GetLength(), false);

			ASSERT(group_start != -1);
			CEdit::SetSel(group_end, group_end);

			return;
		}
	case VK_HOME:
		{
			// Calc first group bounds
			int group_start, group_end;
			CEdit::GetSel(group_start, group_end);
			ASSERT(group_start != -1);
			GetGroupBounds(group_start, group_end, group_start, false);
			if (group_start == -1)
				GetGroupBounds(group_start, group_end, 0, true);

			ASSERT(group_start != -1);
			CEdit::SetSel(group_start, group_start);

			return;
		}

	case VK_UP:
	case VK_DOWN:
		if (step_ && initialized_)
		{
			auto v= GetNumValue() + (chr == VK_UP ? step_ : -step_);
			SetNumValue(v);
			OnChange();
		}
		break;

	case VK_LEFT:
		{
			// Calc first group bounds
			int group_start, group_end;
			CEdit::GetSel(group_start, group_end);
			ASSERT(group_start != -1);
			GetGroupBounds(group_start, group_end, group_start, false);
			if (group_start == -1)
				GetGroupBounds(group_start, group_end, 0, true);

			ASSERT(group_start != -1);

			if (::GetKeyState(VK_SHIFT)&0x80)
			{
				int start, end;
				CEdit::GetSel(start, end);
				CEdit::SetSel(start-1, end);
				return;
			}
			else if (::GetKeyState(VK_CONTROL)&0x80)
			{
				// move to the previous group
				int start, end;
				CEdit::GetSel(start, end);
				ASSERT(start != -1);

				if (start > 1)  // can search previous group
					GetGroupBounds(group_start, group_end, start - 1, false);

				// if previous group was found and it's not the same
				if (group_start != -1 && (group_start != start || group_end != end))
					CEdit::SetSel(group_start, group_end);
				else // no more groups
					MessageBeep((UINT)-1);

				return;
			}
			else
			{
				int start, end;
				CEdit::GetSel(start, end);
				// move to the previous group
				if (start == end && start == group_start)
				{
					if (start > 1)  // can search previous group
						GetGroupBounds(group_start, group_end, start-1, false);

					if (group_start != -1 && group_end < start)  // if previous group was found
						CEdit::SetSel(group_end, group_end);
					else // no more groups
						MessageBeep((UINT)-1);
				}
				else
				{
					int new_start = std::max(start - 1, group_start);
					// additional
					new_start = std::min(new_start, group_end);
					CEdit::SetSel(new_start, new_start);
				}
				return;
			}
		}

	case VK_RIGHT:
		{
			// Calc last group bounds
			int group_start, group_end;
			CEdit::GetSel(group_start, group_end);
			ASSERT(group_start != -1);
			GetGroupBounds(group_start, group_end, group_end, true);
			if (group_start == -1)
				GetGroupBounds(group_start, group_end, str_.GetLength(), false);

			ASSERT(group_start != -1);

			if (::GetKeyState(VK_SHIFT) & 0x80)
			{
				int start, end;
				CEdit::GetSel(start, end);
				CEdit::SetSel(start, end + 1);

				return;
			}
			else if (::GetKeyState(VK_CONTROL)&0x80)
			{
				// move to the next group
				int start, end;
				CEdit::GetSel(start, end);
				ASSERT(start != -1);

				if (end < str_.GetLength() - 1) // can search next group
					GetGroupBounds(group_start, group_end, end + 1, true);

				// if previous group was found and it's not the same
				if (group_start != -1 && (group_start != start || group_end != end))
					CEdit::SetSel(group_start, group_end);
				else // no more groups
					MessageBeep((UINT)-1);

				return;
			}
			else
			{
				int start, end;
				CEdit::GetSel(start, end);
				// move to the next group
				if (start == end && end == group_end)
				{
					if (end < str_.GetLength()-1) // can search next group
						GetGroupBounds(group_start, group_end, start + 1, true);

					if (group_start != -1 && group_start > end) // if next group was found
						CEdit::SetSel(group_start, group_start);
					else // no more groups
						MessageBeep((UINT)-1);
				}
				else
				{
					int new_end = std::min(end + 1, group_end);
					// additional
					new_end = std::max(new_end, group_start);
					CEdit::SetSel(new_end, new_end);
				}
				return;
			}
		}

	case VK_BACK:
		{
			// Special processing
			OnCharBackspace(chr, rep_cnt, flags);
			return;
		}
	case VK_DELETE:
		{
			if (::GetKeyState(VK_SHIFT) & 0x80)
				break;

			// Special processing
			OnCharDelete(chr, rep_cnt, flags);
			return;
		}

	case VK_INSERT:
		{
			if ((::GetKeyState(VK_CONTROL) & 0x80) || (::GetKeyState(VK_SHIFT) & 0x80))
				break;

			if (!mask_.IsEmpty())
				return;

			break;
		}
	}

	CEdit::OnKeyDown(chr, rep_cnt, flags);
}


void NumberEdit::OnChar(UINT chr, UINT rep_cnt, UINT flags)
{
	TCHAR c= (TCHAR) chr;
	if (_istprint(c) && !(::GetKeyState(VK_CONTROL) & 0x80))
	{
		OnCharPrintchar(chr, rep_cnt, flags);
		return;
	}
	else if ((chr == VK_DELETE || chr == VK_BACK) && !mask_.IsEmpty())
		return;

	int begin_old, end_old;
	CEdit::GetSel(begin_old, end_old);

	CEdit::OnChar(chr, rep_cnt, flags);

	DoUpdate(true, begin_old, end_old);
}

//////////////////////////////
// Char routines

BOOL NumberEdit::CheckChar(TCHAR chr, int pos) // returns true if the symbol is valid
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());
	ASSERT(_istprint(chr) != false);

	ASSERT(pos >= 0);

	// Don't use mask
	if (mask_.IsEmpty())
	{
		// Use valid string characters
		if (!valid_.IsEmpty())
			return(valid_.Find(chr) != -1);
		else	// Don't use valid string characters
			return true;
	}
	else
		ASSERT(pos < mask_.GetLength());

	// Use mask
	ASSERT(str_.GetLength() == mask_.GetLength());
	if (input_template_[pos] == _T('_'))
	{
		BOOL is_masked_char = IsMaskedChar(chr, mask_[pos]);

		// Use valid string characters
		if (!valid_.IsEmpty())
			return is_masked_char && valid_.Find(chr) != -1;
		else		// Don't use valid string characters
			return is_masked_char;
	}
	else
		return false;
}


void NumberEdit::OnCharPrintchar(UINT character, UINT rep_cnt, UINT flags)
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	TCHAR chr = (TCHAR) character;
	ASSERT(_istprint(chr) != false);

	// Processing ES_UPPERCASE and ES_LOWERCASE styles
	DWORD dwStyle = GetStyle();
	if (dwStyle & ES_UPPERCASE)
		chr = (TCHAR)_totupper(chr);
	else if (dwStyle & ES_LOWERCASE)
		chr = (TCHAR)_totlower(chr);

	int start_pos, end_pos;

	CEdit::GetSel(start_pos, end_pos);

	ASSERT(start_pos >= 0);
	ASSERT(end_pos >= 0);
	ASSERT(end_pos >= start_pos);

	// Calc group bounds
	int group_start, group_end;
	GetGroupBounds(group_start, group_end, start_pos);

	// Out of range
	if (start_pos < 0 && end_pos > str_.GetLength() || start_pos < group_start || start_pos > group_end || end_pos < group_start ||end_pos > group_end)
	{
		MessageBeep((UINT)-1);
		CEdit::SetSel(group_start, group_end);
		return;
	}

	// No selected chars
	if (start_pos == end_pos)
	{
		// Use mask_
		if (!mask_.IsEmpty())
		{
			// Automaticaly move the cursor to the next group
			if (end_pos == group_end || // at the end of group
				start_pos < group_start || start_pos > group_end) // not in the middle of a group
			{
				// no space for new char
				if (end_pos >= str_.GetLength() - 1)
				{
					MessageBeep((UINT)-1);
					return;
				}
				// can search next group
				else if (end_pos < str_.GetLength() - 1)
					GetGroupBounds(group_start, group_end, end_pos + 1, true);

				// if next group was found
				if (group_start != -1 && group_start > end_pos)
				{
					CEdit::SetSel(group_start, group_start);
					start_pos = group_start;
					end_pos = group_start;
				}
				// no more groups
				else
				{
					MessageBeep((UINT)-1);
					return;
				}
			}

			// Check char in position
			if (!CheckChar(chr, start_pos))
			{
				MessageBeep((UINT)-1);
				return;
			}

			// Replace char in Editbox and str_
			CEdit::SetSel(start_pos, end_pos + 1);
			CEdit::ReplaceSel(CString(chr), true);
			str_.SetAt(end_pos, chr);
			CEdit::SetSel(end_pos + 1, end_pos + 1);

			// Automaticaly move the cursor to the next group
			CEdit::GetSel(start_pos, end_pos);
			if (end_pos == group_end) // at the end of group
			{
				// can search next group
				if (end_pos < str_.GetLength() - 1)
					GetGroupBounds(group_start, group_end, end_pos + 1, true);

				// if next group was found
				if (group_start != -1 && group_start > end_pos)
				{
					CEdit::SetSel(group_start, group_start);
					start_pos = group_start;
					end_pos = group_start;
				}
			}
		}

		// Don't use mask_
		else
		{
			// Check char in position
			if (!CheckChar(chr, start_pos))
			{
				MessageBeep((UINT)-1);
				return;
			}

			// Don't use mask_
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CEdit::OnChar(character, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
	else // Have one or more chars selected
	{
		// Check char in position
		if (!CheckChar(chr, start_pos))
		{
			MessageBeep((UINT)-1);
			return;
		}

		// Replace chars in Editbox and str_
		if (!input_template_.IsEmpty()) // Use input_template_
		{
			// Calc the number of literals with the same mask char
			ASSERT(start_pos >= 0);
			ASSERT(end_pos > 0);
			ASSERT(start_pos <= input_template_.GetLength());

			int same_mask_chars_num = 1;
			int index = start_pos; // an index of the first selected char
			TCHAR mask_char = mask_[index];

			while (index + same_mask_chars_num < group_end)
				if (mask_[index + same_mask_chars_num] == mask_char)
					same_mask_chars_num++;
				else
					break;

			// Make sure the selection has the same mask char
			if (end_pos - start_pos > same_mask_chars_num)
			{
				MessageBeep((UINT)-1);
				CEdit::SetSel(index, index + same_mask_chars_num);
				return;
			}

			// Form the shifted replace string
			ASSERT(index >= group_start);
			ASSERT(index + same_mask_chars_num <= group_end);

			CString replace = str_.Mid(index, same_mask_chars_num);
			if (same_mask_chars_num > 0)
			{
				ASSERT(start_pos <= input_template_.GetLength());
				ASSERT(end_pos <= input_template_.GetLength());
				int range = end_pos - start_pos;
				ASSERT(range>0);

				replace = replace.Right(same_mask_chars_num - range + 1);
				replace += CString(mask_input_template_, range - 1);
				ASSERT(replace.GetLength() > 0);
				replace.SetAt(0, chr);
			}

			// Replace the content with the shifted string
			CEdit::SetSel(index, index + same_mask_chars_num);
			CEdit::ReplaceSel(replace, true);
			CEdit::SetSel(index, index);
			for (int i= 0; i < replace.GetLength(); i++)
				str_.SetAt(index+i, replace[i]);

			CEdit::SetSel(start_pos + 1, start_pos + 1);
		}
		else
		{
			// Don't use mask_input_template_
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CEdit::OnChar(character, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
}


void NumberEdit::OnCharBackspace(UINT chr, UINT rep_cnt, UINT flags)
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	int start_pos, end_pos;
	CEdit::GetSel(start_pos, end_pos);

	ASSERT(start_pos>=0);
	ASSERT(end_pos>=0);
	ASSERT(end_pos>=start_pos);

	// Calc group bounds
	int group_start, group_end;
	GetGroupBounds(group_start, group_end, start_pos);

	// Out of range
	if (start_pos < 0 && end_pos > str_.GetLength() || start_pos < group_start || start_pos > group_end || end_pos < group_start ||end_pos > group_end)
	{
		MessageBeep((UINT)-1);
		CEdit::SetSel(group_start, group_end);
		return;
	}

	// No selected chars
	if (start_pos == end_pos)
	{

		// Use mask_
		if (!mask_.IsEmpty())
		{
			// Automaticaly move the cursor to the previous group
			if (end_pos == group_start) // at the start of group
			{
				// can search previous group
				if (end_pos > 1)
					GetGroupBounds(group_start, group_end, end_pos-1, false);

				// if previous group was found
				if ((group_start != -1) &&(group_end < end_pos))
				{
					CEdit::SetSel(group_end, group_end);
					return;
				}
				// no more group
				else
				{
					MessageBeep((UINT)-1);
					return;
				}
			}

			// Calc the number of literals with the same mask char
			ASSERT(start_pos > 0);
			ASSERT(end_pos > 0);
			ASSERT(group_end <= input_template_.GetLength());

			int same_mask_chars_num = 1;
			int index = start_pos-1; // an index of the char to delete
			TCHAR mask_char = mask_[index];

			while (index + same_mask_chars_num < group_end)
				if (mask_[index + same_mask_chars_num] == mask_char)
					same_mask_chars_num++;
				else
					break;

			// Validate new string(dispensable)
			int i = index;
			for ( ; i + same_mask_chars_num < group_end; i++)
				if (str_[i] != mask_input_template_) // allow mask_input_template_
					if (!IsMaskedChar(str_[i], mask_[i]))
					{
						MessageBeep((UINT)-1);
						return;
					}

			// Form the shifted string
			ASSERT(index >= group_start);
			ASSERT(index + same_mask_chars_num <= group_end);

			CString replace = str_.Mid(index, same_mask_chars_num);
			if (same_mask_chars_num > 0)
			{
				replace = replace.Right(same_mask_chars_num - 1);
				replace += mask_input_template_;
			}

			// Replace the content with the shifted string
			CEdit::SetSel(index, index + same_mask_chars_num);
			CEdit::ReplaceSel(replace, true);
			CEdit::SetSel(index, index);
			for (i = 0; i < replace.GetLength(); i++)
				str_.SetAt(index+i, replace[i]);
		}
		else // Don't use mask_input_template_ - delete symbol
		{
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CWnd::OnKeyDown(chr, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
	// Have one or more chars selected
	else
	{
		if (!input_template_.IsEmpty()) // Use input_template_
		{
			// Calc the number of literals with the same mask char
			ASSERT(start_pos >= 0);
			ASSERT(end_pos > 0);
			ASSERT(start_pos <= input_template_.GetLength());

			int same_mask_chars_num = 1;
			int index = start_pos; // an index of the first selected char
			TCHAR mask_char = mask_[index];

			while (index + same_mask_chars_num < group_end)
				if (mask_[index + same_mask_chars_num] == mask_char)
					same_mask_chars_num++;
				else
					break;

			// Make sure the selection has the same mask char
			if (end_pos - start_pos > same_mask_chars_num)
			{
				MessageBeep((UINT)-1);
				CEdit::SetSel(index, index+same_mask_chars_num);
				return;
			}

			// Form the shifted replace string
			ASSERT(index >= group_start);
			ASSERT(index + same_mask_chars_num <= group_end);

			CString replace = str_.Mid(index, same_mask_chars_num);
			if (same_mask_chars_num > 0)
			{
				ASSERT(start_pos <= input_template_.GetLength());
				ASSERT(end_pos <= input_template_.GetLength());
				int range = end_pos - start_pos;
				ASSERT(range>0);

				replace = replace.Right(same_mask_chars_num - range);
				replace += CString(mask_input_template_, range);
			}

			// Replace the content with the shifted string
			CEdit::SetSel(index, index + same_mask_chars_num);
			CEdit::ReplaceSel(replace, true);
			CEdit::SetSel(index, index);
			for (int i=0; i < replace.GetLength(); i++)
			{
				str_.SetAt(index+i, replace[i]);
			}
		}
		else
		{
			// Don't use mask_input_template_ - delete symbols
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CWnd::OnKeyDown(chr, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
}


void NumberEdit::OnCharDelete(UINT chr, UINT rep_cnt, UINT flags)
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	int start_pos, end_pos;
	CEdit::GetSel(start_pos, end_pos);
	ASSERT(start_pos>=0);
	ASSERT(end_pos>=0);
	ASSERT(end_pos>=start_pos);

	// Calc group bounds
	int group_start, group_end;
	CEdit::GetSel(group_start, group_end);
	GetGroupBounds(group_start, group_end, group_start);

	// Out of range
	if (start_pos < 0 && end_pos > str_.GetLength() || start_pos < group_start || start_pos > group_end || end_pos < group_start ||end_pos > group_end)
	{
		MessageBeep((UINT)-1);
		CEdit::SetSel(group_start, group_end);
		return;
	}

	// No selected chars
	if (start_pos == end_pos)
	{
		if (!mask_.IsEmpty()) // Don't use mask_
		{
			// Make sure the cursor is not at the end of group
			if (end_pos == group_end)
			{
				MessageBeep((UINT)-1);
				return;
			}

			// Calc the number of literals with the same mask char
			ASSERT(start_pos >= 0);
			ASSERT(end_pos >= 0);
			ASSERT(group_end <= input_template_.GetLength());

			int same_mask_chars_num = 1;
			int index = start_pos; // an index of the char to delete
			TCHAR mask_char = mask_[index];

			while (index + same_mask_chars_num < group_end)
				if (mask_[index + same_mask_chars_num] == mask_char)
					same_mask_chars_num++;
				else
					break;

			// Validate new string(dispensable)
			int i = index;
			for ( ; i + same_mask_chars_num < group_end; i++)
				if (str_[i] != mask_input_template_) // allow mask_input_template_
					if (!IsMaskedChar(str_[i], mask_[i]))
					{
						MessageBeep((UINT)-1);
						return;
					}

			// Form the shifted string
			ASSERT(index >= group_start);
			ASSERT(index + same_mask_chars_num <= group_end);

			CString replace = str_.Mid(index, same_mask_chars_num);
			if (same_mask_chars_num > 0)
			{
				replace = replace.Right(same_mask_chars_num - 1);
				replace += mask_input_template_;
			}

			// Replace the content with the shifted string
			CEdit::SetSel(index, index + same_mask_chars_num);
			CEdit::ReplaceSel(replace, true);
			CEdit::SetSel(index, index);
			for (i = 0; i < replace.GetLength(); i++)
				str_.SetAt(index + i, replace[i]);
		}
		else // Don't use mask_input_template_ - delete symbol
		{
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CWnd::OnKeyDown(chr, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
	// Have one or more chars selected
	else
	{
		if (!input_template_.IsEmpty()) // Use input_template_
		{
			// Calc the number of literals with the same mask char
			ASSERT(start_pos >= 0);
			ASSERT(end_pos > 0);
			ASSERT(start_pos <= input_template_.GetLength());

			int same_mask_chars_num = 1;
			int index = start_pos; // an index of the first selected char
			TCHAR mask_char = mask_[index];

			while (index + same_mask_chars_num < group_end)
				if (mask_[index + same_mask_chars_num] == mask_char)
					same_mask_chars_num++;
				else
					break;

			// Make sure the selection has the same mask char
			if (end_pos - start_pos > same_mask_chars_num)
			{
				MessageBeep((UINT)-1);
				CEdit::SetSel(index, index + same_mask_chars_num);
				return;
			}

			// Form the shifted replace string
			ASSERT(index >= group_start);
			ASSERT(index + same_mask_chars_num <= group_end);

			CString replace = str_.Mid(index, same_mask_chars_num);
			if (same_mask_chars_num > 0)
			{
				ASSERT(start_pos <= input_template_.GetLength());
				ASSERT(end_pos <= input_template_.GetLength());
				int range = end_pos - start_pos;
				ASSERT(range>0);

				replace = replace.Right(same_mask_chars_num - range);
				replace += CString(mask_input_template_, range);
			}

			// Replace the content with the shifted string
			CEdit::SetSel(index, index + same_mask_chars_num);
			CEdit::ReplaceSel(replace, true);
			CEdit::SetSel(index, index);
			for (int i= 0; i < replace.GetLength(); i++)
				str_.SetAt(index + i, replace[i]);
		}
		else
		{
			// Don't use mask_input_template_ - delete symbols
			int begin_old, end_old;
			CEdit::GetSel(begin_old, end_old);

			CWnd::OnKeyDown(chr, rep_cnt, flags);

			DoUpdate(true, begin_old, end_old);
		}
	}
}

void NumberEdit::GetGroupBounds(int& begin, int& end, int start_pos, bool forward)
{
	ASSERT(mask_.IsEmpty() == input_template_.IsEmpty());
	ASSERT(mask_.GetLength() == input_template_.GetLength());

	if (!input_template_.IsEmpty()) // use mask
	{
		ASSERT(str_.GetLength() == mask_.GetLength());
		ASSERT(start_pos >= 0);
		ASSERT(start_pos <= input_template_.GetLength());

		if (forward)
		{
			// If start_pos is in the middle of a group
			// Reverse search for the begin of a group
			int i = start_pos;
			if (start_pos > 0)
				if (input_template_[start_pos - 1] == _T('_'))
				{
					do
					{
						i--;
					}
					while (i > 0 && input_template_[i]==_T('_'));
				}

			if (i == input_template_.GetLength())
			{
				begin = -1; // no group
				end = 0;
				return;
			}

			// i points between groups or to the begin of a group
			// Search for the begin of a group
			if (input_template_[i] != _T('_'))
			{
				i = input_template_.Find(_T('_'), i);
				if (i == -1)
				{
					begin = -1; // no group
					end = 0;
					return;
				}
			}

			begin = i;

			// Search for the end of a group
			while (i < input_template_.GetLength() && input_template_[i] == _T('_'))
				i++;

			end = i;
		}
		else // backward
		{
			// If start_pos is in the middle of a group
			// Search for the end of a group
			int i = start_pos;
			while (i < str_.GetLength() && input_template_[i] == _T('_'))
				i++;

			if (i == 0)
			{
				begin = -1; // no group
				end = 0;
				return;
			}

			// i points between groups or to the end of a group
			// Reverse search for the end of a group
			if (input_template_[i - 1] != _T('_'))
			{
				do
				{
					i--;
				}
				while (i > 0 && input_template_[i - 1] != _T('_'));

				if (i == 0)
				{
					begin = -1; // no group
					end = 0;
					return;
				}
			}
			end = i;

			// Search for the begin of a group
			do
			{
				i--;
			}
			while (i > 0 && input_template_[i - 1] == _T('_'));
			begin = i;
		}
	}
	else // don't use mask
	{
		// start_pos ignored
		begin = 0;
		end = str_.GetLength();
	}
}


BOOL NumberEdit::DoUpdate(bool restore_last_good, int begin_old, int end_old)
{
	if (paste_processing_)
		return false;

	paste_processing_ = true;

	CString copy;
	GetWindowText(copy);

	BOOL ret = SetValue(copy, true);
	if (!ret)
		MessageBeep((UINT)-1);

	if (!ret && restore_last_good)
	{
		CString old = str_;
		SetWindowText (old);

		if (begin_old != -1)
			CEdit::SetSel(begin_old, end_old);
	}

	paste_processing_ = false;
	return ret;
}


LRESULT NumberEdit::OnCut(WPARAM, LPARAM)
{
	paste_processing_ = true;

	int begin_old, end_old;
	CEdit::GetSel(begin_old, end_old);

	Default();

	CString copy;
	CWnd::GetWindowText(copy);

	if (!SetValue(copy, true))
		MessageBeep((UINT)-1);

	CWnd::SetWindowText(str_);

	CEdit::SetSel(begin_old, begin_old);
	paste_processing_ = false;

	return 0;
}


LRESULT NumberEdit::OnClear(WPARAM, LPARAM)
{
	paste_processing_ = true;

	int begin_old, end_old;
	CEdit::GetSel(begin_old, end_old);

	Default();

	CString copy;
	CWnd::GetWindowText(copy);

	if (!SetValue(copy, true))
		MessageBeep((UINT)-1);

	CWnd::SetWindowText(str_);

	CEdit::SetSel(begin_old, begin_old);
	paste_processing_ = false;

	return 0;
}


LRESULT NumberEdit::OnPaste(WPARAM, LPARAM)
{
	paste_processing_ = true;

	int begin_old, end_old;
	CEdit::GetSel(begin_old, end_old);

	Default();

	int begin, end;
	CEdit::GetSel(begin, end);
	end = std::max(begin, end);

	CString str;
	CWnd::GetWindowText(str);

	CString paste = str.Mid(begin_old, end - begin_old);
	CString old;
	int left = begin_old;

	if (set_masked_chars_only_)
	{
		old = GetMaskedValue();

		if (!mask_.IsEmpty())
		{
			for (int i= 0; i < input_template_.GetLength() && i < begin_old; ++i)
				if (input_template_[i] != _T('_'))
					left--;
		}
	}
	else
		old = GetValue();

	CString copy = old.Left(left) + paste;
	BOOL overwrite = !mask_.IsEmpty();
	int right = left +(overwrite ? paste.GetLength() : 0);
	if (right < old.GetLength())
		copy += old.Mid(right);

	if (!SetValue(copy, !set_masked_chars_only_))
		MessageBeep((UINT)-1);

	CWnd::SetWindowText(str_);

	CEdit::SetSel(begin_old, begin_old);

	paste_processing_ = false;

	return 0L;
}

///////////////////////////////////
// Replace standard CWnd operations

LRESULT NumberEdit::OnSetText(WPARAM, LPARAM lParam)
{
	if (set_text_processing_ || paste_processing_)
		return Default();

	set_text_processing_ = true;

	BOOL set_value_res = SetValue((LPCTSTR)lParam, !set_masked_chars_only_);
	if (set_value_res)
	{
		LRESULT res = false;
		CString new_validated = GetValue();
		if (new_validated.Compare((LPCTSTR)lParam) != 0)
		{
			// validated new value should differ from lParam
			res = (LRESULT)::SetWindowText(GetSafeHwnd(), (LPCTSTR)new_validated);
		}
		else
			res = Default();

		set_text_processing_ = false;
		return res;
	}

	set_text_processing_ = false;
	return false;
}


LRESULT NumberEdit::OnGetText(WPARAM wParam, LPARAM lParam)
{
	if (paste_processing_)
		return Default();

	int max_count = (int)wParam;
	if (max_count == 0)
		return 0;       // nothing copied

	LPTSTR dest_buf = (LPTSTR)lParam;
	if (dest_buf == nullptr)
		return 0;       // nothing copied

	CString text;
	if (get_masked_chars_only_)
		text = GetMaskedValue();
	else
		text = GetValue();

	// Copy text
	int count = std::min(max_count, text.GetLength());
	LPCTSTR lpcszTmp = text;
	CopyMemory(dest_buf, lpcszTmp, count * sizeof(TCHAR));

	// Add terminating null character if possible
	if (max_count > count)
		dest_buf[count] = _T('\0');
	
	return count * sizeof(TCHAR);
}


LRESULT NumberEdit::OnGetTextLength(WPARAM, LPARAM)
{
	if (paste_processing_)
		return Default();

	CString text;
	if (get_masked_chars_only_)
		text = GetMaskedValue();
	else
		text = GetValue();

	return (LRESULT) text.GetLength();
}


unsigned int NumberEdit::GetNumValue() const
{
	TCHAR buf[64];
	GetWindowText(buf, 64);
	TCHAR* dummy;
	TCHAR* p= buf;
	bool neg= false;
	if (*buf == '-')
	{
		neg = true;
		p++;
	}

	auto val= _tcstoul(p, &dummy, hex_ ? 16 : 10);

	if (neg)
		val = 0 - val;

	return val;
}


void NumberEdit::SetNumValue(unsigned int value)
{
	Block update(in_update_);

	TCHAR buf[64];
	if (hex_)
	{
		if (long_word_)
			wsprintf(buf, _T("%04X:%04X"), (value >> 16) & 0xffff, value & 0xffff);
		else
			wsprintf(buf, _T("%04X"), value);
	}
	else
		wsprintf(buf, _T("%d"), value);

	SetMargins(enable_switch_ ? margin_ : 0, 0);

	SetWindowText(buf);
}


void NumberEdit::SetStep(int step)
{
	step_ = step;
}


void NumberEdit::SetFormat(Format fmt)
{
	auto val= GetNumValue();

	hex_ = fmt == Hexadecimal;
	if (hex_)
	{
		LimitText(0);
		EnableMask(!long_word_ ? L"AAAA" : L"AAAA:AAAA", !long_word_ ? L"____" : L"____:____", _T('0'), L"0123456789abcdefABCDEF");
	}
	else
	{
		DisableMask();
		SetValidChars(L"-1234567890");
		LimitText(11);
	}

	// change display
	SetNumValue(val);
}


void NumberEdit::SetSize(bool long_word)
{
	long_word_ = long_word;
}


void NumberEdit::PreSubclassWindow()
{
	CEdit::PreSubclassWindow();

	// add format switch button

	auto font= GetFont();

	TBBUTTON buttons[]=
	{
		{
			-1,
			CMD_CHANGE_FORMAT,
			TBSTATE_ENABLED,
			BTNS_BUTTON | BTNS_SHOWTEXT,
			{ 0 },
			0,
			0
		},
	};

	CRect rect(0,0,0,0);
	GetClientRect(rect);
	rect.right = rect.bottom * 7 / 10;

	format_switch_.Create(TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE | CCS_NODIVIDER | WS_VISIBLE | WS_CHILD, rect, this, TOOLBAR_ID);
	::SetWindowTheme(format_switch_.m_hWnd, L"", L"");
	format_switch_.SetButtonStructSize(sizeof(buttons[0]));
	format_switch_.SetFont(font);
	format_switch_.SetBitmapSize(CSize(0, 0));
	format_switch_.SetPadding(0, 5);
	format_switch_.AddStrings(L"$\0");
	format_switch_.AddButtons(static_cast<int>(array_count(buttons)), buttons);
	format_switch_.SetButtonWidth(rect.Width(), rect.Width());
	format_switch_.SetDrawTextFlags(0xf | DT_SINGLELINE|DT_EDITCONTROL, DT_LEFT | DT_VCENTER | DT_SINGLELINE|DT_EDITCONTROL);
	margin_ = rect.Width();

	if (!enable_switch_)
		format_switch_.ShowWindow(SW_HIDE);

	SetFormat(Hexadecimal);

	initialized_ = true;
}


void NumberEdit::OnChange()
{
	if (!initialized_ || in_update_)
		return;

	if (CWnd* parent= GetParent())
		parent->PostMessageW(HEX_CHANGED_MSG, GetDlgCtrlID(), LPARAM(this));
}


BOOL NumberEdit::OnEraseBkgnd(CDC* dc)
{
	CRect rect(0,0,0,0);
	GetClientRect(rect);

	auto p= ::GetParent(m_hWnd);
	bool enabled= IsWindowEnabled() && (GetStyle() & ES_READONLY) == 0 && ::IsWindowEnabled(p);

	auto c= enabled ? ::GetSysColor(COLOR_WINDOW) : ::GetSysColor(COLOR_3DFACE);
	dc->FillSolidRect(rect, c);

	return CEdit::OnEraseBkgnd(dc);
}


void NumberEdit::EnableSwitchButton(bool enable)
{
	enable_switch_ = enable;

	if (format_switch_.m_hWnd)
		format_switch_.ShowWindow(enable_switch_ ? SW_SHOWNA : SW_HIDE);
}
