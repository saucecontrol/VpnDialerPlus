#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;

int Run(LPWSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;
	if ( !dlgMain.Create(dlgMain.m_hWnd) )
	{
		ATLTRACE(L"Main dialog creation failed!\n");
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);
	if ( nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE )
		dlgMain.Minimize();

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR lpstrCmdLine, _In_ int nCmdShow)
{
	ATLENSURE_SUCCEEDED(::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

	AtlInitCommonControls(ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_INTERNET_CLASSES);

	ATLENSURE_SUCCEEDED(_Module.Init(NULL, hInstance));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
