//
// FILENAMEDIALOGEX.CPP
// Code contained in this file is hereby released to the public domain. 
// Ian Davis, 1/8/2011
//

//
// CFileNameDialogEx class.   Alternate to CFileDialog, 
// providing control over position of dialog.  Also shows
// Win2k/XP version of dialog when possible.
//
#include "stdafx.h"
#include "FileNameDialogEx.h"
#include "dlgs.h" // to supply "edt1" constant

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CFileNameDialogEx *CFileNameDialogEx::g_this = NULL;
HHOOK CFileNameDialogEx::g_old_filterhook = NULL;


//
// Return TRUE if larger Win2k/XP OPENFILENAME structure available...
//
BOOL CFileNameDialogEx::IsPlacesAvailable() 
{
  OSVERSIONINFO osvi;
  memset (&osvi,0,sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  if (!GetVersionEx(&osvi)) return FALSE;
  return ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5));
}



//
// ComCtl32 uses this on the -first- use of GetOpenFileName only.  
// Afterwards if the OFN_ENABLESIZING is specified, it maintains an 
// internal tracking state of the dialog position & size.  Thus the
// window filter hook.
//
// This OFN hook is mainly to avoid a momentary visual glitch of the 
// OpenFileName dialog appearing ala ComCtl32 before being moved by the 
// filter hook below.  Joy.
//
// Also allows handling of notify events, which are dispatched to
// virtual functions for overriding in derived classes...
//
UINT CALLBACK CFileNameDialogEx::OpenFilenameHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // Center window...
  if ((message == WM_INITDIALOG) && g_this) {
    if (g_this->m_hWnd==NULL) g_this->m_hWnd = ::GetParent(hWnd);
    g_this->CenterWindow();
    return 0;
  }

  if (message==WM_NOTIFY) {
    OFNOTIFY* pNotify = (OFNOTIFY*)lParam;
    switch (pNotify->hdr.code) {
      case CDN_INITDONE:
        g_this->OnInitDone();
        break;

      case CDN_SELCHANGE:
        g_this->OnFileNameChange();
        break;

      case CDN_FOLDERCHANGE:
        g_this->OnFolderChange();
        break;

      case CDN_SHAREVIOLATION:
        return g_this->OnShareViolation(pNotify->pszFile);

      case CDN_FILEOK:
        return g_this->OnFileNameOK();

      case CDN_TYPECHANGE:
        g_this->OnTypeChange();
        break;
    }
  }

  return 0;
}



//
// Force ComCtl32 to use our placement on all subsequent calls.  All messages 
// of our thread pass through here, but we're only interested in WM_INITDIALOG...
//
LRESULT CALLBACK CFileNameDialogEx::WindowsFilterHook(int nCode, WPARAM wParam, LPARAM lParam)
{
  CWPRETSTRUCT *cwp = (CWPRETSTRUCT *)lParam;
  if ((nCode>=0) && cwp && g_this)
    if (cwp->message == WM_INITDIALOG)
      if (g_this->m_dwFlagsEx & OFNX_FORCE_CENTER) {
        if (g_this->m_hWnd==NULL) g_this->m_hWnd = cwp->hwnd;
        g_this->CenterWindow();
      }

  return CallNextHookEx(CFileNameDialogEx::g_old_filterhook, nCode, wParam, lParam);
}



//
// Constructors...
//
CFileNameDialogEx::CFileNameDialogEx(
  BOOL bOpenFileDialog, 
  LPCTSTR window_title, LPCTSTR initial_dir,
  LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
  DWORD dwFlags, DWORD dwFlagsEx, LPCTSTR lpszFilter,
  CWnd* pParentWnd)
{
  Init(bOpenFileDialog, window_title, initial_dir, 
    lpszDefExt, lpszFileName, dwFlags, dwFlagsEx, 
    lpszFilter, pParentWnd);
}


CFileNameDialogEx::CFileNameDialogEx( // CFileDialog compatible...
  BOOL bOpenFileDialog, 
  LPCTSTR lpszDefExt, LPCTSTR lpszFileName, 
  DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
{
  Init(bOpenFileDialog, "", NULL, 
    lpszDefExt, lpszFileName, dwFlags, 0, 
    lpszFilter, pParentWnd);
}


void CFileNameDialogEx::Init(
  BOOL bOpenFileDialog, 
  LPCTSTR window_title, LPCTSTR initial_dir,
  LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
  DWORD dwFlags, DWORD dwFlagsEx, LPCTSTR lpszFilter,
  CWnd* pParentWnd)
{
  m_own_filterhook = FALSE;
  m_bOpenFileDialog = bOpenFileDialog;
  m_dwFlagsEx = dwFlagsEx;
  m_szFile[0]=0;
  m_szFileTitle[0]=0;
  m_szCurDir[0]=0;
  m_hWnd = NULL;

  // If no parent given, assume desktop window...
  if (pParentWnd==NULL)
    m_parent_hWnd = ::GetDesktopWindow();
  else m_parent_hWnd = pParentWnd->GetSafeHwnd();

  // Copy default filename if given...
  if (lpszFileName) strcpy (m_szFile, lpszFileName);

  // Get current folder if initial folder not specified...
  if (initial_dir==NULL)
    ::GetCurrentDirectory((sizeof(m_szCurDir)/sizeof(m_szCurDir[0]))-1,m_szCurDir);
  else strcpy (m_szCurDir,initial_dir);

  // Setup OFN info.  If available & requested, use larger structure
  // so ComCtl32 shows Win2k/XP places bar...
  memset (&m_ofn,0,sizeof(m_ofn));
  if ((dwFlagsEx & OFNX_ENABLE_PLACEBAR) && IsPlacesAvailable())
    m_ofn.lStructSize = sizeof(OPENFILENAME2000);
  else m_ofn.lStructSize = sizeof(OPENFILENAME);

  m_ofn.hInstance = AfxGetResourceHandle();
  m_ofn.hwndOwner = pParentWnd->GetSafeHwnd();
  m_ofn.lpfnHook = CFileNameDialogEx::OpenFilenameHook;
  m_ofn.lpstrTitle = window_title;
  m_ofn.Flags = dwFlags | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;

  m_ofn.lpstrFile = m_szFile;
  m_ofn.nMaxFile = (sizeof(m_szFile)/sizeof(m_szFile[0]))-1; // max # of -characters- (not bytes)
  m_ofn.lpstrDefExt = lpszDefExt;
  m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
  m_ofn.nMaxFileTitle = (sizeof(m_szFileTitle)/sizeof(m_szFileTitle[0]))-1; // max # of -characters- (not bytes)
  m_ofn.lpstrInitialDir = m_szCurDir;

  // Translate filter into commdlg format (lots of \0) -- from MFC CFileDialog...
  m_strFilter = lpszFilter;
  if (lpszFilter != NULL) {
    m_strFilter = lpszFilter;
    LPTSTR pch = m_strFilter.GetBuffer(0); // modify the buffer in place
    // MFC delimits with '|' not '\0'
    while ((pch = _tcschr(pch, '|')) != NULL) *pch++ = '\0';
    m_ofn.lpstrFilter = m_strFilter;
    m_ofn.nFilterIndex = 1;
    // do not call ReleaseBuffer() since the string contains '\0' characters
  }
}



//
// Cleanup.  Make sure filter hook gets unhooked.   Nuke window handle
// to avoid "issues" since dialog no longer exists...
//
void CFileNameDialogEx::Cleanup()
{
  m_hWnd = NULL;
  CFileNameDialogEx::g_this = NULL;

  // Done with hook...
  if (m_own_filterhook) {
    m_own_filterhook = FALSE;
    if (CFileNameDialogEx::g_old_filterhook) {
      UnhookWindowsHookEx(CFileNameDialogEx::g_old_filterhook);
      CFileNameDialogEx::g_old_filterhook = NULL;
    }
  }
}



//
// Simulate the CFileDialog behavior...
//
int CFileNameDialogEx::DoModal()
{
  if (m_parent_hWnd) ::GetWindowRect(m_parent_hWnd, &m_parent_rect);

  // Aquire windows hook so we can override ComCtl32 
  // and position open/save file dialogs where -WE- want...
  ASSERT (CFileNameDialogEx::g_old_filterhook==NULL);
  if (CFileNameDialogEx::g_old_filterhook==NULL) {
    m_own_filterhook = TRUE;
    g_this = this;
    CFileNameDialogEx::g_old_filterhook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, 
      CFileNameDialogEx::WindowsFilterHook, 
      NULL, ::GetCurrentThreadId());
  }

  // See if default filename matches any of filters...
  TCHAR fileext[MAX_PATH];
  _splitpath(m_ofn.lpstrFile,NULL,NULL,NULL,fileext);
  int fileextlen = strlen(fileext);
  if (fileextlen>0) {
    CString filterext;
    bool done=false;
    for (int index=1; !done && (index<32); index++) {
      AfxExtractSubString(filterext, m_ofn.lpstrFilter, 1+(index-1)*2, 0);
      int filterextlen = filterext.GetLength();
      done = (filterextlen==0);
      if (!done) {        
        int pos = filterext.Find(fileext);
        done = (pos+fileextlen)>=filterextlen;
        if (!done) done = filterext.GetAt(pos+fileextlen)==';';
        if (done) m_ofn.nFilterIndex = index; 
      }
    }    
  }

  // Create common dialog.  These return 0 on error.   
  // Non-zero = non-error = it be all good.
  BOOL result=0;
  if (m_bOpenFileDialog)
    result = GetOpenFileName((OPENFILENAME*)&m_ofn);
  else result = GetSaveFileName((OPENFILENAME*)&m_ofn);

  Cleanup();
  return (result) ? IDOK : IDCANCEL; // emulating CFileDialog...
}



//
// Center file dialog in parent window, but keep visible on screen...
//
void CFileNameDialogEx::CenterWindow()
{
  if (m_hWnd) {
    CRect rc; // rect of dialog...
    ::GetWindowRect(m_hWnd,&rc);
    CRect rp(m_parent_rect); // rect of parent...

    int width = rc.Width();
    int height = rc.Height();
    int x = rp.left + (rp.Width() - width)/2;
    int y = rp.top + (rp.Height() - height)/2;

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
      CRect desktop_rc = mi.rcWork;
      if ((x+width)>desktop_rc.right) x = desktop_rc.right-width;
      if ((y+height)>desktop_rc.bottom) y = desktop_rc.bottom-height;
    }

    if (x<0) x = 0;
    if (y<0) y = 0;

    ::SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
  }
}



//
// Set the default filename extension...
//
void CFileNameDialogEx::SetDefExt(LPCSTR lpsz)
{
  ASSERT(::IsWindow(m_hWnd));
  ASSERT(m_ofn.Flags & OFN_EXPLORER);
  ::SendMessage(m_hWnd, CDM_SETDEFEXT, 0, (LPARAM)lpsz);
}



//
// Update current filename with new extension 
// (but only if exactly one filename is listed).
//
CString CFileNameDialogEx::UpdateFileExt(const char *target)
{
  CString tempfile;
  CString newext(GetDefaultFileExt());

  if (!newext.IsEmpty()) {
    if (::IsWindow(m_hWnd)) 
      SetDefExt(newext);

    // Count number of file's...
    int count=0;
    bool anyquote=false;
    enum {IDLE,INQUOTE,INFILE} state = IDLE;
    int targetlen = strlen(target);
    for (int i=0; i<targetlen; i++)
      switch (state) {
        case IDLE :
          if (target[i]=='"') {state = INQUOTE; count++;}
          else {state = INFILE; count++;}
          break;
        case INQUOTE :
          anyquote=true;
          if (target[i]=='"') state = IDLE;
          break;
        case INFILE :
          if (target[i]==' ') state = IDLE;
          break;
      }

    if ((count<2) && !anyquote) {
      // Modify filename if contains old extension...
      if (GetFileTitle().GetLength()==0)
        tempfile.Format("*.%s",newext);
      else tempfile.Format("%s.%s",GetFileTitle(),newext);
      return tempfile;
    }
  }

  tempfile.Empty();
  return tempfile;
}



//
// Get default filename extension...
//
CString CFileNameDialogEx::GetDefaultFileExt()
{
  int nIndex = m_ofn.nFilterIndex;

  // Get new filename extension...
  CString newext;
  if (nIndex==0)
    if (m_ofn.lpstrCustomFilter)
      AfxExtractSubString(newext, m_ofn.lpstrCustomFilter, 1, 0);
    else AfxExtractSubString(newext, m_ofn.lpstrFilter, 1, 0);
  else AfxExtractSubString(newext, m_ofn.lpstrFilter, 1+(nIndex-1)*2, 0);

  if (newext.Find(".*")<0) { // not *.*?
    int pos = newext.Find(";");
    if (pos>0) {
      CString tempfile(newext.Left(pos));
      newext = tempfile;
    }
    if (newext.Left(2)=="*.") newext.Delete(0,2); // remove asterisk
    return newext;
  }

  newext.Empty();
  return newext;
}



//
// Return full pathname (ie: entire "C:\path\basename.ext")
// Adapted from MFC CFileDialog...
//
CString CFileNameDialogEx::GetPathName()
{
  if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL) {
    ASSERT(::IsWindow(m_hWnd));
    CString strResult;
    if (::SendMessage(m_hWnd, CDM_GETSPEC, (WPARAM)MAX_PATH, (LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
      strResult.Empty();
    else strResult.ReleaseBuffer();

    if (!strResult.IsEmpty())
      if (::SendMessage(m_hWnd, CDM_GETFILEPATH, (WPARAM)MAX_PATH, (LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
        strResult.Empty();
      else {
        strResult.ReleaseBuffer();
        return strResult;
      }
  }
  return m_ofn.lpstrFile;
}



//
// Return filename portion of pathname (ie: "basename.ext" of "C:\path\basename.ext")
// Adapted from MFC CFileDialog...
//
CString CFileNameDialogEx::GetFileName()
{
  if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL) {
    ASSERT(::IsWindow(m_hWnd));
    CString strResult;
    if (::SendMessage(m_hWnd, CDM_GETSPEC, (WPARAM)MAX_PATH, (LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
      strResult.Empty();
    else {
      strResult.ReleaseBuffer();
      return strResult;
    }
  }
  return m_ofn.lpstrFileTitle;
}



//
// Return extension of pathname (ie: "ext" of "C:\path\basename.ext")
// Adapted from MFC CFileDialog...
//
CString CFileNameDialogEx::GetFileExt()
{
  if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL) {
    ASSERT(::IsWindow(m_hWnd));
    CString strResult;
    if (::SendMessage(m_hWnd, CDM_GETSPEC, (WPARAM)MAX_PATH, (LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
      strResult.Empty();
    else {
      strResult.ReleaseBuffer();
      int pos = strResult.ReverseFind('.');
      if (pos >= 0)
        return strResult.Right(strResult.GetLength() - pos - 1);
      strResult.Empty();
    }
    return strResult;
  }

  if (m_ofn.nFileExtension == 0)
    return &afxChNil;
  
  return m_ofn.lpstrFile + m_ofn.nFileExtension;
}



//
// Return basename portion of pathname (ie: "basename" from "C:\path\basename.ext")
// Copied from MFC CFileDialog...
//
CString CFileNameDialogEx::GetFileTitle()
{
  CString strResult = GetFileName();
  int pos = strResult.ReverseFind('.');
  if (pos >= 0)
    return strResult.Left(pos);
  return strResult;
}



//
// Send message to active dialog...
// Adapted from MFC CFileDialog...
//
void CFileNameDialogEx::SetControlText(int nID, LPCSTR lpsz)
{
  if (m_hWnd != NULL) {
    ASSERT(::IsWindow(m_hWnd));
    ASSERT(m_ofn.Flags & OFN_EXPLORER);
    ::SendMessage(m_hWnd, CDM_SETCONTROLTEXT, (WPARAM)nID, (LPARAM)lpsz);
  }
}



//
// Enumerating multiple file selections...
// Copied from MFC CFileDialog...
//
POSITION CFileNameDialogEx::GetStartPosition() 
{
  return (POSITION)m_ofn.lpstrFile;
}


CString CFileNameDialogEx::GetNextPathName(POSITION& pos)
{
  BOOL bExplorer = m_ofn.Flags & OFN_EXPLORER;
  TCHAR chDelimiter;
  if (bExplorer)
    chDelimiter = '\0';
  else chDelimiter = ' ';

  LPTSTR lpsz = (LPTSTR)pos;
  if (lpsz == m_ofn.lpstrFile) // first time
  {
    if ((m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0) {
      pos = NULL;
      return m_ofn.lpstrFile;
    }

    // find char pos after first Delimiter
    while(*lpsz != chDelimiter && *lpsz != '\0')
      lpsz = _tcsinc(lpsz);
    lpsz = _tcsinc(lpsz);

    // if single selection then return only selection
    if (*lpsz == 0) {
      pos = NULL;
      return m_ofn.lpstrFile;
    }
  }

  CString strPath = m_ofn.lpstrFile;
  if (!bExplorer)
  {
    LPTSTR lpszPath = m_ofn.lpstrFile;
    while(*lpszPath != chDelimiter)
      lpszPath = _tcsinc(lpszPath);
    strPath = strPath.Left(lpszPath - m_ofn.lpstrFile);
  }

  LPTSTR lpszFileName = lpsz;
  CString strFileName = lpsz;

  // find char pos at next Delimiter
  while(*lpsz != chDelimiter && *lpsz != '\0')
    lpsz = _tcsinc(lpsz);

  if (!bExplorer && *lpsz == '\0')
    pos = NULL;
  else {
    if (!bExplorer)
      strFileName = strFileName.Left(lpsz - lpszFileName);

    lpsz = _tcsinc(lpsz);
    if (*lpsz == '\0') // if double terminated then done
      pos = NULL;
    else pos = (POSITION)lpsz;
  }

  // only add '\\' if it is needed
  if (!strPath.IsEmpty())
  {
    // check for last back-slash or forward slash (handles DBCS)
    LPCTSTR lpsz = _tcsrchr(strPath, '\\');
    if (lpsz == NULL)
      lpsz = _tcsrchr(strPath, '/');
    // if it is also the last character, then we don't need an extra
    if (lpsz != NULL &&
      (lpsz - (LPCTSTR)strPath) == strPath.GetLength()-1)
    {
      ASSERT(*lpsz == '\\' || *lpsz == '/');
      return strPath + strFileName;
    }
  }
  return strPath + '\\' + strFileName;
}


//
// OFN hook event handlers...
// Adapted from MFC CFileDialog...
//
UINT CFileNameDialogEx::OnShareViolation(LPCTSTR)
{
  ASSERT(this);
  return OFN_SHAREWARN; // dialog shows warning of a sharing violation
}


BOOL CFileNameDialogEx::OnFileNameOK()
{
  ASSERT(this);
  return 0; // all filenames ok by default
}


void CFileNameDialogEx::OnLBSelChangedNotify(UINT, UINT, UINT)
{
  ASSERT(this);
}


void CFileNameDialogEx::OnInitDone()
{
  ASSERT(this);
  CenterWindow();
}


void CFileNameDialogEx::OnFileNameChange()
{
  ASSERT(this);
}


void CFileNameDialogEx::OnFolderChange()
{
  ASSERT(this);
}


void CFileNameDialogEx::OnTypeChange()
{
  // Detect changes to filetype.  Change current filename 
  // to match, then ypdate edit control...
  ASSERT(this);
  CString new_filename = UpdateFileExt(GetFileName());
  if (!new_filename.IsEmpty()) SetControlText(edt1,new_filename); // edt1 from "DLGS.H"
}

