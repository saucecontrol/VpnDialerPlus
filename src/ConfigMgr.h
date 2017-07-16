// ConfigMgr.h : interface of the CConfigMgr class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CConnection
{
public:
	//CConnection() : m_hRasConn(NULL), m_bConnected(false) {}
	~CConnection() {}

	explicit CConnection(const CConnection& conn)
	{
		sName = conn.sName;
		sKeepAlive = conn.sKeepAlive;
		asRoutes = conn.asRoutes;

		m_bConnected = conn.m_bConnected;
		m_hRasConn = conn.m_hRasConn;
		m_evt = const_cast<CConnection&>(conn).m_evt;
		m_timer = const_cast<CConnection&>(conn).m_timer;
	}

	explicit CConnection(const CString& name) : sName(name), m_hRasConn(NULL), m_bConnected(false) {}

	CString sName;
	CString sKeepAlive;
	CSimpleArray<CString> asRoutes;

	bool m_bConnected;
	HRASCONN m_hRasConn;
	CEvent m_evt;
	CHandle m_timer;
};

typedef CSimpleMap<CString, CConnection, CSimpleMapEqualHelperFalse<CString, CConnection> > ConnectionMap;

class CConfigMgr
{
public:
	CConfigMgr() {}
	~CConfigMgr() {}

	bool Init();
	bool ConfigExists();
	bool LoadConfig(bool bCreate = false);
	bool LoadConnection(CConnection& conn);
	bool SaveConnection(const CConnection& conn);

	CString LastError;

private:
	LPCWSTR CONFIG_FOLDER         = L"VPN Dialer+";
	LPCWSTR CONFIG_FILE           = L"VpnDialerPlus.config.xml";
	LPCWSTR CONFIG_ELM_CONNECTION = L"Connection";
	LPCWSTR CONFIG_ELM_ADDROUTE   = L"AddRoute";
	LPCWSTR CONFIG_ATTR_NAME      = L"Name";
	LPCWSTR CONFIG_ATTR_ADDRESS   = L"Address";
	LPCWSTR CONFIG_ATTR_MASKBITS  = L"MaskBits";
	LPCWSTR CONFIG_ATTR_KEEPALIVE = L"KeepAlive";

	CComPtr<IXMLDOMDocument2> m_pXml;
	CComQIPtr<IXMLDOMElement> m_pElmRoot;

	BSTR GetXPath(LPCWSTR lpszConn);
	void SetErrorFromCOM();
	CString m_sPath;
};
