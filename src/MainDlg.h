#pragma once

#include "ConfigMgr.h"

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>, public CMessageFilter,
                 public CIdleHandler, public CWinDataExchange<CMainDlg>, public IWorkerThreadClient
{
friend class CLogonDlg;
friend class CSettingsDlg;

public:
	CMainDlg() : m_bDblClick(false) {}
	~CMainDlg() {}

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
		MESSAGE_HANDLER(WM_USER, OnUser)
		MESSAGE_HANDLER(WM_MENUCOMMAND, OnMenuCommand)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnMenuOpen)
		COMMAND_HANDLER(IDC_BUTTON_CONNECT, BN_CLICKED, OnBnClickedConnect)
		COMMAND_HANDLER(IDC_BUTTON_DISCONNECT, BN_CLICKED, OnBnClickedDisconnect)
		COMMAND_HANDLER(IDC_BUTTON_PROPERTIES, BN_CLICKED, OnBnClickedProperties)
		COMMAND_HANDLER(IDC_BUTTON_SETTINGS, BN_CLICKED, OnBnClickedSettings)
		COMMAND_HANDLER(IDC_BUTTON_NEW, BN_CLICKED, OnBnClickedNew)
		COMMAND_HANDLER(IDC_COMBO_CONNECTIONS, CBN_SELCHANGE, OnCbnSelchangeConnections)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_BUTTON_CONNECT, m_btnConnect);
		DDX_CONTROL_HANDLE(IDC_BUTTON_DISCONNECT, m_btnDisconnect);
		DDX_CONTROL_HANDLE(IDC_BUTTON_PROPERTIES, m_btnProperties);
		DDX_CONTROL_HANDLE(IDC_BUTTON_SETTINGS, m_btnSettings);
		DDX_CONTROL_HANDLE(IDC_BUTTON_NEW, m_btnNew);
		DDX_CONTROL_HANDLE(IDC_COMBO_CONNECTIONS, m_cboConnections);
		DDX_CONTROL_HANDLE(IDC_STATIC_STATUS, m_stcStatus);
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUser(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMenuCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMenuOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelchangeConnections(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

	bool IsVpnConnection(LPCWSTR pszEntryName);
	void PopulateVPNList();
	void UpdateConnections();
	void UpdateUI();
	void Connect(CConnection& conn);
	void Disconnect(CConnection& conn);
	void PostConnect(CConnection& conn);
	void RasDialog(CString& sConnName);
	void Minimize();
	void NotifyIcon(bool bShow);
	void CreateMenu();
	int  ReportError(LPCWSTR szErr, UINT_PTR nRes, UINT nType = MB_OK | MB_ICONERROR);
	static CString GetErrorString(DWORD dwErr);

	// RasDialFunc2 
	static void CALLBACK RasDialCallback(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hRasConn, UINT unMsg, RASCONNSTATE RasConnState, DWORD dwError, DWORD dwExtendedError);

	// IWorkerThreadClient
	HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject);
	HRESULT CloseHandle(HANDLE hObject);

private:
	LPCWSTR EMPTY_STRING = L"";

	CWorkerThread<> m_threadConNotify;
	CWorkerThread<> m_threadDisNotify;
	CWorkerThread<> m_threadKeepAlive;

	CString m_sSelectedConnection;
	ConnectionMap m_ConnMap;

	CButton m_btnConnect;
	CButton m_btnDisconnect;
	CButton m_btnProperties;
	CButton m_btnSettings;
	CButton m_btnNew;
	CComboBox m_cboConnections;
	CStatic m_stcStatus;
	CEvent m_evt;

	CMenu m_menu;
	bool m_bDblClick;
};
