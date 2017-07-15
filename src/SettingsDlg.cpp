#include "stdafx.h"
#include "resource.h"

#include "SettingsDlg.h"

LRESULT CSettingsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	SetWindowText(m_Conn.sName + L" Dial Settings");
	DoDataExchange(DDX_LOAD);

	for ( int i = 0; i < m_Conn.asRoutes.GetSize(); i++ )
	{
		m_lstRoutes.AddString(m_Conn.asRoutes[i]);
	}

	if ( !m_Conn.sKeepAlive.IsEmpty() )
	{
		u_long ulKeepAlive = ::ntohl(::inet_addr(CW2A(m_Conn.sKeepAlive)));
		m_ipKeepAlive.SetAddress(ulKeepAlive);
	}

	return TRUE;
}

LRESULT CSettingsDlg::OnClickedOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_Conn.asRoutes.RemoveAll();
	for ( int i = 0; i < m_lstRoutes.GetCount(); i++ )
	{
		CString sItem;
		m_lstRoutes.GetText(i, sItem);
		m_Conn.asRoutes.Add(sItem);
	}

	u_long ulKeepAlive;
	m_ipKeepAlive.GetAddress(&ulKeepAlive);

	if ( ulKeepAlive != 0 )
	{
		in_addr iaKeepAlive;
		iaKeepAlive.S_un.S_addr = ::htonl(ulKeepAlive);

		m_Conn.sKeepAlive = CA2W(::inet_ntoa(iaKeepAlive));
	}

	DoDataExchange(DDX_SAVE);
	EndDialog(TRUE);

	return 0;
}

LRESULT CSettingsDlg::OnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(FALSE);

	return 0;
}

LRESULT CSettingsDlg::OnLbnSelchangeRoutes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_btnRemove.EnableWindow();

	return 0;
}

LRESULT CSettingsDlg::OnIpnFieldchanged(int /*idCtrl*/, LPNMHDR /*pNMHDR*/, BOOL& /*bHandled*/)
{
	if ( m_ipNet.IsBlank() || m_ipMask.IsBlank() )
		m_btnAdd.EnableWindow(FALSE);
	else
		m_btnAdd.EnableWindow();

	return 0;
}

LRESULT CSettingsDlg::OnBnClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_btnAdd.EnableWindow(FALSE);

	u_long ulNet, ulMask, ulMaskT;
	m_ipNet.GetAddress(&ulNet);
	m_ipMask.GetAddress(&ulMaskT);

	ulMask = 0;
	while ( (ulMaskT & 0x80000000) == 0x80000000 )
	{
		ulMask >>= 1;
		ulMask |= 0x80000000;
		ulMaskT <<= 1;
	}
	ulNet &= ulMask;

	in_addr iaNet;
	iaNet.S_un.S_addr = ::htonl(ulNet);

	CString sRoute = CA2W(::inet_ntoa(iaNet));
	sRoute.AppendFormat(L"/%d", CConnection::ConvertMaskToBits(ulMask));

	m_lstRoutes.AddString(sRoute);

	m_ipNet.ClearAddress();
	m_ipMask.ClearAddress();

	return 0;
}

LRESULT CSettingsDlg::OnBnClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_btnRemove.EnableWindow(FALSE);
	m_btnAdd.EnableWindow();

	CString sSel;
	m_lstRoutes.GetText(m_lstRoutes.GetCurSel(), sSel);

	CString sNet = sSel.Left(sSel.Find(L"/"));
	CString sMask = sSel.Mid(sSel.Find(L"/") + 1);

	u_long ulNet = ::ntohl(::inet_addr(CW2A(sNet)));
	u_long ulMask = CConnection::ConvertBitsToMask(StrToInt(sMask));

	m_ipNet.SetAddress(ulNet);
	m_ipMask.SetAddress(ulMask);

	m_lstRoutes.DeleteString(m_lstRoutes.GetCurSel());

	return 0;
}