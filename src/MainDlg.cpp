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
	CenterWindow();

	HANDLE hIconLg = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ICON_MAIN), IMAGE_ICON,
	                             ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	HANDLE hIconSm = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ICON_MAIN), IMAGE_ICON,
	                             ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(static_cast<HICON>(hIconLg), TRUE);
	SetIcon(static_cast<HICON>(hIconSm), FALSE);

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	CConfigMgr config;
	if ( !config.Init() )
	{
		ReportError(config.LastError, IDS_ERR_CONFIGURATION);
		CloseDialog(0);
		return FALSE;
	}

	DoDataExchange(DDX_LOAD);
	PopulateVPNList();

	ATLENSURE_SUCCEEDED(m_threadConNotify.Initialize());
	ATLENSURE_SUCCEEDED(m_threadDisNotify.Initialize());
	ATLENSURE_SUCCEEDED(m_threadKeepAlive.Initialize());

	m_evt.Create(NULL, TRUE, FALSE, NULL);
	ATLENSURE_SUCCEEDED(m_threadConNotify.AddHandle(m_evt, this, NULL));

	DWORD dwErr = ::RasConnectionNotification(static_cast<HRASCONN>(INVALID_HANDLE_VALUE), m_evt, RASCN_Connection);
	if ( dwErr != ERROR_SUCCESS )
		return !ReportError(GetErrorString(dwErr), IDS_ERR_RASCONNECTIONNOTIFICATION);

	UpdateConnections();
	UpdateUI();
	CreateMenu();

	return TRUE;
}

LRESULT CMainDlg::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;

	if ( wParam == SC_MINIMIZE )
	{
		bHandled = TRUE;
		Minimize();
	}

	return 0;
}

LRESULT CMainDlg::OnUser(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;

	// since icon disappears on dbl click, make sure no other icon receives the buttonup msg
	if ( lParam == WM_LBUTTONDBLCLK )
	{
		m_bDblClick = true;
	}
	else if ( lParam == WM_LBUTTONUP && m_bDblClick )
	{
		m_bDblClick = false;
		OnMenuOpen(0, 0, NULL, bHandled);
	}
	else if ( lParam == WM_RBUTTONUP )
	{
		bHandled = TRUE;

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
				disc.AppendMenu(0, static_cast<UINT_PTR>(0), sConn);
			else
				conn.AppendMenu(0, static_cast<UINT_PTR>(0), sConn);
		}

		CString sNone((LPCWSTR)IDS_LBL_EMPTYLIST);
		if ( conn.GetMenuItemCount() == 0 )
			conn.AppendMenu(MF_GRAYED, static_cast<UINT_PTR>(0), sNone);
		if ( disc.GetMenuItemCount() == 0 )
			disc.AppendMenu(MF_GRAYED, static_cast<UINT_PTR>(0), sNone);

		CPoint mouse;
		::GetCursorPos(&mouse);
		::SetForegroundWindow(m_hWnd);

		popup.TrackPopupMenu(TPM_VERNEGANIMATION, mouse.x, mouse.y, m_hWnd);

		// KB 135788
		PostMessage(WM_NULL);
	}

	return 0;
}

LRESULT CMainDlg::OnMenuCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CMenuHandle mh(reinterpret_cast<HMENU>(lParam));
	int nMenu = PtrToInt(reinterpret_cast<const void*>(wParam));

	if ( mh.GetMenuItemID(nMenu) )
		SendMessage(WM_COMMAND, mh.GetMenuItemID(nMenu), lParam);
	else
	{
		CString sMenu;
		int nLen = mh.GetMenuStringLen(nMenu, MF_BYPOSITION) + 1;

		mh.GetMenuString(nMenu, sMenu.GetBuffer(nLen), nLen, MF_BYPOSITION);
		sMenu.ReleaseBuffer();

		CConnection& conn = m_ConnMap.GetValueAt(m_ConnMap.FindKey(sMenu));
		if ( conn.m_hRasConn )
			Disconnect(conn);
		else
			Connect(conn);
	}

	return 0;
}

LRESULT CMainDlg::OnMenuOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	bHandled = TRUE;

	NotifyIcon(false);
	ShowWindow(SW_SHOW);

	return ::SetForegroundWindow(m_hWnd);
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
	ATLENSURE_SUCCEEDED(m_threadConNotify.Shutdown());
	ATLENSURE_SUCCEEDED(m_threadDisNotify.Shutdown());
	ATLENSURE_SUCCEEDED(m_threadKeepAlive.Shutdown());

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

LRESULT CMainDlg::OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	RasDialog(m_sSelectedConnection);
	return 0;
}

LRESULT CMainDlg::OnBnClickedSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CConfigMgr config;
	config.Init();
	if ( !config.LoadConfig(!config.ConfigExists()) )
	{
		if ( ReportError(_S(IDS_MSG_NEWCONFIG), IDS_ERR_INVALIDCONFIG, MB_YESNO | MB_ICONEXCLAMATION) == IDNO || !config.LoadConfig(true))
			return ReportError(config.LastError, IDS_ERR_CONFIGURATION);
	}

	CSettingsDlg setdlg(m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)));
	setdlg.m_Conn.asRoutes.RemoveAll();
	setdlg.m_Conn.sKeepAlive.Empty();

	config.LoadConnection(setdlg.m_Conn);

	if ( setdlg.DoModal() && !config.SaveConnection(setdlg.m_Conn) )
		return ReportError(config.LastError, IDS_ERR_CONFIGURATION);

	return 0;
}

LRESULT CMainDlg::OnBnClickedNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CString sEmpty;
	RasDialog(sEmpty);
	PopulateVPNList();

	return 0;
}

LRESULT CMainDlg::OnCbnSelchangeConnections(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if ( m_cboConnections.GetCurSel() == CB_ERR )
		m_sSelectedConnection.Empty();
	else
	{
		int nLen = m_cboConnections.GetLBTextLen(m_cboConnections.GetCurSel());
		m_cboConnections.GetLBText(m_cboConnections.GetCurSel(), m_sSelectedConnection.GetBuffer(nLen));
		m_sSelectedConnection.ReleaseBuffer();
	}

	UpdateUI();
	return 0;
}

bool CMainDlg::IsVpnConnection(LPCWSTR pszEntryName)
{
	DWORD dwCb = 0;
	DWORD dwErr = ::RasGetEntryProperties(NULL, pszEntryName, NULL, &dwCb, NULL, NULL);
	if ( dwErr != ERROR_BUFFER_TOO_SMALL )
		return !ReportError(GetErrorString(dwErr), IDS_ERR_RASGETENTRYPROPERTIES);

	CHeapPtr<RASENTRY> rasEntry;
	ATLENSURE(rasEntry.AllocateBytes(dwCb));
	::memset(rasEntry.m_pData, 0, dwCb);
	rasEntry->dwSize = sizeof(RASENTRY);

	dwErr = ::RasGetEntryProperties(NULL, pszEntryName, rasEntry, &dwCb, NULL, NULL);
	if ( dwErr != ERROR_SUCCESS )
		return !ReportError(GetErrorString(dwErr), IDS_ERR_RASGETENTRYPROPERTIES);

	return rasEntry->dwType == RASET_Vpn;
}

void CMainDlg::PopulateVPNList()
{
	DWORD dwCb = 0;
	DWORD dwEntries = 0;
	DWORD dwErr = ::RasEnumEntries(NULL, NULL, NULL, &dwCb, &dwEntries);
	if ( dwErr == ERROR_SUCCESS )
		return;

	if ( dwErr != ERROR_BUFFER_TOO_SMALL )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASENUMENTRIES);
		return;
	}

	CHeapPtr<RASENTRYNAME> rasEntryNames;
	ATLENSURE(rasEntryNames.AllocateBytes(dwCb));
	::memset(rasEntryNames.m_pData, 0, dwCb);
	rasEntryNames->dwSize = sizeof(RASENTRYNAME);

	dwErr = ::RasEnumEntries(NULL, NULL, rasEntryNames, &dwCb, &dwEntries);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASENUMENTRIES);
		return;
	}

	CConfigMgr config;
	bool configure = config.Init() && config.LoadConfig();

	for ( DWORD i = 0; i < dwEntries; i++ )
	{
		CString sName = rasEntryNames[i].szEntryName;
		if ( IsVpnConnection(sName) && m_ConnMap.FindKey(sName) == -1 )
		{
			CConnection conn(sName);
			if ( configure )
				config.LoadConnection(conn);

			m_ConnMap.Add(sName, conn);
			m_cboConnections.AddString(sName);
		}
	}
}

void CMainDlg::UpdateConnections()
{
	DWORD dwCb = 0;
	DWORD dwEntries = 0;
	DWORD dwErr = ::RasEnumConnections(NULL, &dwCb, &dwEntries);
	if ( dwErr == ERROR_SUCCESS )
		return;

	if ( dwErr != ERROR_BUFFER_TOO_SMALL )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASENUMCONNECTIONS);
		return;
	}

	CHeapPtr<RASCONN> rasConns;
	ATLENSURE(rasConns.AllocateBytes(dwCb));
	::memset(rasConns.m_pData, 0, dwCb);
	rasConns->dwSize = sizeof(RASCONN);

	dwErr = ::RasEnumConnections(rasConns, &dwCb, &dwEntries);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASENUMCONNECTIONS);
		return;
	}

	for ( DWORD i = 0; i < dwEntries; i++ )
	{
		RASCONNSTATUS rcs = {0};
		rcs.dwSize = sizeof(RASCONNSTATUS);

		::RasGetConnectStatus(rasConns[i].hrasconn, &rcs);
		if ( rcs.rasconnstate == RASCS_Connected )
		{
			int nPos = m_ConnMap.FindKey(CString(rasConns[i].szEntryName));
			if ( nPos != -1 )
			{
				CConnection& conn = m_ConnMap.GetValueAt(nPos);
				conn.m_hRasConn = rasConns[i].hrasconn;

				if ( !conn.m_bConnected )
					PostConnect(conn);
			}
		}
	}
}

void CMainDlg::UpdateUI()
{
	if ( m_sSelectedConnection.IsEmpty() )
	{
		m_stcStatus.SetWindowText(EMPTY_STRING);

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

		if ( m_ConnMap.GetValueAt(m_ConnMap.FindKey(m_sSelectedConnection)).m_hRasConn )
		{
			m_btnConnect.ShowWindow(SW_HIDE);
			m_btnDisconnect.ShowWindow(SW_SHOW);

			m_stcStatus.SetWindowText(_S(IDS_STATUS_CONNECTED));
		}
		else
		{
			m_btnConnect.ShowWindow(SW_SHOW);
			m_btnDisconnect.ShowWindow(SW_HIDE);

			m_stcStatus.SetWindowText(EMPTY_STRING);
		}
	}
}

void CMainDlg::Connect(CConnection& conn)
{
	RASENTRY rasEntry = {0};
	rasEntry.dwSize = sizeof(RASENTRY);

	DWORD dwErr = ::RasGetEntryProperties(NULL, conn.sName, &rasEntry, &rasEntry.dwSize, NULL, NULL);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASGETENTRYPROPERTIES);
		return;
	}

	BOOL bDoLogon = (rasEntry.dwfOptions & RASEO_UseLogonCredentials) != RASEO_UseLogonCredentials;
	BOOL bPasswordSaved = FALSE;

	RASDIALPARAMS rasDialParams = {0};
	rasDialParams.dwSize = sizeof(RASDIALPARAMS);
	ATLENSURE_SUCCEEDED(::StringCchCopy(rasDialParams.szEntryName, RAS_MaxEntryName, conn.sName));

	dwErr = ::RasGetEntryDialParams(NULL, &rasDialParams, &bPasswordSaved);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASDIALGETENTRYDIALPARAMS);
		return;
	}

	if ( !bPasswordSaved && bDoLogon )
	{
		CLogonDlg logondlg;
		logondlg.m_sUser = rasDialParams.szUserName;
		logondlg.m_sConnection = m_sSelectedConnection;
		
		if ( !logondlg.DoModal() )
			return;

		ATLENSURE_SUCCEEDED(::StringCchCopy(rasDialParams.szUserName, UNLEN, logondlg.m_sUser));
		ATLENSURE_SUCCEEDED(::StringCchCopy(rasDialParams.szPassword, PWLEN, logondlg.m_sPass));
	}

	rasDialParams.dwCallbackId = reinterpret_cast<ULONG_PTR>(this);

	dwErr = ::RasDial(NULL, NULL, &rasDialParams, 2, &CMainDlg::RasDialCallback, &conn.m_hRasConn);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASDIAL);
		return;
	}

	UpdateUI();
}

void CMainDlg::Disconnect(CConnection& conn)
{
	CCursor cur;
	cur.LoadSysCursor(IDC_WAIT);
	SetCursor(cur);
	m_btnDisconnect.EnableWindow(FALSE);

	::RasHangUp(conn.m_hRasConn);
	conn.m_bConnected = false;
	conn.m_hRasConn = NULL;
	if (conn.m_timer)
	{
		ATLENSURE_SUCCEEDED(m_threadKeepAlive.RemoveHandle(conn.m_timer));
		conn.m_timer.m_h = NULL;
	}

	m_btnDisconnect.EnableWindow();
	cur.DestroyCursor();
	cur.LoadSysCursor(IDC_ARROW);
	SetCursor(cur);

	UpdateUI();
}

void CMainDlg::PostConnect(CConnection& conn)
{
	if ( conn.asRoutes.GetSize() > 0 )
	{
		if ( conn.sName == m_sSelectedConnection )
		{
			m_stcStatus.SetWindowText(_S(IDS_STATUS_ADDINGROUTES));
			::Sleep(600);
		}

		RASPPPIP rasIP = {0};
		rasIP.dwSize = sizeof(rasIP);

		DWORD dwCb = sizeof(rasIP);
		DWORD dwErr = ::RasGetProjectionInfo(conn.m_hRasConn, RASP_PppIp, &rasIP, &dwCb);
		if ( dwErr != ERROR_SUCCESS )
		{
			ReportError(GetErrorString(dwErr), IDS_ERR_RASGETPROJECTIONINFO);
			return;
		}

		IPAddr ulIP = ::inet_addr(CW2A(rasIP.szIpAddress));

		dwCb = 0;
		dwErr = ::GetIpForwardTable(NULL, &dwCb, FALSE);
		if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
		{
			ReportError(GetErrorString(dwErr), IDS_ERR_GETIPFORWARDTABLE);
			return;
		}

		CHeapPtr<MIB_IPFORWARDTABLE> routeTable;
		ATLENSURE(routeTable.AllocateBytes(dwCb));
		::memset(routeTable.m_pData, 0, dwCb);

		dwErr = ::GetIpForwardTable(routeTable, &dwCb, FALSE);
		if ( dwErr != ERROR_SUCCESS )
		{
			ReportError(GetErrorString(dwErr), IDS_ERR_GETIPFORWARDTABLE);
			//return;
		}

		bool bRouteFound = false;
		MIB_IPFORWARDROW route = {0};
		for ( u_long i = 0; i < routeTable->dwNumEntries; i++ )
		{
			if ( routeTable->table[i].dwForwardNextHop == ulIP && routeTable->table[i].dwForwardDest != ~0UL )
			{
				::memcpy(&route, &routeTable->table[i], sizeof(MIB_IPFORWARDROW));

				dwErr = ::DeleteIpForwardEntry(&route);
				if ( dwErr != ERROR_SUCCESS )
				{
					ReportError(GetErrorString(dwErr), IDS_ERR_DELETEIPFORWARDENTRY);
					//return;
				}

				bRouteFound = true;
				break;
			}
		}

		if ( bRouteFound )
		{
			for ( int i = 0; i < conn.asRoutes.GetSize(); i++ )
			{
				IN_ADDR iaNet, iaMask;
				LPCWSTR szRoute = conn.asRoutes[i];
				::RtlIpv4StringToAddress(szRoute, TRUE, &szRoute, &iaNet);
				::ConvertLengthToIpv4Mask(::StrToInt(szRoute + 1), &iaMask.s_addr);

				route.dwForwardDest = iaNet.s_addr;
				route.dwForwardMask = iaMask.s_addr;
				route.dwForwardType = MIB_IPNET_TYPE_DYNAMIC;
				route.dwForwardProto = MIB_IPPROTO_NETMGMT;

				dwErr = ::CreateIpForwardEntry(&route);
				if ( dwErr != ERROR_SUCCESS )
				{
					ReportError(GetErrorString(dwErr), IDS_ERR_CREATEIPFORWARDENTRY);
					//return;
				}
			}
		}
	}

	if ( !conn.sKeepAlive.IsEmpty() )
		ATLENSURE_SUCCEEDED(m_threadKeepAlive.AddTimer(6000, this, (DWORD_PTR)&conn, &conn.m_timer.m_h));

	if ( !conn.m_evt )
	{
		conn.m_evt.Create(NULL, TRUE, FALSE, NULL);
		ATLENSURE_SUCCEEDED(m_threadDisNotify.AddHandle(conn.m_evt, this, (DWORD_PTR)&conn));
	}

	DWORD dwErr = ::RasConnectionNotification(conn.m_hRasConn, conn.m_evt, RASCN_Disconnection);
	if ( dwErr != ERROR_SUCCESS )
	{
		ReportError(GetErrorString(dwErr), IDS_ERR_RASCONNECTIONNOTIFICATION);
		//return;
	}

	conn.m_bConnected = true;
}

void CMainDlg::RasDialog(CString& sConnName)
{
	RASENTRYDLG rasDlg = {0};
	rasDlg.dwSize = sizeof(RASENTRYDLG);
	rasDlg.hwndOwner = m_hWnd;
	rasDlg.dwFlags = RASEDFLAG_NoRename;

	LPWSTR lpszConn = NULL;
	if ( sConnName.IsEmpty() )
		rasDlg.dwFlags |= RASEDFLAG_NewTunnelEntry;
	else
		lpszConn = sConnName.GetBuffer(0);

	if ( !::RasEntryDlg(NULL, lpszConn, &rasDlg) && rasDlg.dwError != ERROR_SUCCESS )
		ReportError(GetErrorString(rasDlg.dwError), IDS_ERR_RASENTRYDLG);

}

void CMainDlg::Minimize()
{
	NotifyIcon(true);
	ShowWindow(SW_HIDE);

	::SetProcessWorkingSetSize(::GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
}

void CMainDlg::NotifyIcon(bool bShow)
{
	CIcon icon;
	icon.LoadIcon(IDR_ICON_MAIN, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));

	NOTIFYICONDATA nid = {0};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 75;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER;
	nid.hIcon = icon.m_hIcon;
	ATLENSURE_SUCCEEDED(::StringCchCopy(nid.szTip, _countof(nid.szTip), _S(IDS_NIF_TIP)));

	::Shell_NotifyIcon(bShow ? NIM_ADD : NIM_DELETE, &nid);
}

void CMainDlg::CreateMenu()
{
	m_menu.DestroyMenu();
	m_menu.LoadMenu(IDR_MENU_POPUP);

	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	m_menu.GetMenuInfo(&mi);

	mi.dwStyle |= MNS_NOTIFYBYPOS;
	m_menu.SetMenuInfo(&mi);

	m_menu.GetSubMenu(0).SetMenuDefaultItem(4, TRUE);
}

int CMainDlg::ReportError(LPCWSTR szErr, UINT_PTR nRes, UINT nType)
{
	return MessageBox(szErr, _S(nRes), nType);
}

CString CMainDlg::GetErrorString(DWORD dwErr)
{
	dwErr = dwErr == 0 ? ::GetLastError() : dwErr;
	CString sErr;
	LPWSTR lpMsgBuf = NULL;

	if ( dwErr >= RASBASE && dwErr <= RASBASEEND )
	{
		::RasGetErrorString(dwErr, sErr.GetBuffer(512), 512);
		sErr.ReleaseBuffer();
	}
	else if ( ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
	                          dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, NULL) )
	{
		sErr = lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
		ATLENSURE(sErr.LoadString(IDS_ERR_UNKNOWN));

	sErr.AppendFormat(IDS_FMT_ERRORCODE, static_cast<long>(dwErr), dwErr);
	return sErr;
}

void CALLBACK CMainDlg::RasDialCallback(ULONG_PTR dwCallbackId, DWORD /*dwSubEntry*/, HRASCONN hRasConn, UINT /*unMsg*/, RASCONNSTATE RasConnState, DWORD dwError, DWORD /*dwExtendedError*/)
{
	CMainDlg* pMainDlg = reinterpret_cast<CMainDlg*>(dwCallbackId);
	CConnection* pConn = NULL;

	for ( int i = 0; i < pMainDlg->m_ConnMap.GetSize(); i++ )
	{
		CConnection& conn = pMainDlg->m_ConnMap.GetValueAt(i);
		if ( conn.m_hRasConn == hRasConn )
			pConn = &conn;
	}

	if ( !pConn )
		return;

	CString sMsg;
	switch ( RasConnState )
	{
		case RASCS_OpenPort      : ATLENSURE(sMsg.LoadString(IDS_STATUS_OPENINGPORT)); break;
		case RASCS_ConnectDevice : ATLENSURE(sMsg.LoadString(IDS_STATUS_CONNECTING)); break;
		case RASCS_Authenticate  : ATLENSURE(sMsg.LoadString(IDS_STATUS_AUTHENTICATING)); break;
		case RASCS_AuthProject   : ATLENSURE(sMsg.LoadString(IDS_STATUS_PROJECTING)); break;
		case RASCS_Connected     : ATLENSURE(sMsg.LoadString(IDS_STATUS_CONNECTED)); break;
	}

	if ( !sMsg.IsEmpty() && pConn->sName == pMainDlg->m_sSelectedConnection )
		pMainDlg->m_stcStatus.SetWindowText(sMsg);

	if ( dwError != ERROR_SUCCESS )
	{
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
		CConnection& conn = *reinterpret_cast<CConnection*>(dwParam);
		if ( conn.m_evt == hObject )
		{
			conn.m_evt.Reset();
			Disconnect(conn);
		}
		else if ( conn.m_timer == hObject )
		{
			CHAR SendData[] = "VPN Dialer+ Keep-Alive ICMP Echo";
			CHAR ReplyBuffer[sizeof(ICMP_ECHO_REPLY) + _countof(SendData) - 1 + 8] = {0};

			HANDLE hIcmp = ::IcmpCreateFile();
			::IcmpSendEcho(hIcmp, inet_addr(CW2A(conn.sKeepAlive)), SendData, _countof(SendData) - 1, NULL, ReplyBuffer, _countof(ReplyBuffer), 2000);
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
			CConnection& conn = m_ConnMap.GetValueAt(i);
			if ( conn.m_evt == hObject )
				conn.m_evt.Close();
			else if ( conn.m_timer == hObject )
				conn.m_timer.Close();
		}

	return S_OK;
}
