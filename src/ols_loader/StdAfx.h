// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0F10237C_3BCB_4484_A91D_2763EE435B4C__INCLUDED_)
#define AFX_STDAFX_H__0F10237C_3BCB_4484_A91D_2763EE435B4C__INCLUDED_

#ifdef WIN32

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define B115200 115200
#define B921600 921600
#define _T(x) (x)

typedef long speed_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // WIN32

#endif // !defined(AFX_STDAFX_H__0F10237C_3BCB_4484_A91D_2763EE435B4C__INCLUDED_)
