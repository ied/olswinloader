#ifndef _ENUMUSB_H_
#define _ENUMUSB_H_

#include <basetyps.h>
#include <winioctl.h>
#include <setupapi.h>
//#include <string.h>
//#include <stdio.h>

#include <commctrl.h>

#pragma warning (disable:4200)
#include <usbioctl.h>
#include <usbiodef.h>
#include "usb100.h"

#undef _MP
#include "usb200.h"


//
// CEnumerateUSB class...
//
class CEnumerateUSB
{
// Construction
public:
	CEnumerateUSB();
	~CEnumerateUSB();

  // Operations...
  CString GetStrName_iProduct(int vid, int pid);
  void EnumControllers();
  bool EnumRootHUB(HANDLE hHostCtrl, char *devpath);
  bool EnumerateHub(CString namestr);
  CString GetHubName(HANDLE hHub, int portnum);
  USB_DESCRIPTOR_REQUEST *GetConfigDescriptor(HANDLE hHub, int portnum);
  CString GetDescriptorString(HANDLE hHub, int portnum, int strindex);

  // Attributes...
  bool m_target_hit;
  int m_target_vid;
  int m_target_pid;
  CString m_iProduct_strname;
};


#endif // _ENUMUSB_H_
