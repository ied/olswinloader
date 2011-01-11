#if !defined(AFX_ADVANCEDDIALOG_H__66C25A8C_9E3F_4EB9_97F4_BB87B41F5AD3__INCLUDED_)
#define AFX_ADVANCEDDIALOG_H__66C25A8C_9E3F_4EB9_97F4_BB87B41F5AD3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AdvancedDialog.h : header file
//

#include "ols_winloaderDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CAdvancedDialog dialog

class CAdvancedDialog : public CDialog
{
// Construction
public:
	CAdvancedDialog(CWinLoaderDialog* pParent = NULL);   // standard constructor
  CWinLoaderDialog *m_parent;
  int m_port_speed;

// Dialog Data
	//{{AFX_DATA(CAdvancedDialog)
	enum { IDD = IDD_ADVANCED_DIALOG };
	CComboBox	m_portspeed_combo;
	BOOL	m_backup_fpga;
	BOOL	m_ignore_flashrom_jedec;
	BOOL	m_noerase_flashrom;
	BOOL	m_query_ols_status;
	BOOL	m_run_after_upload;
	BOOL	m_selftest;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvancedDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvancedDialog)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDefault();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADVANCEDDIALOG_H__66C25A8C_9E3F_4EB9_97F4_BB87B41F5AD3__INCLUDED_)
