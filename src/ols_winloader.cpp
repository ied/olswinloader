
//////////////////////////////////////////////////////////////////////////////////
//
// OLS_WINLOADER.CPP
//
// Copyright (C) 2011 Ian Davis
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
//
//////////////////////////////////////////////////////////////////////////////////
//
// Details: http://www.mygizmos.org/ols
//
// Main application for the Open Logic Sniffer Windows FPGA Image Loader.
// Performs basic initialization then kicks off the dialog.
//
//////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ols_winloader.h"
#include "ols_winloaderDlg.h"
#include "Misc.h"
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinLoaderApp

BEGIN_MESSAGE_MAP(CWinLoaderApp, CWinApp)
	//{{AFX_MSG_MAP(CWinLoaderApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinLoaderApp construction

CWinLoaderApp::CWinLoaderApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWinLoaderApp object

CWinLoaderApp theApp;


/////////////////////////////////////////////////////////////////////////////
//
// CWinLoaderApp initialization
//
BOOL CWinLoaderApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

  // Setup INI filename in user's "Application Data" folder...
  TCHAR appdata[4096];
  SHGetFolderPath(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,appdata);
  CString temp(appdata);
  temp = AddSlash(temp)+"ols_winloader.ini";
  if (m_pszProfileName) {free((void*)m_pszProfileName); m_pszProfileName=NULL;}
  m_pszProfileName = _tcsdup(temp);

  // User gave us an image file on command line?
  if (__argc>1) {
  }

/*
  // If we're being uninstalled, remove filename associations from registry...
  if ((__argc>1) && (stricmp(__targv[1],"/UNINSTALL")==0)) {
    CRegMime::UpdateRegistration();
    return FALSE;
  }
*/

	CWinLoaderDialog dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
