/****************************************************************************
 * 
 *  $Id: pntypes.h,v 1.106 1999/03/04 02:56:28 gregc Exp $
 * 
 *  Copyright (C) 1995-1999 RealNetworks, Inc. All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary 
 *  information of Progressive Networks, Inc, and is licensed
 *  subject to restrictions on use and distribution.
 *
 *  This file defines data types that are too be used in all cross-platform
 *  Progressive Networks modules.
 *
 */

#ifdef _MACINTOSH
#pragma once
#endif

#ifndef _PNTYPES_H_
#define _PNTYPES_H_

#if (defined(_MSC_VER) && (_MSC_VER > 1100) && defined(_BASETSD_H_))
#error For VC++ 6.0 or higher you must include pntypes.h before other windows header files.
#endif

#if defined _WINDOWS || defined _OSF1 || defined _ALPHA

#ifndef RN_LITTLE_ENDIAN
#define RN_LITTLE_ENDIAN 1
#endif

#ifndef RN_BIG_ENDIAN 
#define RN_BIG_ENDIAN    0
#endif

#else

#ifndef RN_LITTLE_ENDIAN
#define RN_LITTLE_ENDIAN 0
#endif

#ifndef RN_BIG_ENDIAN 
#define RN_BIG_ENDIAN    1
#endif

#endif /* !_WINDOWS || !_OSF1 || !_ALPHA */

typedef char                    INT8;   /* signed 8 bit value */
typedef unsigned char           UINT8;  /* unsigned 8 bit value */
typedef short int               INT16;  /* signed 16 bit value */
typedef unsigned short int      UINT16; /* unsigned 16 bit value */
#if (defined _UNIX && (defined _ALPHA || defined _OSF1))
typedef int                     INT32;  /* signed 32 bit value */
typedef unsigned int            UINT32; /* unsigned 32 bit value */
typedef unsigned int            UINT;
#else
typedef long int                INT32;  /* signed 32 bit value */
typedef unsigned long int       UINT32; /* unsigned 32 bit value */
typedef unsigned int            UINT;
#endif /* (defined _UNIX && (defined _ALPHA || OSF1)) */

#if (defined _UNIX && defined _IRIX)
#ifdef __LONG_MAX__
#undef __LONG_MAX__
#endif
#define __LONG_MAX__ 2147483647
#endif

#ifndef BOOL
typedef int     BOOL;                   /* signed int value (0 or 1) */
#endif

/*
 * Added for ease of reading.
 * Instead of using __MWERKS__ you can  now use _MACINTOSH
 */
#ifdef __MWERKS__
    #if __dest_os==__macos
	#ifndef _MACINTOSH
	#define _MACINTOSH  1

	#ifdef powerc 
	#define _MACPPC
	#else
	#define _MAC68K
	#endif

	#endif
    #endif
#endif

#if defined (_SCO_SV) && !defined (MAXPATHLEN)
#include <limits.h>
#define MAXPATHLEN    _POSIX_PATH_MAX
#define PATH_MAX      _POSIX_PATH_MAX
#endif

#ifdef _SCO_UW
#include <stdio.h> //for sprintf
#endif


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define LANGUAGE_CODE	"EN"

#ifdef _WIN16
#define MAX_PATH	260
#define PRODUCT_ID	"play16"
#define PLUS_PRODUCT_ID	"plus16"
#else
#define PRODUCT_ID	"play32"
#define PLUS_PRODUCT_ID	"plus32"
#endif

#define MAX_DISPLAY_NAME        256
#define PN_INVALID_VALUE	(ULONG32)0xffffffff

#define PN_DELETE(x) ((x) ? (delete (x), (x) = 0) : 0)
#define PN_VECTOR_DELETE(x) ((x) ? (delete [] (x), (x) = 0) : 0)
#define PN_RELEASE(x) ((x) ? ((x)->Release(), (x) = 0) : 0)

#define RA_FILE_MAGIC_NUMBER    0x2E7261FDL /* RealAudio File Identifier */
#define RM_FILE_MAGIC_NUMBER    0x2E524D46L /* RealMedia File Identifier */
#define RIFF_FILE_MAGIC_NUMBER  0x52494646L /* RIFF (AVI etc.) File Identifier */

typedef INT32   LONG32;                 /* signed 32 bit value */
typedef UINT32  ULONG32;                /* unsigned 32 bit value */
typedef UINT8   UCHAR;                  /* unsigned 8 bit value */
typedef INT8    CHAR;                   /* signed 8 bit value */

typedef UINT8   BYTE;
typedef INT32   long32;
typedef UINT32  u_long32;

#ifndef _MACINTOSH
typedef INT8    Int8;
#endif
typedef UINT8   u_Int8;
typedef INT16   Int16;
typedef UINT16  u_Int16;
typedef INT32   Int32;
typedef UINT32  u_Int32;

typedef ULONG32                 UFIXED32;           /* FIXED point value  */
#define FLOAT_TO_FIXED(x)   ((UFIXED32) ((x) * (1L << 16) + 0.5))
#define FIXED_TO_FLOAT(x)   ((float) ((((float)x)/ (float)(1L <<16))))

/* 
 * UFIXED32 is a 32 value where the upper 16 bits are the unsigned integer 
 * portion of value, and the lower 16 bits are the fractional part of the 
 * value 
 */

typedef const char*             PCSTR;

/*
 *  FOURCC's are 32bit codes used in Tagged File formats like
 *  the RealMedia file format.
 */
#ifndef FOURCC
typedef UINT32                  FOURCC;         
#endif

#ifndef PN_FOURCC
#define PN_FOURCC( ch0, ch1, ch2, ch3 )                                    \
                ( (UINT32)(UINT8)(ch0) | ( (UINT32)(UINT8)(ch1) << 8 ) |        \
                ( (UINT32)(UINT8)(ch2) << 16 ) | ( (UINT32)(UINT8)(ch3) << 24 ) )
#endif

typedef UINT16 PrefKey;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL                0
#else
#define NULL                ((void *)0)
#endif
#endif

#ifndef _WINDOWS	// defined in windef.h on Windows platform
#ifndef HIWORD
#define HIWORD(x) ((x) >> 16)
#endif
#ifndef LOWORD
#define LOWORD(x) ((x) & 0xffff)
#endif
#endif
 
/*
 * NOTE: Some defines in windows.h and other common header files in windows
 * can cause the inline version of these handy macros to fail. As a result it
 * is easier to just provide the macro style defines...
 *
 * ALSO NOTE: that "inline" is a c++ -ism, so this part only applies if
 * a c++ compiler is being used!
 */

#if defined (__cplusplus) && !(defined(_WIN32) || defined(_WINDOWS) || defined(_MACINTOSH))
    /* inline function versions */
    inline int min(int a, int b) 
    {
            return a < b ? a : b;
    }
     
    inline int max(int a, int b) 
    {
            return a > b ? a : b;
    }
#else /* macro versions */
#       ifndef max
#               define max(a, b)  (((a) > (b)) ? (a) : (b))
#       endif
#       ifndef min
#               define min(a, b)  (((a) < (b)) ? (a) : (b))
#       endif
#endif /* end of #if !(defined(_WIN32) || defined(_WINDOWS)) */

/*--------------------------------------------------------------------------
|   ZeroInit - initializes a block of memory with zeros
--------------------------------------------------------------------------*/
#define ZeroInit(pb)    memset((void *)pb,0,sizeof(*(pb))) 

#ifndef __MACTYPES__
typedef unsigned char   Byte;
#endif

/*
/////////////////////////////////////////////////////////////////////////////
// PNEXPORT needed for RA.H and RAGUI.H, should be able to be defined
// and used in cross platform code...
/////////////////////////////////////////////////////////////////////////////
*/
#if defined(_WIN32) || defined(_WINDOWS)
#ifdef _WIN32
#define PNEXPORT            __declspec(dllexport) __stdcall
#define PNEXPORT_PTR        __stdcall *
#else /* Windows, but not 32 bit... */
#define PNEXPORT            _pascal __export
#define PNEXPORT_PTR        _pascal *
#define WAVE_FORMAT_PCM	    1
#define LPCTSTR				LPCSTR
#endif
#else /* Not Windows... */
#define PNEXPORT
#define PNEXPORT_PTR        *
#endif

#if defined(_WIN32) || defined(_WINDOWS) || defined (_MACINTOSH)|| defined (_UNIX)  
typedef void (*RANOTIFYPROC)( void* );
#endif

#if defined(EXPORT_CLASSES) && defined(_WINDOWS)     
#ifdef _WIN32
#define PNEXPORT_CLASS __declspec(dllexport)
#else
#define PNEXPORT_CLASS __export
#endif // _WIN32
#else
#define PNEXPORT_CLASS
#endif // EXPORT_CLASSES 


/*
 *  STDMETHODCALLTYPE
 */
#ifndef STDMETHODCALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#ifdef _MPPC_
#define STDMETHODCALLTYPE       __cdecl
#else
#define STDMETHODCALLTYPE       __stdcall
#endif
#elif defined(_WIN16)
// XXXTW I made the change below on 5/18/98.  The __export was causing 
//       conflicts with duplicate CPNBuffer methods in being linked into
//       rpupgrd and rpdestpn.  Also, the warning was "export imported".
//       This was fixed by removing the __export.  The __export is also
//       causing the same problem in pndebug methods.
//#define STDMETHODCALLTYPE       __export far _cdecl
#define STDMETHODCALLTYPE       far _cdecl
#else
#define STDMETHODCALLTYPE
#endif
#endif

/*
 *  STDMETHODVCALLTYPE  (V is for variable number of arguments)
 */
#ifndef STDMETHODVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDMETHODVCALLTYPE      __cdecl
#else
#define STDMETHODVCALLTYPE
#endif
#endif

/*
 *  STDAPICALLTYPE
 */
#ifndef STDAPICALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#define STDAPICALLTYPE          __stdcall
#elif defined(_WIN16)
#define STDAPICALLTYPE          __export FAR PASCAL
#else
#define STDAPICALLTYPE
#endif
#endif

/*
 *  STDAPIVCALLTYPE (V is for variable number of arguments)
 */
#ifndef STDAPIVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDAPIVCALLTYPE         __cdecl
#else
#define STDAPIVCALLTYPE
#endif
#endif

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      PN_GET_MAJOR_VERSION()
//
//  Purpose:
//
//      Returns the Major version portion of the encoded product version 
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface 
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The major version number of the RealAudio application interface DLL
//
//
*/
#define PN_GET_MAJOR_VERSION(prodVer)   ((prodVer >> 28) & 0xF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      PN_GET_MINOR_VERSION()
//
//  Purpose:
//
//      Returns the minor version portion of the encoded product version 
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface 
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The minor version number of the RealAudio application interface DLL
//
//
*/
#define PN_GET_MINOR_VERSION(prodVer)   ((prodVer >> 20) & 0xFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      PN_GET_RELEASE_NUMBER()
//
//  Purpose:
//
//      Returns the release number portion of the encoded product version 
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface 
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The release number of the RealAudio application interface DLL
//
//
*/
#define PN_GET_RELEASE_NUMBER(prodVer)  ((prodVer >> 12) & 0xFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      PN_GET_BUILD_NUMBER()
//
//  Purpose:
//
//      Returns the build number portion of the encoded product version 
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface 
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The build number of the RealAudio application interface DLL
//
//
*/
#define PN_GET_BUILD_NUMBER(prodVer)    (prodVer & 0xFFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      PN_ENCODE_PROD_VERSION()
//
//  Purpose:
//
//      Encodes a major version, minor version, release number, and build
//      number into a product version for testing against the product version
//      of the RealAudio application interface DLL returned from a call to 
//      RaGetProductVersion().
//
//  Parameters:
//
//      major
//      The major version number to encode.
//
//      mimor
//      The minor version number to encode.
//
//      release
//      The release number to encode.
//
//      build
//      The build number to encode.
//
//  Return:
//
//      The encoded product version.
//
//  NOTES:
//      
//      Macintosh DEVELOPERS especially, make sure when using the PN_ENCODE_PROD_VERSION
//      that you are passing a ULONG32 or equivalent for each of the parameters. 
//      By default a number passed in as a constant is a short unless it requires more room,
//      so designate the constant as a long by appending a L to the end of it.  
//      Example:
//          WORKS:
//              PN_ENCODE_VERSION(2L,1L,1L,0L);
//
//          DOES NOT WORK:
//              PN_ENCODE_VERSION(2,1,1,0);
//
*/

#define PN_ENCODE_PROD_VERSION(major,minor,release,build)   \
            ((ULONG32)((ULONG32)major << 28) | ((ULONG32)minor << 20) | \
            ((ULONG32)release << 12) | (ULONG32)build)

#define PN_ENCODE_ADD_PRIVATE_FIELD(ulversion,ulprivate) \
			((ULONG32)((ULONG32)(ulversion) & (UINT32)0xFFFFFF00) | (ULONG32)(ulprivate) )

#define PN_EXTRACT_PRIVATE_FIELD(ulversion)(ulversion & (UINT32)0xFF)

#define PN_EXTRACT_MAJOR_VERSION(ulversion) ((ulversion)>>28)
#define PN_EXTRACT_MINOR_VERSION(ulversion) (((ulversion)>>20) & (UINT32)0xFF)
	
#ifdef _AIX
    typedef int                 tv_sec_t;
    typedef int                 tv_usec_t;
#elif (defined _HPUX)
    typedef UINT32              tv_sec_t;
    typedef INT32               tv_usec_t;
#else
    typedef INT32               tv_sec_t;
    typedef INT32               tv_usec_t;
#endif /* _AIX */

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifdef __GNUC__
#define PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME friend class SilenceGCCWarnings;
#else
#define PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME
#endif

typedef ULONG32 PNXRESOURCE;
typedef ULONG32 PNXHANDLE;
typedef ULONG32 PNXIMAGE;

// Macro which indicates that a particular variable is unused. Use this to
// avoid compiler warnings.
#define UNUSED(x)

/*
 * For VC++ 6.0 and higher we need to include this substitute header file
 * in place of the standard header file basetsd.h, since this standard 
 * header file conflicts with our definitions.
 */
#if defined(_MSC_VER) && (_MSC_VER > 1100)
#include "pnbastsd.h"
#endif

#endif /* _PNTYPES_H_ */
