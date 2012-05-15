//
// enumusb.cpp : implementation file
//
#include "stdafx.h"
#include "enumusb.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFGUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)\
  EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

/* 3ABF6F2D-71C4-462a-8A92-1E6861E6AF27 */
DEFGUID(GUID_DEVINTERFACE_USB_HOST_CONTROLLER, 0x3abf6f2d, 0x71c4, 0x462a, 0x8a, 0x92, 0x1e, \
             0x68, 0x61, 0xe6, 0xaf, 0x27);


/////////////////////////////////////////////////////////////////////////////
// CEnumerateUSB

CEnumerateUSB::CEnumerateUSB()
{
  m_target_hit = false;
  m_target_vid=0;
  m_target_pid=0;
  m_iProduct_strname.Empty();
}

CEnumerateUSB::~CEnumerateUSB()
{
}


CString CEnumerateUSB::GetStrName_iProduct(int vid, int pid)
{
  m_target_hit = false;
  m_target_vid = vid;
  m_target_pid = pid;
  EnumControllers();
  return m_iProduct_strname;
}



//
// Enumerate all USB controllers...
//
void CEnumerateUSB::EnumControllers()
{
  HDEVINFO hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

  SP_DEVICE_INTERFACE_DATA devinfo;
  devinfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  int index=0;
  while (!m_target_hit && SetupDiEnumDeviceInterfaces(hDevInfo, 0, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, index, &devinfo)) {
    // Get size of buffer needed for host controller details...
    ULONG datalen;
    SetupDiGetDeviceInterfaceDetail(hDevInfo, &devinfo, NULL, 0, &datalen, NULL);
    {
      // Allocate buffer & get host controller details...
      PSP_DEVICE_INTERFACE_DETAIL_DATA dev_details = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(datalen);
      if (dev_details) {
        dev_details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &devinfo, dev_details, datalen, &datalen, NULL)) {
          // Open host controller...
          HANDLE hHostCtrl = CreateFile(dev_details->DevicePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
          if (hHostCtrl != INVALID_HANDLE_VALUE) {
            EnumRootHUB(hHostCtrl, dev_details->DevicePath);
            CloseHandle(hHostCtrl);
          }
        }
        free (dev_details);
      }
    }
    index++;
  }

  SetupDiDestroyDeviceInfoList(hDevInfo);
}



//
// Enumerate a root hub...
//
bool CEnumerateUSB::EnumRootHUB(HANDLE hHostCtrl, char *devpath)
{
  USB_ROOT_HUB_NAME nameinfo;
  DWORD bytelen;
  
  // Get length of root hub name...
  bytelen = sizeof(USB_ROOT_HUB_NAME);
  if (!DeviceIoControl(hHostCtrl, IOCTL_USB_GET_ROOT_HUB_NAME, 0, 0, &nameinfo, sizeof(USB_ROOT_HUB_NAME), &bytelen, NULL))
    return false;

  bytelen = nameinfo.ActualLength;
  USB_ROOT_HUB_NAME *name_wstr = (USB_ROOT_HUB_NAME *)malloc(bytelen);
  if (name_wstr==NULL) return false;

  // Now get name of root hub (wide chars)...
  if (!DeviceIoControl(hHostCtrl, IOCTL_USB_GET_ROOT_HUB_NAME, 0, 0, name_wstr, nameinfo.ActualLength, &bytelen, NULL)) {
    free (name_wstr);
    return false;
  }

  // Convert from wstr to char...
  CString namestr(name_wstr->RootHubName); 
  free (name_wstr);

  // Go enumerate the root hub...
  //TRACE ("roothubname = '%s'\n",namestr);
  return EnumerateHub(namestr);
}



//
// Enumerate a hub...
//
bool CEnumerateUSB::EnumerateHub(CString namestr)
{
  if (namestr.IsEmpty()) return false;

  // Convert to full hub device name...
  namestr.Insert(0,_T("\\\\.\\")); 

  // Open the hub...
  HANDLE hHub = CreateFile(namestr, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hHub == INVALID_HANDLE_VALUE) return false;

  // Get USB_NODE_INFORMATION for hub, so know number of ports...
  USB_NODE_INFORMATION node_info;
  DWORD bytelen = sizeof(USB_NODE_INFORMATION);
  if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &node_info, bytelen, &node_info, bytelen, &bytelen, NULL)) {
    int numports = node_info.u.HubInformation.HubDescriptor.bNumberOfPorts;
    //TRACE ("number of ports=%d\n", numports);

    bytelen = 50*sizeof(USB_NODE_CONNECTION_INFORMATION);
    USB_NODE_CONNECTION_INFORMATION *cinfo = (USB_NODE_CONNECTION_INFORMATION*)malloc(bytelen);
    if (cinfo) {
      for (int i=0; i<numports; i++) {
        memset (cinfo, 0, bytelen);
        cinfo->ConnectionIndex = i;

        DWORD cbytelen = bytelen;
        if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION, cinfo, cbytelen, cinfo, cbytelen, &cbytelen, NULL)) {

          if (cinfo->ConnectionStatus == DeviceConnected) {
            int idVendor = cinfo->DeviceDescriptor.idVendor;
            int idProduct = cinfo->DeviceDescriptor.idProduct;
            int iManufacturer = cinfo->DeviceDescriptor.iManufacturer;
            int iProduct = cinfo->DeviceDescriptor.iProduct;
            int iSerialNumber = cinfo->DeviceDescriptor.iSerialNumber;

            if (iProduct) {
              CString product_str;
              product_str = GetDescriptorString(hHub, i, iProduct);
              //TRACE ("vid=%x pid=%x iProduct = %d '%s'\n", idVendor, idProduct, iProduct, product_str);

              if ((idVendor == m_target_vid) && (idProduct == m_target_pid)) {
                m_iProduct_strname = product_str;
                m_target_hit = true;
                break;
              }
            }
/*
            // Get configuration descriptor.   First find out how much space we need...
            USB_DESCRIPTOR_REQUEST *config_desc = GetConfigDescriptor(hHub,i);
            if (config_desc) {
            }
*/
          }

          if (cinfo->DeviceIsHub)
            EnumerateHub(GetHubName(hHub, i));
        }
      }
      free (cinfo);
    }
  }

  CloseHandle(hHub);
  return true;
}



//
// Get name of a hub...
//
CString CEnumerateUSB::GetHubName(HANDLE hHub, int portnum)
{
  USB_NODE_CONNECTION_NAME nameinfo;
  DWORD bytelen = sizeof(nameinfo);

  nameinfo.ConnectionIndex = portnum;
  if (!DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, &nameinfo, bytelen, &nameinfo, bytelen, &bytelen, NULL))
    return "";

  bytelen = nameinfo.ActualLength;
  USB_NODE_CONNECTION_NAME *name_wstr = (USB_NODE_CONNECTION_NAME *)malloc(bytelen);
  if (name_wstr==NULL) return "";

  memset (name_wstr, 0, bytelen);
  name_wstr->ConnectionIndex = portnum;
  if (!DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, name_wstr, bytelen, name_wstr, bytelen, &bytelen, NULL)) {
    free (name_wstr);
    return "";
  }

  // Convert from wstr to char...
  CString namestr(name_wstr->NodeName); 
  free (name_wstr);

  //TRACE ("external hubname = '%s'\n",namestr);
  return namestr;
}



//
// Get string corresponding to descriptor index...
//
#define MAXDESCRIPTORSTRLEN 1024
CString CEnumerateUSB::GetDescriptorString(HANDLE hHub, int portnum, int strindex)
{
  BYTE reqbuf[sizeof(USB_DESCRIPTOR_REQUEST)+MAXDESCRIPTORSTRLEN];
  USB_DESCRIPTOR_REQUEST *rbuf = (USB_DESCRIPTOR_REQUEST*)&reqbuf;
  USB_STRING_DESCRIPTOR *rstr = (USB_STRING_DESCRIPTOR*)(rbuf+1);

  memset (&reqbuf,0,sizeof(reqbuf));
  rbuf->ConnectionIndex = portnum;
  rbuf->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE<<8) | strindex;
  rbuf->SetupPacket.wIndex = 0;
  rbuf->SetupPacket.wLength = MAXDESCRIPTORSTRLEN;
  DWORD rbytelen = sizeof(reqbuf);

  if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, rbuf, rbytelen, rbuf, rbytelen, &rbytelen, NULL))
    return "";

  // Sanity checks...
  if (rbytelen<sizeof(USB_STRING_DESCRIPTOR)) return "";
  if (rstr->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE) return "";
  if (rstr->bLength != (rbytelen - sizeof(USB_DESCRIPTOR_REQUEST))) return "";
  if (rstr->bLength & 1) return "";

  // Convert Wide string to Ascii...
  CString temp(rstr->bString);
  return temp;
}



//
// Get configuration descriptor for USB port...
//
USB_DESCRIPTOR_REQUEST *CEnumerateUSB::GetConfigDescriptor(HANDLE hHub, int portnum)
{
  // Get configuration descriptor.   First find out how much space we need...
  BYTE reqbuf[sizeof(USB_DESCRIPTOR_REQUEST)+sizeof(USB_CONFIGURATION_DESCRIPTOR)];
  USB_DESCRIPTOR_REQUEST *rbuf = (USB_DESCRIPTOR_REQUEST*)&reqbuf;

  memset (&reqbuf,0,sizeof(reqbuf));
  rbuf->ConnectionIndex = portnum;
  rbuf->SetupPacket.wValue = USB_CONFIGURATION_DESCRIPTOR_TYPE<<8;
  rbuf->SetupPacket.wLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
  DWORD rbytelen = sizeof(reqbuf);

  if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, rbuf, rbytelen, rbuf, rbytelen, &rbytelen, NULL))
    return NULL;

  // Allocate space & read descriptor...
  USB_CONFIGURATION_DESCRIPTOR *rdesc = (USB_CONFIGURATION_DESCRIPTOR*)(rbuf+1);
  rbytelen = sizeof(USB_DESCRIPTOR_REQUEST) + rdesc->wTotalLength;
  rbuf = (USB_DESCRIPTOR_REQUEST*)malloc(rbytelen);
  if (rbuf==NULL) return NULL;

  rbuf->ConnectionIndex = portnum;
  rbuf->SetupPacket.wValue = USB_CONFIGURATION_DESCRIPTOR_TYPE<<8;
  rbuf->SetupPacket.wLength = rdesc->wTotalLength;
  if (!DeviceIoControl(hHub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, rbuf, rbytelen, rbuf, rbytelen, &rbytelen, NULL)) {
    free(rbuf);
    return NULL;
  }

  return rbuf;
}


