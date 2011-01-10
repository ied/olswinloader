
//////////////////////////////////////////////////////////////////////////////////
//
// ADVANCEDDIALOG.CPP
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
//  Supports selecting advanced options for configuring the FPGA update process.
//
//////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "ols_winloader.h"
#include "AdvancedDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


struct {int commprop_SettableBaud; int baud;} g_ols_portspeed_list[] = {
  {BAUD_19200,  19200},
  {BAUD_38400,  38400},
  {BAUD_57600,  57600},
  {BAUD_115200, 115200},
  {BAUD_USER,   230400},
  {BAUD_USER,   460800},
  {BAUD_USER,   921600},
  {0,0}
};

/////////////////////////////////////////////////////////////////////////////
// CAdvancedDialog dialog


CAdvancedDialog::CAdvancedDialog(CWinLoaderDialog* pParent /*=NULL*/)
	: CDialog(CAdvancedDialog::IDD, pParent)
{
  m_parent = pParent;
  m_port_speed = (pParent) ? pParent->m_port_speed : FALSE;
	//{{AFX_DATA_INIT(CAdvancedDialog)
	m_backup_fpga = (pParent) ? pParent->m_backup_fpga : FALSE;
	m_ignore_flashrom_jedec = (pParent) ? pParent->m_ignore_flashrom_jedec : FALSE;
	m_noerase_flashrom = (pParent) ? pParent->m_noerase_flashrom : FALSE;
	m_query_ols_status = (pParent) ? pParent->m_query_ols_status : FALSE;
	m_run_after_upload = (pParent) ? pParent->m_run_after_upload : FALSE;
	m_selftest = (pParent) ? pParent->m_selftest : FALSE;
	//}}AFX_DATA_INIT
}


void CAdvancedDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvancedDialog)
	DDX_Control(pDX, IDC_PORTSPEED, m_portspeed_combo);
	DDX_Check(pDX, IDC_BACKUP, m_backup_fpga);
	DDX_Check(pDX, IDC_IGNOREJEDIC, m_ignore_flashrom_jedec);
	DDX_Check(pDX, IDC_NOERASE, m_noerase_flashrom);
	DDX_Check(pDX, IDC_OLSSTATUS, m_query_ols_status);
	DDX_Check(pDX, IDC_RUNMODE, m_run_after_upload);
	DDX_Check(pDX, IDC_SELFTEST, m_selftest);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvancedDialog, CDialog)
	//{{AFX_MSG_MAP(CAdvancedDialog)
	ON_BN_CLICKED(IDDEFAULT, OnDefault)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdvancedDialog message handlers

BOOL CAdvancedDialog::OnInitDialog() 
{
  CString temp;
	CDialog::OnInitDialog();
	
	// Init port speed dialog...
  for (int i=0; g_ols_portspeed_list[i].baud>0; i++) { 
    temp.Format (_T("%d bps"), g_ols_portspeed_list[i].baud);
    int cindex = m_portspeed_combo.AddString(temp);
    m_portspeed_combo.SetItemData(cindex, g_ols_portspeed_list[i].baud);
    if (m_port_speed == g_ols_portspeed_list[i].baud)
      m_portspeed_combo.SetCurSel(cindex);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CAdvancedDialog::OnOK() 
{
  UpdateData();

  m_port_speed = B921600;
  int index = m_portspeed_combo.GetCurSel();
  if (index>=0) m_port_speed = m_portspeed_combo.GetItemData(index);

  if (m_parent) {
    m_parent->m_port_speed = m_port_speed;
	  m_parent->m_backup_fpga = m_backup_fpga>0;
	  m_parent->m_ignore_flashrom_jedec = m_ignore_flashrom_jedec>0;
	  m_parent->m_noerase_flashrom = m_noerase_flashrom>0;
	  m_parent->m_query_ols_status = m_query_ols_status>0;
	  m_parent->m_run_after_upload = m_run_after_upload>0;
	  m_parent->m_selftest = m_selftest>0;
  }

	theApp.WriteProfileInt(INI_SECTION,"PortSpeed",m_port_speed);
	theApp.WriteProfileInt(INI_SECTION,"BackupFPGA",m_backup_fpga);
	theApp.WriteProfileInt(INI_SECTION,"IgnoreFlashROMJedec",m_ignore_flashrom_jedec);
	theApp.WriteProfileInt(INI_SECTION,"NoEraseFlashROM",m_noerase_flashrom);
	theApp.WriteProfileInt(INI_SECTION,"QueryOLSStatus",m_query_ols_status);
	theApp.WriteProfileInt(INI_SECTION,"RunAfterUpload",m_run_after_upload);
	theApp.WriteProfileInt(INI_SECTION,"PerformSelfTest",m_selftest);
	
	CDialog::OnOK();
}


void CAdvancedDialog::OnDefault() 
{
	m_port_speed = B921600;
  for (int i=0; g_ols_portspeed_list[i].baud>0; i++) { 
    if (m_port_speed == (int)m_portspeed_combo.GetItemData(i))
      m_portspeed_combo.SetCurSel(i);
  }

	m_backup_fpga = false;
	m_ignore_flashrom_jedec = false;
	m_noerase_flashrom = false;
	m_query_ols_status = false;
	m_run_after_upload = true;
	m_selftest = true;
  UpdateData(FALSE);
}
