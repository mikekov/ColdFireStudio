#pragma once

class NumberEdit : public CEdit
{
	DECLARE_DYNAMIC(NumberEdit)

// Construction
public:
	NumberEdit();
	virtual ~NumberEdit();

// Implementation
public:
	static const int HEX_CHANGED_MSG= WM_APP + 1000;

	void EnableMask(LPCTSTR mask, LPCTSTR input_template, TCHAR mask_input_template= _T('_'), LPCTSTR valid= nullptr);
	void DisableMask();

	void SetValidChars(LPCTSTR valid= nullptr);
	void EnableGetMaskedCharsOnly(bool enable= true) { get_masked_chars_only_ = enable; }
	void EnableSetMaskedCharsOnly(bool enable= true) { set_masked_chars_only_ = enable; }

	void SetWindowText(LPCTSTR string);
	int GetWindowText(_Out_writes_to_(max_count, return + 1) LPTSTR string_buf, _In_ int max_count) const;
	void GetWindowText(CString& rstrString) const;

	unsigned int GetNumValue() const;
	void SetNumValue(unsigned int value);

	// enable format switch button inside edit control
	void EnableSwitchButton(bool enable);

	// step for incrementing/decrementing number in edit box with key up/down; 0 to turn off
	void SetStep(int step);

	enum Format { Hexadecimal, Decimal };
	void SetFormat(Format fmt);

	// call before subclassing to prepare right mask: 4 byte hex, or 2 byte hex
	void SetSize(bool long_word);

protected:
	virtual bool IsMaskedChar(TCHAR chr, TCHAR mask_char) const;
	virtual void PreSubclassWindow();

	const CString GetValue() const { return str_; }
	const CString GetMaskedValue(bool with_spaces= true) const;
	BOOL SetValue(LPCTSTR string, bool with_delimiters= true);

private:
	BOOL CheckChar(TCHAR chr, int pos);
	void OnCharPrintchar(UINT chr, UINT rep_cnt, UINT flags);
	void OnCharBackspace(UINT chr, UINT rep_cnt, UINT flags);
	void OnCharDelete(UINT chr, UINT rep_cnt, UINT flags);
	void GetGroupBounds(int& begin, int& end, int start_pos= 0, bool forward= true);
	BOOL DoUpdate(bool restore_last_good= true, int begin_old= -1, int end_old= -1);
	void OnChange();

// Attributes
private:
	CString str_;					// Initial value
	CString mask_;					// The mask string
	CString input_template_;		// "_" char = character entry
	TCHAR   mask_input_template_;	// Default char
	CString valid_;					// Valid string characters
	bool get_masked_chars_only_;
	bool set_masked_chars_only_;
	bool paste_processing_;
	bool set_text_processing_;
	int step_;						// increment/decrement
	bool hex_;						// hex format?
	bool in_update_;
	bool long_word_;
	bool initialized_;
	int margin_;					// left margin
	CToolBarCtrl format_switch_;
	bool enable_switch_;

protected:
	afx_msg void OnChar(UINT chr, UINT rep_cnt, UINT flags);
	afx_msg void OnKeyDown(UINT chr, UINT rep_cnt, UINT flags);
	afx_msg int OnCreate(LPCREATESTRUCT create_struct);
	afx_msg LRESULT OnCut(WPARAM, LPARAM);
	afx_msg LRESULT OnClear(WPARAM, LPARAM);
	afx_msg LRESULT OnPaste(WPARAM, LPARAM);
	afx_msg LRESULT OnSetText(WPARAM, LPARAM);
	afx_msg LRESULT OnGetText(WPARAM, LPARAM);
	afx_msg LRESULT OnGetTextLength(WPARAM, LPARAM);
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	void OnCustomDraw(NMHDR* nm_hdr, LRESULT* result);
	void OnChangeFormat();

	DECLARE_MESSAGE_MAP()
};
