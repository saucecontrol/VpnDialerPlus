// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently,
//  but are changed infrequently

#pragma once

#define WINVER        _WIN32_WINNT_WIN6
#define _WIN32_WINNT  _WIN32_WINNT_WIN6
#define _WIN32_IE     _WIN32_IE_WIN6
#define NTDDI_VERSION NTDDI_WIN6

#define OEMRESOURCE
#define _ATL_NO_COM
#define _ATL_CSTRING_NO_CRT

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlddx.h>

#include <atlsync.h>
#include <atlutil.h>
#include <atltypes.h>

#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>

#include <iphlpapi.h>
#include <icmpapi.h>

#include <msxml6.h>

#pragma comment(lib, "rasapi32.lib")
#pragma comment(lib, "rasdlg.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' processorArchitecture='*' language='*'\"")
