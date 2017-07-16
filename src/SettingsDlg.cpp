#include "stdafx.h"
#include "resource.h"

#include "SettingsDlg.h"

LRESULT CSettingsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CString sTitle;
	sTitle.Format(IDS_FMT_SETTINGSTITLE, m_Conn.sName.GetString());
	SetWindowText(sTitle);

	CenterWindow(GetParent());
	DoDataExchange(DDX_LOAD);

	for ( int i = 0; i < m_Conn.asRoutes.GetSize(); i++ )
	{
		m_lstRoutes.AddString(m_Conn.asRoutes[i]);
	}

	if ( !m_Conn.sKeepAlive.IsEmpty() )
	{
		IPAddr ulKeepAlive = ::inet_addr(CW2A(m_Conn.sKeepAlive));
		m_ipKeepAlive.SetAddress(::ntohl(ulKeepAlive));
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

	IPAddr ulKeepAlive;
	m_ipKeepAlive.GetAddress(&ulKeepAlive);

	m_Conn.sKeepAlive.Empty();
	if ( ulKeepAlive != 0 )
	{
		IN_ADDR iaKeepAlive;
		iaKeepAlive.s_addr = ::htonl(ulKeepAlive);

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

	IPAddr ulNet, ulMask, ulMaskT;
	m_ipNet.GetAddress(&ulNet);
	m_ipMask.GetAddress(&ulMaskT);

	ulMask = 0;
	while ( (ulMaskT & 0x80000000UL) == 0x80000000UL )
	{
		ulMask = ulMask >> 1 | 0x80000000UL;
		ulMaskT <<= 1;
	}
	ulNet &= ulMask;

	IN_ADDR iaNet, iaMask;
	iaNet.s_addr = ::htonl(ulNet);
	::ConvertIpv4MaskToLength(::htonl(ulMask), &iaMask.s_net);

	CString sRoute;
	sRoute.Format(L"%s/%d", CA2W(::inet_ntoa(iaNet)).m_psz, static_cast<int>(iaMask.s_net));
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

	IN_ADDR iaNet, iaMask;
	LPCWSTR szRoute = sSel;
	::RtlIpv4StringToAddress(szRoute, TRUE, &szRoute, &iaNet);
	::ConvertLengthToIpv4Mask(::StrToInt(szRoute + 1), &iaMask.s_addr);

	m_ipNet.SetAddress(::ntohl(iaNet.s_addr));
	m_ipMask.SetAddress(::ntohl(iaMask.s_addr));

	m_lstRoutes.DeleteString(m_lstRoutes.GetCurSel());

	return 0;
}