#define STRICT
#define _WIN32_WINNT 0x501
#include <windows.h>
//#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <trace.h>
#include <vsstyle.h>
//#include <tmschema.h>

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#include <vector>
