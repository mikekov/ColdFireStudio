/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MainFrame.h"


// wys³anie komunikatu do wszystkich okien otwartych dokumentów
void Broadcast::SendMessageToViews(UINT msg, WPARAM wParam/*= 0*/, LPARAM lParam/*= 0*/)
{
	CWinApp* app= AfxGetApp();
	POSITION posTempl= app->GetFirstDocTemplatePosition();
	while (posTempl != nullptr)
	{
		CDocTemplate* templ= app->GetNextDocTemplate(posTempl);
		POSITION posDoc= templ->GetFirstDocPosition();
		while (posDoc != nullptr)
		{
			CDocument* doc= templ->GetNextDoc(posDoc);
			POSITION posView = doc->GetFirstViewPosition();
			while (posView != nullptr)
			{
				CView* view = doc->GetNextView(posView);
				view->SendMessage(msg,wParam,lParam);
			}
		}
	}
}


// wys³anie komunikatu do okien zapisanych w g_windows[]
void Broadcast::SendMessageToPopups(UINT msg, WPARAM wParam/*= 0*/, LPARAM lParam/*= 0*/)
{
	for (int i= 0; MainFrame::windows_[i]; ++i)
	{
		HWND wnd= *MainFrame::windows_[i];
		if (wnd && ::IsWindow(wnd))
			::SendMessage(wnd, msg, wParam, lParam);
	}
}
