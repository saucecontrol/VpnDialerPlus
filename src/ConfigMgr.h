// ConfigMgr.h : interface of the CConfigMgr class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CConnection
{
public:
	CConnection() : m_hRasConn(NULL), m_bConnected(false) {}
	~CConnection() {}

	explicit CConnection(const CConnection& conn)
	{
		sName = conn.sName;
		sKeepAlive = conn.sKeepAlive;
		asRoutes = conn.asRoutes;

		m_evt = const_cast<CConnection&>(conn).m_evt;
		m_hRasConn = conn.m_hRasConn;
		m_bConnected = conn.m_bConnected;
	}

	static u_long ConvertBitsToMask(int nBits);
	static int ConvertMaskToBits(u_long ulMask);

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
	bool SaveConnection(CConnection& conn);

	CString LastError;

private:
	CComPtr<IXMLDOMDocument2> m_pXml;
	CComQIPtr<IXMLDOMElement> m_pElmRoot;

	void SetErrorFromCOM();
	CString m_sPath;
};
