/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _broadcast_
#define _broadcast_


class Broadcast
{
public:
	enum WinMsg
	{
		WM_USER_OFFSET = WM_APP + 0x100,
		WM_USER_REMOVE_ERR_MARK = WM_USER_OFFSET,
		WM_APP_STATE_CHANGED
	};

	static void SendMessageToViews(UINT msg, WPARAM wParam= 0, LPARAM lParam= 0);
	static void SendMessageToPopups(UINT msg, WPARAM wParam= 0, LPARAM lParam= 0);
};

#endif
