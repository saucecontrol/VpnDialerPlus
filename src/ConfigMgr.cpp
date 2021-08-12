// ConfigMgr.cpp : implementation of the CConfigMgr class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "ConfigMgr.h"

bool CConfigMgr::Init()
{
	if ( m_pXml )
		return true;

	HRESULT hr = m_pXml.CoCreateInstance(__uuidof(DOMDocument60));
	if ( FAILED(hr) )
		return !LastError.LoadString(IDS_ERR_MSXML6);

	CComPtr<IXMLDOMDocument2> pXsd;
	CComPtr<IXMLDOMSchemaCollection2> pSC;
	ATLENSURE_SUCCEEDED(pXsd.CoCreateInstance(__uuidof(DOMDocument60)));
	ATLENSURE_SUCCEEDED(pSC.CoCreateInstance(__uuidof(XMLSchemaCache60)));

	CComBSTR sProp(L"NewParser");
	CComVariant vVal(true);
	m_pXml->setProperty(sProp, vVal);
	pXsd->setProperty(sProp, vVal);

	CResource res;
	res.Load(L"XML", L"config.xsd");
	CComBSTR xsd(res.GetSize(), static_cast<LPCSTR>(res.Lock()));

	VARIANT_BOOL bS = VARIANT_FALSE;
	ATLENSURE(SUCCEEDED(pXsd->loadXML(xsd, &bS)) && bS);
	ATLENSURE_SUCCEEDED(pSC->add(NULL, CComVariant(pXsd)));
	ATLENSURE_SUCCEEDED(m_pXml->putref_schemas(CComVariant(pSC)));

	LPWSTR pwszPath = m_sPath.GetBuffer(MAX_PATH + 1);
	::SHGetFolderPathAndSubDir(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, CONFIG_FOLDER, pwszPath);
	::PathAppend(pwszPath, CONFIG_FILE);
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

	LastError.Empty();

	VARIANT_BOOL bS = VARIANT_FALSE;
	HRESULT hr = m_pXml->load(CComVariant(m_sPath), &bS);
	if ( (FAILED(hr) || !bS) && bCreate )
	{
		CResource res;
		res.Load(L"XML", L"config.xml");
		CComBSTR xml(res.GetSize(), static_cast<LPCSTR>(res.Lock()));

		ATLENSURE(SUCCEEDED(m_pXml->loadXML(xml, &bS)) && bS);

		hr = m_pXml->save(CComVariant(m_sPath));
	}

	if ( SUCCEEDED(hr) && bS )
	{
		m_pXml->get_documentElement(&m_pElmRoot);
		return true;
	}

	SetErrorFromCOM();
	return false;
}

bool CConfigMgr::LoadConnection(CConnection& conn)
{
	ATLASSERT(m_pElmRoot);

	CComBSTR bstrXPath;
	bstrXPath.Attach(GetXPath(conn.sName));

	CComPtr<IXMLDOMNode> pNode;
	HRESULT hr = m_pElmRoot->selectSingleNode(bstrXPath, &pNode);
	if ( FAILED(hr) || !pNode )
		return false;

	CComQIPtr<IXMLDOMElement> pElm(pNode);
	CComVariant vt;

	pElm->getAttribute(CComBSTR(CONFIG_ATTR_KEEPALIVE), &vt);
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

		pElmRoute->getAttribute(CComBSTR(CONFIG_ATTR_ADDRESS), &vt);
		sRoute = vt.bstrVal;
		vt.Clear();

		sRoute.AppendChar(L'/');

		pElmRoute->getAttribute(CComBSTR(CONFIG_ATTR_MASKBITS), &vt);
		sRoute.Append(vt.bstrVal); 
		vt.Clear();

		conn.asRoutes.Add(sRoute);
	}

	return true;
}

bool CConfigMgr::SaveConnection(const CConnection &conn)
{
	ATLASSERT(m_pElmRoot);

	CComBSTR bstrXPath;
	bstrXPath.Attach(GetXPath(conn.sName));

	CComPtr<IXMLDOMNode> pNode;
	HRESULT hr = m_pElmRoot->selectSingleNode(bstrXPath, &pNode);
	if ( FAILED(hr) || !pNode )
	{
		CComPtr<IXMLDOMElement> pElmNew;
		m_pXml->createElement(CComBSTR(CONFIG_ELM_CONNECTION), &pElmNew);
		pElmNew->setAttribute(CComBSTR(CONFIG_ATTR_NAME), CComVariant(conn.sName));

		pNode = pElmNew;

		CComPtr<IXMLDOMNode> pNodeNew;
		m_pElmRoot->appendChild(pNode, &pNodeNew);
	}

	CComQIPtr<IXMLDOMElement> pElm(pNode);
	if ( !conn.sKeepAlive.IsEmpty() )
		pElm->setAttribute(CComBSTR(CONFIG_ATTR_KEEPALIVE), CComVariant(conn.sKeepAlive));
	else
		pElm->removeAttribute(CComBSTR(CONFIG_ATTR_KEEPALIVE));

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
		CComBSTR sNet = conn.asRoutes[i].Left(slashPos);
		CComBSTR sMask = conn.asRoutes[i].Mid(slashPos + 1);

		CComPtr<IXMLDOMElement> pElmNew;
		m_pXml->createElement(CComBSTR(CONFIG_ELM_ADDROUTE), &pElmNew);
		pElmNew->setAttribute(CComBSTR(CONFIG_ATTR_ADDRESS), CComVariant(sNet));
		pElmNew->setAttribute(CComBSTR(CONFIG_ATTR_MASKBITS), CComVariant(sMask));

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

BSTR CConfigMgr::GetXPath(LPCWSTR lpszConn)
{
	CComBSTR bstrXPath(CONFIG_ELM_CONNECTION);
	bstrXPath += L"[@";
	bstrXPath += CONFIG_ATTR_NAME;
	bstrXPath += L"='";
	bstrXPath += lpszConn;
	bstrXPath += L"']";

	return bstrXPath.Detach();
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
		ATLENSURE(LastError.LoadString(IDS_ERR_COM));
}
