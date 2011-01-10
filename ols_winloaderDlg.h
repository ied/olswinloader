// ols_winloaderDlg.h : header file
//

#if !defined(AFX_OLS_WINLOADERDLG_H__30762BD2_CB11_4FEB_9B0F_DBD473180D84__INCLUDED_)
#define AFX_OLS_WINLOADERDLG_H__30762BD2_CB11_4FEB_9B0F_DBD473180D84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "enumser.h"
#include "ButtonVE_Image.h"

#define INI_SECTION "OLSWINLOADER"

/////////////////////////////////////////////////////////////////////////////
// CWinLoaderDialog dialog

class CWinLoaderDialog : public CDialog
{
// Construction
public:
	CWinLoaderDialog(CWnd* pParent = NULL);	// standard constructor

  // Attributes...
  CString m_target_file;
  CString m_backup_file;
  int m_target_format;      // File format (based on filename extension).
  int m_target_portnum;     // Default port number...
  CString m_port;
  int m_port_speed;
  const TCHAR *m_flashid_name;

	bool m_backup_fpga; //
	bool m_ignore_flashrom_jedec; //
	bool m_noerase_flashrom; //
	bool m_query_ols_status; //
	bool m_run_after_upload; //
	bool m_selftest; //

  CEnumerateSerial m_portlist;
  int m_portlist_comwidth; // pixel width of widest COM port label (ie: "COM12")
  int m_portlist_height; // pixel height of highest COM port label
  CToolTipCtrl* m_pToolTip;

  // Operations...
  void UpdateOKButton();

  // Worker thread handler...
  bool CheckUserAbort(int tenths);
	static DWORD WINAPI ThreadProc (LPVOID lpArg);
	DWORD ThreadProc (void);
  HANDLE m_hThread; // worker thread
  bool m_cancel_worker;

// Dialog Data
	//{{AFX_DATA(CWinLoaderDialog)
	enum { IDD = IDD_OLS_WINLOADER_DIALOG };
	CButtonVE_Image	m_advanced_button;
	CButtonVE_Image	m_refresh_button;
	CStatic	m_status2;
	CStatic	m_status;
	CListBox	m_analyzerport_list;
	CEdit	m_imagefile;
	CButton	m_browse_button;
	CButtonVE_Image	m_ok_button;
	CButtonVE_Image	m_cancel_button;
	CButtonVE_Image	m_close_button;
	CStatic	m_instructions;
	CProgressCtrl	m_progress;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinLoaderDialog)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWinLoaderDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnClose();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void OnCancel();
	afx_msg void OnBrowse();
	afx_msg void OnRefreshPortList();
	afx_msg void OnUpdateImagefile();
	afx_msg void OnSelchangeAnalyzerportList();
	afx_msg void OnAdvance();
	//}}AFX_MSG
  LRESULT OnUploadStatus(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OLS_WINLOADERDLG_H__30762BD2_CB11_4FEB_9B0F_DBD473180D84__INCLUDED_)
