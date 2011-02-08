
//////////////////////////////////////////////////////////////////////////////////
//
// OLS_WINLOADERDLG.CPP
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
// The main dialog handler for the Open Logic Sniffer Windows FPGA Image Loader.
// Implements code for handling port list & pathname, and buttons for refreshing.
// showing advanced option dialog, and oh... the background worker thread
// tha actually does the updating (based on ols_loader).
//
//////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ols_winloader.h"
#include "ols_winloaderDlg.h"
#include "FileNameDialogEx.h"
#include "AdvancedDialog.h"
#include "AboutDlg.h"
#include "Misc.h"

#include "ols.h"
#include "pic_bootloader.h"
#include "serial.h"
#include "data_file.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// List of known devices to draw BOLD in the port list.
//
CString g_known_devices[] = {
  _T("Logic Sniffer"), 
  _T("")};


//
// List of known file type resource ID's...
//
int g_fpgafiletypes[] = {
  IDR_HEXFILE,
  IDR_MCSFILE,
  IDR_BINFILE,
  IDR_BITFILE,
  -1};


//
// Local defines...
//
#define FPGA_BACKUP_FILENAME "OLS_FPGAROM_BACKUP"
#define PIC_BACKUP_FILENAME "OLS_PICFIRMWARE_BACKUP"
#define PORTLIST_COLUMNS_MARGIN 10
#define OLS_UPLOAD_STATUS WM_USER+100

#define OLS_WORKERSTATUS_BADPORT                1
#define OLS_WORKERSTATUS_BADSPEED               2
#define OLS_WORKERSTATUS_QUERYID                3
#define OLS_WORKERSTATUS_BADOLSID               4
#define OLS_WORKERSTATUS_JEDIC_READFAIL         5
#define OLS_WORKERSTATUS_JEDIC_UNKNOWN          6
#define OLS_WORKERSTATUS_JEDIC_VALID            7
#define OLS_WORKERSTATUS_OLSSTATUS_READFAIL     8
#define OLS_WORKERSTATUS_OLSSTATUS_VALID        9
#define OLS_WORKERSTATUS_SELFTEST               10
#define OLS_WORKERSTATUS_SELFTESTFAIL           11
#define OLS_WORKERSTATUS_BADBUFFER              12
#define OLS_WORKERSTATUS_READROM                13
#define OLS_WORKERSTATUS_READROMFAIL            14
#define OLS_WORKERSTATUS_FPGA_WRITEFILE         15
#define OLS_WORKERSTATUS_FPGA_WRITEFILE_FAIL    16
#define OLS_WORKERSTATUS_FPGA_WRITEFILE_SUCCESS 17
#define OLS_WORKERSTATUS_READFILE               18
#define OLS_WORKERSTATUS_READFILEFAIL           19
#define OLS_WORKERSTATUS_COUNTDOWN              20
#define OLS_WORKERSTATUS_FPGA_ERASEROM          21
#define OLS_WORKERSTATUS_FPGA_ERASEFAIL         22
#define OLS_WORKERSTATUS_WRITEROM               23
#define OLS_WORKERSTATUS_WRITEROMFAIL           24
#define OLS_WORKERSTATUS_RUNOK                  25
#define OLS_WORKERSTATUS_RUNFAIL                26
#define OLS_WORKERSTATUS_CANCEL                 27
#define OLS_WORKERSTATUS_BOOTLOADER_SWITCH      28
#define OLS_WORKERSTATUS_BOOTLOADER_FAILED      29
#define OLS_WORKERSTATUS_BOOTLOADER_SUCCESS     30
#define OLS_WORKERSTATUS_READPIC                31
#define OLS_WORKERSTATUS_READPIC_FAIL           32
#define OLS_WORKERSTATUS_PIC_WRITEFILE_FAIL     33
#define OLS_WORKERSTATUS_PIC_WRITEFILE_SUCCESS  34
#define OLS_WORKERSTATUS_PIC_ERASEROM           35
#define OLS_WORKERSTATUS_PIC_ERASEFAIL          36
#define OLS_WORKERSTATUS_WRITEPIC               37
#define OLS_WORKERSTATUS_WRITEPIC_FAIL          38
#define OLS_WORKERSTATUS_VERIFYPIC              39
#define OLS_WORKERSTATUS_VERIFYPIC_FAIL         40

//
// Command line parameters...
//
// To batch together, 
//    ols_winloader -E -J -NORUN fpga_image.bit
//    ols_winloader -INTRO pic_firmware.hex
//
#define PARAM_PORTNUM       0
#define PARAM_PORTSPEED     1
#define PARAM_MAKEBACKUP    2
#define PARAM_NOBACKUP      3
#define PARAM_ERASEROM      4
#define PARAM_NOERASEROM    5
#define PARAM_QUERYSTATUS   6
#define PARAM_NOQUERYSTATUS 7
#define PARAM_IGNOREJEDIC   8
#define PARAM_CHECKJEDIC    9
#define PARAM_RUNAFTER      10
#define PARAM_NORUN         11
#define PARAM_SELFTEST      12
#define PARAM_NOSELFTEST    13
#define PARAM_INTRO         14
#define PARAM_DEFAULTS      15

strtabletype g_paramtable[] = {
  {"-P", PARAM_PORTNUM},     {"-PORT", PARAM_PORTNUM},
  {"-T", PARAM_PORTSPEED},   {"-SPEED", PARAM_PORTSPEED},
  {"-B", PARAM_MAKEBACKUP},  {"-BACKUP", PARAM_MAKEBACKUP},
  {"-NB", PARAM_NOBACKUP},   {"-NOBACKUP", PARAM_NOBACKUP},
  {"-R", PARAM_MAKEBACKUP},  {"-READ", PARAM_MAKEBACKUP},
  {"-NR", PARAM_NOBACKUP},   {"-NOREAD", PARAM_NOBACKUP},
  {"-E", PARAM_ERASEROM},    {"-ERASE", PARAM_ERASEROM},
  {"-NE", PARAM_NOERASEROM}, {"-NOERASE", PARAM_NOERASEROM},
  {"-Q", PARAM_QUERYSTATUS}, {"-QUERY", PARAM_QUERYSTATUS}, {"-STATUS", PARAM_QUERYSTATUS},
  {"-NQ", PARAM_NOQUERYSTATUS}, {"-NOQUERY", PARAM_NOQUERYSTATUS}, {"-NOSTATUS", PARAM_NOQUERYSTATUS},
  {"-I", PARAM_IGNOREJEDIC}, {"-IGNORE", PARAM_IGNOREJEDIC}, {"-IGNORE_JEDIC", PARAM_IGNOREJEDIC},
  {"-J", PARAM_CHECKJEDIC},  {"-JEDIC", PARAM_CHECKJEDIC},
  {"-D", PARAM_DEFAULTS},    {"-DEFAULTS", PARAM_DEFAULTS},
  {"-RUN", PARAM_RUNAFTER},
  {"-NORUN", PARAM_NORUN},
  {"-SELFTEST", PARAM_SELFTEST},
  {"-NOTEST", PARAM_NOSELFTEST}, 
  {"-NOSELFTEST", PARAM_NOSELFTEST},
  {"-INTRO", PARAM_INTRO},
  {"",-1}};


/////////////////////////////////////////////////////////////////////////////
// CWinLoaderDialog dialog

CWinLoaderDialog::CWinLoaderDialog(CWnd* pParent /*=NULL*/)
  : CDialog(CWinLoaderDialog::IDD, pParent)
{
  m_flashid_name = NULL;
  m_cancel_worker = false;
  m_hThread = NULL;
  m_pToolTip = NULL;
  m_target_format=-1;
  m_target_portnum=-1;
  OnRefreshPortList();

  m_port_speed = B921600;
  m_backup_fpga = false;
  m_ignore_flashrom_jedec = false;
  m_noerase_flashrom = false;
  m_query_ols_status = false;
  m_run_after_upload = true;
  m_selftest = true;
  m_upload_mode = UPLOAD_FPGA;
  m_update_mode_force = false;

  //{{AFX_DATA_INIT(CWinLoaderDialog)
	//}}AFX_DATA_INIT
  // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinLoaderDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CWinLoaderDialog)
  DDX_Control(pDX, IDADVANCE, m_advanced_button);
  DDX_Control(pDX, IDC_REFRESH, m_refresh_button);
  DDX_Control(pDX, IDC_STATUS2, m_status2);
  DDX_Control(pDX, IDC_STATUS, m_status);
  DDX_Control(pDX, IDC_ANALYZERPORT_LIST, m_analyzerport_list);
  DDX_Control(pDX, IDC_IMAGEFILE, m_imagefile);
  DDX_Control(pDX, IDC_BROWSE, m_browse_button);
  DDX_Control(pDX, IDOK, m_ok_button);
  DDX_Control(pDX, IDCANCEL, m_cancel_button);
  DDX_Control(pDX, IDCLOSE, m_close_button);
  DDX_Control(pDX, IDC_STATIC_HELP, m_instructions);
  DDX_Control(pDX, IDC_PROGRESS, m_progress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWinLoaderDialog, CDialog)
  //{{AFX_MSG_MAP(CWinLoaderDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDCLOSE, OnClose)
  ON_WM_DRAWITEM()
  ON_WM_MEASUREITEM()
  ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
  ON_BN_CLICKED(IDC_REFRESH, OnRefreshPortList)
  ON_EN_UPDATE(IDC_IMAGEFILE, OnUpdateImagefile)
  ON_LBN_SELCHANGE(IDC_ANALYZERPORT_LIST, OnSelchangeAnalyzerportList)
  ON_BN_CLICKED(IDADVANCE, OnAdvance)
  //}}AFX_MSG_MAP
  ON_MESSAGE(OLS_UPLOAD_STATUS, OnUploadStatus)
  ON_MESSAGE(WM_PICBOOTLOADER_STATUS, OnUploadStatus)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWinLoaderDialog message handlers

BOOL CWinLoaderDialog::PreTranslateMessage(MSG* pMsg) 
{
  if (m_pToolTip) m_pToolTip->RelayEvent(pMsg);
  return CDialog::PreTranslateMessage(pMsg);
}


BOOL CWinLoaderDialog::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    CString strAboutMenu;
    strAboutMenu.LoadString(IDS_ABOUTBOX);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);      // Set big icon
  SetIcon(m_hIcon, FALSE);    // Set small icon

  // Create tool tips...
  m_pToolTip = new CToolTipCtrl;
  if (m_pToolTip->Create(this)) {
    m_pToolTip->AddTool(&m_refresh_button,_T("Refresh Port List"));
    m_pToolTip->AddTool(&m_browse_button,_T("Browse for FPGA image or PIC firmware file"));
    m_pToolTip->AddTool(&m_advanced_button,_T("Advanced Configuration Options"));
    m_pToolTip->AddTool(&m_ok_button,_T("Start Image Upload"));
  }
  
  // Setup button images...
  HBITMAP hRefresh = (HBITMAP)(LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_REFRESH), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
  m_refresh_button.ModifyStyle(0,BS_BITMAP);
  m_refresh_button.SetImage(hRefresh);
  m_refresh_button.SetImageShadow(TRUE);

  HBITMAP hStart = (HBITMAP)(LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_START), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
  m_ok_button.ModifyStyle(0,BS_BITMAP|BS_OWNERDRAW);
  m_ok_button.SetImage(hStart);
  m_ok_button.SetImageShadow(TRUE);
  m_ok_button.m_button_default = TRUE;

  HBITMAP hCancel = (HBITMAP)(LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_CANCEL), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
  m_cancel_button.ModifyStyle(0,BS_BITMAP);
  m_cancel_button.SetImage(hCancel);
  m_cancel_button.SetImageShadow(TRUE);

  HBITMAP hAdvanced = (HBITMAP)(LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_ADVANCED), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
  m_advanced_button.ModifyStyle(0,BS_BITMAP);
  m_advanced_button.SetImage(hAdvanced);
  m_advanced_button.SetImageShadow(TRUE);

  // Get defaults from INI file...
  m_target_file = theApp.GetProfileString(INI_SECTION,"TargetFile","");
  m_target_portnum = theApp.GetProfileInt(INI_SECTION,"TargetPort",-1);
  m_port_speed = theApp.GetProfileInt(INI_SECTION,"PortSpeed",B921600);
  m_backup_fpga = theApp.GetProfileInt(INI_SECTION,"BackupFPGA",false)>0;
  m_ignore_flashrom_jedec = theApp.GetProfileInt(INI_SECTION,"IgnoreFlashROMJedec",false)>0;
  m_noerase_flashrom = theApp.GetProfileInt(INI_SECTION,"NoEraseFlashROM",false)>0;
  m_query_ols_status = theApp.GetProfileInt(INI_SECTION,"QueryOLSStatus",false)>0;
  m_run_after_upload = theApp.GetProfileInt(INI_SECTION,"RunAfterUpload",true)>0;
  m_selftest = theApp.GetProfileInt(INI_SECTION,"PerformSelfTest",true)>0;
  m_alternate_intro = false;

  // Parse command line options...
  for (int aindex = 1; aindex < __argc; aindex++) {
    CString temp(__targv[aindex]);
    if ((temp[0] == _T('/')) || (temp[0] == _T('-'))) { // parameters ignored
      switch (strtablecompare(temp,g_paramtable)) {
        case PARAM_PORTNUM : m_target_portnum = atol(__targv[++aindex]); break;
        case PARAM_PORTSPEED : m_port_speed = atol(__targv[++aindex]); break;
        case PARAM_MAKEBACKUP : m_backup_fpga = true; break;
        case PARAM_NOBACKUP : m_backup_fpga = false; break;
        case PARAM_NOERASEROM : m_noerase_flashrom = true; break;
        case PARAM_ERASEROM : m_noerase_flashrom = false; break;
        case PARAM_QUERYSTATUS : m_query_ols_status = true; break; 
        case PARAM_NOQUERYSTATUS : m_query_ols_status = false; break; 
        case PARAM_IGNOREJEDIC : m_ignore_flashrom_jedec = true; break;
        case PARAM_CHECKJEDIC : m_ignore_flashrom_jedec = false; break;
        case PARAM_RUNAFTER : m_run_after_upload = true; break; 
        case PARAM_NORUN : m_run_after_upload = false; break; 
        case PARAM_SELFTEST : m_selftest = true; break;
        case PARAM_NOSELFTEST : m_selftest = false; break;
        case PARAM_INTRO : m_alternate_intro = true; break;
        case PARAM_DEFAULTS : {
          m_port_speed = B921600;
          m_backup_fpga = false;
          m_ignore_flashrom_jedec = false;
          m_noerase_flashrom = false;
          m_query_ols_status = false;
          m_run_after_upload = true;
          m_selftest = true;
          break;
        }
      }
    }
    else if (FileExists(temp)) { // filename?
      CString ext(GetExtensionPortion(temp));

      int target_format = -1;
      for (int i=0; g_fpgafiletypes[i]>=0; i++)
        if (ext.CompareNoCase(GetFiletypeExtension(g_fpgafiletypes[i]))==0) {
          target_format = g_fpgafiletypes[i];
          break;
        }

      if (target_format>=0) m_target_file = temp;
    }
  }

  m_imagefile.SetWindowText(m_target_file);
  m_imagefile.SetSel(m_target_file.GetLength(),-1);

  // Init the port list...
  OnRefreshPortList();

  // Enable/disable OK button depending on selections...
  OnUpdateImagefile();

  // Update introduction if requested...
  if (m_alternate_intro) {
    CWnd *help = GetDlgItem(IDC_STATIC_HELP);
    if (help) {
      CString temp;
      help->GetWindowText(temp);
      int start = temp.Find(_T("On OLS"));
      if (start>0) {
        int end = temp.Find(_T("."),start);
        if (end>0) {
          temp.Delete(start,end-start);
          temp.Insert(start,_T("OLS should already be in UPDATE mode (two green LED's turned on)"));
          help->SetWindowText(temp);
        }
      }
    }
  }
  
  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWinLoaderDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialog::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinLoaderDialog::OnPaint() 
{
  if (IsIconic())
  {
/*
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
*/
  }
  else
  {
    CDialog::OnPaint();
  }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWinLoaderDialog::OnQueryDragIcon()
{
  return (HCURSOR) m_hIcon;
}


//
// Owner draw entries in the port number listbox...
//
void CWinLoaderDialog::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpItem) 
{
  CString temp;
  if ((nIDCtl == IDC_ANALYZERPORT_LIST) && (lpItem != NULL)) {
    int index = lpItem->itemID;
    COLORREF fgcolor, bgcolor, infocolor;
    CRect rc;

    CDC dc;
    dc.Attach(lpItem->hDC);

    if ((lpItem->itemState & ODS_SELECTED)>0) {
      fgcolor = GetSysColor(COLOR_HIGHLIGHTTEXT);
      bgcolor = GetSysColor(COLOR_HIGHLIGHT);
    }
    else {
      fgcolor = GetSysColor(COLOR_WINDOWTEXT);
      bgcolor = GetSysColor(COLOR_WINDOW);
    }

    if ((lpItem->itemState & ODS_DISABLED)>0) {
      if ((lpItem->itemState & ODS_SELECTED)>0) {
        fgcolor = GetSysColor(COLOR_3DFACE);
        bgcolor = GetSysColor(COLOR_HIGHLIGHT);
      }
      else {
        fgcolor = GetSysColor(COLOR_WINDOWTEXT);
        bgcolor = GetSysColor(COLOR_3DFACE);
      }
      if (index==0) {
        GetClientRect(&rc);
        dc.FillSolidRect(rc, bgcolor);
      }
    }

    if (m_portlist.GetNumPorts()<1)
      dc.TextOut(lpItem->rcItem.left, lpItem->rcItem.top, _T("No serial ports found."));
    else {
      infocolor = bgcolor; // color of description

      if ((lpItem->itemState & ODS_DISABLED)==0)
        bgcolor = RGB(GetRValue(bgcolor)*31/32,GetGValue(bgcolor)*31/32,GetBValue(bgcolor)*31/32);

      rc = lpItem->rcItem;
      rc.right = rc.left+m_portlist_comwidth+PORTLIST_COLUMNS_MARGIN/2;
      dc.FillSolidRect(rc, bgcolor);

      rc.left = rc.right;
      rc.right = lpItem->rcItem.right;
      dc.FillSolidRect(rc, infocolor);

      dc.SetTextColor(fgcolor);
      dc.SetBkColor(bgcolor);

      CFont font,font2,*oldfont;
      font.CreateStockObject(ANSI_VAR_FONT);

      int portnum = m_portlist.GetPortNum(index);
      CString desc = m_portlist.GetDescription(index);

      COMMPROP *cp;
      cp = m_portlist.GetCommPropPtr(index);

      // See if device is a known flavor...
      BOOL known_desc = FALSE;
      for (int i=0; !known_desc && !g_known_devices[i].IsEmpty(); i++) 
        if (desc.Find(g_known_devices[i])>0) 
          known_desc = TRUE;

      if (known_desc) { // draw known devices in bold...
        LOGFONT lfont;
        font.GetLogFont(&lfont);
        lfont.lfWeight = FW_BOLD;
        font2.CreateFontIndirect(&lfont);  
        oldfont = dc.SelectObject(&font2);
      }
      else oldfont = dc.SelectObject(&font);

      // Draw COM port & description...
      if (portnum == 0xFFFF)
        temp = _T("OLS-HID");
      else temp.Format(_T("COM%d"),portnum);
      dc.TextOut(lpItem->rcItem.left+2, lpItem->rcItem.top+1, temp);

      dc.SetBkColor(infocolor);
      dc.TextOut(lpItem->rcItem.left+m_portlist_comwidth+PORTLIST_COLUMNS_MARGIN, lpItem->rcItem.top+1, desc);

      if (lpItem->itemState & ODS_FOCUS) {
        dc.DrawFocusRect(&(lpItem->rcItem));
      }

      dc.SelectObject(oldfont);
    }
    dc.Detach();
    return;
  }
  
  CDialog::OnDrawItem(nIDCtl, lpItem);
}



//
// Return measurements for owner drawn listbox entries...
//
void CWinLoaderDialog::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpItem) 
{
  if ((nIDCtl == IDC_ANALYZERPORT_LIST) && (lpItem != NULL)) {
    int i;
    CDC dc;
    CString temp;
    CSize sz;
    dc.CreateCompatibleDC(NULL);

    CFont font,font2;
    font.CreateStockObject(ANSI_VAR_FONT);
    LOGFONT lfont;
    font.GetLogFont(&lfont);
    lfont.lfWeight = FW_BOLD;
    font2.CreateFontIndirect(&lfont);  

    CFont *oldfont = dc.SelectObject(&font2);

    int max_port_width = 0;
    int max_desc_width = 0;
    int max_height = 0;
    for (i=0; i<m_portlist.GetNumPorts(); i++) {
      int portnum = m_portlist.GetPortNum(i);
      if (portnum == 0xFFFF)
        temp = _T("OLS-HID");
      else temp.Format(_T("COM%d"),portnum);
      sz = dc.GetTextExtent(temp);
      if (sz.cx > max_port_width) max_port_width = sz.cx;
      if (sz.cy > max_height) max_height = sz.cy;
      sz = dc.GetTextExtent(m_portlist.GetDescription(i));
      if (sz.cx > max_desc_width) max_desc_width = sz.cx;
      if (sz.cy > max_height) max_height = sz.cy;
    }

    dc.SelectObject(oldfont);

    m_portlist_comwidth = max_port_width;
    m_portlist_height = max_height+2;

    lpItem->itemWidth = max_port_width+PORTLIST_COLUMNS_MARGIN+max_desc_width;
    lpItem->itemHeight = m_portlist_height;
    return;
  }
  
  CDialog::OnMeasureItem(nIDCtl, lpItem);
}




//
// Refresh port list...
//
void CWinLoaderDialog::OnRefreshPortList() 
{
  m_portlist.EnumeratePorts();
  if (m_portlist.GetNumPorts()==0) {
    m_portlist.AddPort(1);
    m_portlist.AddPort(2);
    m_portlist.AddPort(3);
    m_portlist.AddPort(4);
  }

  if (CheckBootloaderMode()) {
    CSerialPortInfo *portinfo = m_portlist.AddPort(0xFFFF);
    if (portinfo) portinfo->SetLocationInfo("OLS in Bootloader Mode");
  }

  if (::IsWindow(m_hWnd)) {
    m_analyzerport_list.ResetContent();
    if (m_portlist.GetNumPorts()>0) {
      CString temp;
      int i,j;

      for (i=0; i<m_portlist.GetNumPorts(); i++) {
        int portnum = m_portlist.GetPortNum(i);
        CString desc = m_portlist.GetDescription(i);

        if (portnum == 0xFFFF)
          temp = _T("OLS-HID");
        else temp.Format(_T("COM%d %s"),portnum,desc);
        m_analyzerport_list.AddString(temp);

        // Detect first port with a known device name.  
        // Select it for the default value (if no default available)...
        if (m_target_portnum<0)
          for (j=0; !g_known_devices[j].IsEmpty(); j++) 
            if (desc.Find(g_known_devices[j])>0) {
              m_target_portnum = portnum;
              break;
            }
      }

      for (i=0; i<m_portlist.GetNumPorts(); i++)
        if (m_portlist.GetPortNum(i)==m_target_portnum) {
          m_analyzerport_list.SetCurSel(i);
          return;
        }

      m_analyzerport_list.SetCurSel(0);
    }
  }
}


//
// Check if OLS already in bootloader mode...
//
bool CWinLoaderDialog::CheckBootloaderMode()
{
  boot_rsp response;
	PicBootloader device;
  return (device.getversion(&response) == PICRESULT_NOERROR);
}


//
// User made selection in port list...
//
void CWinLoaderDialog::OnSelchangeAnalyzerportList() 
{
  m_target_portnum = -1;
  m_port.Empty();
  int portindex = m_analyzerport_list.GetCurSel();
  if (portindex>=0) {
    m_target_portnum = m_portlist.GetPortNum(portindex);
    m_port = GetPortDeviceName(m_target_portnum);
  }
  UpdateOKButton();
}


//
// Browse for filename...
//
void CWinLoaderDialog::OnBrowse() 
{
  CFileFilterString filter;
  filter.AddFilter("FPGA or PIC Images",IDR_HEXFILE,IDR_MCSFILE,IDR_BINFILE,IDR_BITFILE,-1);
  filter.AddFilter(-1);

  CString defext(GetFiletypeExtension(IDR_HEXFILE));

  CFileNameDialogEx dlg (
    TRUE, _T("Select FPGA Image or PIC Firmware File"), m_target_file, defext, m_target_file,
    OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
    OFNX_ENABLE_PLACEBAR,
    filter, this);
  dlg.m_ofn.nFilterIndex = 0;

  if (dlg.DoModal() == IDOK) {
    m_target_file = dlg.GetPathName();
    m_imagefile.SetWindowText(m_target_file);
  }
}


//
// Enable/disable OK button depending if minimal criteria met...
//
void CWinLoaderDialog::OnUpdateImagefile() 
{
  OnSelchangeAnalyzerportList();

  m_imagefile.GetWindowText(m_target_file);
  CString ext(GetExtensionPortion(m_target_file));

  m_target_format = -1;
  for (int i=0; g_fpgafiletypes[i]>=0; i++)
    if (ext.CompareNoCase(GetFiletypeExtension(g_fpgafiletypes[i]))==0) {
      m_target_format = g_fpgafiletypes[i];
      break;
    }

  UpdateOKButton();
}


//
// Advanced button handler...
//
void CWinLoaderDialog::OnAdvance() 
{
  CAdvancedDialog dlg(this);
  dlg.DoModal();
}



//
// Update if OK button enabled or not.  Need valid port & filename...
//
void CWinLoaderDialog::UpdateOKButton()
{
  if ((m_target_portnum<0) || (m_port.IsEmpty())) {
    m_ok_button.EnableWindow(FALSE);
    return;
  }
  m_ok_button.EnableWindow((m_target_format>=0) && !m_target_file.IsEmpty());
}



//
// OK button handler...
//
void CWinLoaderDialog::OnOK() 
{
  // Setups for worker...
  m_progress.SetRange(0,100);
  m_flashid_name = NULL;  
  m_cancel_worker = false;
  OnUpdateImagefile();

  // Sanity checks...
  CString caption;
  GetWindowText (caption);

  if ((m_target_portnum<0) || (m_port.IsEmpty())) {
    MessageBox ("Serial port not selected!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  if (m_target_format<0) {
    MessageBox ("Image file type unknown!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;    
  }

  if (m_target_file.IsEmpty()) {
    MessageBox ("Image file not specified!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;    
  }

  if (!FileExists(m_target_file)) {
    MessageBox ("Image file not found!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;    
  }


  // Examine file... See if its for PIC or fpga image...
  DWORD buffer_size = 1024*1024;
  BYTE *buffer = (BYTE*)malloc(buffer_size);
  unsigned int read_size=0;
  int result=1;
  if (buffer)
    switch (m_target_format) {
      case IDR_MCSFILE :
      case IDR_HEXFILE : 
        result = HEX_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
        break;

      case IDR_BINFILE : 
        result = BIN_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
        break;

      case IDR_BITFILE :
        result = BIT_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
        break;

      default :
        MessageBox ("Image file type unknown!",caption,MB_ICONEXCLAMATION|MB_OK);
        return;
    }

  free (buffer);
  buffer = NULL;

  if (!result && (read_size==0)) { // file found, but nothing read, bad file...
    MessageBox ("Bad Image file!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  if (result) {
    MessageBox ("Unable to load Image file!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  if (read_size < (OLS_FLASH_SIZE/2)) { // PIC firmware
    MessageBox ("Image file too small!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  TRACE ("Image File Read Size = %d bytes\n", read_size);

  m_upload_mode = UPLOAD_FPGA;
  if (read_size <= OLS_FLASH_SIZE) // PIC firmware
    m_upload_mode = UPLOAD_PIC;
  else if (read_size <= 150000) {
    MessageBox ("FPGA image too small!",caption,MB_ICONEXCLAMATION|MB_OK);
    return;
  }

  if ((m_upload_mode == UPLOAD_PIC) && !m_alternate_intro)
    if (MessageBox (
      _T("CAUTION: You requested a -PIC firmware- update.\n\n"
         "Updating the PIC firmware may leave your Logic Sniffer unusable,\n"
         "unless you've a PICKit or Bus Pirate for manual restore.\n\n"
         "Are you positive you wish to update the PIC firmware on your Logic Sniffer?\n\n"),
      _T("Confirm PIC Firmware Update"), 
      MB_ICONWARNING|MB_YESNOCANCEL|MB_DEFBUTTON2|MB_APPLMODAL) != IDYES)
    {
      return;
    }

  m_update_mode_force = false;
  if (m_upload_mode == UPLOAD_PIC)
    if (read_size == (OLS_FLASH_SIZE-8))
      m_update_mode_force = true;
    else if (read_size != OLS_FLASH_SIZE) {
      CString temp;
      temp.Format(
        _T("WARNING: The requested -PIC firmware- is a non-standard size.\n\n"
           "OLS_WINLOADER expects PIC firmware to be exactly %d bytes.\n"
           "Your new firmware is %d bytes.  This may indicate corrupted\n"
           "firmware image.\n\n"
           "Click Yes to ignor discrepancy or No to abort.\n\n"),
           OLS_FLASH_SIZE,read_size);

      if (MessageBox (temp, _T("Confirm PIC Firmware Update"), MB_ICONWARNING|MB_YESNOCANCEL|MB_DEFBUTTON2|MB_APPLMODAL) != IDYES)
        return;

      m_update_mode_force = true;
    }

  // Create backup file name.  Try to create a unique name.  
  // However, after 32 tries just use the default...
  int backupnum = 0;
  char *backupdefault = (m_upload_mode==UPLOAD_FPGA) ? FPGA_BACKUP_FILENAME : PIC_BACKUP_FILENAME;
  do {
    if ((backupnum==0) || (backupnum>=31))
      m_backup_file.Format("%s%s%s",GetPathnamePortion(m_target_file),backupdefault,GetFiletypeExtension(IDR_HEXFILE));
    else m_backup_file.Format("%s%s%d%s",GetPathnamePortion(m_target_file),backupdefault,backupnum,GetFiletypeExtension(IDR_HEXFILE));
    backupnum++;
  } while ((backupnum<32) && (FileExists(m_backup_file)));

  // Hide instructions...
  m_instructions.ModifyStyle(WS_VISIBLE,0); 

  // Show progress & status lines...
  m_status.SetWindowText("");
  m_status2.SetWindowText("");
  m_status.ModifyStyle(0,WS_VISIBLE);
  m_status2.ModifyStyle(0,WS_VISIBLE);
  m_progress.ModifyStyle(0,WS_VISIBLE); 

  // Disable port list, image file & OK button...
  m_analyzerport_list.EnableWindow(FALSE);
  m_refresh_button.EnableWindow(FALSE);
  m_imagefile.EnableWindow(FALSE);
  m_browse_button.EnableWindow(FALSE);
  m_ok_button.EnableWindow(FALSE);
  m_advanced_button.EnableWindow(FALSE);
  Invalidate();

  // Save select filename for next time...
  theApp.WriteProfileString(INI_SECTION,"TargetFile",m_target_file);
  theApp.WriteProfileInt(INI_SECTION,"TargetPort",m_target_portnum);

  // Launch worker thread...
  DWORD dwThreadId = 0;
  m_hThread = ::CreateThread(0,0,ThreadProc,LPVOID(this),0,&dwThreadId);
  if (m_hThread == 0)
    m_status.SetWindowText(_T("ERROR: Unable to start worker to perform upload!"));
}


void CWinLoaderDialog::OnCancel() 
{
  if (m_hThread) {
    m_cancel_worker = true;
    WaitForSingleObject(m_hThread,1000); // wait until worker thread dead
  }
  if (m_pToolTip) {
    delete m_pToolTip;
    m_pToolTip = NULL;
  }
  CDialog::OnCancel();
}


void CWinLoaderDialog::OnClose() 
{
  OnCancel();
}


//
// Worker callback for status updates...
//
LRESULT CWinLoaderDialog::OnUploadStatus(WPARAM wParam, LPARAM lParam)
{
  CString temp;
  uint8_t flashjedic[OLD_FLASHID_INFOSIZE];
  int buffer_size, percentage, page;
  int status = wParam & 0xFFFF;

  bool failed = false;
  switch (status) {
    case OLS_WORKERSTATUS_BADPORT : // Error opening serial port...
      m_status.SetWindowText(_T("ERROR: Unable to open serial port!"));
      failed = true;
      break;

    case OLS_WORKERSTATUS_BADSPEED :
      m_status.SetWindowText(_T("ERROR: Unable to configure serial port!"));
      failed = true;
      break;

    case OLS_WORKERSTATUS_QUERYID :
      temp.Format (_T("Attempting to query OLS...  %s"), (lParam) ? _T("Found!") : _T(""));
      m_status.SetWindowText(temp);
      break;

    case OLS_WORKERSTATUS_BADOLSID : // Error reading OLS ID...
      temp.Format (_T("ERROR: %s   Verify OLS in Update mode && retry."),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_JEDIC_READFAIL : // Error reading flash JEDIC ID...
      temp.Format(_T("ERROR: Unable to read JEDIC ID!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_JEDIC_UNKNOWN : // Unknown flash type...
      *((DWORD*)(&flashjedic)) = lParam;
      temp.Format(_T("ERROR: Unknown flash type %02x %02x %02x %02x"), flashjedic[0], flashjedic[1], flashjedic[2], flashjedic[3]);
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_JEDIC_VALID : 
      temp.Format(_T("Detected flash type: %s"), m_flashid_name);
      m_status.SetWindowText(temp);
      break;

    case OLS_WORKERSTATUS_OLSSTATUS_VALID :
      temp.Format(_T("OLS status 0x%02x"), lParam);
      m_status.SetWindowText(temp);
      break;

    case OLS_WORKERSTATUS_OLSSTATUS_READFAIL : // Error reading OLS status...
      temp.Format(_T("ERROR: Unable to read OLS status!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_SELFTEST :
      temp.Format (_T("Performing OLS self test...  %s"), (lParam) ? _T("PASSED!") : _T(""));
      m_status.SetWindowText(temp);
      break;
    
    case OLS_WORKERSTATUS_SELFTESTFAIL : // Error performing OLS self test...
      temp.Format(_T("ERROR: OLS self test failed!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_BADBUFFER : // Error allocating memory...
      buffer_size = lParam;
      temp.Format(_T("ERROR: Unable to allocate %d byte buffer"), buffer_size);
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_READROM :
      percentage = lParam;
      if (percentage==0) m_status.SetWindowText("Reading FPGA ROM (for backup)...");
      temp.Format("(%d%% complete)",percentage);
      m_status2.SetWindowText(temp);
      m_progress.SetPos(percentage);
      break;

    case OLS_WORKERSTATUS_READROMFAIL : // Error reading flash ROM...
      page = (wParam>>16) & 0xFFFF;
      temp.Format(_T("ERROR: Reading flash page %d failed!  %s"),page,OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_FPGA_WRITEFILE :
      m_status2.SetWindowText("");
      m_status.SetWindowText(_T("Saving FPGA image backup file..."));
      break;

    case OLS_WORKERSTATUS_FPGA_WRITEFILE_FAIL : // Error writing backup of FPGA flash ROM to disk...
      temp.Format(_T("ERROR: Writing FPGA backup failed!  %s"),FileResultErrorMsg(lParam,m_backup_file,NULL));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_FPGA_WRITEFILE_SUCCESS :
      m_status2.SetWindowText("");
      m_status.SetWindowText(_T("FPGA flash ROM backup saved!"));
      break;
    
    case OLS_WORKERSTATUS_READFILE :
      m_status2.SetWindowText("");
      m_status.SetWindowText(_T("Loading new FPGA image file..."));
      break;

    case OLS_WORKERSTATUS_READFILEFAIL : // Reading file failed...
      temp.Format(_T("ERROR: Reading file failed!  %s"),FileResultErrorMsg(lParam,m_target_file,NULL));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_COUNTDOWN :
      temp.Format(_T("OLS update starts in %d seconds..."),(lParam+9)/10);
      m_status.SetWindowText(temp);
      break;

    case OLS_WORKERSTATUS_FPGA_ERASEROM :
      m_status.SetWindowText(_T("Erasing FPGA ROM prior to writing..."));
      break;

    case OLS_WORKERSTATUS_FPGA_ERASEFAIL : // Flash erase failed...
      temp.Format(_T("ERROR: Flash erase failed!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_WRITEROM :
      percentage = lParam;
      if (percentage==0) m_status.SetWindowText("Writing FPGA ROM...");
      temp.Format("(%d%% complete)",percentage);
      m_status2.SetWindowText(temp);
      m_progress.SetPos(percentage);
      if (percentage>99) {
        m_status.SetWindowText("Done!");
        m_ok_button.ModifyStyle(WS_VISIBLE,0);
        m_cancel_button.ModifyStyle(WS_VISIBLE,0);
        m_advanced_button.ModifyStyle(WS_VISIBLE,0);
        m_close_button.ModifyStyle(0,WS_VISIBLE);
        Invalidate();
      }
      break;

    case OLS_WORKERSTATUS_WRITEROMFAIL : // Error writing flash ROM...
      page = (wParam>>16) & 0xFFFF;
      temp.Format(_T("ERROR: Writing flash page %d failed!  %s"),page,OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_RUNOK :
      m_status.SetWindowText(_T("RUN mode enabled.  Update complete!"));
      m_ok_button.ModifyStyle(WS_VISIBLE,0);
      m_cancel_button.ModifyStyle(WS_VISIBLE,0);
      m_advanced_button.ModifyStyle(WS_VISIBLE,0);
      m_close_button.ModifyStyle(0,WS_VISIBLE);
      Invalidate();
      break;

    case OLS_WORKERSTATUS_RUNFAIL :
      temp.Format(_T("ERROR: Enabling RUN mode failed!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_CANCEL : // User cancelled the operation.
      m_status.SetWindowText(_T("Cancelled by user."));
      failed = true;
      break;

    case OLS_WORKERSTATUS_BOOTLOADER_SWITCH :
      m_status.SetWindowText(_T("Switching OLS into bootloader mode..."));
      break;

    case OLS_WORKERSTATUS_BOOTLOADER_FAILED : // Unable to switch into bootloader mode...
      m_status.SetWindowText(_T("ERROR: Unable to switch into bootloader mode."));
      failed = true;
      break;

    case OLS_WORKERSTATUS_BOOTLOADER_SUCCESS : // Unable to switch into bootloader mode...
      temp.Format(_T("Bootloader switch Successful!"));
      m_status.SetWindowText(temp);
      break;

    case OLS_WORKERSTATUS_READPIC :
      m_status.SetWindowText(_T("Backing up PIC firmware..."));
      break;

    case OLS_WORKERSTATUS_READPIC_FAIL :
      temp.Format(_T("ERROR: Backing up PIC firmware failed!  Error=%d"),lParam);
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_PIC_WRITEFILE_FAIL : // Error writing backup of PIC flash to disk...
      temp.Format(_T("ERROR: Writing PIC backup failed!  %s"),FileResultErrorMsg(lParam,m_backup_file,NULL));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_PIC_WRITEFILE_SUCCESS :
      m_status2.SetWindowText("");
      m_status.SetWindowText(_T("PIC firmware backup saved!"));
      break;

    case OLS_WORKERSTATUS_PIC_ERASEROM :
      m_status.SetWindowText(_T("Erasing PIC firmware flash prior to writing..."));
      break;

    case OLS_WORKERSTATUS_PIC_ERASEFAIL : // Flash erase failed...
      temp.Format(_T("ERROR: Flash erase failed!  %s"),OLSResultErrorMsg(lParam));
      m_status.SetWindowText(temp);
      failed = true;
      break;

    case OLS_WORKERSTATUS_WRITEPIC :
      m_status.SetWindowText(_T("Writing PIC firmware..."));
      break;

    case OLS_WORKERSTATUS_WRITEPIC_FAIL :
      m_status.SetWindowText(_T("ERROR: Writing PIC firmware failed!"));
      failed = true;
      break;

    case OLS_WORKERSTATUS_VERIFYPIC :
      m_status.SetWindowText(_T("Verifying PIC firmware..."));
      break;

    case OLS_WORKERSTATUS_VERIFYPIC_FAIL :
      m_status.SetWindowText(_T("ERROR: Verifying PIC firmware failed!"));
      failed = true;
      break;

    case PICSTATUS_INIT_XFER :
    case PICSTATUS_START_XFER :
      break;

    case PICSTATUS_UPDATE_XFER :
      percentage = lParam;
      temp.Format("(%d%% complete)",percentage);
      m_status2.SetWindowText(temp);
      m_progress.SetPos(percentage);
      break;

    case PICSTATUS_DONE_XFER :
      m_status2.SetWindowText("");
      m_progress.SetPos(100);
      break;

    case PICRESULT_DEVICE_NOT_FOUND :
      m_status.SetWindowText(_T("ERROR: PIC microcontroller not found."));
      failed = true;
      break;

    default :
      if ((status>=PICRESULT_FIRST_ERROR) && (status<PICRESULT_LAST_ERROR)) {
        temp.Format(_T("ERROR:  PIC bootloader fault.  Error=%d"),status);
        m_status.SetWindowText(temp);
        failed = true;
      }
      break;
  }

  if (failed) {
    m_analyzerport_list.EnableWindow(TRUE);
    m_refresh_button.EnableWindow(TRUE);
    m_imagefile.EnableWindow(TRUE);
    m_browse_button.EnableWindow(TRUE);

    m_ok_button.EnableWindow(TRUE);
    m_ok_button.SetWindowText(_T("Retry"));
    m_advanced_button.EnableWindow(TRUE);

    m_ok_button.ModifyStyle(0,WS_VISIBLE);
    m_cancel_button.ModifyStyle(0,WS_VISIBLE);
    m_advanced_button.ModifyStyle(0,WS_VISIBLE);
    m_close_button.ModifyStyle(WS_VISIBLE,0);
    Invalidate();
  }

  return 0;
}



///////////////////////////////////////////////////////
// 
// Worker thread for the heavy lifting...
//
DWORD WINAPI CWinLoaderDialog::ThreadProc (LPVOID lpArg)
{
  // Route the method to the actual object
  CWinLoaderDialog* pThis = reinterpret_cast<CWinLoaderDialog*>(lpArg);
  return pThis->ThreadProc();
}



bool CWinLoaderDialog::CheckUserAbort(int msecs)
{
  msecs /= 100;
  if (msecs<1) msecs=1;
  for (int i=msecs; i>0; i--) {
    Sleep(100);
    if (m_cancel_worker) {
      TRACE ("Worker %08x cancelled.\n", m_hThread);
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_CANCEL, 0);
      return (true);
    }
  }
  return (false);
}



DWORD CWinLoaderDialog::ThreadProc (void)
{
  int result, i, pages, wrsize;
  int dev_fd = -1;
  int percent = 0;
  int last_percent = -1;
  int PUMP_FlashID = -1;
  boot_rsp response;

  DWORD buffer_size;
  BYTE *buffer = NULL;
	PicBootloader device;
  device.m_progress_hWnd = m_hWnd;

  // First a quick check (if uploading PIC) to see if its already in bootloader mode...
  if (m_upload_mode==UPLOAD_PIC)
    if (CheckBootloaderMode())
      goto found_bootloader;

  // Open serial port...
  dev_fd = serial_open(m_port);
  if (dev_fd < 0) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BADPORT, 0);
    goto cleanup;
  }

  // Setup port speed...
  if (serial_setup(dev_fd, m_port_speed) < 0 ) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BADSPEED, 0);
    goto cleanup;
  }

  // Get OLS ID...
  PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_QUERYID, 0);
  uint8_t olsid[OLS_ID_INFOSIZE];
  result = PUMP_GetID(dev_fd, olsid, sizeof(olsid));
  if (result) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BADOLSID, result);
    goto cleanup;
  }
  TRACE("Found OLS HW: %d, FW: %d.%d, Boot: %d\n", olsid[1], olsid[3], olsid[4], olsid[6]);
  if (CheckUserAbort(100)) goto cleanup;

  PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_QUERYID, 1 | (olsid[1]<<16));
  if (CheckUserAbort(1000)) goto cleanup;

  // Get flash type...
  if (m_upload_mode==UPLOAD_FPGA) {
    m_flashid_name = _T("Unknown Flash Type");
    uint8_t flashjedic[OLD_FLASHID_INFOSIZE];

    if (m_ignore_flashrom_jedec) {
      PUMP_FlashID=0; // Assume ATMEL AT45DB041D...
      memcpy (flashjedic, PUMP_Flash[0].jedec_id, 4);
      m_flashid_name = PUMP_Flash[PUMP_FlashID].name;
    }
    else {
      result = PUMP_GetFlashID(dev_fd, &PUMP_FlashID, flashjedic, sizeof(flashjedic), m_ignore_flashrom_jedec);
      if (result) {
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_JEDIC_READFAIL, result);
        goto cleanup;
      }
      if (PUMP_FlashID<0) {
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_JEDIC_UNKNOWN, *((DWORD*)(&flashjedic)));
        goto cleanup;
      }
      m_flashid_name = PUMP_Flash[PUMP_FlashID].name;
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_JEDIC_VALID, PUMP_FlashID);
      if (CheckUserAbort(1000)) goto cleanup;
    }
  }

  // Query OLS status...
  if (m_query_ols_status) {
    BYTE status;
    result = PUMP_GetStatus(dev_fd, &status);
    if (result) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_OLSSTATUS_READFAIL, result);
      goto cleanup;
    }
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_OLSSTATUS_VALID, status);
    if (CheckUserAbort(1000)) goto cleanup;
  }

  // Perform self test...
  if (m_selftest) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_SELFTEST, 0);
    result = PUMP_selftest(dev_fd);
    if (result) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_SELFTESTFAIL, result);
      goto cleanup;
    }
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_SELFTEST, 1);
    if (CheckUserAbort(1000)) goto cleanup;
  }

  // Switch into bootloader mode, if updating the PIC...
  if (m_upload_mode==UPLOAD_PIC) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BOOTLOADER_SWITCH, 0);
    if (CheckUserAbort(1000)) goto cleanup;
    PUMP_EnterBootloader(dev_fd);

    int count=0;
    result=-1;
    while ((count<5) && (result != PICRESULT_NOERROR)) {
      Sleep(1000);
	    result = device.getversion(&response);
    }
    if (result != PICRESULT_NOERROR)  {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BOOTLOADER_FAILED, result);
      device.reset();
      goto cleanup;
    }
  }

found_bootloader:

  // Allocate buffer needed...
  if (m_upload_mode==UPLOAD_PIC)
    buffer_size = 1024*1024;
  else buffer_size = PUMP_Flash[PUMP_FlashID].pages * PUMP_Flash[PUMP_FlashID].page_size;

  buffer = (BYTE*)malloc(buffer_size);
  if (buffer == NULL) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BADBUFFER, buffer_size);
    goto cleanup;
  }
  memset(buffer, 0, buffer_size);

  // Indicate found the bootloader...
  if (m_upload_mode==UPLOAD_PIC) {
    m_bootloader_version = response.get_fw_ver;
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_BOOTLOADER_SUCCESS, buffer_size);
    if (CheckUserAbort(1000)) {
      device.reset();
      goto cleanup;
    }
  }

  // Backup ROM...
  if (m_backup_fpga)
    switch  (m_upload_mode) {
      case UPLOAD_PIC : { // backup PIC firmware...
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READPIC, 0);
        BUFPTR picbuffer=NULL;
        size_t picbufsize=0;
        result = device.read(&picbuffer,&picbufsize);
        if (result || (picbuffer==NULL)) {
          PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READPIC_FAIL, result);
          device.reset();
          goto cleanup;
        }

        result = HEX_WriteFile(m_backup_file, picbuffer, picbufsize);
        free (picbuffer);
        picbuffer = NULL;

        if (result) {
          PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_PIC_WRITEFILE_FAIL, result);
          device.reset();
          goto cleanup;
        }

        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_PIC_WRITEFILE_SUCCESS, result);
        if (CheckUserAbort(1000)) {
          device.reset();
          goto cleanup;
        }
        break;
      }

      case UPLOAD_FPGA : { // backup FPGA serial flash ROM...
        pages = PUMP_Flash[PUMP_FlashID].pages;

        last_percent = -1;
        for (i = 0; i < pages; i ++) {
          if (m_cancel_worker) {
            TRACE ("Worker %08x cancelled.\n", m_hThread);
            PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_CANCEL, 0);
            goto cleanup;
          }
          // Update dialog...
          percent = (100*(i+1))/pages;
          if (percent>99) percent = (i<(pages-1)) ? 99 : 100;
          if (percent != last_percent) {
            last_percent = percent;
            PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READROM, percent);
          }

          int result = PUMP_FlashRead(dev_fd, PUMP_FlashID, i, buffer + (PUMP_Flash[PUMP_FlashID].page_size * i));
          if (result) {
            PostMessage(OLS_UPLOAD_STATUS, (((DWORD)i)<<16)|(DWORD)OLS_WORKERSTATUS_READROMFAIL, result);
            goto cleanup;
          }
        }

        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_FPGA_WRITEFILE, 0);

        wrsize = pages * PUMP_Flash[PUMP_FlashID].page_size;
        result = HEX_WriteFile(m_backup_file, buffer, wrsize); 
        if (result) {
          PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_FPGA_WRITEFILE_FAIL, result);
          goto cleanup;
        }

        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_FPGA_WRITEFILE_SUCCESS, result);
        if (CheckUserAbort(1000)) goto cleanup;
        break;
      }
    }

  // Read new FPGA image file from disk...
  PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READFILE, 0);
  unsigned int read_size;

  switch (m_target_format) {
    case IDR_MCSFILE :
    case IDR_HEXFILE : 
      result = HEX_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
      break;

    case IDR_BINFILE : 
      result = BIN_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
      break;

    case IDR_BITFILE :
      result = BIT_ReadFile(m_target_file, buffer, buffer_size, &read_size); 
      break;

    default :
      result = FILERESULT_ERROR_BADFILEFORMAT;
      break;
  }

  if (!result && (read_size==0)) // file found, but nothing read, bad file...
    result = FILERESULT_ERROR_BADFILEFORMAT;

  if (result) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READFILEFAIL, result);
    goto cleanup;
  }
  TRACE ("Binary size = %ld\n", (long int)read_size);

  // Sanity check on the file size...
  if (m_upload_mode == UPLOAD_PIC) 
	  if (m_update_mode_force) 
      read_size = OLS_FLASH_SIZE;
    else if (read_size != OLS_FLASH_SIZE) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_READFILEFAIL, result);
      goto cleanup;
    }

  // Give user a chance to cancel...
  for (i=50; i>=0; i--) {
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_COUNTDOWN, i);
    if (CheckUserAbort(100)) {
      if (m_upload_mode == UPLOAD_PIC) device.reset();
      goto cleanup;
    }
  }
  
  //
  // PIC firmware update...
  //
  if (m_upload_mode == UPLOAD_PIC) {
    // Erase PIC flash...
    if (!m_noerase_flashrom) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_PIC_ERASEROM, 0);
      result = device.erase();
      if (result) {
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_PIC_ERASEFAIL, 0);
        device.reset();
        goto cleanup;
      }
    }

    // Write new PIC firmware...
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_WRITEPIC, 0);
	  result = device.write(buffer, read_size);
    if (result) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_WRITEPIC_FAIL, result);
      device.reset();
      goto cleanup;
    }

    // Verify new PIC firmware...
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_VERIFYPIC, 0);
    result = device.verify(buffer,read_size);
    if (result) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_VERIFYPIC_FAIL, result);
      device.reset();
      goto cleanup;
    }

    // Reset after upload...
    result = device.reset();
    if (result) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_RUNFAIL, result);
      goto cleanup;
    }
    PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_RUNOK, 0);
  }


  //
  // FPGA update...
  //
  if (m_upload_mode == UPLOAD_FPGA) {
    // Erase ROM...
    if (!m_noerase_flashrom) {
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_FPGA_ERASEROM, 0);
      result = PUMP_FlashErase(dev_fd, PUMP_FlashID);
      if (result) {
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_FPGA_ERASEFAIL, result);
        goto cleanup;
      }
    }

    // Compute # of pages to write... 
    pages = (read_size+PUMP_Flash[PUMP_FlashID].page_size-1)/PUMP_Flash[PUMP_FlashID].page_size;
    if (pages > PUMP_Flash[PUMP_FlashID].pages)
      pages = PUMP_Flash[PUMP_FlashID].pages;

    // Write the ROM...
    last_percent = -1;
    for (i = 0; i < pages; i++) {
      if (m_cancel_worker) {
        TRACE ("Worker %08x cancelled.\n", m_hThread);
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_CANCEL, 0);
        goto cleanup;
      }
      // Update dialog...
      percent = (100*(i+1))/pages;
      if (percent>99) percent = (i<(pages-1)) ? 99 : 100;
      if (percent != last_percent) {
        last_percent = percent;
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_WRITEROM, percent);
      }
      // Write page...
      int result = PUMP_FlashWrite(dev_fd, PUMP_FlashID, i, buffer + (PUMP_Flash[PUMP_FlashID].page_size * i));
      if (result) {
        PostMessage(OLS_UPLOAD_STATUS, (((DWORD)i)<<16)|(DWORD)OLS_WORKERSTATUS_WRITEROMFAIL, result);
        goto cleanup;
      }
    }

    // Run after upload...
    if (m_run_after_upload) {
      result = PUMP_EnterRunMode(dev_fd);
      if (result) {
        PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_RUNFAIL, result);
        goto cleanup;
      }
      PostMessage(OLS_UPLOAD_STATUS, OLS_WORKERSTATUS_RUNOK, 0);
    }
  }

cleanup:
  if (buffer) {free(buffer); buffer=NULL;}
  serial_close(dev_fd);
  m_hThread = NULL;
  return 0;
}




