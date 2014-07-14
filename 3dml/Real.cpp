//******************************************************************************
//  Based on exauddev sample program from 
//  RealSystem (TM) G2 Software Development Kit.
//
//  Copyright (C) 1995,1996,1997 RealNetworks, Inc.
//  All rights reserved.
//
//  http://www.real.com/devzone
//
//  This program contains proprietary information of RealNetworks, Inc.,
//  and is licensed subject to restrictions on use and distribution.
//******************************************************************************
 
#include <stdio.h>
#include "Classes.h"
#include "Main.h"
#include "Parser.h"
#include "Platform.h"
#include "Real.h"
#ifdef SUPPORT_A3D
#include "ia3dapi.h"
#include "ia3dutil.h"
#endif

// Static variables.

static ULONG32 ulBitsPerSec;
static ULONG32 ulBitsPerMS;
static ULONG32 ulMSPerBlock;
static IRMACommonClassFactory *pCommonClassFactory;

#define BUFFERED_BLOCKS 40

//------------------------------------------------------------------------------
// ExampleClientAdviseSink class.
//------------------------------------------------------------------------------

ExampleClientAdviseSink::ExampleClientAdviseSink(IUnknown *pUnknown)
{
	m_lRefCount = 0;
	m_pUnknown = NULL;

	if (pUnknown) {
		IRMAPlayer *pPlayer;

		m_pUnknown = pUnknown;
		m_pUnknown->AddRef();

		if (m_pUnknown->QueryInterface(IID_IRMAPlayer, (void**)&pPlayer) 
			== PNR_OK) {
			pPlayer->AddAdviseSink(this);
			pPlayer->Release();
		}
    }
}

ExampleClientAdviseSink::~ExampleClientAdviseSink(void)
{
    if (m_pUnknown)
		m_pUnknown->Release();
}

STDMETHODIMP
ExampleClientAdviseSink::OnPosLength(UINT32 ulPosition, UINT32 ulLength)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnPresentationOpened(void)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnPresentationClosed(void)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnStatisticsChanged(void)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnPreSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnPostSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnStop(void)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnPause(UINT32 ulTime)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnBegin(UINT32 ulTime)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnBuffering(UINT32 ulFlags, UINT16 unPercentComplete)
{
	return(PNR_OK);
}

STDMETHODIMP
ExampleClientAdviseSink::OnContacting(const char *pHostName)
{
	return(PNR_OK);
}

STDMETHODIMP_(UINT32) 
ExampleClientAdviseSink::AddRef(void)
{
	return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(UINT32)
ExampleClientAdviseSink::Release(void)
{
	if (InterlockedDecrement(&m_lRefCount) > 0)
		return(m_lRefCount);
	delete this;
	return(0);
}

STDMETHODIMP 
ExampleClientAdviseSink::QueryInterface(REFIID riid, void **ppvObj)
{
	if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = (IUnknown*)(IRMAClientAdviseSink*)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMAClientAdviseSink)) {
		AddRef();
		*ppvObj = (IRMAClientAdviseSink*)this;
		return(PNR_OK);
    }
	*ppvObj = NULL;
	return(PNR_NOINTERFACE);
}

//------------------------------------------------------------------------------
// ExampleAuthenticationManager class.
//------------------------------------------------------------------------------

ExampleAuthenticationManager::ExampleAuthenticationManager()
{
		m_lRefCount = 0;
}

ExampleAuthenticationManager::~ExampleAuthenticationManager()
{
}

STDMETHODIMP
ExampleAuthenticationManager::HandleAuthenticationRequest(
	IRMAAuthenticationManagerResponse *pResponse)
{
	string username, password;

	if (get_password(&username, &password))
		pResponse->AuthenticationRequestDone(PNR_OK, username, password);
	else
		pResponse->AuthenticationRequestDone(PNR_NOT_AUTHORIZED, NULL, NULL);
	return(PNR_OK);
}

STDMETHODIMP_(UINT32)
ExampleAuthenticationManager::AddRef(void)
{
	return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(UINT32)
ExampleAuthenticationManager::Release(void)
{
	if (InterlockedDecrement(&m_lRefCount) > 0)
		return(m_lRefCount);
	delete this;
	return(0);
}

STDMETHODIMP
ExampleAuthenticationManager::QueryInterface(REFIID riid, void **ppvObj)
{
	if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = (IUnknown *)(IRMAAuthenticationManager *)this;
		return(PNR_OK);
	} else if (IsEqualIID(riid, IID_IRMAAuthenticationManager)) {
		AddRef();
		*ppvObj = (IRMAAuthenticationManager *)this;
		return PNR_OK;
	}
	*ppvObj = NULL;
	return(PNR_NOINTERFACE);
}

//------------------------------------------------------------------------------
// ExampleClientContext class.
//------------------------------------------------------------------------------

ExampleClientContext::ExampleClientContext()
{
	m_lRefCount = 0;
	m_pClientSink = NULL;
	m_pErrorSink = NULL;
	m_pAuthMgr = NULL;
	m_pSiteSupplier = NULL;
}

ExampleClientContext::~ExampleClientContext()
{
    Close();
};

void 
ExampleClientContext::Init(IUnknown *pUnknown)
{
	if ((m_pErrorSink = new ExampleErrorSink()) != NULL)
		m_pErrorSink->AddRef();   
	if ((m_pClientSink = new ExampleClientAdviseSink(pUnknown)) != NULL)
		m_pClientSink->AddRef();
	if ((m_pAuthMgr = new ExampleAuthenticationManager()) != NULL)
		m_pAuthMgr->AddRef();
	if ((m_pSiteSupplier = new ExampleSiteSupplier(pUnknown)) != NULL)
		m_pSiteSupplier->AddRef();
}

void 
ExampleClientContext::Close(void)
{
	PN_RELEASE(m_pClientSink);
	PN_RELEASE(m_pErrorSink);
	PN_RELEASE(m_pAuthMgr);
	PN_RELEASE(m_pSiteSupplier);
}

STDMETHODIMP_(UINT32)
ExampleClientContext::AddRef(void)
{
	return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(UINT32)
ExampleClientContext::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
        return(m_lRefCount);
	delete this;
	return(0);
}

STDMETHODIMP
ExampleClientContext::QueryInterface(REFIID riid, void **ppvObj)
{
	if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = this;
		return(PNR_OK);
    } else if (m_pClientSink && m_pClientSink->QueryInterface(riid, ppvObj)
		== PNR_OK)
		return(PNR_OK);
    else if (m_pErrorSink && m_pErrorSink->QueryInterface(riid, ppvObj)
		== PNR_OK)
		return(PNR_OK);
    else if (m_pAuthMgr && m_pAuthMgr->QueryInterface(riid, ppvObj) == PNR_OK)
		return(PNR_OK);
    else if (m_pSiteSupplier && m_pSiteSupplier->QueryInterface(riid, ppvObj)
		== PNR_OK)
		return(PNR_OK);
	*ppvObj = NULL;
	return(PNR_NOINTERFACE);
}

//------------------------------------------------------------------------------
// ExampleErrorSink class.
//------------------------------------------------------------------------------

ExampleErrorSink::ExampleErrorSink() 
{
	m_lRefCount = 0;
}

ExampleErrorSink::~ExampleErrorSink()
{
}

STDMETHODIMP 
ExampleErrorSink::ErrorOccurred(const UINT8	unSeverity, const ULONG32 ulRMACode,
	const ULONG32 ulUserCode, const char *pUserString, const char *pMoreInfoURL)
{
    return(PNR_OK);
}

void
ExampleErrorSink::ConvertErrorToString(const ULONG32 ulRMACode, char *pszBuffer)
{
	sprintf(pszBuffer, "Error code %d", ulRMACode);
}

STDMETHODIMP_(ULONG32)
ExampleErrorSink::AddRef(void)
{
	return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(ULONG32)
ExampleErrorSink::Release(void)
{
	if (InterlockedDecrement(&m_lRefCount) > 0)
		return(m_lRefCount);
	delete this;
	return(0);
}

STDMETHODIMP
ExampleErrorSink::QueryInterface(REFIID riid, void **ppvObj)
{
	if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = (IUnknown *)(IRMAErrorSink *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMAErrorSink)) {
		AddRef();
		*ppvObj = (IRMAErrorSink *)this;
		return(PNR_OK);
    }
    *ppvObj = NULL;
    return(PNR_NOINTERFACE);
}

//------------------------------------------------------------------------------
// ExampleWindowlessSite class.
//------------------------------------------------------------------------------

ExampleWindowlessSite::ExampleWindowlessSite(IUnknown* pContext, 
											 IUnknown* pUnkOuter)
{
	m_lRefCount = 0;
	m_pUser = NULL;
	m_pParentSite = NULL;
	m_pUnkOuter = pUnkOuter;
	m_pWatcher = NULL;
	m_pContext = pContext;
	m_pVideoSurface = NULL;
	m_bIsVisible = TRUE;
	m_pValues = NULL;
	m_lZOrder = 0;
	m_bInDestructor = FALSE;
	m_pCCF = NULL;
	m_bDamaged = FALSE;
	m_bInRedraw = FALSE;

   // addref the context

    m_pContext->AddRef();

    // get the CCF

    m_pContext->QueryInterface(IID_IRMACommonClassFactory, (void **)&m_pCCF);
	pCommonClassFactory = m_pCCF;

    // addref CCF

    m_pCCF->CreateInstance(CLSID_IRMAValues, (void **)&m_pValues);

    /* If we are not being aggregated, then point our outer
     * unknown to our own implementation of a non-delegating
     * unknown... this is like aggregating ourselves<g>
     */

    if (pUnkOuter == NULL)
		m_pUnkOuter = (IUnknown*)(NonDelegatingUnknown *)this;

    m_size.cx = 0;
    m_size.cy = 0;
    m_position.x = 0;
    m_position.y = 0;

    // create video surface

    m_pVideoSurface = new ExampleVideoSurface(m_pContext, this);
    m_pVideoSurface->AddRef();

    m_bIsChildWindow = TRUE;
}

ExampleWindowlessSite::~ExampleWindowlessSite()
{
    m_bInDestructor = TRUE;

    // Clean up all child sites

    int n = 0;
    int count = m_Children.Count();
    for (n = 0; n < count; n++) {
 		ExSiteInfo *pSiteInfo = (ExSiteInfo *)m_Children.GetFirst();
		DestroyChild(pSiteInfo->m_pSite);
		delete pSiteInfo;
		m_Children.RemoveHead();
    }

    // clean up passive site watchers 

    count = m_PassiveSiteWatchers.Count();
    for (n = 0; n < count; n++) {
		IRMAPassiveSiteWatcher *pWatcher =
			(IRMAPassiveSiteWatcher *)m_PassiveSiteWatchers.GetFirst();
		PN_RELEASE(pWatcher);
		m_PassiveSiteWatchers.RemoveHead();
    }

    // release intefaces

    PN_RELEASE(m_pValues);
    PN_RELEASE(m_pCCF);
    PN_RELEASE(m_pContext);
}

void		
ExampleWindowlessSite::SetParentSite(ExampleWindowlessSite *pParentSite)
{
    m_pParentSite = pParentSite;
}

STDMETHODIMP 
ExampleWindowlessSite::NonDelegatingQueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = (IUnknown *)(IRMASite *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMAValues)) {
		AddRef();
		*ppvObj = (IRMAValues *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMASite)) {
		AddRef();
		*ppvObj = (IRMASite *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMASite2)) {
		AddRef();
		*ppvObj = (IRMASite2 *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMASiteWindowless)) {
		AddRef();
		*ppvObj = (IRMASiteWindowless *)this;
		return(PNR_OK);
    }
    *ppvObj = NULL;
    return(PNR_NOINTERFACE);
}

STDMETHODIMP_(ULONG32) 
ExampleWindowlessSite::NonDelegatingAddRef(void)
{
    return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(ULONG32) 
ExampleWindowlessSite::NonDelegatingRelease(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
        return(m_lRefCount);
    delete this;
    return(0);
}

STDMETHODIMP 
ExampleWindowlessSite::QueryInterface(REFIID riid, void **ppvObj)
{
    return(m_pUnkOuter->QueryInterface(riid,ppvObj));
}

STDMETHODIMP_(ULONG32) 
ExampleWindowlessSite::AddRef(void)
{
    return(m_pUnkOuter->AddRef());
}

STDMETHODIMP_(ULONG32) 
ExampleWindowlessSite::Release(void)
{
    return(m_pUnkOuter->Release());
}

STDMETHODIMP 
ExampleWindowlessSite::AttachUser(IRMASiteUser *pUser)
{
    HRESULT result = PNR_FAIL;

    if (m_pUser) 
		return(PNR_UNEXPECTED);

    IRMASite *pOuterSite;
    m_pUnkOuter->QueryInterface(IID_IRMASite, (void**)&pOuterSite);
    if (pOuterSite) {
		result = pUser->AttachSite(pOuterSite);
		pOuterSite->Release();
		if (result == PNR_OK) {
			m_pUser = pUser;
			m_pUser->AddRef();
		}
   }
   return(result);
}

STDMETHODIMP 
ExampleWindowlessSite::DetachUser(void)
{
    HRESULT result;

    if (!m_pUser) 
		return(PNR_UNEXPECTED);

    result = m_pUser->DetachSite();
    if (result == PNR_OK) {
		m_pUser->Release();
		m_pUser = NULL;
    }
    return(result);
}

STDMETHODIMP 
ExampleWindowlessSite::GetUser(REF(IRMASiteUser *)pUser)
{
    if (!m_pUser) 
		return(PNR_UNEXPECTED);
    pUser = m_pUser;
    pUser->AddRef();
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::CreateChild(REF(IRMASite *)pChildSite)
{
    /* Create an instance of ExampleWindowlessSite, let it know it's
     * in child window mode.
     */
    ExampleWindowlessSite *pChildSiteWindowless = 
		new ExampleWindowlessSite(m_pContext);
    pChildSiteWindowless->AddRef();
    pChildSiteWindowless->SetParentSite(this);

    /* Get the IRMASite interface from the child to return to the
     * outside world.
     */
    pChildSiteWindowless->QueryInterface(IID_IRMASite, (void **)&pChildSite);

    /*
     * Store the ExampleWindowlessSite in a list of child windows, always keep
     * a reference to it.
     * ExSiteInfo does an AddRef in constructor
     */    
    ExSiteInfo *pSiteInfo = new ExSiteInfo(pChildSite,
		(void*)pChildSiteWindowless);   
    m_Children.Add(pSiteInfo);
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::DestroyChild(IRMASite *pChildSite)
{
    /*
     * Verify that the site is actually a child site and find
     * its position in the list in the event that it is a child.
     */
    ListPosition prevPos;
    ExSiteInfo *pSiteInfo = NULL;
    MapSiteToSiteInfo(pChildSite, pSiteInfo, prevPos);
    if (!pSiteInfo) 
		return(PNR_UNEXPECTED);

    /* 
     * Cleanup the list items for this child site.
     */
    if (!m_bInDestructor) {
        delete pSiteInfo;
		if (prevPos)
			m_Children.RemoveAfter(prevPos);
		else
			m_Children.RemoveHead();
    }
    return(PNR_OK);
}

BOOL 
ExampleWindowlessSite::MapSiteToSiteInfo(IRMASite *pChildSite, 
										 ExSiteInfo *&pSiteInfo, 
										 ListPosition &prevPos)
{
    BOOL result = FALSE;

    // iterate child site list

    pSiteInfo = NULL;
    ListPosition pos = m_Children.GetHeadPosition();
    prevPos = pos;
    while (pos) {
        pSiteInfo = (ExSiteInfo*) m_Children.GetAt(pos);
        if (pSiteInfo->m_pSite == pChildSite) {
            result = TRUE;
			if (prevPos == m_Children.GetHeadPosition())
				prevPos = NULL;
			break;
		}
		prevPos = pos;
		pos = m_Children.GetNextPosition(pos);	
    }
    return(result);
}

STDMETHODIMP 
ExampleWindowlessSite::AttachWatcher(IRMASiteWatcher *pWatcher)
{
    if (m_pWatcher) 
		return(PNR_UNEXPECTED);

    m_pWatcher = pWatcher;
    if (m_pWatcher) {
		m_pWatcher->AddRef();
		m_pWatcher->AttachSite(this);
    }
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::DetachWatcher(void)
{
    if (!m_pWatcher) 
		return(PNR_UNEXPECTED);
    m_pWatcher->DetachSite();
    PN_RELEASE(m_pWatcher);
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::SetSize(PNxSize size)
{
    HRESULT result = PNR_OK;

    /* before we do anything, we give the SiteWatcher a chance to 
     * influence this operation.
     */
    if (m_pWatcher)
		result = m_pWatcher->ChangingSize(m_size, size);
    
    if (result == PNR_OK && size.cx != 0 && size.cy != 0) {
		m_size = size;

		IRMAPassiveSiteWatcher *pWatcher = NULL;
		ListPosition pos = m_PassiveSiteWatchers.GetHeadPosition();
		while (pos) {
			pWatcher = 
				(IRMAPassiveSiteWatcher *)m_PassiveSiteWatchers.GetAt(pos);
			pWatcher->SizeChanged(&m_size);
			pos = m_PassiveSiteWatchers.GetNextPosition(pos);	
		}
    }
    return(result);
}

STDMETHODIMP 
ExampleWindowlessSite::SetPosition(PNxPoint position)
{
    HRESULT result = PNR_OK;

    /*
     * Before we do anything, we give the SiteWatcher a chance to 
     * influence this operation.
     */
    if (m_pWatcher)
		result = m_pWatcher->ChangingPosition(m_position, position);
    
    if (result == PNR_OK) {
		m_position = position;

		// iterate child site list

		IRMAPassiveSiteWatcher *pWatcher = NULL;
		ListPosition pos = m_PassiveSiteWatchers.GetHeadPosition();
		while (pos) {
			pWatcher = 
				(IRMAPassiveSiteWatcher *)m_PassiveSiteWatchers.GetAt(pos);
			pWatcher->PositionChanged(&m_position);
			pos = m_PassiveSiteWatchers.GetNextPosition(pos);	
		}
    }
    return(result);
}

STDMETHODIMP 
ExampleWindowlessSite::GetSize(REF(PNxSize)size)
{
    size = m_size;
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::GetPosition(REF(PNxPoint)position)
{
    position = m_position;
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::DamageRect(PNxRect rect)
{
    m_bDamaged = TRUE;
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::DamageRegion(PNxRegion region)
{
    m_bDamaged = TRUE;
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::ForceRedraw()
{
    // make sure we have a visible window and are not re-enterering and we have
	// damage

    if (!m_bInRedraw && m_bDamaged && m_bIsVisible) {
		AddRef();
		m_bInRedraw = TRUE;

		/*
		 * set up the event structure to simulate an X "Expose" event
		 */
		PNxEvent event;
		memset(&event,0,sizeof(event));
		event.event = WM_PAINT;

		/*
		 * call our handy helper routine that takes care of everything
		 * else for us.
		 */
		ForwardUpdateEvent(&event);

		m_bInRedraw = FALSE;
		m_bDamaged = FALSE;
		Release();
    }
    return(PNR_OK);
}

void
ExampleWindowlessSite::ForwardUpdateEvent(PNxEvent* pEvent)
{
    BOOL bHandled = FALSE;

    AddRef();

    /* 
     * send the basic updateEvt event to the user
     */
    m_pUser->HandleEvent(pEvent);

    /*
     * If the user doesn't handle the standard update event then
     * send them the cross platform RMA_SURFACE_UPDATE event don't
     * damage the original event structure
     */
    if (!pEvent->handled && m_pUser) {
    	PNxEvent event;

		memset(&event, 0, sizeof(PNxEvent));
		event.event = RMA_SURFACE_UPDATE;
		event.param1 = m_pVideoSurface;
		event.result = 0;
		event.handled = FALSE;
		m_pUser->HandleEvent(&event);
		bHandled = event.handled;
    } else
    	bHandled = TRUE;

    Release();
}

STDMETHODIMP 
ExampleWindowlessSite::SetPropertyULONG32(const char *pPropertyName,
										  ULONG32 uPropertyValue)
{
    if (!m_pValues) 
		return(PNR_UNEXPECTED);
    return(m_pValues->SetPropertyULONG32(pPropertyName, uPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetPropertyULONG32(const char *pPropertyName,
										  REF(ULONG32)uPropertyValue)
{
    if (!m_pValues) 
		return(PNR_UNEXPECTED);
    return(m_pValues->GetPropertyULONG32(pPropertyName, uPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetFirstPropertyULONG32(REF(const char *)pPropertyName,
											   REF(ULONG32)uPropertyValue)
{
    if (!m_pValues) 
		return(PNR_UNEXPECTED);
    return(m_pValues->GetPropertyULONG32(pPropertyName, uPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetNextPropertyULONG32(REF(const char *)pPropertyName,
											  REF(ULONG32)uPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetNextPropertyULONG32(pPropertyName, uPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::SetPropertyBuffer(const char *pPropertyName,
										 IRMABuffer *pPropertyValue)
{
    if (!m_pValues) 
		return(PNR_UNEXPECTED);
    return(m_pValues->SetPropertyBuffer(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetPropertyBuffer(const char *pPropertyName,
										 REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetPropertyBuffer(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetFirstPropertyBuffer(REF(const char *)pPropertyName,
											  REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetFirstPropertyBuffer(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetNextPropertyBuffer(REF(const char *)pPropertyName,
											 REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetNextPropertyBuffer(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::SetPropertyCString(const char *pPropertyName,
										  IRMABuffer *pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->SetPropertyCString(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetPropertyCString(const char *pPropertyName,
										  REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetPropertyCString(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetFirstPropertyCString(REF(const char *)pPropertyName,
											   REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetFirstPropertyCString(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::GetNextPropertyCString(REF(const char *)pPropertyName,
											  REF(IRMABuffer *)pPropertyValue)
{
    if (!m_pValues)
		return(PNR_UNEXPECTED);
    return(m_pValues->GetNextPropertyCString(pPropertyName, pPropertyValue));
}

STDMETHODIMP 
ExampleWindowlessSite::UpdateSiteWindow(PNxWindow *pWindow)
{
    return(PNR_OK);
}

STDMETHODIMP 
ExampleWindowlessSite::ShowSite(BOOL bShow)
{
    m_bIsVisible = bShow;
    return(PNR_OK);
}

STDMETHODIMP_(BOOL) 
ExampleWindowlessSite::IsSiteVisible(void)
{
    BOOL bIsVisible = m_bIsVisible;
    if (m_pParentSite)
        bIsVisible &= m_pParentSite->IsSiteVisible();
    return(bIsVisible);
}

STDMETHODIMP
ExampleWindowlessSite::SetZOrder(INT32 lZOrder)
{
    if (!m_pParentSite) 
		return(PNR_UNEXPECTED);
    m_lZOrder = lZOrder;
    return(PNR_OK);
}

STDMETHODIMP
ExampleWindowlessSite::GetZOrder(REF(INT32)lZOrder)
{
    if (!m_pParentSite) 
		return(PNR_UNEXPECTED);
    lZOrder = m_lZOrder;
    return(PNR_OK);
}

STDMETHODIMP
ExampleWindowlessSite::MoveSiteToTop(void)
{
    if (!m_pParentSite)
		return(PNR_UNEXPECTED);
    return(PNR_NOTIMPL);
}

STDMETHODIMP
ExampleWindowlessSite::GetVideoSurface(REF(IRMAVideoSurface*)pSurface)
{
    PN_RESULT result = PNR_FAIL;

    if (m_pVideoSurface)
		result = m_pVideoSurface->QueryInterface(IID_IRMAVideoSurface, 
			(void**)&pSurface);
    return(result);
}

STDMETHODIMP_(UINT32)
ExampleWindowlessSite::GetNumberOfChildSites(void)
{
    return((UINT32)m_Children.Count());
}

STDMETHODIMP
ExampleWindowlessSite::AddPassiveSiteWatcher(IRMAPassiveSiteWatcher* pWatcher)
{
    pWatcher->AddRef();
    m_PassiveSiteWatchers.Add(pWatcher);
    return(PNR_OK);
}

STDMETHODIMP
ExampleWindowlessSite::RemovePassiveSiteWatcher(IRMAPassiveSiteWatcher* pWatcher)
{
    // iterate child site list

    IRMAPassiveSiteWatcher *pThisWatcher = NULL;
    ListPosition pos = m_PassiveSiteWatchers.GetHeadPosition();
    ListPosition prevPos = pos;
    while (pos) {
		pThisWatcher = 
			(IRMAPassiveSiteWatcher *)m_PassiveSiteWatchers.GetAt(pos);
		if (pWatcher == pThisWatcher) {
			PN_RELEASE(pWatcher);
			if (prevPos == m_PassiveSiteWatchers.GetHeadPosition())
				m_PassiveSiteWatchers.RemoveHead();
			else
				m_PassiveSiteWatchers.RemoveAfter(prevPos);
			break;
		}
		prevPos = pos;
		pos = m_PassiveSiteWatchers.GetNextPosition(pos);	
    }
    return(PNR_OK);
}

STDMETHODIMP
ExampleWindowlessSite::EventOccurred(PNxEvent *pEvent)
{
    return(PNR_OK);
}

PNxWindow*
ExampleWindowlessSite::GetParentWindow(void)
{
    return(NULL);
}

STDMETHODIMP
ExampleWindowlessSite::SetCursor(PNxCursor cursor, REF(PNxCursor)oldCursor)
{
    return(PNR_NOTIMPL);
}

//------------------------------------------------------------------------------
// ExSiteInfo class.
//------------------------------------------------------------------------------

ExSiteInfo::ExSiteInfo(IRMASite *pSite, void *pThisPointer)
{
    m_pSite = pSite;
    m_pSite->AddRef();
    pSite->QueryInterface(IID_IRMASiteWindowless, (void **)&m_pSiteWindowless);
    pSite->QueryInterface(IID_IRMASite2, (void **)&m_pSite2);
};

ExSiteInfo::~ExSiteInfo() 
{
    PN_RELEASE(m_pSite);
    PN_RELEASE(m_pSiteWindowless);
    PN_RELEASE(m_pSite2);
}

ExampleSiteSupplier::ExampleSiteSupplier(IUnknown *pUnkPlayer)
{
	m_lRefCount = 0;
	m_pSiteManager = NULL;
	m_pCCF = NULL;
	m_pUnkPlayer = pUnkPlayer;
	m_pWindowlessSite = NULL;

    if (m_pUnkPlayer) {
		m_pUnkPlayer->QueryInterface(IID_IRMASiteManager,
				(void **)&m_pSiteManager);
		m_pUnkPlayer->QueryInterface(IID_IRMACommonClassFactory,
				(void **)&m_pCCF);
		m_pUnkPlayer->AddRef();
    }
};

ExampleSiteSupplier::~ExampleSiteSupplier()
{
    PN_RELEASE(m_pSiteManager);
    PN_RELEASE(m_pCCF);
    PN_RELEASE(m_pUnkPlayer);
}

STDMETHODIMP 
ExampleSiteSupplier::SitesNeeded(UINT32	uRequestID, IRMAValues *pProps)
{
    /*
     * If there are no properties, then we can't really create a
     * site, because we have no idea what type of site is desired!
     */
    if (!pProps)
		return(PNR_INVALID_PARAMETER);

    HRESULT	result = PNR_OK;
    IRMAValues *pSiteProps = NULL;
    IRMABuffer *pValue = NULL;
    UINT32 style = 0;
    IRMASite *pSite	= NULL;

    // instead of using the CCF to create a IRMASiteWindowed, create our 
	// windowless site here...

    m_pWindowlessSite = new ExampleWindowlessSite(m_pUnkPlayer);
    if (!m_pWindowlessSite)
		goto exit;
    m_pWindowlessSite->AddRef();

    result = m_pWindowlessSite->QueryInterface(IID_IRMASite,(void **)&pSite);
    if (result != PNR_OK)
		goto exit;

    result = m_pWindowlessSite->QueryInterface(IID_IRMAValues,
		(void **)&pSiteProps);
    if (result != PNR_OK)
		goto exit;
	
    /*
     * We need to figure out what type of site we are supposed to
     * to create. We need to "switch" between site user and site
     * properties. So look for the well known site user properties
     * that are mapped onto sites...
     */
    result = pProps->GetPropertyCString("playto", pValue);
    if (result == PNR_OK) {
		pSiteProps->SetPropertyCString("channel", pValue);
		PN_RELEASE(pValue);
    } else {
		result = pProps->GetPropertyCString("name", pValue);
		if (result == PNR_OK) {
			pSiteProps->SetPropertyCString("LayoutGroup", pValue);
    	    PN_RELEASE(pValue);
		}
    }

    /*
     * We need to wait until we have set all the properties before
     * we add the site.
     */
    result = m_pSiteManager->AddSite(pSite);
    if (result != PNR_OK)
		goto exit;

    m_CreatedSites.SetAt((void*)uRequestID, pSite);
    pSite->AddRef();

exit:
    PN_RELEASE(pSiteProps);
    PN_RELEASE(pSite);
    return(result);
}

STDMETHODIMP 
ExampleSiteSupplier::SitesNotNeeded(UINT32 uRequestID)
{
    IRMASite *pSite = NULL;
    void *pVoid = NULL;

    if (!m_CreatedSites.Lookup((void*)uRequestID, pVoid))
		return PNR_INVALID_PARAMETER;
    pSite = (IRMASite*)pVoid;

    m_pSiteManager->RemoveSite(pSite);

    // delete our windowless site now (sets to NULL)

    PN_RELEASE(m_pWindowlessSite);

    // ref count = 1; deleted from this object's view!

    pSite->Release();

    m_CreatedSites.RemoveKey((void*)uRequestID);

    return(PNR_OK);
}

STDMETHODIMP 
ExampleSiteSupplier::BeginChangeLayout(void)
{
    return(PNR_OK);
}

STDMETHODIMP 
ExampleSiteSupplier::DoneChangeLayout(void)
{
    return(PNR_OK);
}

STDMETHODIMP_(ULONG32) 
ExampleSiteSupplier::AddRef(void)
{
    return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(ULONG32) 
ExampleSiteSupplier::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
        return(m_lRefCount);
    delete this;
    return(0);
}

STDMETHODIMP 
ExampleSiteSupplier::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)) {
		AddRef();
		*ppvObj = (IUnknown *)(IRMASiteSupplier *)this;
		return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMASiteSupplier)) {
		AddRef();
		*ppvObj = (IRMASiteSupplier *)this;
		return(PNR_OK);
    }
    *ppvObj = NULL;
    return(PNR_NOINTERFACE);
}

//------------------------------------------------------------------------------
// FiveMinuteNode class.
//------------------------------------------------------------------------------

FiveMinuteNode::FiveMinuteNode() 
: m_pData(0)
, m_pNext(0)
{
}

FiveMinuteNode::~FiveMinuteNode()
{
}

FiveMinuteList::FiveMinuteList()
: m_pHead(0)
, m_pLast(0)
, m_pTail(0)
, m_ulNumElements(0)
{
}

FiveMinuteList::~FiveMinuteList()
{
}


UINT32
FiveMinuteList::Count(void)
{
    return(m_ulNumElements);
}

void
FiveMinuteList::AddHead(void *p)
{
    FiveMinuteNode* pNode = new FiveMinuteNode;
    pNode->m_pData = p;
    pNode->m_pNext = m_pHead;
    m_pHead = pNode;
    if (!pNode->m_pNext)
		m_pTail = pNode;
    m_ulNumElements++;
}

void
FiveMinuteList::Add(void *p)
{
    FiveMinuteNode* pNode = new FiveMinuteNode;
    pNode->m_pData = p;
    pNode->m_pNext = NULL;

    if (m_pTail) {
		m_pTail->m_pNext = pNode;
		m_pTail = pNode;
    } else {
		m_pHead = pNode;
		m_pTail = pNode;
    }
    m_ulNumElements++;
}


void *
FiveMinuteList::GetFirst(void)
{
    if (!m_pHead)
		return(0);

    m_pLast = m_pHead;
    return(m_pHead->m_pData);
}

void *
FiveMinuteList::GetNext(void)
{
    if (!m_pLast)
		return(0);

    if (!m_pLast->m_pNext) {
		m_pLast = 0;
		return(0);
    }

    m_pLast = m_pLast->m_pNext;
    return(m_pLast->m_pData);
}

void *
FiveMinuteList::RemoveHead(void)
{
    m_pLast = 0;
    if (!m_pHead)
		return(0);

    void* pRet = m_pHead->m_pData;
    FiveMinuteNode* pDel = m_pHead;
    m_pHead = m_pHead->m_pNext;
    delete pDel;
    m_ulNumElements--;
    if (!m_pHead)
		m_pTail = 0;

    return(pRet);
}

void *
FiveMinuteList::GetLast(void)
{
    if (!m_pTail)
		return(0);

    return(m_pTail->m_pData);
}

ListPosition
FiveMinuteList::GetHeadPosition(void)
{
    return(m_pHead);
}

ListPosition
FiveMinuteList::GetNextPosition(ListPosition lp)
{
    if (!lp)
		return(0);

    return(lp->m_pNext);
}

void
FiveMinuteList::InsertAfter(ListPosition lp, void *p)
{
    if (!lp)
		return;

    FiveMinuteNode* pNode = new FiveMinuteNode;
    pNode->m_pData = p;
    pNode->m_pNext = lp->m_pNext;
    lp->m_pNext = pNode;
    if (lp == m_pTail)
		m_pTail = pNode;
    m_ulNumElements++;
}

void *
FiveMinuteList::RemoveAfter(ListPosition lp)
{
    if (!lp)
		return(0);

    FiveMinuteNode *pDel;
    pDel = lp->m_pNext;
    if (!pDel)
		return(0);

    void *pRet = pDel->m_pData;
    lp->m_pNext = pDel->m_pNext;
    if (m_pTail == pDel)
		m_pTail = lp;
    delete pDel;
    m_ulNumElements--;
    return(pRet);
}

void *
FiveMinuteList::GetAt(ListPosition lp)
{
    if (!lp)
		return(0);

    return(lp->m_pData);
}

//------------------------------------------------------------------------------
// FiveMinuteMap class.
//------------------------------------------------------------------------------

void * 
FiveMinuteMap::GetFirstValue(void)
{
    m_nCursor = 0;

    if (m_nMapSize)
		return m_pValueArray[m_nCursor];
    else
		return NULL;
}

void *
FiveMinuteMap::GetNextValue(void)
{
    m_nCursor++;

    if (m_nCursor < m_nMapSize)
		return m_pValueArray[m_nCursor];
    else
		return(NULL);
}

BOOL 
FiveMinuteMap::Lookup(void *Key, void *&Value) const
{
    BOOL bFound = FALSE;
    int nIndex = 0;

    // If Key is alrady in the list, replace value

    for (; nIndex < m_nMapSize; nIndex++) {
		if (m_pKeyArray[nIndex] == Key) {
			Value = m_pValueArray[nIndex];
			bFound = TRUE;
			break;
		}
    }
    return(bFound);    
}

void 
FiveMinuteMap::RemoveKey(void *Key)
{
    BOOL bFound = FALSE;
    int nIndex = 0;

    // If Key is alrady in the list, replace value

    for (; nIndex < m_nMapSize; nIndex++) {
		if (m_pKeyArray[nIndex] == Key) {
			if (nIndex < (m_nMapSize-1)) {
				memmove(&(m_pKeyArray[nIndex]), &(m_pKeyArray[nIndex+1]), 
					sizeof(void *)*(m_nMapSize-(nIndex+1)));
				memmove(&(m_pValueArray[nIndex]), &(m_pValueArray[nIndex+1]),
					sizeof(void *)*(m_nMapSize-(nIndex+1)));
			}
			m_nMapSize--;
			break;
		}
    }
}

void 
FiveMinuteMap::RemoveValue(void *Value)
{
    BOOL bFound = FALSE;
    int nIndex = 0;

    // If Value is alrady in the list, replace value

    for (; nIndex < m_nMapSize; nIndex++) {
		if (m_pValueArray[nIndex] == Value) {
			if (nIndex < (m_nMapSize-1)) {
				memmove(&(m_pKeyArray[nIndex]), &(m_pKeyArray[nIndex+1]),
					sizeof(void *)*(m_nMapSize-(nIndex+1)));
				memmove(&(m_pValueArray[nIndex]), &(m_pValueArray[nIndex+1]),
					sizeof(void *)*(m_nMapSize-(nIndex+1)));
			}
			m_nMapSize--;
			break;
		}
    }
}

void 
FiveMinuteMap::SetAt(void *Key, void *Value)
{
    int nIndex = 0;

    // If Key is alrady in the list, replace value

    for (; nIndex < m_nMapSize; nIndex++) {
		if (m_pKeyArray[nIndex] == Key) {
			m_pValueArray[nIndex] = Value;
			return;
		}
    }

    // If we have room, add it to the end!

    if (m_nAllocSize == m_nMapSize) {
		m_nAllocSize += AllocationSize;
		void **pNewKeys   = new void *[m_nAllocSize];
		void **pNewValues = new void *[m_nAllocSize];

		memcpy(pNewKeys,m_pKeyArray,sizeof(void *)*m_nMapSize);
		memcpy(pNewValues,m_pValueArray,sizeof(void *)*m_nMapSize);

		delete []m_pKeyArray;
		delete []m_pValueArray;

		m_pKeyArray = pNewKeys;
		m_pValueArray = pNewValues;
    }

    m_pKeyArray[m_nMapSize] = Key;
    m_pValueArray[m_nMapSize] = Value;
    m_nMapSize++;
}

//------------------------------------------------------------------------------
// ExampleVideoSurface class.
//------------------------------------------------------------------------------

ExampleVideoSurface::ExampleVideoSurface(IUnknown* pContext, 
										 ExampleWindowlessSite* pSiteWindowless)
    : m_lRefCount(0)
    , m_pContext(pContext)
    , m_pSiteWindowless(pSiteWindowless)
    , m_pBitmapInfo(NULL)
{ 
    if (m_pContext)
		m_pContext->AddRef();

    memset(&m_lastBitmapInfo, 0, sizeof(RMABitmapInfoHeader));
}

ExampleVideoSurface::~ExampleVideoSurface()
{
    PN_RELEASE(m_pContext);
}

STDMETHODIMP 
ExampleVideoSurface::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)) {
        AddRef();
        *ppvObj = this;
        return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMAVideoSurface)) {
        AddRef();
        *ppvObj = (IRMAVideoSurface *)this;
        return PNR_OK;
    }
    *ppvObj = NULL;
    return(PNR_NOINTERFACE);
}

STDMETHODIMP_(ULONG32) 
ExampleVideoSurface::AddRef(void)
{
    return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(ULONG32) 
ExampleVideoSurface::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
        return(m_lRefCount);
    delete this;
    return(0);
}

PN_RESULT	
ExampleVideoSurface::Init(void)
{
    return(PNR_OK);
}

STDMETHODIMP
ExampleVideoSurface::Blt(UCHAR *pImageData, RMABitmapInfoHeader *pBitmapInfo,
						 REF(PNxRect)inDestRect, REF(PNxRect)inSrcRect)
{
    BeginOptimizedBlt(pBitmapInfo);
    return(OptimizedBlt(pImageData, inDestRect, inSrcRect));
}

STDMETHODIMP
ExampleVideoSurface::BeginOptimizedBlt(RMABitmapInfoHeader *pBitmapInfo)
{
	int pixel_format;

	// Sanity check.

    if (!pBitmapInfo)
		return(PNR_FAIL);

	// Only accept RMA_RGB or RMA_YUV420_ID formats; we don't have converters
	// for the rest yet, and it's not clear they're used very often.

	switch (pBitmapInfo->biCompression) {
	case RMA_RGB:
		pixel_format = RGB24;
		break;
	case RMA_YUV420_ID:
		pixel_format = YUV12;
		break;
	default:
		return(PNR_FAIL);
	}

	// If the format has changed...
	// XXX -- init_video_textures() needs to recreate video textures if they
	// already exist, because if this gets executed more than once we're sunk...
	
	if (m_lastBitmapInfo.biWidth != pBitmapInfo->biWidth ||
	    m_lastBitmapInfo.biHeight != pBitmapInfo->biHeight ||
	    m_lastBitmapInfo.biBitCount != pBitmapInfo->biBitCount ||
	    m_lastBitmapInfo.biCompression != pBitmapInfo->biCompression) {

		// Remember the format so we don't change it again unnecessarily.

        m_pBitmapInfo = pBitmapInfo;
	    m_lastBitmapInfo.biWidth = pBitmapInfo->biWidth;
	    m_lastBitmapInfo.biHeight = pBitmapInfo->biHeight;
	    m_lastBitmapInfo.biBitCount = pBitmapInfo->biBitCount;
	    m_lastBitmapInfo.biCompression = pBitmapInfo->biCompression;

		// Initialise the video textures using the width and height of the
		// bitmap.

		init_video_textures(pBitmapInfo->biWidth, pBitmapInfo->biHeight,
			pixel_format);
	}
    return(PNR_OK);
}

STDMETHODIMP
ExampleVideoSurface::OptimizedBlt(UCHAR *pImageBits, REF(PNxRect)rDestRect,
								  REF(PNxRect)rSrcRect)
{
    if (!m_pBitmapInfo)
		return(PNR_UNEXPECTED);

	// Draw the frame.
	// XXX -- We should be passing in the source and destination rectangles
	// here, just in case they don't apply to the whole frame.

	draw_frame(pImageBits);
    return(PNR_OK);
}

STDMETHODIMP
ExampleVideoSurface::EndOptimizedBlt(void)
{
    m_pBitmapInfo = NULL;

    return(PNR_OK);
}

STDMETHODIMP
ExampleVideoSurface::GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE)ulType)
{
    if (m_pBitmapInfo)
        ulType =  m_pBitmapInfo->biCompression;

   return(PNR_OK);
}

STDMETHODIMP
ExampleVideoSurface::GetPreferredFormat(REF(RMA_COMPRESSION_TYPE)ulType)
{
    ulType = RMA_RGB;
    return(PNR_OK);
}

//------------------------------------------------------------------------------
// FiveMinuteQueue class.
//------------------------------------------------------------------------------

void
FiveMinuteQueue::Add(void *pElement)
{
    FiveMinuteElement *pElem = new FiveMinuteElement(pElement);
    FiveMinuteElement *pPtr;
    FiveMinuteElement **ppPtr;

    for (ppPtr = &m_pElemList; (pPtr = *ppPtr) != 0; ppPtr = &pPtr->m_pNext)
		;
    *ppPtr = pElem;
}

void *
FiveMinuteQueue::Remove(void)
{
    if (!m_pElemList)
		return(0);

    FiveMinuteElement *pElem = m_pElemList;
    void *pPtr = pElem->m_pPtr;
    m_pElemList = m_pElemList->m_pNext;
    delete pElem;
    return(pPtr);
}

#ifdef SUPPORT_A3D

//------------------------------------------------------------------------------
// ExampleAudioServices class.
//------------------------------------------------------------------------------

ExampleAudioServices::ExampleAudioServices()
{
	m_ulMSecs = 0;
	m_lLastTime = 0;
	m_ulAudioTime = 0;
}

void
ExampleAudioServices::Init(void)
{
	m_ulMSecs = 0;
	m_lLastTime = 0;
	m_ulAudioTime = 0;
}

void
ExampleAudioServices::Close(void)
{
	m_ulMSecs = 0;
	m_lLastTime = 0;
	m_ulAudioTime = 0;
}

void
ExampleAudioServices::AudioOut(char *pData, int nDataLen, int nFrameNo,
							   int nMSecs)
{
	if (streaming_sound_ptr && streaming_sound_ptr->sound_buffer_ptr)
		update_sound_buffer(streaming_sound_ptr->sound_buffer_ptr, pData, 
			nDataLen, (nFrameNo % BUFFERED_BLOCKS) * nDataLen);
	m_ulMSecs = nMSecs;
}

ULONG32
ExampleAudioServices::GetAudioClock(void)
{
	LONG32 this_time;
	LONG32 delta_time;
	DWORD wave_pos;

    // This value indicates the time of the last bit of audio played
    // by the physical audio device.

	if (streaming_sound_ptr && streaming_sound_ptr->sound_buffer_ptr)
		((LPA3DSOURCE)streaming_sound_ptr->sound_buffer_ptr)->GetWavePosition(
			&wave_pos);
	else
		wave_pos = 0;
	this_time = (wave_pos * 8) / ulBitsPerMS;
	delta_time = this_time - m_lLastTime;
	m_lLastTime = this_time;
	if (delta_time < 0)
		delta_time += BUFFERED_BLOCKS * ulMSPerBlock;
	m_ulAudioTime += delta_time;
	return(m_ulAudioTime);
}

//------------------------------------------------------------------------------
// ExampleAudioDevice class.
//------------------------------------------------------------------------------

ExampleAudioDevice::ExampleAudioDevice(IRMAPlayer *pPlayer)
    : m_lRefCount(0)
    , m_pResponse(NULL)
    , m_uVolume(0)
    , m_bPaused(FALSE)
    , m_bInitialized(FALSE)
    , m_pAudioServices(NULL)
    , m_pPlayer(pPlayer)
    , m_hCallback(NULL)
    , m_pQueue(NULL)
	, m_ulLastTickCount(0)
    , m_ulGranularity(100) // default to 100ms until we know the real value
{
    m_pAudioServices = new ExampleAudioServices;
    m_pQueue = new FiveMinuteQueue;

    // We need the player so we can use its scheduler.

    if (m_pPlayer)
		m_pPlayer->AddRef();
}

ExampleAudioDevice::~ExampleAudioDevice()
{
    // In case someone didn't close us...

    if (m_pResponse) {
		m_pResponse->Release();
		m_pResponse = NULL;
    }

	if (m_pAudioServices) {
		delete m_pAudioServices;
		m_pAudioServices = NULL;
    }

    if (m_pPlayer) {
		m_pPlayer->Release();
		m_pPlayer = NULL;
    }
}

STDMETHODIMP_(UINT32)
ExampleAudioDevice::AddRef(void)
{
    return(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(UINT32)
ExampleAudioDevice::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
        return(m_lRefCount);
    delete this;
    return(0);
}

STDMETHODIMP
ExampleAudioDevice::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)) {
        AddRef();
        *ppvObj = (IUnknown *)(IRMAAudioDevice *)this;
        return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMAAudioDevice)) {
        AddRef();
        *ppvObj = (IRMAAudioDevice *)this;
        return(PNR_OK);
    } else if (IsEqualIID(riid, IID_IRMACallback)) {
        AddRef();
        *ppvObj = (IRMACallback *)this;
        return(PNR_OK);
    }
    *ppvObj = NULL;
    return(PNR_NOINTERFACE);
}

// This routine initializes our audio device and prepares it for writing.
// We must store the IRMAAudioDeviceResponse pointer to notify the player of
// time syncs.

STDMETHODIMP 
ExampleAudioDevice::Open(const RMAAudioFormat *pFormat,
						 IRMAAudioDeviceResponse *pResponse)
{
	wave *streaming_wave_ptr;
	WAVEFORMATEX *streaming_wave_format_ptr;

	// Check for invalid parameters.

    if (m_bInitialized || m_pResponse || !m_pPlayer)
		return(PNR_UNEXPECTED);

    if (!pResponse || !pFormat)
		return(PNR_INVALID_PARAMETER);

    if (!m_pAudioServices || !m_pQueue)
		return(PNR_OUTOFMEMORY);

	// If there is no streaming sound object, simply return failure so that we
	// get no audio stream.

	if (streaming_sound_ptr == NULL)
		return(PNR_FAIL);

	// Save the pointer to the response object.

    m_pResponse = pResponse;
    m_pResponse->AddRef();

	// Save the audio format.

    memcpy(&m_format, pFormat, sizeof(RMAAudioFormat));

	// Calculate the transmission speed of the stream in bits per second.

	ulBitsPerSec = m_format.uChannels * m_format.uBitsPerSample * 
		m_format.ulSamplesPerSec;
	ulBitsPerMS = ulBitsPerSec / 1000;
	ulMSPerBlock = (m_format.uMaxBlockSize * 8) / ulBitsPerMS;

	// Set the granularity of updates to 50 ms.

	m_ulGranularity = 50;

    // Initialize our OS's audio services.

    m_pAudioServices->Init();
    m_bInitialized = TRUE;

	// Create and initialise the streaming wave format.

	if ((streaming_wave_format_ptr = new WAVEFORMATEX) == NULL)
		return(PNR_OK);
	streaming_wave_format_ptr->wFormatTag = WAVE_FORMAT_PCM;
	streaming_wave_format_ptr->nChannels = m_format.uChannels;
	streaming_wave_format_ptr->nSamplesPerSec = m_format.ulSamplesPerSec;
	streaming_wave_format_ptr->wBitsPerSample = m_format.uBitsPerSample;
	streaming_wave_format_ptr->nBlockAlign = m_format.uChannels * 
		(m_format.uBitsPerSample / 8);
	streaming_wave_format_ptr->nAvgBytesPerSec = m_format.ulSamplesPerSec *
		streaming_wave_format_ptr->nBlockAlign;
	streaming_wave_format_ptr->cbSize = 0;

	// Initialise the streaming wave object.

	streaming_wave_ptr = streaming_sound_ptr->wave_ptr;
	streaming_wave_ptr->format_ptr = streaming_wave_format_ptr;
	streaming_wave_ptr->data_size = m_format.uMaxBlockSize * BUFFERED_BLOCKS; 

	// Create the sound buffer for the streaming sound object.

	create_sound_buffer(streaming_sound_ptr);
    return(PNR_OK);
}

// This routine frees up the resources in use by our audio device.

STDMETHODIMP 
ExampleAudioDevice::Close(const BOOL bFlush)
{
    if (!m_bInitialized || !m_pAudioServices || !m_pPlayer)
		return PNR_UNEXPECTED;

	// Destroy the sound buffer for the streaming sound object.

	destroy_sound_buffer(streaming_sound_ptr);

	// Release the response object.

	if (m_pResponse) {
		m_pResponse->Release();
		m_pResponse = NULL;
    }

	// Drain the queued audio buffers.

    if (bFlush)
		Drain();

    // Close our OS's audio services.

    m_pAudioServices->Close();
    m_bInitialized = FALSE;
    return(PNR_OK);
}

// Put the audio device into an active state.  It must be resumed to play
// audio.  In our implementation we schedule a callback with the player
// engine because in this sample we want to buffer the audio data and push
// it to our example OS audio services. 

STDMETHODIMP 
ExampleAudioDevice::Resume(void)
{
    if (!m_bInitialized || !m_bPaused)
		return(PNR_UNEXPECTED);

    // Use the scheduler to schedule a callback for processing our buffered
    // audio data.

	ScheduleCallback(); 
	if (m_hCallback) {
		play_sound(streaming_sound_ptr, true);
		m_bPaused = FALSE;
		return(PNR_OK);
	}
	return(PNR_FAIL);
}

// Pause the audio device and put it into an inactive state.  This state
// should prevent any more audio from being played until Resume() is called.

STDMETHODIMP 
ExampleAudioDevice::Pause(void)
{
    if (!m_bInitialized || m_bPaused)
		return(PNR_UNEXPECTED);

	RemoveCallback();
	stop_sound(streaming_sound_ptr);
	m_bPaused = TRUE;
    return(PNR_OK);
}

// This function receives a packet of raw audio data and may do whatever it
// wishes with it.  You could immediately write it to your OS's audio device
// or dump it to a file or draw a visual representation of it.  In our sample
// we buffer the data in a queue and actually process it on a scheduled
// callback.  (On some multithreaded OSes one might wish to run a separate
// thread to process this data as it is buffered.)

STDMETHODIMP 
ExampleAudioDevice::Write(const RMAAudioData *pAudioData)
{
    if (!pAudioData)
		return(PNR_INVALID_PARAMETER);

    if (!m_bInitialized)
		return(PNR_UNEXPECTED);

    if (!m_pResponse)
		return(PNR_FAIL);

    // Initialise the next portion of the sound buffer with the audio data
	// immediately if playback is paused.

	if (m_bPaused) {
		IRMABuffer *pBuf = pAudioData->pData;
		m_pAudioServices->AudioOut((char *)pBuf->GetBuffer(), 
			pBuf->GetSize(), pAudioData->ulAudioTime / 100 - 1, 
			pAudioData->ulAudioTime);
	}

    // Buffer the audio data and let our scheduler feed it to our OS's audio
    // services.

	else {
		RMAAudioData *pMyData = new RMAAudioData;
		if (pMyData) {

			// Make our own copy of the RMAAudioData struct

			memcpy(pMyData, pAudioData, sizeof(RMAAudioData));

			// keep a ref on the IRMABuffer member until we process it.

			pMyData->pData->AddRef(); 

			// queue my audio data

			m_pQueue->Add((void*)pMyData);

			// Make sure we have a callback scheduled to process the data

			ScheduleCallback();
		}
	}

	return(PNR_OK);
}

// When this function is called the audio level of your audio device should
// be initialized.  In this sample we ignore it because we are just dumping
// audio timestamps to the log file, nothing more.

STDMETHODIMP_(BOOL)
ExampleAudioDevice::InitVolume(const UINT16	uMinVolume,
							   const UINT16	uMaxVolume)
{
    return(TRUE);
}

// This function should cause your audio device to change the volume level. 

STDMETHODIMP 
ExampleAudioDevice::SetVolume(const UINT16 uVolume)
{
    m_uVolume = uVolume;
    return(PNR_OK);
}

STDMETHODIMP_(UINT16) 
ExampleAudioDevice::GetVolume(void)
{
    return(m_uVolume);
}

// This function should reinitialize your audio device.

STDMETHODIMP 
ExampleAudioDevice::Reset(void)
{
    m_bPaused = FALSE;
    if (m_pAudioServices) {
		m_pAudioServices->Close();
		m_pAudioServices->Init();
    }
	RemoveCallback();
    return(PNR_OK);
}

// This function should cause the audio device to discard any audio data it
// may have pending.  When this function is called it indicates that the
// audio data that has been written to the audio device is no longer needed.
// For instance, since the core generally buffers a second or two of audio
// to the audio device if a user stops a presentation mid-stream there will
// be one or two seconds of audio data waiting in the audio device's buffers
// that should not be played.  (Otherwise the user would hear one or two
// seconds of audio after he/she stopped playback.) 

STDMETHODIMP 
ExampleAudioDevice::Drain()
{
    // Flush pending buffers and free the memory.

	RMAAudioData *pMyData = (RMAAudioData *)m_pQueue->Remove();
	while (pMyData) {
		pMyData->pData->Release();
		delete pMyData;
		pMyData = (RMAAudioData *)m_pQueue->Remove();
	}
    RemoveCallback();
    return(PNR_OK);
}

// This function is used to determine if the audio device supports a
// requested audio format.  Our implementation accepts any format because
// we don't actually use the audio data-- we just look at the timestamps. 

STDMETHODIMP 
ExampleAudioDevice::CheckFormat(const RMAAudioFormat *pFormat)
{
	return(PNR_OK);
}

// This function is critical to optimal playback.  It *MUST* return the
// most accurate value as possible, otherwise the audio could get out of
// synch with the video, buffering problems could occur or the entire
// presentation could slowly grind to a slow crawl.  Whatever value is
// returned here is what the player engine uses to determine what the
// user is currently hearing.
//
// We do not want to simply return the timestamp of the last audio packet 
// passed to IRMAAudioDevice::Write().  That is merely the time of the
// last packet buffered.  What we really want to do is query the OS's
// audio services and find out how far along in the audio presentation it is.
//
// Our sample uses an example audio services that acts like a simplistic
// audio API of an imaginary OS.  We ask it what time it is according to
// its clock and return that value.

STDMETHODIMP 
ExampleAudioDevice::GetCurrentAudioTime(REF(ULONG32)ulCurrentTime)
{
    if (m_bInitialized && m_pAudioServices)
		ulCurrentTime = m_pAudioServices->GetAudioClock();
    else
		ulCurrentTime = 0;
    return(PNR_OK);
}

// This is the callback called by the scheduler.  Our sample uses it to
// process the buffered audio data and push it downstream to our example
// audio services.

STDMETHODIMP 
ExampleAudioDevice::Func(void)
{
    // We just got our callback, so reinitialize our handle to NULL.

    m_hCallback = NULL;

    RMAAudioData *pMyData = (RMAAudioData*)m_pQueue->Remove();
    if (pMyData) {
		IRMABuffer *pBuf = pMyData->pData;

		// Write this audio data to our OS's audio services.

		m_pAudioServices->AudioOut((char *)pBuf->GetBuffer(), 
			pBuf->GetSize(), pMyData->ulAudioTime / 100 - 1, 
			pMyData->ulAudioTime);

		// Let the player engine know we just processed some audio and
		// give it the audio service's current clock time.

		if (m_pResponse)
			m_pResponse->OnTimeSync(m_pAudioServices->GetAudioClock());

		// Release our reference on the IRMABuffer and free our copy of the 
		// RMAAudioData structure.

		pBuf->Release();
		delete pMyData;

		// Schedule another callback to handle more audio data.

		ScheduleCallback();
    }
    return(PNR_OK);
}

// This function is used internally to setup a callback with the player
// engine.

void
ExampleAudioDevice::ScheduleCallback(void)
{
    if (m_hCallback)
		return;

    // Use the scheduler to schedule a callback for processing our buffered
    // audio data.

	IRMAScheduler *pScheduler = NULL;
    if (m_pPlayer->QueryInterface(IID_IRMAScheduler, (void **)&pScheduler)
		== PNR_OK) {
		m_hCallback = pScheduler->RelativeEnter((IRMACallback *)this,
			m_ulGranularity);
		pScheduler->Release();
		pScheduler = NULL;
    }
}


// This function is used internally to remove a callback with the player
// engine.

void
ExampleAudioDevice::RemoveCallback(void)
{
    if (!m_hCallback)
		return;

    // Use the scheduler to remove a scheduled callback and stop processing
    // our buffered audio data.

	IRMAScheduler *pScheduler = NULL;
    if (m_pPlayer->QueryInterface(IID_IRMAScheduler, (void**)&pScheduler) 
		== PNR_OK) {
		pScheduler->Remove(m_hCallback);
		m_hCallback = NULL;
		pScheduler->Release();
		pScheduler = NULL;
    }
}

#endif // SUPPORT_A3D