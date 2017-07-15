// ConfigMgr.cpp : implementation of the CConfigMgr class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ConfigMgr.h"

u_long CConnection::ConvertBitsToMask(int nBits)
{
	u_long ulMask = 0;
	for ( int i = 0; i < nBits; i++ )
		ulMask = _lrotr(++ulMask, 1);

	return ulMask;
}

int CConnection::ConvertMaskToBits(u_long ulMask)
{
	int nBits = 0;
	while ( ((ulMask = _lrotl(ulMask, 1)) & 1) == 1 )
		nBits++;

	return nBits;
}

bool CConfigMgr::Init()
{
	if ( m_pXml )
		return true;

	HRESULT hr = m_pXml.CoCreateInstance(__uuidof(DOMDocument60));
	if ( FAILED(hr) )
	{
		LastError = L"Failed to create MSXML2.DOMDocument.6.0.  Please check MSXML 6.0 installation.";
		return false;
	}

	CComPtr<IXMLDOMDocument2> pXsd;
	ATLENSURE_SUCCEEDED(pXsd.CoCreateInstance(__uuidof(DOMDocument60)));

	m_pXml->put_async(VARIANT_FALSE);
	m_pXml->setProperty(L"newParser", CComVariant(true));

	pXsd->put_async(VARIANT_FALSE);
	pXsd->setProperty(L"newParser", CComVariant(true));

	CResource res;
	res.Load(L"XML", L"config.xsd");
	CComBSTR xsd(res.GetSize(), static_cast<LPCSTR>(res.Lock()));

	VARIANT_BOOL bS = VARIANT_FALSE;
	pXsd->loadXML(xsd, &bS);

	CComPtr<IXMLDOMSchemaCollection2> pSC;
	ATLENSURE_SUCCEEDED(pSC.CoCreateInstance(__uuidof(XMLSchemaCache60)));

	pSC->add(NULL, CComVariant(pXsd));
	m_pXml->putref_schemas(CComVariant(pSC));

	LPWSTR pwszPath = m_sPath.GetBuffer(MAX_PATH + 1);
	::SHGetFolderPathAndSubDir(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, L"VPN Dialer+", pwszPath);
	::PathAppend(pwszPath, L"VpnDialerPlus.config.xml");
	m_sPath.ReleaseBuffer();

	return true;
}

bool CConfigMgr::ConfigExists()
{
	return ::PathFileExists(m_sPath);
}

bool CConfigMgr::LoadConfig(bool bCreate)
{
	ATLASSERT(m_pXml);
	if ( m_pElmRoot )
		return true;

	HRESULT hr = NULL;
	LastError.Empty();

	VARIANT_BOOL bS = VARIANT_FALSE;
	hr = m_pXml->load(CComVariant(m_sPath), &bS);
	if ( SUCCEEDED(hr) && bS == VARIANT_TRUE )
	{
		m_pXml->get_documentElement(&m_pElmRoot);
		return true;
	}
	else
	{
		if ( bCreate )
		{
			CResource res;
			res.Load(L"XML", L"config.xml");
			CComBSTR xml(res.GetSize(), static_cast<LPCSTR>(res.Lock()));

			hr = m_pXml->loadXML(xml, &bS);
			if ( SUCCEEDED(hr) && bS == VARIANT_TRUE )
			{
				hr = m_pXml->save(CComVariant(m_sPath));
				if ( FAILED(hr) )
				{
					SetErrorFromCOM();
					return false;
				}
			}
			else
			{
				SetErrorFromCOM();
				return false;
			}

			m_pXml->get_documentElement(&m_pElmRoot);
			return true;
		}

		SetErrorFromCOM();
		return false;
	}
}

bool CConfigMgr::LoadConnection(CConnection& conn)
{
	ATLASSERT(m_pElmRoot);

	CComBSTR bstrXPath(L"Connection[@Name=\"");
	bstrXPath += conn.sName;
	bstrXPath += "\"]";

	CComPtr<IXMLDOMNode> pNode;
	HRESULT hr = m_pElmRoot->selectSingleNode(bstrXPath, &pNode);
	if ( FAILED(hr) || !pNode )
		return false;

	CComQIPtr<IXMLDOMElement> pElm(pNode);
	CComVariant vt;

	pElm->getAttribute(L"KeepAlive", &vt);
	conn.sKeepAlive = vt.bstrVal;
	vt.Clear();

	CComPtr<IXMLDOMNodeList> pNL;
	pElm->get_childNodes(&pNL);

	long lLen = 0;
	pNL->get_length(&lLen);

	for ( long i = 0; i < lLen; i++ )
	{
		pNode.Release();
		pNL->get_item(i, &pNode);

		CString sRoute;
		CComQIPtr<IXMLDOMElement> pElmRoute(pNode);

		pElmRoute->getAttribute(L"Address", &vt);
		sRoute = vt.bstrVal;
		vt.Clear();

		pElmRoute->getAttribute(L"MaskBits", &vt);
		sRoute.AppendFormat(L"/%s", vt.bstrVal); 
		vt.Clear();

		conn.asRoutes.Add(sRoute);
	}

	return true;
}

bool CConfigMgr::SaveConnection(CConnection &conn)
{
	ATLASSERT(m_pElmRoot);

	CComBSTR bstrXPath(L"Connection[@Name=\"");
	bstrXPath += conn.sName;
	bstrXPath += "\"]";

	CComPtr<IXMLDOMNode> pNode;
	HRESULT hr = m_pElmRoot->selectSingleNode(bstrXPath, &pNode);
	if ( FAILED(hr) || !pNode )
	{
		CComPtr<IXMLDOMElement> pElmNew;
		m_pXml->createElement(L"Connection", &pElmNew);
		pElmNew->setAttribute(L"Name", CComVariant(conn.sName));

		pNode = pElmNew;

		CComPtr<IXMLDOMNode> pNodeNew;
		m_pElmRoot->appendChild(pNode, &pNodeNew);
	}

	CComQIPtr<IXMLDOMElement> pElm(pNode);
	if ( !conn.sKeepAlive.IsEmpty() )
		pElm->setAttribute(L"KeepAlive", CComVariant(conn.sKeepAlive));
	else
		pElm->removeAttribute(L"KeepAlive");

	pNode.Release();
	while ( SUCCEEDED(pElm->get_lastChild(&pNode)) && pNode )
	{
		CComPtr<IXMLDOMNode> pNodeOld;
		pElm->removeChild(pNode, &pNodeOld);
		pNode.Release();
	}

	for ( int i = 0; i < conn.asRoutes.GetSize(); i++ )
	{
		int slashPos = conn.asRoutes[i].Find(L'/');
		CString sNet = conn.asRoutes[i].Left(slashPos);
		CString sMask = conn.asRoutes[i].Mid(slashPos + 1);

		CComPtr<IXMLDOMElement> pElmNew;
		m_pXml->createElement(L"AddRoute", &pElmNew);
		pElmNew->setAttribute(L"Address", CComVariant(sNet));
		pElmNew->setAttribute(L"MaskBits", CComVariant(sMask));

		pNode = pElmNew;

		CComPtr<IXMLDOMNode> pNodeNew;
		pElm->appendChild(pNode, &pNodeNew);
	}

	hr = m_pXml->save(CComVariant(m_sPath));
	if ( FAILED(hr) )
	{
		SetErrorFromCOM();
		return false;
	}

	return true;
}

void CConfigMgr::SetErrorFromCOM()
{
	CComPtr<IErrorInfo> pErr;

	HRESULT hr = ::GetErrorInfo(0, &pErr);
	if ( SUCCEEDED(hr) )
	{
		CComBSTR bstrErr;
		pErr->GetDescription(&bstrErr);
		LastError = bstrErr;
	}
	else
		LastError = L"Unknown COM Error";
}
