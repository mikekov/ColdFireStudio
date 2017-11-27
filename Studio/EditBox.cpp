/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "EditBox.h"

extern COLORREF CalcColor(COLORREF rgb_color1, COLORREF rgb_color2, float bias);


struct EditBox::Impl : CEdit
{
	void OnEditChanged();
	void OnEndEdit(NMHDR* hdr, LRESULT* result);
	HBRUSH CtlColor(CDC* dc, UINT flags);
	void OnKeyDown(UINT chr, UINT rept_count, UINT flags);

	struct Combo : CComboBoxEx
	{
		Combo() : impl(nullptr)
		{}

		EditBox::Impl* impl;

		void OnEditChange(NMHDR* hdr, LRESULT* result);
		void OnEditChange2();
		LRESULT OnRefresh(WPARAM, LPARAM);

		DECLARE_MESSAGE_MAP();
	};

	Combo combo_;

	EditBox::Entry EntryType(unsigned int* num= nullptr, CString* text= nullptr);
	void SetNumber(unsigned int num, EditBox::Entry format);

	std::map<CString, int> symbols_;
	CBrush normal_;
	CBrush error_;

	EditBox* edit_;
	std::function<void (EditBox& box)> change_;

	Impl()
	{
		auto c= ::GetSysColor(COLOR_WINDOW);
		normal_.CreateSolidBrush(c);
		error_.CreateSolidBrush(::CalcColor(c, RGB(255,0,0), 0.80f));
		edit_ = nullptr;
		combo_.impl = this;
	}

	DECLARE_MESSAGE_MAP();
};


BEGIN_MESSAGE_MAP(EditBox::Impl::Combo, CComboBoxEx)
	ON_NOTIFY_REFLECT(CBEN_ENDEDIT, &EditBox::Impl::Combo::OnEditChange)
	ON_CONTROL_REFLECT(CBN_SELENDOK, &EditBox::Impl::Combo::OnEditChange2)
	ON_MESSAGE(WM_APP, &EditBox::Impl::Combo::OnRefresh)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(EditBox::Impl, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, &EditBox::Impl::OnEditChanged)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


void EditBox::Impl::Combo::OnEditChange(NMHDR* hdr, LRESULT* result)
{
//TRACE(L"code: %d\n", hdr->code);
	if (hdr->code == CBEN_ENDEDIT)
	{
		NMCBEENDEDIT& e= *reinterpret_cast<NMCBEENDEDIT*>(hdr);
		if (e.fChanged)
			impl->OnEditChanged();
		TRACE(L"n: %x\n", e.iWhy);
	}
}


void EditBox::Impl::Combo::OnEditChange2()
{
	// postpone notification till edit box is updated with new text form the list
	PostMessage(WM_APP);
}


LRESULT EditBox::Impl::Combo::OnRefresh(WPARAM, LPARAM)
{
	impl->OnEditChanged();
	return 0;
}


void EditBox::Impl::OnKeyDown(UINT chr, UINT rept_count, UINT flags)
{
	if (chr == VK_UP || chr == VK_DOWN)
	{
		if (combo_.m_hWnd && !combo_.GetDroppedState())
		{
			unsigned int n= 0;
			auto type= EntryType(&n);
			if (type == HexNum || type == DecNum)
			{
				if (chr == VK_UP)
					n++;
				else
					n--;
				SetNumber(n, type);
				if (change_)
					change_(*edit_);
			}
			return;
		}
	}

	CEdit::OnKeyDown(chr, rept_count, flags);
}


void EditBox::Impl::OnEditChanged()
{
	Invalidate();

	if (change_)
		change_(*edit_);
}


HBRUSH EditBox::Impl::CtlColor(CDC* dc, UINT flags)
{
	auto e= EntryType();
	auto& brush= e == Other ? error_ : normal_;
	LOGBRUSH lb;
	brush.GetLogBrush(&lb);
	dc->SetBkColor(lb.lbColor);
	return brush;
}


EditBox::EditBox() : impl_(new Impl())
{
	impl_->edit_ = this;
}


EditBox::~EditBox()
{}


void EditBox::Subclass(CWnd* parent, int id)
{
	ASSERT(impl_->m_hWnd == nullptr);

	HWND hwnd= nullptr;
	parent->GetDlgItem(id, &hwnd);
	ASSERT(hwnd);

	if (hwnd == nullptr)
		return;

	wchar_t klass[100];
	klass[::RealGetWindowClass(hwnd, klass, static_cast<UINT>(array_count(klass)))] = 0;

	if (wcscmp(klass, L"Edit") == 0)
	{
		impl_->SubclassWindow(hwnd);
	}
	else if (wcscmp(klass, L"ComboBoxEx32") == 0)
	{
		impl_->combo_.SubclassWindow(hwnd);
		HWND edit= reinterpret_cast<HWND>(::SendMessage(hwnd, CBEM_GETEDITCONTROL, 0, 0));
		ASSERT("Drop down combo expected" && edit);
		if (edit)
			impl_->SubclassWindow(edit);
	}
	else
	{
		ASSERT("Class not supported" && false);
	}
}


void EditBox::Impl::SetNumber(unsigned int num, EditBox::Entry format)
{
	CString str;
	if (format == HexNum)
		str.Format(L"$%x", num);
	else if (format == DecNum)
		str.Format(L"%u", num);		//todo: signed if needed
	else
	{
		ASSERT(false);
		return;
	}
	SetWindowText(str);
}


EditBox::Entry EditBox::Impl::EntryType(unsigned int* num, CString* str)
{
	CString text;
	GetWindowText(text);
	text.Trim();
	if (text.IsEmpty())
		return Empty;
	const wchar_t* p= text;
	bool hex= false;
	if (*p == '$')
	{
		hex = true;
		p++;
	}
	else if (*p == '0' && (p[1] == 'x' || p[1] == 'X'))
	{
		hex = true;
		p += 2;
	}
	else if (::isdigit(*p))
	{
		hex = false;
	}
	else
	{
		// symbols
		auto upper= text;
		upper.MakeUpper();
		auto it= symbols_.find(upper);
		if (it != symbols_.end())
		{
			if (num)
				*num = it->second;
			return Symbol;
		}

		// todo: recognize expressions

		//

		return Other;
	}

	wchar_t* end= nullptr;

	auto n= wcstoul(p, &end, hex ? 16 : 10);
	if (*end != 0)
		return Other;
	if (num)
		*num = n;
	return hex ? HexNum : DecNum;
}


//bool EditBox::HasNumber() const
//{
//	switch (impl_->EntryType(nullptr))
//	{
//	case EditBox::HexNum:
//	case EditBox::DecNum:
//		return true;
//
//	default:
//		return false;
//	}
//}


unsigned int EditBox::GetValue() const
{
	unsigned int n= 0;
	impl_->EntryType(&n);
	return n;
}


void EditBox::SetValue(unsigned int value)
{
	auto t= impl_->EntryType(nullptr);
	if (t != EditBox::HexNum && t != EditBox::DecNum)
		t = EditBox::HexNum;
	impl_->SetNumber(value, t);
}


CString EditBox::GetText() const
{
	CString str;
	impl_->GetWindowText(str);
	return str;
}


void EditBox::SetText(const wchar_t* text)
{
	impl_->SetWindowText(text);
}


void EditBox::ChangeCallback(const std::function<void (EditBox& box)>& fn)
{
	impl_->change_ = fn;
}


void EditBox::AddSymbol(CString name, int value)
{
	auto upper= name;
	impl_->symbols_[upper.MakeUpper()] = value;

	COMBOBOXEXITEM item;
	memset(&item, 0, sizeof(item));
	const wchar_t* n= name;
	item.pszText = const_cast<wchar_t*>(n);
	item.mask = CBEIF_TEXT;
	item.iItem = impl_->combo_.GetCount();
	impl_->combo_.InsertItem(&item);
}


void EditBox::RemoveSymbol(CString name)
{
	impl_->symbols_.erase(name);
	// todo: remove from combo
}


void EditBox::SetFont(CFont& font)
{
	impl_->SetFont(&font);
}


//bool EditBox::HasSymbol() const
//{
//	return impl_->EntryType(nullptr) == EditBox::Symbol;
//}
//
//
//bool EditBox::HasExpression() const
//{
//	//todo
//	return false;
//}


EditBox::Entry EditBox::GetEntry(unsigned int* number, CString* text) const
{
	return impl_->EntryType(number, text);
}
