#pragma once

#include "ConfigMgr.h"

class CSettingsDlg : public CDialogImpl<CSettingsDlg>, public CWinDataExchange<CSettingsDlg>
{
friend class CMainDlg;

public:
	CSettingsDlg(CConnection& conn) : m_Conn(conn) {}
	~CSettingsDlg() {}

	enum { IDD = IDD_SETTINGSDLG };

	BEGIN_MSG_MAP(CSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
		COMMAND_HANDLER(IDC_LIST_ROUTES, LBN_SELCHANGE, OnLbnSelchangeRoutes)
		COMMAND_HANDLER(IDC_BUTTON_REMOVE, BN_CLICKED, OnBnClickedRemove)
		COMMAND_HANDLER(IDC_BUTTON_ADD, BN_CLICKED, OnBnClickedAdd)
		NOTIFY_HANDLER(IDC_IPADDRESS_NET, IPN_FIELDCHANGED, OnIpnFieldchanged)
		NOTIFY_HANDLER(IDC_IPADDRESS_MASK, IPN_FIELDCHANGED, OnIpnFieldchanged)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsDlg)
		DDX_CONTROL_HANDLE(IDC_IPADDRESS_NET, m_ipNet)
		DDX_CONTROL_HANDLE(IDC_IPADDRESS_MASK, m_ipMask)
		DDX_CONTROL_HANDLE(IDC_IPADDRESS_KEEPALIVE, m_ipKeepAlive)
		DDX_CONTROL_HANDLE(IDC_LIST_ROUTES, m_lstRoutes)
		DDX_CONTROL_HANDLE(IDC_BUTTON_ADD, m_btnAdd)
		DDX_CONTROL_HANDLE(IDC_BUTTON_REMOVE, m_btnRemove)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClickedOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLbnSelchangeRoutes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnIpnFieldchanged(int /*idCtrl*/, LPNMHDR /*pNMHDR*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CConnection& m_Conn;

	CIPAddressCtrl m_ipNet;
	CIPAddressCtrl m_ipMask;
	CIPAddressCtrl m_ipKeepAlive;
	CListBox m_lstRoutes;
	CButton m_btnAdd;
	CButton m_btnRemove;
};
