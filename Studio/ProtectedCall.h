#pragma once

bool ProtectedCall(const std::function<void()>& f, CString failure_message);

bool ProtectedCallMsg(const std::function<void(CString& ret_msg)>& f, CString failure_message);
