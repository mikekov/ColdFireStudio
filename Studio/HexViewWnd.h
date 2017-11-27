//
//	MFC C++ interface to the HexView win32 control
//

#pragma once

#define SEQUENCE64
#include "..\HexView\HexView.h"

class HexViewWnd : public CWnd
{
public:
	// Constructors
	HexViewWnd(HWND hWnd = nullptr)
	{}

	BOOL Create(CWnd* parent, RECT rect, LPCTSTR szWindowName = nullptr,
			DWORD dwStyle = 0, DWORD dwExStyle = 0, UINT id= 0U, LPVOID lpCreateParam = nullptr)
	{
		InitHexView();
		return CWnd::CreateEx(dwExStyle, GetWndClassName(), szWindowName, dwStyle, rect, parent, id, lpCreateParam);
	}

// Attributes
	static LPCTSTR GetWndClassName()
	{
		return WC_HEXVIEW;
	}

	UINT SetStyle(UINT mask, UINT style)
	{
		return HexView_SetStyle(m_hWnd, mask, style);
	}

	UINT SetGrouping(UINT bytes)
	{
		return HexView_SetGrouping(m_hWnd, bytes);
	}

	UINT GetStyle()
	{
		return HexView_GetStyle(m_hWnd);
	}

	UINT GetStyleMask(UINT mask)
	{
		return HexView_GetStyleMask(m_hWnd, mask);
	}

	UINT GetGrouping()
	{
		return HexView_GetGrouping(m_hWnd);
	}

	HBOOKMARK AddBookmark(PBOOKMARK param)
	{
		return HexView_AddBookmark(m_hWnd, param);
	}

	UINT DelBookmark(PBOOKMARK param)
	{
		return HexView_DelBookmark(m_hWnd, param);
	}

	UINT SetSearchPattern(PBYTE data, ULONG length)
	{
		return HexView_SetSearchPattern(m_hWnd, data, length);
	}

	size_w GetCurPos()
	{
		size_w pos = 0;
		HexView_GetCurPos(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelStart()
	{
		size_w pos = 0;
		HexView_GetSelStart(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelEnd()
	{
		size_w pos = 0;
		HexView_GetSelEnd(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelSize()
	{
		size_w len = 0;
		HexView_GetSelSize(m_hWnd, &len);
		return len;
	}

	size_w GetFileSize()
	{
		size_w len = 0;
		HexView_GetFileSize(m_hWnd, &len);
		return len;
	}

	UINT GetDataCur(PBYTE data, ULONG length)
	{
		return HexView_GetDataCur(m_hWnd, data, length);
	}

	UINT GetDataAdv(PBYTE data, ULONG length)
	{
		return HexView_GetDataAdv(m_hWnd, data, length);
	}

	UINT SetDataCur(PBYTE data, ULONG length)
	{
		return HexView_SetDataCur(m_hWnd, data, length);
	}

	UINT SetDataAdv(PBYTE data, ULONG length)
	{
		return HexView_SetDataAdv(m_hWnd, data, length);
	}

	UINT Undo()
	{
		return HexView_Undo(m_hWnd);
	}

	UINT Redo()
	{
		return HexView_Redo(m_hWnd);
	}

	UINT Cut()
	{
		return HexView_Cut(m_hWnd);
	}

	UINT Copy()
	{
		return HexView_Copy(m_hWnd);
	}

	UINT Paste()
	{
		return HexView_Paste(m_hWnd);
	}

	UINT Delete()
	{
		return HexView_Delete(m_hWnd);
	}

	UINT SetEditMode(UINT edit_mode)
	{
		return HexView_SetEditMode(m_hWnd, edit_mode);
	}

	UINT GetEditMode()
	{
		return HexView_GetEditMode(m_hWnd);
	}

	UINT OpenFile(LPCTSTR file_name, UINT method)
	{
		return HexView_OpenFile(m_hWnd, file_name, method);
	}

	UINT SaveFile(LPCTSTR file_name, UINT method)
	{
		return HexView_SaveFile(m_hWnd, file_name, method);
	}

	BOOL CanUndo()
	{
		return HexView_CanUndo(m_hWnd);
	}

	BOOL CanRedo()
	{
		return HexView_CanRedo(m_hWnd);
	}

	HMENU SetContextMenu(HMENU menu)
	{
		return HexView_SetContextMenu(m_hWnd, menu);
	}

	BOOL Clear()
	{
		return HexView_Clear(m_hWnd);
	}

	BOOL ClearBookmarks()
	{
		return HexView_ClearBookmarks(m_hWnd);
	}

	BOOL GetBookmark(HBOOKMARK bookmark, PBOOKMARK bookm)
	{
		return HexView_GetBookmark(m_hWnd, bookmark, bookm);
	}

	//HBOOKMARK EnumBookmark(HBOOKMARK bookmark, PBOOKMARK bookm)
	//{
	//	return HexView_EnumBook(m_hWnd, bookmark, bookm);
	//}

	BOOL SetBookmark(HBOOKMARK bookmark, PBOOKMARK bookm)
	{
		return HexView_SetBookmark(m_hWnd, bookmark, bookm);
	}

	UINT SetCurPos(size_w pos)
	{
		return HexView_SetCurPos(m_hWnd, pos);
	}

	UINT SetSelStart(size_w pos)
	{
		return HexView_SetSelStart(m_hWnd, pos);
	}

	UINT SetSelEnd(size_w pos)
	{
		return HexView_SetSelEnd(m_hWnd, pos);
	}

	UINT ScrollTo(size_w pos)
	{
		return HexView_ScrollTo(m_hWnd, pos);
	}

	int FormatData(HEXFMT_PARAMS *fmtparam)
	{
		return HexView_FormatData(m_hWnd, fmtparam);
	}

	UINT GetLineLen()
	{
		return HexView_GetLineLen(m_hWnd);
	}

	UINT SetLineLen(UINT len)
	{
		return HexView_SetLineLen(m_hWnd, len);
	}

	BOOL IsDragLoop()
	{
		return HexView_IsDragLoop(m_hWnd);
	}

	UINT SelectAll()
	{
		return HexView_SelectAll(m_hWnd);
	}

	BOOL FindInit(PBYTE data, UINT len)
	{
		return HexView_FindInit(m_hWnd, data, len);
	}

	BOOL FindNext(size_w *pos, UINT options)
	{
		return HexView_FindNext(m_hWnd, pos, options);
	}

	BOOL FindPrev(size_w *pos, UINT options)
	{
		return HexView_FindPrev(m_hWnd, pos, options);
	}

	BOOL FindCancel()
	{
		return HexView_FindCancel(m_hWnd);
	}

	HANDLE GetFileHandle()
	{
		return HexView_GetFileHandle(m_hWnd);
	}

	UINT GetCurPane()
	{
		return HexView_GetCurPane(m_hWnd);
	}

	UINT SetCurPane(UINT pane)
	{
		return HexView_SetCurPane(m_hWnd, pane);
	}

	UINT GetFileName(LPTSTR szName, UINT len)
	{
		return HexView_GetFileName(m_hWnd, szName, len);
	}

	BOOL IsReadOnly()
	{
		return HexView_IsReadOnly(m_hWnd);
	}

	BOOL GetCurCoord(POINT* coord)
	{
		return HexView_GetCurCoord(m_hWnd, coord);
	}

	BOOL Revert()
	{
		return HexView_Revert(m_hWnd);
	}

	UINT ImportFile(LPCTSTR file_name, UINT method)
	{
		return HexView_ImportFile(m_hWnd, file_name, method);
	}

	ULONG SetCurSel(size_w selStart, size_w selEnd)
	{
		return HexView_SetCurSel(m_hWnd, selStart, selEnd);
	}

	ULONG SetData(size_w offset, BYTE* buf, ULONG len)
	{
		return HexView_SetData(m_hWnd, offset, buf, len);
	}

	ULONG GetData(size_w offset, BYTE* buf, ULONG len)
	{
		return HexView_GetData(m_hWnd, offset, buf, len);
	}

	ULONG FillData(BYTE* buf, ULONG buflen, size_w len)
	{
		return HexView_FillData(m_hWnd, buf, buflen, len);
	}

	ULONG PassData(BYTE* buf, ULONG len)
	{
		return HexView_InitBufShared(m_hWnd, buf, len);
	}

	ULONG SetBaseAddress(ULONG address)
	{
		HexView_SetAddressOffset(m_hWnd, address);
		return 0;
	}

	// HVC_* index
	void SetColor(UINT index, COLORREF c)
	{
		HexView_SetColor(m_hWnd, index, c);
	}

	COLORREF GetColor(UINT index)
	{
		return HexView_GetColor(m_hWnd, index);
	}

	UINT SetFontSpacing(int xspace, int yspace)
	{
		return HexView_SetFontSpacing(m_hWnd, xspace, yspace);
	}

	void SetPadding(int padding_left, int padding_right)
	{
		return HexView_SetPadding(m_hWnd, padding_left, padding_right);
	}

	UINT ScrollTop(size_w pos)
	{
		HexView_ScrollTop(m_hWnd, pos);
//		return HexView_SetCurPos(m_hWnd, pos);
		return 0;
	}
};
