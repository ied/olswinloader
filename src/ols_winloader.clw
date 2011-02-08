; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CWinLoaderDialog
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ols_winloader.h"
LastPage=0

ClassCount=4
Class1=CAboutDlg
Class2=CWinLoaderApp
Class3=CWinLoaderDialog

ResourceCount=3
Resource1=IDD_OLS_WINLOADER_DIALOG
Resource2=IDD_ABOUTBOX
Class4=CAdvancedDialog
Resource3=IDD_ADVANCED_DIALOG

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=AboutDlg.h
ImplementationFile=AboutDlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDC_STATIC_ICON

[CLS:CWinLoaderApp]
Type=0
BaseClass=CWinApp
HeaderFile=ols_winloader.h
ImplementationFile=ols_winloader.cpp

[CLS:CWinLoaderDialog]
Type=0
BaseClass=CDialog
HeaderFile=ols_winloaderDlg.h
ImplementationFile=ols_winloaderDlg.cpp
LastObject=CWinLoaderDialog
Filter=D
VirtualFilter=dWC

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=7
Control1=IDC_STATIC_ICON,static,1342177539
Control2=IDC_STATIC,static,1342308480
Control3=IDOK,button,1342373889
Control4=IDC_LINKS,static,1342308480
Control5=IDC_STATIC,static,1342308480
Control6=IDC_STATIC,static,1342308480
Control7=IDC_STATIC,static,1342308480

[DLG:IDD_OLS_WINLOADER_DIALOG]
Type=1
Class=CWinLoaderDialog
ControlCount=17
Control1=IDC_ANALYZERPORT_LIST,listbox,1352728915
Control2=IDC_IMAGEFILE,edit,1350631552
Control3=IDC_REFRESH,button,1342242827
Control4=IDC_BROWSE,button,1342242816
Control5=IDOK,button,1342242827
Control6=IDCANCEL,button,1342242827
Control7=IDCLOSE,button,1073807360
Control8=IDC_STATIC,static,1342177298
Control9=IDC_STATIC,static,1342308865
Control10=IDC_STATIC,static,1342308866
Control11=IDC_STATIC,static,1342308866
Control12=IDC_STATIC,static,1342177298
Control13=IDC_STATIC_HELP,static,1342308352
Control14=IDC_PROGRESS,msctls_progress32,1082130432
Control15=IDC_STATUS,static,1073872896
Control16=IDC_STATUS2,static,1073873409
Control17=IDADVANCE,button,1342242827

[DLG:IDD_ADVANCED_DIALOG]
Type=1
Class=CAdvancedDialog
ControlCount=14
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_BACKUP,button,1342242819
Control4=IDC_NOERASE,button,1342242819
Control5=IDC_OLSSTATUS,button,1342242819
Control6=IDC_RUNMODE,button,1342242819
Control7=IDC_SELFTEST,button,1342242819
Control8=IDC_IGNOREJEDIC,button,1342242819
Control9=IDC_PORTSPEED,combobox,1342242819
Control10=IDC_STATIC,static,1342308866
Control11=IDC_STATIC,static,1342308865
Control12=IDC_STATIC,static,1342177298
Control13=IDC_STATIC,static,1342177298
Control14=IDDEFAULT,button,1342242816

[CLS:CAdvancedDialog]
Type=0
HeaderFile=AdvancedDialog.h
ImplementationFile=AdvancedDialog.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CAdvancedDialog

