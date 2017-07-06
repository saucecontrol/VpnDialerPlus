#include "stdafx.h"
#include "resource.h"

#include "LogonDlg.h"

LRESULT CLogonDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	SetWindowText(m_sConnection + L" Authentication");
	DoDataExchange(DDX_LOAD);

	return TRUE;
}

LRESULT CLogonDlg::OnClickedOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if ( DoDataExchange(DDX_SAVE) )
		EndDialog(TRUE);

	return 0;
}

LRESULT CLogonDlg::OnClickedCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(FALSE);

	return 0;
}
