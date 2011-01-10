// FileNameDialogEx.h: interface for the CFileNameDialogEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILENAMEDIALOGEX_H__5CAB092B_9B9F_4F39_B586_76790C1824A5__INCLUDED_)
#define AFX_FILENAMEDIALOGEX_H__5CAB092B_9B9F_4F39_B586_76790C1824A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct OPENFILENAME2000 : public OPENFILENAME {
  void *pvReserved; 
  DWORD dwReserved; 
  DWORD FlagsEx;
};


#define OFNX_FORCE_CENTER     0x00000001
#define OFNX_ENABLE_PLACEBAR  0x00000002


typedef TCHAR strtype[1024];

//
// Build file filters CFileDialog.   Example Usage:
//   filter.AddFilter(IDR_MP3TYPE);
//   filter.AddFilter("Sound Files",IDR_MIDITYPE,IDR_WAVETYPE,-1);
//   filter.AddFilter(-1);
//
// Note: The last line should either be:
//   filter.AddFilter(-1);
//     -or-
//   filter += "|";
//
class CFileFilterString : public CString
{
public:
  void AddExt(CString desc, CString ext) 
  {
    CString temp;
    temp.Format("%s (*%s)|*%s|",desc,ext,ext);
    (*this) += temp;
  }
  void AddFilter(int resource_id) 
  {
    CString temp, desc, ext;
    strtype szFullText;

    if (resource_id<0) {
      AfxLoadString(AFX_IDS_ALLFILTER, szFullText);
      temp.Format("%s|*.*||",szFullText);
    }
    else {
      AfxLoadString(resource_id, szFullText, sizeof(szFullText));
      AfxExtractSubString(desc, szFullText, 3, '\n');
      AfxExtractSubString(ext, szFullText, 4, '\n');
      temp.Format("%s|*%s|",desc,ext);
    }
    (*this) += temp;
  }
  void AddFilter (char *msg, ...)
  {
    CString temp, ext, dlist, elist;
    strtype szFullText;
    va_list args;
    va_start(args,msg);          /* get variable arg pointer */
    int count=0;
    int resource_id = va_arg(args,int);
    while (resource_id>0) {
      AfxLoadString(resource_id, szFullText, sizeof(szFullText));
      AfxExtractSubString(ext, szFullText, 4, '\n');
      dlist += (count>0) ? ", " : "*.";
      dlist += ext.Mid(1);
      elist += (count>0) ? ";*" : "*";
      elist += ext;
      resource_id = va_arg(args,int);
      count++;
    }
    va_end(args);                    /* finish the arglist */
    if (count==0)
      temp.Format("%s (*.*)|*.*||",msg);
    else temp.Format("%s (%s)|%s|",msg,dlist,elist);
    (*this) += temp;
  }
};



class CFileNameDialogEx  
{
public:
	CFileNameDialogEx(
    BOOL bOpenFileDialog, 
    LPCTSTR window_title = NULL,
    LPCTSTR initial_dir = NULL,
    LPCTSTR lpszDefExt = NULL, 
    LPCTSTR lpszFileName = NULL,
	  DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
    DWORD dwFlagsEx = OFNX_ENABLE_PLACEBAR,
    LPCTSTR lpszFilter = NULL, 
    CWnd* pParentWnd = NULL);

  CFileNameDialogEx( // CFileDialog compatible constructor
    BOOL bOpenFileDialog, 
    LPCTSTR lpszDefExt = NULL, 
    LPCTSTR lpszFileName = NULL, 
    DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
    LPCTSTR lpszFilter = NULL, 
    CWnd* pParentWnd = NULL);

	virtual ~CFileNameDialogEx() {Cleanup();}

  // Attributes...
  BOOL m_bOpenFileDialog;
  DWORD m_dwFlagsEx;
  HWND m_parent_hWnd;
  CRect m_parent_rect;
  HWND m_hWnd;

  OPENFILENAME2000 m_ofn;
  TCHAR m_szFile[MAX_PATH];
  TCHAR m_szFileTitle[MAX_PATH];
  TCHAR m_szCurDir[MAX_PATH];
  CString m_strFilter;
  BOOL m_own_filterhook;

  // Pointer to ourself, for use by callbacks...
  static CFileNameDialogEx *g_this;
  static HHOOK g_old_filterhook;

  // Callbacks...
  static UINT CALLBACK OpenFilenameHook(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK WindowsFilterHook(int nCode, WPARAM wParam, LPARAM lParam);

  // Operations...
  void Init(
    BOOL bOpenFileDialog, 
    LPCTSTR window_title, LPCTSTR initial_dir,
    LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	  DWORD dwFlags, DWORD dwFlagsEx, LPCTSTR lpszFilter,
    CWnd* pParentWnd);
  void Cleanup();
  int DoModal();

  BOOL IsPlacesAvailable();
  void CenterWindow();
  void SetDefExt(LPCSTR lpsz);
  void SetControlText(int nID, LPCSTR lpsz);
  CString UpdateFileExt(const char *target);

  // Helpers for parsing filename...
  CString GetDefaultFileExt();
  CString GetPathName();
  CString GetFileName();
  CString GetFileExt();
  CString GetFileTitle();
  BOOL GetReadOnlyPref() const
	  {return m_ofn.Flags & OFN_READONLY ? TRUE : FALSE;}

	// Enumerating multiple file selections
	POSITION GetStartPosition();
	CString GetNextPathName(POSITION& pos);

  // OFN hook event handlers...
  virtual UINT OnShareViolation(LPCTSTR);
  virtual BOOL OnFileNameOK();
  virtual void OnLBSelChangedNotify(UINT, UINT, UINT);
  virtual void OnInitDone();
  virtual void OnFileNameChange();
  virtual void OnFolderChange();
  virtual void OnTypeChange();
};

#endif // !defined(AFX_FILENAMEDIALOGEX_H__5CAB092B_9B9F_4F39_B586_76790C1824A5__INCLUDED_)
