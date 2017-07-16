#include "stdafx.h"
#include "resource.h"

#include "LogonDlg.h"

LRESULT CLogonDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CString sTitle;
	sTitle.Format(IDS_FMT_LOGONTITLE, m_sConnection.GetString());
	SetWindowText(sTitle);

	CenterWindow(GetParent());
	DoDataExchange(DDX_LOAD);

	return TRUE;
}

LRESULT CLogonDlg::OnClickedOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if ( DoDataExchange(DDX_SAVE) )
		EndDialog(TRUE);

	return 0;
}

LRESULT CLogonDlg::OnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(FALSE);

	return 0;
}
