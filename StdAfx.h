// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__C64BEF8C_1AE1_4D48_B3AC_8ACB164B334C__INCLUDED_)
#define AFX_STDAFX_H__C64BEF8C_1AE1_4D48_B3AC_8ACB164B334C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// Make section alignment really small
#pragma comment(linker, "/FILEALIGN:512")
//#pragma comment(linker, "/ALIGN:4096")

// Merge sections
//#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.text=.text")
#pragma comment(linker, "/MERGE:.reloc=.text")

// Favour small code
#pragma optimize("gsy", on)


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>	    	// MFC support for Internet Explorer 4 Common Controls
#include <afxpriv.h>
#include <afxmt.h>
#include <afxole.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			    // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>

#include <direct.h>
#include <math.h>
#include <mmsystem.h>
#include <atlbase.h>
#include <multimon.h>
#include <setupapi.h>

//#include <windows.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define B115200 115200
#define B921600 921600

typedef long speed_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C64BEF8C_1AE1_4D48_B3AC_8ACB164B334C__INCLUDED_)
