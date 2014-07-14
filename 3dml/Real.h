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

#include "pntypes.h"
#include "pnwintyp.h"
#include "pncom.h"
#include "rmacomm.h"
#include "rmawin.h"
#include "rmaclsnk.h"
#include "rmaerror.h"
#include "rmamon.h"
#include "rmacore.h"
#include "rmaauth.h"
#include "rmaerror.h"
#include "rmapckts.h"
#include "rmaengin.h"
#include "rmasite2.h"
#include "rmaevent.h"
#include "rmavsurf.h"
#include "rmafiles.h"
#include "rmaausvc.h"

class ExampleClientAdviseSink : public IRMAClientAdviseSink
{
    public:
		ExampleClientAdviseSink(IUnknown *pUnknown);
		~ExampleClientAdviseSink();
		STDMETHOD(OnPosLength)(UINT32 ulPosition, UINT32 ulLength);
		STDMETHOD(OnPresentationOpened)(void);
		STDMETHOD(OnPresentationClosed)(void);
		STDMETHOD(OnStatisticsChanged)(void);
		STDMETHOD(OnPreSeek)(UINT32 ulOldTime, UINT32 ulNewTime);
		STDMETHOD(OnPostSeek)(UINT32 ulOldTime, UINT32 ulNewTime);
		STDMETHOD(OnStop)(void);
		STDMETHOD(OnPause)(UINT32 ulTime);
		STDMETHOD(OnBegin)(UINT32 ulTime);
		STDMETHOD(OnBuffering)(UINT32 ulFlags, UINT16 unPercentComplete);
		STDMETHOD(OnContacting) (const char *pHostName);
	    STDMETHOD(QueryInterface)(REFIID ID, void **ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);

	private:
		INT32 m_lRefCount;
		IUnknown *m_pUnknown;
};

class ExampleAuthenticationManager : public IRMAAuthenticationManager
{
	public:
		ExampleAuthenticationManager();
		~ExampleAuthenticationManager();
		STDMETHOD(HandleAuthenticationRequest)
			(IRMAAuthenticationManagerResponse *pResponse);
		STDMETHOD (QueryInterface)(REFIID ID, void **ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);

	private:
		INT32 m_lRefCount;
};

class ExampleErrorSink : public IRMAErrorSink
{
	public:
		ExampleErrorSink();
		~ExampleErrorSink();
	    STDMETHOD(ErrorOccurred)(const UINT8 unSeverity, const UINT32 ulRMACode,
			const UINT32 ulUserCode, const char *pUserString, 
			const char *pMoreInfoURL);
	    STDMETHOD(QueryInterface)(REFIID ID, void **ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);

    private:
		INT32 m_lRefCount;
		void ConvertErrorToString(const ULONG32 ulRMACode, char* pszBuffer);
};

class NonDelegatingUnknown {
	public:
		STDMETHOD(NonDelegatingQueryInterface)(REFIID riid, void **ppvObj) PURE;
		STDMETHOD_(ULONG32,NonDelegatingAddRef)(void) PURE;
		STDMETHOD_(ULONG32,NonDelegatingRelease)(void) PURE;
};

class ExSiteInfo {
	public:
		ExSiteInfo(IRMASite *pSite, void *pThisPointer);
		~ExSiteInfo();

		IRMASite *m_pSite;
		IRMASiteWindowless *m_pSiteWindowless;
		IRMASite2 *m_pSite2;
};

class FiveMinuteNode
{
	public:
		FiveMinuteNode();
		~FiveMinuteNode();
		void *m_pData;
		FiveMinuteNode *m_pNext;
};

typedef FiveMinuteNode* ListPosition;
    
class FiveMinuteList
{
	public:
		FiveMinuteList();
		~FiveMinuteList();

		void AddHead(void *);
		void Add(void *);
		void *GetFirst(void);
		void *GetNext(void);
		void *GetLast(void);
		void *RemoveHead(void);
		UINT32 Count(void);

		ListPosition GetHeadPosition(void);
		ListPosition GetNextPosition(ListPosition);
		void InsertAfter(ListPosition, void *);
		void *RemoveAfter(ListPosition);
		void *GetAt(ListPosition);

	private:
		FiveMinuteNode *m_pLast;
		FiveMinuteNode *m_pHead;
		FiveMinuteNode *m_pTail;
		UINT32 m_ulNumElements;
};

class FiveMinuteMap
{
	private:
		const int AllocationSize;
		void **m_pKeyArray;
		void **m_pValueArray;
		int m_nMapSize;
		int m_nAllocSize;
		int m_nCursor;

    public:
		FiveMinuteMap()
			: m_pKeyArray(NULL)
			, m_pValueArray(NULL)
			, m_nMapSize(0)
			, m_nAllocSize(0)
			, m_nCursor(0)
			, AllocationSize(10)
		{};
		~FiveMinuteMap() {
			if (m_pKeyArray)
				delete []m_pKeyArray;
			if (m_pValueArray)
				delete []m_pValueArray;
		};
		int GetCount(void) {return (m_nMapSize); }
		void *GetFirstValue(void);
		void *GetNextValue(void);
		BOOL Lookup(void *Key, void *&Value) const;
		void RemoveKey(void *Key);
		void RemoveValue(void *Value);
		void SetAt(void *Key, void *Value);
};

class ExampleVideoSurface;

class ExampleWindowlessSite : 
	public NonDelegatingUnknown,
	public IRMASite,
	public IRMASite2,
	public IRMASiteWindowless, 
	public IRMAValues
{
	private:
		LONG32 m_lRefCount;
		IRMASiteUser *m_pUser;
		FiveMinuteList m_Children;
		BOOL  m_bDamaged;
		ExampleWindowlessSite *m_pParentSite;
		IRMAValues *m_pValues;    
		ExampleVideoSurface *m_pVideoSurface;
		IRMACommonClassFactory *m_pCCF;
		IUnknown *m_pUnkOuter;
		IRMASiteWatcher *m_pWatcher;
		IUnknown *m_pContext;

		PNxSize	m_size;
		PNxPoint m_position;
		INT32 m_lZOrder;
		FiveMinuteList m_PassiveSiteWatchers;
		BOOL m_bIsVisible;
		BOOL m_bInDestructor;
		BOOL m_bIsChildWindow;
		BOOL m_bInRedraw;

		~ExampleWindowlessSite();
		void SetParentSite(ExampleWindowlessSite *pParentSite);
		void ForwardUpdateEvent(PNxEvent *pEvent);
		BOOL MapSiteToSiteInfo(IRMASite *pChildSite, ExSiteInfo *&pSiteInfo, 
			ListPosition &prevPos);

	public:
		ExampleWindowlessSite *GetParentSite() { return(m_pParentSite); }
        ExampleWindowlessSite(IUnknown* pContext, IUnknown* pUnkOuter = NULL);
	    STDMETHOD(NonDelegatingQueryInterface)(REFIID riid, void **ppvObj);
		STDMETHOD_(ULONG32,NonDelegatingAddRef)(void);
		STDMETHOD_(ULONG32,NonDelegatingRelease)(void);
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
		STDMETHOD_(ULONG32,AddRef)(void);
		STDMETHOD_(ULONG32,Release)(void);
	    STDMETHOD(EventOccurred)(PNxEvent *pEvent);
		STDMETHOD_(PNxWindow*,GetParentWindow)(void);
		STDMETHOD(AttachUser)(IRMASiteUser *pUser);
		STDMETHOD(DetachUser)(void);
		STDMETHOD(GetUser)(REF(IRMASiteUser *)pUser);
		STDMETHOD(CreateChild)(REF(IRMASite *)pChildSite);
		STDMETHOD(DestroyChild)(IRMASite *pChildSite);
	    STDMETHOD(AttachWatcher)(IRMASiteWatcher *pWatcher);
		STDMETHOD(DetachWatcher)(void);
		STDMETHOD(SetPosition)(PNxPoint	position);
		STDMETHOD(GetPosition)(REF(PNxPoint)position);
		STDMETHOD(SetSize)(PNxSize size);
		STDMETHOD(GetSize)(REF(PNxSize)size);
		STDMETHOD(DamageRect)(PNxRect rect);
		STDMETHOD(DamageRegion)(PNxRegion region);
		STDMETHOD(ForceRedraw)(void);
		STDMETHOD(SetPropertyULONG32)(const char *pPropertyName,
			ULONG32 uPropertyValue);
		STDMETHOD(GetPropertyULONG32)(const char *pPropertyName,
			REF(ULONG32)uPropertyName);
		STDMETHOD(GetFirstPropertyULONG32)(REF(const char *)pPropertyName,
			REF(ULONG32)uPropertyValue);
		STDMETHOD(GetNextPropertyULONG32)(REF(const char *)pPropertyName,
			REF(ULONG32)uPropertyValue);
		STDMETHOD(SetPropertyBuffer)(const char *pPropertyName,
			IRMABuffer *pPropertyValue);
		STDMETHOD(GetPropertyBuffer)(const char *pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
		STDMETHOD(GetFirstPropertyBuffer)(REF(const char *)pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
		STDMETHOD(GetNextPropertyBuffer)(REF(const char *)pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
		STDMETHOD(SetPropertyCString)(const char *pPropertyName,
			IRMABuffer *pPropertyValue);
		STDMETHOD(GetPropertyCString)(const char *pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
		STDMETHOD(GetFirstPropertyCString)(REF(const char *)pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
		STDMETHOD(GetNextPropertyCString)(REF(const char *)pPropertyName,
			REF(IRMABuffer *)pPropertyValue);
	    STDMETHOD(UpdateSiteWindow)(PNxWindow *pWindow);
	    STDMETHOD(ShowSite)(BOOL bShow);                       
		STDMETHOD_(BOOL, IsSiteVisible)(void);
	    STDMETHOD(SetZOrder)(INT32 lZOrder);
		STDMETHOD(GetZOrder)(REF(INT32)lZOrder);
		STDMETHOD(MoveSiteToTop)(void);                       		
		STDMETHOD(GetVideoSurface)(REF(IRMAVideoSurface *)pSurface);
		STDMETHOD_(UINT32,GetNumberOfChildSites)(void);
		STDMETHOD(AddPassiveSiteWatcher)(IRMAPassiveSiteWatcher *pWatcher);
		STDMETHOD(RemovePassiveSiteWatcher)(IRMAPassiveSiteWatcher *pWatcher);
		STDMETHOD(SetCursor)(PNxCursor cursor, REF(PNxCursor)oldCursor);
};

class ExampleSiteSupplier :  public IRMASiteSupplier
{
    public:
		ExampleSiteSupplier(IUnknown *pUnkPlayer);
	    ~ExampleSiteSupplier();
		STDMETHOD(SitesNeeded)(UINT32 uRequestID, IRMAValues* pSiteProps);
		STDMETHOD(SitesNotNeeded)(UINT32 uRequestID);
		STDMETHOD(BeginChangeLayout)(void);
		STDMETHOD(DoneChangeLayout)(void);
		STDMETHOD(QueryInterface)(REFIID ID, void** ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);

    private:
		INT32 m_lRefCount;
		IRMASiteManager *m_pSiteManager;
		IRMACommonClassFactory *m_pCCF;
		IUnknown *m_pUnkPlayer;
		FiveMinuteMap m_CreatedSites;
	    ExampleWindowlessSite *m_pWindowlessSite;
};

class ExampleVideoSurface : IRMAVideoSurface
{
	private:
		LONG32 m_lRefCount;
		IUnknown *m_pContext;
		RMABitmapInfoHeader *m_pBitmapInfo;
		ExampleWindowlessSite *m_pSiteWindowless;
		RMABitmapInfoHeader m_lastBitmapInfo;

	public:
		ExampleVideoSurface(IUnknown *pContext, 
			ExampleWindowlessSite *pSiteWindowless);
		~ExampleVideoSurface();
		PN_RESULT Init(void);
		STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
		STDMETHOD_(ULONG32,AddRef)(void);
		STDMETHOD_(ULONG32,Release)(void);
		STDMETHOD(Blt)(UCHAR *pImageData, RMABitmapInfoHeader *pBitmapInfo, 
			REF(PNxRect)inDestRect,	REF(PNxRect)inSrcRect);
		STDMETHOD(BeginOptimizedBlt)(RMABitmapInfoHeader *pBitmapInfo);
		STDMETHOD(OptimizedBlt)(UCHAR *pImageBits, REF(PNxRect)rDestRect, 
			REF(PNxRect)rSrcRect);
		STDMETHOD(EndOptimizedBlt)(void);
		STDMETHOD(GetOptimizedFormat)(REF(RMA_COMPRESSION_TYPE)ulType);
		STDMETHOD(GetPreferredFormat)(REF(RMA_COMPRESSION_TYPE)ulType);
};

class ExampleClientContext : public IUnknown
{
    public:
		ExampleClientContext();
		~ExampleClientContext();
		void Init(IUnknown *pUnknown);
		void Close(void);
		STDMETHOD(QueryInterface)(REFIID ID, void **ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);


	private:
		INT32 m_lRefCount;
		ExampleClientAdviseSink *m_pClientSink;
		ExampleErrorSink *m_pErrorSink;
		ExampleAuthenticationManager *m_pAuthMgr;
		ExampleSiteSupplier *m_pSiteSupplier;
};

class FiveMinuteElement
{
    friend class FiveMinuteQueue;

    public:
		FiveMinuteElement(void *pPtr)
		{
			m_pPtr = pPtr;
			m_pNext = 0;
		};

    private:
		void *m_pPtr;
		FiveMinuteElement *m_pNext;
};

class FiveMinuteQueue
{
    public:
		FiveMinuteQueue() : m_pElemList(NULL) {};

		void Add(void *pElement);
		void *Remove(void);

    private:
		FiveMinuteElement *m_pElemList;
};

#ifdef SUPPORT_A3D

class ExampleAudioServices
{
	public:
		ExampleAudioServices();

		void Init(void);
		void Close(void);
		void AudioOut(char *pData, int nDataLen, int nDataStart, int nMSecs);
		ULONG32 GetAudioClock(void);

	protected:
		LONG32 m_lLastTime;
		ULONG32 m_ulMSecs;
		ULONG32 m_ulAudioTime;
};

class ExampleAudioDevice : public IRMAAudioDevice, public IRMACallback
{
	public:
		ExampleAudioDevice(IRMAPlayer *pPlayer);

	    STDMETHOD(QueryInterface)(REFIID ID, void **ppInterfaceObj);
		STDMETHOD_(UINT32, AddRef)(void);
		STDMETHOD_(UINT32, Release)(void);

		STDMETHOD(Open)(const RMAAudioFormat *pFormat,
			IRMAAudioDeviceResponse *pResponse);
		STDMETHOD(Close)(const BOOL bFlush);
		STDMETHOD(Resume)(void);
		STDMETHOD(Pause)(void);
		STDMETHOD(Write)(const RMAAudioData *pAudioData);
		STDMETHOD_(BOOL, InitVolume)(const UINT16 uMinVolume,
			const UINT16 uMaxVolume);
		STDMETHOD(SetVolume)(const UINT16 uVolume);
		STDMETHOD_(UINT16, GetVolume)(void);
		STDMETHOD(Reset)(void);
		STDMETHOD(Drain)(void);
		STDMETHOD(CheckFormat)(const RMAAudioFormat *pFormat);
		STDMETHOD(GetCurrentAudioTime)(REF(ULONG32)ulCurrentTime);
	    STDMETHOD(Func)(void);

	private:
		INT32 m_lRefCount;
		IRMAAudioDeviceResponse *m_pResponse;
		UINT16 m_uVolume;
		BOOL m_bPaused;
		BOOL m_bInitialized;
		RMAAudioFormat m_format;
		ExampleAudioServices *m_pAudioServices;
		IRMAPlayer *m_pPlayer;
		IRMAStream *m_pStream;
		CallbackHandle m_hCallback;
		FiveMinuteQueue *m_pQueue;
		ULONG32	m_ulGranularity;
		ULONG32 m_ulLastTickCount;

		void ScheduleCallback(void);
		void RemoveCallback(void);

		~ExampleAudioDevice();
};

#endif