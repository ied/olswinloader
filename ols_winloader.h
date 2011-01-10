// ols_winloader.h : main header file for the OLS_WINLOADER application
//

#if !defined(AFX_OLS_WINLOADER_H__5FDEC821_01F9_4AC6_B918_EDFEE082AAAC__INCLUDED_)
#define AFX_OLS_WINLOADER_H__5FDEC821_01F9_4AC6_B918_EDFEE082AAAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWinLoaderApp:
// See ols_winloader.cpp for the implementation of this class
//

class CWinLoaderApp : public CWinApp
{
public:
	CWinLoaderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinLoaderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWinLoaderApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern CWinLoaderApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OLS_WINLOADER_H__5FDEC821_01F9_4AC6_B918_EDFEE082AAAC__INCLUDED_)
