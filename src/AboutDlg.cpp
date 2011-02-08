
//////////////////////////////////////////////////////////////////////////////////
//
// ABOUTDLG.CPP
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
// The about dialog which shows copyright & give reference info...
//
//////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_LINKS, m_links);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_STATIC_ICON, OnStaticIcon)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
/*
  m_links.SetWindowText (
    "For more information see:\n"
    "   <a href=\"http://dangerousprototypes.com/open-logic-sniffer\">http://dangerousprototypes.com/open-logic-sniffer</a>\n"
    "   <a href=\"http://www.gadgetfactory.net/gf/project/bufferflylogic\">http://www.gadgetfactory.net/gf/project/bufferflylogic</a>\n"
    "   <a href=\"http://www.mygizmos.org/ols\">http://www.mygizmos.org/ols</a>\n"
  );
*/
  // Make Windows use the 48x48 icon...
  CWnd *iconwnd = GetDlgItem(IDC_STATIC_ICON);
  HICON hIcon = (HICON)(LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR));
  iconwnd->SendMessage(STM_SETICON, (WPARAM)hIcon, 0L);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CAboutDlg::OnStaticIcon() 
{
	ShellExecute(::GetDesktopWindow(), _T("open"), _T("http://www.mygizmos.org/ols/#WINLOADER"), NULL,NULL,SW_SHOW);
}
