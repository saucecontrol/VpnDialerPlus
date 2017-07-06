#include "stdafx.h"
#include "resource.h"

#include "AboutDlg.h"
#include "LogonDlg.h"
#include "SettingsDlg.h"
#include "MainDlg.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ICON_MAIN), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ICON_MAIN), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	CConfigMgr config;
	if ( !config.Init() )
	{
		MessageBox(config.LastError, L"Configuration Error", MB_OK | MB_ICONERROR);
		CloseDialog(0);
		return 1;
	}

	DoDataExchange(DDX_LOAD);
	PopulateVPNList();

	m_threadConNotify.Initialize();
	m_threadDisNotify.Initialize();
	m_threadKeepAlive.Initialize();

	m_evt.Create(NULL, TRUE, FALSE, NULL);
	m_threadConNotify.AddHandle(m_evt, this, NULL);
	::RasConnectionNotification((HRASCONN)INVALID_HANDLE_VALUE, m_evt, RASCN_Connection);

	UpdateConnections();
	UpdateUI();
	CreateMenu();

	return TRUE;
}

LRESULT CMainDlg::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;

	if ( wParam == SC_MINIMIZE )
	{
		Minimize(true);

		bHandled = TRUE;
	}

	return 0;
}

LRESULT CMainDlg::OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;

	// since icon disappears on dbl click, make sure no other icon receives the buttonup msg
	if ( lParam == WM_LBUTTONDBLCLK )
	{
		m_bDblClick = true;
	}
	else if ( lParam == WM_LBUTTONUP )
	{
		if ( m_bDblClick )
		{
			m_bDblClick = false;
			OnMenuOpen(0, 0, NULL, bHandled);
		}
	}
	else if ( lParam == WM_RBUTTONUP )
	{
		CMenuHandle popup = m_menu.GetSubMenu(0);
		CMenuHandle conn = popup.GetSubMenu(0);
		CMenuHandle disc = popup.GetSubMenu(1);

		for ( int i = conn.GetMenuItemCount(); i > 0 ; i-- )
			conn.RemoveMenu(i - 1, MF_BYPOSITION);

		for ( int i = disc.GetMenuItemCount(); i > 0 ; i-- )
			disc.RemoveMenu(i - 1, MF_BYPOSITION);

		// get items from combobox so they'll be sorted
		for ( int i = 0; i < m_cboConnections.GetCount(); i++ )
		{
			CString sConn;
			m_cboConnections.GetLBText(i, sConn.GetBuffer(m_cboConnections.GetLBTextLen(i)));
			sConn.ReleaseBuffer();

			if ( m_ConnMap.GetValueAt(m_ConnMap.FindKey(sConn)).m_hRasConn )
				disc.AppendMenu(0, (UINT_PTR)0, sConn);
			else
				conn.AppendMenu(0, (UINT_PTR)0, sConn);
		}

		if ( conn.GetMenuItemCount() == 0 )
			conn.AppendMenu(MF_GRAYED, (UINT_PTR)0, L"(none)");

		if ( disc.GetMenuItemCount() == 0 )
			disc.AppendMenu(MF_GRAYED, (UINT_PTR)0, L"(none)");

		CPoint mouse;
		::GetCursorPos(&mouse);
		::SetForegroundWindow(m_hWnd);

		popup.TrackPopupMenu(TPM_VERNEGANIMATION, mouse.x, mouse.y, m_hWnd);

		// KB 135788
		PostMessage(WM_NULL);

		bHandled = TRUE;
	}

	return 0;
}

LRESULT CMainDlg::OnMenuCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CMenuHandle mh((HMENU)lParam);
	int nMenu = PtrToInt((void*)wParam);

	if ( mh.GetMenuItemID(nMenu) )
		SendMessage(WM_COMMAND, mh.GetMenuItemID(nMenu), lParam);
	else
	{
		CString sMenu;
		int nLen = mh.GetMenuStringLen(nMenu, MF_BYPOSITION) + 1;

		mh.GetMenuString(nMenu, sMenu.GetBuffer(nLen), nLen, MF_BYPOSITION);
		sMenu.ReleaseBuffer();

		CConnection& conn = m_ConnMap.GetValueAt(m_ConnMap.FindKey(sMenu));
		if (conn.m_hRasConn)
			Disconnect(conn);
		else
			Connect(conn);
	}

	return 0;
}

LRESULT CMainDlg::OnMenuOpen(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
	NotifyIcon(false);
	ShowWindow(SW_SHOW);
	::SetForegroundWindow(m_hWnd);

	bHandled = TRUE;

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	m_threadConNotify.Shutdown();
	m_threadDisNotify.Shutdown();
	m_threadKeepAlive.Shutdown();

	NotifyIcon(false);

	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnBnClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Connect(m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)));
	return 0;
}

LRESULT CMainDlg::OnBnClickedDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Disconnect(m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)));
	return 0;
}

LRESULT CMainDlg::OnBnClickedSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CConfigMgr config;
	config.Init();

	bool bValid = config.LoadConfig(!config.ConfigExists());
	if ( !bValid )
	{
		int resp = MessageBox(L"The configuration file is not present or is invalid.\n"
								L"Do you want to create a new (empty) configuration file?", 
								L"Invalid Configuration", MB_YESNO | MB_ICONEXCLAMATION);
		if ( resp == IDYES )
			bValid = config.LoadConfig(true);

		if ( !bValid )
		{
			MessageBox(config.LastError, L"Configuration Error", MB_OK | MB_ICONERROR);
			return 0;
		}
	}

	CSettingsDlg setdlg(m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)));
	setdlg.m_Conn.asRoutes.RemoveAll();
	setdlg.m_Conn.sKeepAlive.Empty();

	config.LoadConnection(setdlg.m_Conn);

	if ( !setdlg.DoModal() )
		return 0;

	if ( !config.SaveConnection(setdlg.m_Conn) )
		MessageBox(config.LastError, L"Error Saving Configuration", MB_OK | MB_ICONERROR);

	return 0;
}

LRESULT CMainDlg::OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	RasDialog(m_sSelectedConnection);
	return 0;
}

LRESULT CMainDlg::OnBnClickedNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	RasDialog(CString());
	PopulateVPNList();
	return 0;
}

LRESULT CMainDlg::OnCbnSelchangeConnections(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if ( m_cboConnections.GetCurSel() == CB_ERR )
		m_sSelectedConnection.Empty();
	else
	{
		m_cboConnections.GetLBText(m_cboConnections.GetCurSel(),
		                           m_sSelectedConnection.GetBuffer(m_cboConnections.GetLBTextLen(m_cboConnections.GetCurSel())));
		m_sSelectedConnection.ReleaseBuffer();
	}

	UpdateUI();
	return 0;
}

bool CMainDlg::IsVpnConnection(LPWSTR pszEntryName)
{
	bool ret = false;

	// Use RasGetEntryProperties to make sure connection is a VPN
	CWin32Heap heap(::GetProcessHeap());
	PVOID mem = heap.Allocate(sizeof(RASENTRY));
	::ZeroMemory(mem, sizeof(RASENTRY));

	LPRASENTRY lpRasEntry = (LPRASENTRY)mem;
	lpRasEntry->dwSize = sizeof(RASENTRY);

	// First call should fail with ERROR_BUFFER_TOO_SMALL and return required size
	DWORD dwCb = sizeof(RASENTRY);
	DWORD dwErr = ERROR_SUCCESS;
	dwErr = ::RasGetEntryProperties(NULL, pszEntryName, lpRasEntry, &dwCb, NULL, NULL);
	if ( dwErr == ERROR_BUFFER_TOO_SMALL)
	{
		mem = heap.Reallocate(mem, dwCb);
		lpRasEntry = (LPRASENTRY)mem;
	}
	
	dwErr = ::RasGetEntryProperties(NULL, pszEntryName, lpRasEntry, &dwCb, NULL, NULL);
	if ( dwErr == ERROR_SUCCESS )
	{
		if ( lpRasEntry->dwType == RASET_Vpn )
			ret = true;
	}
	else
		MessageBox(GetErrorString(dwErr), L"RasGetEntryProperties Error", MB_OK | MB_ICONERROR);

	heap.Free(mem);
	return ret;
}

void CMainDlg::PopulateVPNList()
{
	// Use RasEnumEntries to populate connection list drop-down
	CWin32Heap heap(::GetProcessHeap());
	PVOID mem = heap.Allocate(sizeof(RASENTRYNAME));
	::ZeroMemory(mem, sizeof(RASENTRYNAME));

	LPRASENTRYNAME lpRasEntryName = (LPRASENTRYNAME)mem;
	lpRasEntryName->dwSize = sizeof(RASENTRYNAME);

	// First call should fail with ERROR_BUFFER_TOO_SMALL and return required size
	DWORD dwCb = sizeof(RASENTRYNAME);
	DWORD dwEntries = 0;
	DWORD dwErr = ERROR_SUCCESS;
	dwErr = ::RasEnumEntries(NULL, NULL, lpRasEntryName, &dwCb, &dwEntries);
	if ( dwErr == ERROR_BUFFER_TOO_SMALL )
	{
		mem = heap.Reallocate(mem, dwCb);
		lpRasEntryName = (LPRASENTRYNAME)mem;
	}

	dwErr = ::RasEnumEntries(NULL, NULL, lpRasEntryName, &dwCb, &dwEntries);
	if ( dwErr == ERROR_SUCCESS )
	{
		for ( DWORD i = 0; i < dwEntries; i++ )
			if ( IsVpnConnection(lpRasEntryName[i].szEntryName) )
			{
				if ( m_ConnMap.FindKey(CString(lpRasEntryName[i].szEntryName)) == -1 )
				{
					CConnection conn;
					conn.sName = lpRasEntryName[i].szEntryName;
					m_ConnMap.Add(conn.sName, conn);
					m_cboConnections.AddString(conn.sName);
				}
			}

		CConfigMgr config;
		if ( config.Init() && config.LoadConfig() )
		{
			for ( int i = 0; i < m_ConnMap.GetSize(); i++ )
				config.LoadConnection(m_ConnMap.GetValueAt(i));
		}
	}
	else if ( dwErr != ERROR_INVALID_SIZE )  // stupid bogus Vista error
		MessageBox(GetErrorString(dwErr) , L"RasEnumEntries Error", MB_OK | MB_ICONERROR);

	heap.Free(mem);
}

void CMainDlg::UpdateConnections()
{
	// Use RasEnumConnections to check for existing connections
	CWin32Heap heap(::GetProcessHeap());
	PVOID mem = heap.Allocate(sizeof(RASCONN));
	::ZeroMemory(mem, sizeof(RASCONN));

	LPRASCONN lpRasConn = (LPRASCONN)mem;
	lpRasConn->dwSize = sizeof(RASCONN);

	// First call should fail with ERROR_BUFFER_TOO_SMALL and return required size
	DWORD dwCb = sizeof(RASCONN);
	DWORD dwEntries = 0;
	DWORD dwErr = ERROR_SUCCESS;
	dwErr = ::RasEnumConnections(lpRasConn, &dwCb, &dwEntries);
	if ( dwErr == ERROR_BUFFER_TOO_SMALL )
	{
		mem = heap.Reallocate(mem, dwCb);
		lpRasConn = (LPRASCONN)mem;
	}

	dwErr = ::RasEnumConnections(lpRasConn, &dwCb, &dwEntries);
	if ( dwErr == ERROR_SUCCESS )
	{
		for ( DWORD i = 0; i < dwEntries; i++ )
		{
			RASCONNSTATUS rcs = {0};
			rcs.dwSize = sizeof(RASCONNSTATUS);

			::RasGetConnectStatus(lpRasConn[i].hrasconn, &rcs);
			if ( rcs.rasconnstate == RASCS_Connected )
			{
				int nPos = m_ConnMap.FindKey(CString(lpRasConn[i].szEntryName));
				if ( nPos != -1 )
				{
					CConnection& conn = m_ConnMap.GetValueAt(nPos);
					if ( conn.m_hRasConn != lpRasConn[i].hrasconn )
						conn.m_hRasConn = lpRasConn[i].hrasconn;

					if ( !conn.m_bConnected )
						PostConnect(conn);
				}
			}
		}
	}
	else if ( dwErr != ERROR_INVALID_SIZE )  // stupid bogus Vista error
		MessageBox(GetErrorString(dwErr) , L"RasEnumConnections Error", MB_OK | MB_ICONERROR);

	heap.Free(mem);
}
	
void CMainDlg::UpdateUI()
{
	if (m_sSelectedConnection.IsEmpty())
	{
		m_stcStatus.SetWindowText(NULL);

		m_btnConnect.EnableWindow(FALSE);
		m_btnDisconnect.EnableWindow(FALSE);
		m_btnSettings.EnableWindow(FALSE);
		m_btnProperties.EnableWindow(FALSE);
	}
	else
	{
		m_btnConnect.EnableWindow();
		m_btnDisconnect.EnableWindow();
		m_btnSettings.EnableWindow();
		m_btnProperties.EnableWindow();

		if (m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)).m_hRasConn)
		{
			m_btnConnect.ShowWindow(SW_HIDE);
			m_btnDisconnect.ShowWindow(SW_SHOW);

			m_stcStatus.SetWindowText(L"Connected");
		}
		else
		{
			m_btnConnect.ShowWindow(SW_SHOW);
			m_btnDisconnect.ShowWindow(SW_HIDE);

			m_stcStatus.SetWindowText(NULL);
		}
	}
}
	
void CMainDlg::Connect(CConnection& conn)
{
	RASENTRY RasEntry = {0};
	BOOL bDoLogon = true;

	RasEntry.dwSize = sizeof(RASENTRY);
	DWORD dwErr = ::RasGetEntryProperties(NULL, conn.sName, &RasEntry, &RasEntry.dwSize, NULL, NULL);
	if ( dwErr != ERROR_SUCCESS )
	{
		MessageBox(GetErrorString(dwErr), L"RasGetEntryProperties Error", MB_OK | MB_ICONERROR);
		return;
	}

	if ( (RasEntry.dwfOptions & RASEO_UseLogonCredentials) == RASEO_UseLogonCredentials )
		bDoLogon = false;

	// Use RasDial to dial connection
	RASDIALPARAMS RasDialParams = {0};
	BOOL bPasswordSaved = false;

	RasDialParams.dwSize = sizeof(RASDIALPARAMS);
	::lstrcpyn(RasDialParams.szEntryName, conn.sName, RAS_MaxEntryName);

	dwErr = ::RasGetEntryDialParams(NULL, &RasDialParams, &bPasswordSaved);
	if ( dwErr != ERROR_SUCCESS )
	{
		MessageBox(GetErrorString(dwErr), L"RasDialGetEntryDialParams Error", MB_OK | MB_ICONERROR);
		return;
	}

	if ( !bPasswordSaved && bDoLogon )
	{
		CLogonDlg logondlg;
		logondlg.m_sUser = RasDialParams.szUserName;
		logondlg.m_sConnection = m_sSelectedConnection;
		
		// user pressed cancel in logon dialog.  bail out.
		if ( !logondlg.DoModal() )
			return;

		::lstrcpyn(RasDialParams.szUserName, logondlg.m_sUser, UNLEN);
		::lstrcpyn(RasDialParams.szPassword, logondlg.m_sPass, PWLEN);
	}

	RasDialParams.dwCallbackId = (ULONG_PTR)this;

	dwErr = ::RasDial(NULL, NULL, &RasDialParams, 2, &CMainDlg::RasDialCallback, &conn.m_hRasConn);
	if ( dwErr != ERROR_SUCCESS )
	{
		MessageBox(GetErrorString(dwErr), L"RasDial Error", MB_OK | MB_ICONERROR);
		return;
	}

	UpdateUI();
}

void CMainDlg::Disconnect(CConnection& conn)
{
	CCursor cur;
	cur.LoadOEMCursor(IDC_WAIT);
	SetCursor(cur);
	m_btnDisconnect.EnableWindow(FALSE);

	::RasHangUp(conn.m_hRasConn);
	conn.m_bConnected = false;
	conn.m_hRasConn = NULL;
	m_threadKeepAlive.RemoveHandle(conn.m_timer);

	m_btnDisconnect.EnableWindow();
	cur.DestroyCursor();
	cur.LoadOEMCursor(IDC_ARROW);
	SetCursor(cur);

	UpdateUI();
}

void CMainDlg::PostConnect(CConnection& conn)
{
	if ( conn.asRoutes.GetSize() > 0 )
	{
		if ( conn.sName == m_sSelectedConnection )
			m_stcStatus.SetWindowText(L"Adding static routes...");

		::Sleep(1000);

		RASPPPIP rasip;
		rasip.dwSize = sizeof(rasip);
		DWORD dwCb = sizeof(rasip);
		DWORD dwErr = ERROR_SUCCESS;

		dwErr = ::RasGetProjectionInfo(conn.m_hRasConn, RASP_PppIp, &rasip, &dwCb);
		if ( dwErr != ERROR_SUCCESS )
		{
			MessageBox(GetErrorString(dwErr), L"RasGetProjectionInfo Error", MB_OK | MB_ICONERROR);
			return;
		}

		u_long ulIP = ::inet_addr(CW2A(rasip.szIpAddress));
		MIB_IPFORWARDROW route;
		bool bRouteFound = false;

		CWin32Heap heap(::GetProcessHeap());
		PVOID mem = heap.Allocate(sizeof(MIB_IPFORWARDTABLE));

		PMIB_IPFORWARDTABLE pIpForwardTable = (PMIB_IPFORWARDTABLE)mem;
		dwCb = sizeof(PMIB_IPFORWARDTABLE);

		dwErr = ::GetIpForwardTable(pIpForwardTable, &dwCb, FALSE);
		if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
		{
			mem = heap.Reallocate(mem, dwCb);
			pIpForwardTable = (PMIB_IPFORWARDTABLE)mem;
		}

		dwErr = ::GetIpForwardTable(pIpForwardTable, &dwCb, FALSE);
		if ( dwErr == ERROR_SUCCESS )
		{
			for ( u_long i = 0; i < pIpForwardTable->dwNumEntries; i++ )
			{
				if ( pIpForwardTable->table[i].dwForwardNextHop == ulIP
						&& pIpForwardTable->table[i].dwForwardDest != 0xFFFFFFFF )
				{
					::memcpy(&route, &pIpForwardTable->table[i], sizeof(MIB_IPFORWARDROW));

					dwErr = ::DeleteIpForwardEntry(&route);
					if ( dwErr != ERROR_SUCCESS )
					{
						MessageBox(GetErrorString(dwErr), L"DeleteIpForwardEntry Error", MB_OK | MB_ICONERROR);
						//return;
					}

					bRouteFound = true;
					break;
				}
			}
		}
		else
		{
			MessageBox(GetErrorString(dwErr), L"GetIpForwardTable Error", MB_OK | MB_ICONERROR);
			//return;
		}
		heap.Free(mem);

		if ( bRouteFound )
		{
			for ( int i = 0; i < conn.asRoutes.GetSize(); i++ )
			{
				CString sNet = conn.asRoutes[i].Left(conn.asRoutes[i].Find(L"/"));
				CString sMask = conn.asRoutes[i].Mid(conn.asRoutes[i].Find(L"/") + 1);

				route.dwForwardDest = ::inet_addr(CW2A(sNet));
				route.dwForwardMask = ::htonl(conn.ConvertBitsToMask(::StrToInt(sMask)));
				route.dwForwardType = MIB_IPNET_TYPE_DYNAMIC;
				route.dwForwardProto = MIB_IPPROTO_NETMGMT;

				dwErr = ::CreateIpForwardEntry(&route);
				if ( dwErr != ERROR_SUCCESS )
				{
					MessageBox(GetErrorString(dwErr), L"CreateIpForwardEntry Error", MB_OK | MB_ICONERROR);
					//return;
				}
			}
		}
	}

	if ( !conn.sKeepAlive.IsEmpty() )
		m_threadKeepAlive.AddTimer(6000, this, (DWORD_PTR)&conn, &conn.m_timer.m_h);

	if ( !conn.m_evt )
	{
		conn.m_evt.Create(NULL, TRUE, FALSE, NULL);
		m_threadDisNotify.AddHandle(conn.m_evt, this, (DWORD_PTR)&conn);
	}

	DWORD dwErr = ::RasConnectionNotification(conn.m_hRasConn, conn.m_evt, RASCN_Disconnection);
	if ( dwErr != ERROR_SUCCESS )
	{
		MessageBox(GetErrorString(dwErr), L"RasConnectionNotification Error", MB_OK | MB_ICONERROR);
		//return;
	}

	conn.m_bConnected = true;
}

void CMainDlg::RasDialog(CString& sConnName)
{
	RASENTRYDLG red = {0};
	red.dwSize = sizeof(RASENTRYDLG);
	red.hwndOwner = m_hWnd;
	red.dwFlags = RASEDFLAG_NoRename;

	LPWSTR lpszConn = NULL;
	if ( sConnName.IsEmpty() )
		red.dwFlags |= RASEDFLAG_NewTunnelEntry;
	else
		lpszConn = const_cast<LPWSTR>(sConnName.GetString());

	//if ( !::RasEntryDlg(NULL, lpszConn, &red) && red.dwError != ERROR_SUCCESS )
	//	MessageBox(GetErrorString(red.dwError), L"RasEntryDlg Error", MB_OK | MB_ICONERROR);

	::RasEntryDlg(NULL, lpszConn, &red);
}

void CMainDlg::Minimize(bool bShow)
{
	if ( bShow )
	{
		CWindow tray = ::FindWindow(L"Shell_TrayWnd", NULL);
		if ( tray.m_hWnd )
		{
			CWindow nawin = ::FindWindowEx(tray, NULL, L"TrayNotifyWnd", NULL);
			if ( nawin.m_hWnd )
			{
				CRect myrect;
				CRect narect;

				GetWindowRect(&myrect);
				nawin.GetWindowRect(&narect);

				::DrawAnimatedRects(m_hWnd, IDANI_CAPTION, &myrect, &narect);
			}
		}
	}

	NotifyIcon(true);
	ShowWindow(SW_HIDE);

	::SetProcessWorkingSetSize(::GetCurrentProcess(), -1, -1);
}

void CMainDlg::NotifyIcon(bool bShow)
{
	CIcon icon;
	icon.LoadIcon(IDR_ICON_MAIN, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));

	NOTIFYICONDATA nid = {0};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 69;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER;
	nid.hIcon = icon.m_hIcon;
	::lstrcpy(nid.szTip, L"Double-click to restore");

	::Shell_NotifyIcon(bShow ? NIM_ADD : NIM_DELETE, &nid);
}

void CMainDlg::CreateMenu()
{
	m_menu.DestroyMenu();
	m_menu.LoadMenu(IDR_MENU_POPUP);

	MENUINFO mi;
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	m_menu.GetMenuInfo(&mi);

	mi.dwStyle |= MNS_NOTIFYBYPOS;
	m_menu.SetMenuInfo(&mi);

	m_menu.GetSubMenu(0).SetMenuDefaultItem(4, TRUE);
}

CString CMainDlg::GetErrorString(DWORD dwErr)
{
	dwErr = dwErr == 0 ? ::GetLastError() : dwErr;
	CString sErr;

	LPVOID lpMsgBuf = NULL;

	if ( dwErr >= RASBASE && dwErr <= RASBASEEND )
	{
		::RasGetErrorString(dwErr, sErr.GetBuffer(256), 256);
		sErr.ReleaseBuffer();
	}
	else if ( ::FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPWSTR)&lpMsgBuf, 0, NULL) )
	{
		sErr = (LPWSTR)lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
		sErr = L"Unknown Error";

	sErr.AppendFormat(L"\n\nError code: %d (0x%x)", dwErr, dwErr);

	return sErr;
}

void CALLBACK CMainDlg::RasDialCallback(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hRasConn, UINT unMsg, RASCONNSTATE RasConnState, DWORD dwError, DWORD dwExtendedError)
{
	CMainDlg* pMainDlg = (CMainDlg*)dwCallbackId;
	CConnection* pConn = &pMainDlg->m_ConnMap.GetValueAt(0);

	for ( int i = 0; i < pMainDlg->m_ConnMap.GetSize(); i++ )
	{
		if ( pMainDlg->m_ConnMap.GetValueAt(i).m_hRasConn == hRasConn )
			pConn = &pMainDlg->m_ConnMap.GetValueAt(i);
	}

	CString sMsg;
	switch ( RasConnState )
	{
		case RASCS_OpenPort : sMsg = L"Opening port..."; break;
		//case RASCS_PortOpened : sMsg = L"Port opened..."; break;
		case RASCS_ConnectDevice : sMsg = L"Connecting to VPN server..."; break;
		//case RASCS_DeviceConnected : sMsg = L"Connected to VPN server..."; break;
		case RASCS_Authenticate : sMsg = L"Verifying user name and password..."; break;
		//case RASCS_Authenticated : sMsg = L"Authenticated..."; break;
		case RASCS_AuthProject : sMsg = L"Registering computer on the remote network..."; break;
		//case RASCS_Projected : sMsg = L"Registered..."; break;
		case RASCS_Connected : sMsg = L"Connected"; break;
		//case RASCS_Disconnected : sMsg = L"Disconnected."; break;
		//default : CString su; su.Format(L"%d", RasConnState); ::MessageBox(NULL, su, L"", MB_OK); 
	}

	if ( !sMsg.IsEmpty() && pConn->sName == pMainDlg->m_sSelectedConnection )
		pMainDlg->m_stcStatus.SetWindowText(sMsg);

	if ( dwError != ERROR_SUCCESS )
	{
		if ( pConn )
			pMainDlg->Disconnect(*pConn);

		if ( pConn->sName == pMainDlg->m_sSelectedConnection )
			pMainDlg->m_stcStatus.SetWindowText(GetErrorString(dwError));
	}
}

HRESULT CMainDlg::Execute(DWORD_PTR dwParam, HANDLE hObject)
{
	if ( m_evt == hObject )
	{
		m_evt.Reset();
		UpdateConnections();
		UpdateUI();
	}
	else
	{
		CConnection& conn = *(CConnection*)dwParam;
		if ( conn.m_evt == hObject )
		{
			conn.m_evt.Reset();
			Disconnect(conn);
		}
		else if ( conn.m_timer == hObject )
		{
			LPSTR SendData = "VPN Dialer+ Keep-Alive ICMP Echo";
			LPVOID ReplyBuffer = (LPVOID)_alloca(sizeof(ICMP_ECHO_REPLY) + sizeof(SendData));

			HANDLE hIcmp = ::IcmpCreateFile();

			::IcmpSendEcho(hIcmp, inet_addr(CW2A(conn.sKeepAlive)), 
				SendData, sizeof(SendData), NULL,
				ReplyBuffer, sizeof(ICMP_ECHO_REPLY) + sizeof(SendData), 2000);

			::IcmpCloseHandle(hIcmp);
		}
	}

	return S_OK;
}

HRESULT CMainDlg::CloseHandle(HANDLE hObject)
{
	if ( m_evt == hObject )
		m_evt.Close();
	else
		for ( int i = 0; i < m_ConnMap.GetSize(); i++ )
		{
			if ( m_ConnMap.GetValueAt(i).m_evt == hObject )
				m_ConnMap.GetValueAt(i).m_evt.Close();
			else if ( m_ConnMap.GetValueAt(i).m_timer == hObject )
				m_ConnMap.GetValueAt(i).m_timer.Close();
		}

	return S_OK;
}
