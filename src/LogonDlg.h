#pragma once

class CLogonDlg : public CDialogImpl<CLogonDlg>, public CWinDataExchange<CLogonDlg>
{
friend class CMainDlg;

public:
	enum { IDD = IDD_LOGONDLG };

	BEGIN_MSG_MAP(CLogonDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CLogonDlg)
		DDX_TEXT_LEN(IDC_EDIT_USER, m_sUser, UNLEN)
		DDX_TEXT_LEN(IDC_EDIT_PASS, m_sPass, PWLEN)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClickedOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClickedCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CString m_sUser;
	CString m_sPass;
	CString m_sConnection;
};
