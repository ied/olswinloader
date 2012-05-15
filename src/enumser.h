/*
Module : enumser.h
Purpose: Defines the interface for a class to enumerate the serial ports installed on a PC
         using a number of different approaches
Created: PJN / 03-11-1998

Copyright (c) 1998 - 2010 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/


///////////////////////// Macros / Structs etc ////////////////////////////////

#pragma once

#ifndef __ENUMSER_H__
#define __ENUMSER_H__

///////////////////////// Includes ////////////////////////////////////////////                      

#if defined _AFX
  #ifndef __AFXTEMPL_H__
    #include <afxtempl.h> 
    #pragma message("To avoid this message, please put afxtempl.h in your pre compiled header (normally stdafx.h)")
  #endif
#else
  #ifndef __ATLSTR_H__
    #include <atlstr.h>
    #pragma message("To avoid this message, please put atlstr.h in your pre compiled header (normally stdafx.h). Note non MFC mode is not supported on VC 6")
  #endif  
#endif


///////////////////////// Classes /////////////////////////////////////////////


//
// Self cleaning heap allocations...
//
template<class INFO> class CHeapPtr
{
public:
  CHeapPtr(int memsize=0){m_pData=NULL; if (memsize>0) Allocate(memsize);}
  ~CHeapPtr(){Cleanup();}
  void Cleanup() {if (m_pData) free(m_pData); m_pData=NULL; m_memsize=m_sizeof=0;}

  INFO *Allocate(int memsize) {
    Cleanup();
    m_memsize = memsize;
    m_sizeof = sizeof(INFO)*m_memsize;
    m_pData = (INFO*)malloc(m_sizeof);
    return m_pData;
  }

  inline void FillZero() {memset(m_pData,0,m_sizeof);}
  inline int NumIndexes() {return m_memsize;}
  inline int SizeOf() {return m_sizeof;}

  // Allow usage as a pointer...
  inline CHeapPtr<INFO>* operator->() {return this;}

  // Allow usage as a pointer...
  inline operator INFO*() {return m_pData;}

private:
  INFO *m_pData;
  INT m_memsize, m_sizeof;
};



//
// Library manager...
//
class CLoadLib
{
  HINSTANCE m_lib_handle;
public:
  CLoadLib (CString libname) {m_lib_handle = LoadLibrary(libname);}
  ~CLoadLib() {if (m_lib_handle != NULL) FreeLibrary(m_lib_handle);}
  operator HINSTANCE() {return m_lib_handle;}
};



//
// Class to hold info for a given port...
//
class CSerialPortInfo
{
public:
  CSerialPortInfo() {
    m_portnum=-1; 
    m_hardwareid=NULL;
    m_devicedesc=NULL; 
    m_friendlyname=NULL; 
    m_manufacturer=NULL; 
    m_locationinfo=NULL; 
    m_physlocation=NULL;
    Cleanup();
  }
  ~CSerialPortInfo() 
    {Cleanup();}

  void MoveData(CSerialPortInfo &src) {
    m_portnum = src.m_portnum; src.m_portnum = -1;
    m_hardwareid = src.m_hardwareid; src.m_hardwareid = NULL;
    m_devicedesc = src.m_devicedesc; src.m_devicedesc = NULL;
    m_friendlyname = src.m_friendlyname; src.m_friendlyname = NULL;
    m_manufacturer = src.m_manufacturer; src.m_manufacturer = NULL;
    m_locationinfo = src.m_locationinfo; src.m_locationinfo = NULL;
    m_physlocation = src.m_physlocation; src.m_physlocation = NULL;
    SetCommProp(src.m_commprop);
  }

  void Cleanup() {
    m_portnum=-1; 
    if (m_hardwareid) {free (m_hardwareid); m_hardwareid = NULL;}
    if (m_devicedesc) {free (m_devicedesc); m_devicedesc = NULL;}
    if (m_friendlyname) {free (m_friendlyname); m_friendlyname = NULL;}
    if (m_manufacturer) {free (m_manufacturer); m_manufacturer = NULL;}
    if (m_locationinfo) {free (m_locationinfo); m_locationinfo = NULL;}
    if (m_physlocation) {free (m_physlocation); m_physlocation = NULL;}
  }

  inline void SetPortNum (int portnum) {m_portnum=portnum;}
  inline void SetHardwareID (TCHAR *desc) {
    if (m_hardwareid) {free (m_hardwareid); m_hardwareid = NULL;}
    if (desc) m_hardwareid = _tcsdup(desc);
  }
  inline void SetDeviceDesc (TCHAR *desc) {
    if (m_devicedesc) {free (m_devicedesc); m_devicedesc = NULL;}
    if (desc) m_devicedesc = _tcsdup(desc);
  };
  inline void SetFriendlyName (TCHAR *fname) {
    if (m_friendlyname) {free (m_friendlyname); m_friendlyname=NULL;}
    if (fname) m_friendlyname = _tcsdup(fname);
  }
  inline void SetManufacturer (TCHAR *manufacturer) {
    if (m_manufacturer) {free (m_manufacturer); m_manufacturer=NULL;}
    if (manufacturer) m_manufacturer = _tcsdup(manufacturer);
  }
  inline void SetLocationInfo (TCHAR *locinfo) {
    if (m_locationinfo) {free (m_locationinfo); m_locationinfo=NULL;}
    if (locinfo) m_locationinfo = _tcsdup(locinfo);
  }
  inline void SetPhysLocation (TCHAR *physloc) {
    if (m_physlocation) {free (m_physlocation); m_physlocation=NULL;}
    if (physloc) m_physlocation = _tcsdup(physloc);
  }

  inline void SetCommProp (COMMPROP &commprop) 
    {memcpy (&m_commprop, &commprop, sizeof(commprop));}

  CString GetPortDeviceName (int portnum) {
    CString sPort;
    sPort.Format(_T("\\\\.\\COM%d"),portnum);
    return sPort;
  }
  CString GetPortDeviceName() {return GetPortDeviceName(GetPortNum());}

  inline int GetPortNum() {return m_portnum;}
  inline CString GetHardwareID() {if (m_hardwareid==NULL) return ""; else return m_hardwareid;}
  inline CString GetDeviceDesc() {if (m_devicedesc==NULL) return ""; else return m_devicedesc;}
  inline CString GetFriendlyName() {if (m_friendlyname==NULL) return ""; else return m_friendlyname;}
  inline CString GetManufacturer() {if (m_manufacturer==NULL) return ""; else return m_manufacturer;}
  inline CString GetLocationInfo() {if (m_locationinfo==NULL) return ""; else return m_locationinfo;}
  inline CString GetPhysLocation() {if (m_physlocation==NULL) return ""; else return m_physlocation;}

  inline TCHAR *GetHardwareIDPtr() {return m_hardwareid;}
  inline TCHAR *GetDeviceDescPtr() {return m_devicedesc;}
  inline TCHAR *GetFriendlyNamePtr() {return m_friendlyname;}
  inline TCHAR *GetManufacturerPtr() {return m_manufacturer;}
  inline TCHAR *GetLocationInfoPtr() {return m_locationinfo;}
  inline TCHAR *GetPhysLocationPtr() {return m_physlocation;}
  inline COMMPROP *GetCommPropPtr () {return &m_commprop;}

//private:
  int m_portnum;
  TCHAR *m_hardwareid;
  TCHAR *m_devicedesc;
  TCHAR *m_friendlyname;
  TCHAR *m_manufacturer;
  TCHAR *m_locationinfo;
  TCHAR *m_physlocation;
  COMMPROP m_commprop;
};



//
// The enumerator class...
//
class CEnumerateSerial
{
public:
  // Constructor/Destructor...
  CEnumerateSerial() {m_maxports=256; m_numports=0; m_portinfo = new CSerialPortInfo[m_maxports];}
  ~CEnumerateSerial() {
    ResetPortList(); 
    delete [] m_portinfo;
  }

  // Operations...
  BOOL EnumeratePorts();

  inline int GetNumPorts() 
    {return m_numports;}

  inline CSerialPortInfo *GetPortInfo(int index) {
    if ((index<0) || (index>=m_numports) || (m_portinfo==NULL)) return NULL; else 
    return &m_portinfo[index];
  }

  inline int GetPortNum (int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return -1; 
    return portinfo->GetPortNum();
  }

  CString GetPortDeviceName (int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetPortDeviceName();
  }

  inline CString GetHardwareID(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetHardwareID();
  }

  inline CString GetDeviceDesc(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetDeviceDesc();
  }

  inline CString GetFriendlyName(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetFriendlyName();
  }

  inline CString GetManufacturer(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetManufacturer();
  }

  inline CString GetLocationInfo(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetLocationInfo();
  }

  inline CString GetPhysLocation(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return ""; 
    return portinfo->GetPhysLocation();
  }

  inline TCHAR *GetHardwareIDPtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetHardwareIDPtr();
  }

  inline TCHAR *GetDeviceDescPtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetDeviceDescPtr();
  }

  inline TCHAR *GetFriendlyNamePtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetFriendlyNamePtr();
   }

  inline TCHAR *GetManufacturerPtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetManufacturerPtr();
  }

  inline TCHAR *GetLocationInfoPtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetLocationInfoPtr();
  }

  inline TCHAR *GetPhysLocationPtr(int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetPhysLocationPtr();
  }

  inline COMMPROP *GetCommPropPtr (int index) {
    CSerialPortInfo *portinfo = GetPortInfo(index); if (portinfo==NULL) return NULL; 
    return portinfo->GetCommPropPtr();
  }

  CString GetDescription(int index)
  {
    CString desc;

    /*
    TRACE ("LocationInfo: %s\n", GetLocationInfo(index));
    TRACE ("HardwareID:   %s\n", GetHardwareID(index));
    TRACE ("DeviceDesc:   %s\n", GetDeviceDesc(index));
    TRACE ("FriendlyName: %s\n", GetFriendlyName(index));
    TRACE ("Manufacturer: %s\n", GetManufacturer(index));
    TRACE ("PhysLocation: %s\n", GetPhysLocation(index));
    */

    desc = GetDeviceDesc(index);
    if (desc.IsEmpty()) desc = GetFriendlyName(index);
    if (desc.IsEmpty()) desc = GetManufacturer(index);
    if (desc.IsEmpty()) desc = _T("Communications Port");

    if (desc.Left(1) != "(") {
      desc.Insert(0,"(");
      desc += ")";
    }

    return desc;
  }


  //
  // Add a new port (sorted numerically)...
  //
  CSerialPortInfo *AddPort (int portnum) {
    if (m_portinfo==NULL) return NULL;
    if (m_numports>=m_maxports) return NULL;
    CSerialPortInfo *newport = &m_portinfo[m_numports++];

    // Insertion sort if needed...
    for (int i=0; i<(m_numports-1); i++) 
      if (m_portinfo[i].GetPortNum()>portnum) {
        for (int j=m_numports-1; j>i; j--) m_portinfo[j].MoveData(m_portinfo[j-1]);
        newport = &m_portinfo[i];
        break;
      }
    
    newport->SetPortNum(portnum);
    return newport;
  }

  //
  // Cleanup port list...
  //  
  void ResetPortList() {
    for (int i=0; i<m_numports; i++) m_portinfo[i].Cleanup(); m_numports=0;
  }

protected:
  BOOL IsNumber(TCHAR *strdata) {
    int nLen = _tcslen(strdata);
    for (int i=0; i<nLen; i++) if (_istdigit(strdata[i])==0) return FALSE;
    return (nLen>0);
  }

  // Attributes...
  CSerialPortInfo *m_portinfo;
  int m_numports;
  int m_maxports;
};

#endif //__ENUMSER_H__
