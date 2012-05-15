/***************************************************************************
 *   Copyright (C) 2007 by Diolan                                          *
 *   www.diolan.com                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "stdafx.h"
#include <wtypes.h>
#include <setupapi.h>
#include "pic_bootloader.h"

extern "C" {
#include <hidsdi.h>
}

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")



//
// Reset the PIC...
//
int PicBootloader::reset()
{
  boot_cmd command;
  memset(&command, 0, sizeof(boot_cmd));
  command.reset.cmd = BOOT_RESET;

  int result = open();
  if (result) return result;

  result = send_command(&command);
  close();

  return result;
}



//
// Erase flash memory on PIC...
//
int PicBootloader::erase()
{
  boot_cmd command;
  boot_rsp response;
  memset(&command, 0, sizeof(boot_cmd));
  memset(&response, 0, sizeof(boot_rsp));
  command.erase_flash.cmd = BOOT_ERASE_FLASH;

  int result = open();
  if (result) return result;

  command.erase_flash.echo = ++m_command_id;
  command.erase_flash.addr_hi = (unsigned char)((OLS_WRITEADDR_START >> 8) & 0xFF);
  command.erase_flash.addr_lo = (unsigned char)(OLS_WRITEADDR_START & 0xFF);
  command.erase_flash.size_x64 = (OLS_WRITEADDR_END-OLS_WRITEADDR_START)/0x400; // number of 1Kbyte blocks to erase
  result = transaction(&command, &response);
  close();

  return result;
}



//
// Return bootloader version...
//
int PicBootloader::getversion(boot_rsp * response)
{
  boot_cmd command;
  memset(&command, 0, sizeof(boot_cmd));
  memset(response, 0, sizeof(boot_rsp));
  command.get_fw_ver.cmd = BOOT_GET_FW_VER;

  int result = open();
  if (result) return result;

  result = transaction(&command, response);
  close();

  return result;
}


//
// Allocate a buffer, then read PIC flash memory into it...
//
int PicBootloader::read(BUFPTR *bufptr, size_t *bufsize)
{
  boot_cmd command;
  boot_rsp response;
  int result;

  const int xfersize = OLS_FLASH_SIZE;
  BUFPTR buffer = new unsigned char[xfersize];
  if (buffer==NULL) {
    result = PICRESULT_INVALID_BUFFER;
    progress (result, 0);
    return result;
  }

  memset(buffer, 0, xfersize);
  if (bufptr) *bufptr = buffer;
  if (bufsize) *bufsize = xfersize;

  int last_percent = 0;
  progress (PICSTATUS_INIT_XFER, last_percent);

  result = open();
  if (result) {
    progress (result, 0);
    return result;
  }

  progress (PICSTATUS_START_XFER, last_percent);
  for (size_t address=0; address<xfersize;) {
    memset (&command, 0xFF, sizeof(boot_cmd));
    memset (&response, 0xFF, sizeof(boot_rsp));

    size_t xferlen = sizeof(response.read_flash.data);
    if (xferlen > (xfersize-address)) xferlen = xfersize-address;

    command.header.cmd = BOOT_READ_FLASH;
    command.read_flash.echo = ++m_command_id;
    command.read_flash.addr_hi = (unsigned char)((address >> 8) & 0xFF);
    command.read_flash.addr_lo = (unsigned char)(address & 0xFF);
    command.read_flash.size8 = xferlen;

    result = transaction(&command, &response);
    if (result) {
      progress (result, last_percent);
      close(); 
      return result;
    }

    memcpy (&buffer[address], &(response.read_flash.data), xferlen);
    address += xferlen;

    int percent = (address*100)/xfersize;
    if (percent>100) percent=100;
    if (percent != last_percent) {
      last_percent = percent;  
      progress (PICSTATUS_UPDATE_XFER, last_percent);
    }
  }

  close();
  progress (PICSTATUS_DONE_XFER, 100);
  return PICRESULT_NOERROR;
}



//
// Verify flash memory on PIC against buffer...
//
int PicBootloader::verify(BUFPTR bufptr, size_t bufsize)
{
  BUFPTR readptr=NULL;
  size_t readsize=0;
  if ((bufptr == NULL) || (bufsize != OLS_FLASH_SIZE)) 
    return PICRESULT_INVALID_BUFFER;

  int result = read(&readptr, &readsize);
  if (result) return result;

  if ((readptr == NULL) || (readsize != bufsize))
    result = PICRESULT_VERIFY_FAIL;
  else
    for (size_t address=OLS_WRITEADDR_START; address<OLS_WRITEADDR_END; address++)
      if (bufptr[address] != readptr[address]) {
        result = PICRESULT_VERIFY_FAIL;
        break;
      }

  if (readptr) free(readptr);
  return result;
}



//
// Write buffer to flash memory on PIC...
//
int PicBootloader::write(BUFPTR buffer, size_t bufsize)
{
  boot_cmd command;
  boot_rsp response;
  int count,result;
  size_t address;

  size_t startaddr = OLS_WRITEADDR_START;
  size_t endaddr = OLS_WRITEADDR_END;

  if ((buffer==NULL) || (bufsize != OLS_FLASH_SIZE)) {
    result = PICRESULT_INVALID_BUFFER;
    progress (result, 0);
    return result;
  }

  int last_percent = 0;
  progress (PICSTATUS_INIT_XFER, last_percent);

  result = open();
  if (result) {
    progress (result, 0);
    return result;
  }

  progress (PICSTATUS_START_XFER, last_percent);
  for (count=0, address=startaddr; address<endaddr; count++) {
    size_t xferlen = OLS_WRITE_XFERLEN;
    if (xferlen > (endaddr-address)) xferlen = endaddr-address;

    memset (&command, 0xFF, sizeof(boot_cmd));
    memset (&response, 0xFF, sizeof(boot_rsp));
    command.header.cmd = BOOT_WRITE_FLASH;
    command.write_flash.echo = ++m_command_id;
    command.write_flash.addr_hi = (unsigned char)((address >> 8) & 0xFF);
    command.write_flash.addr_lo = (unsigned char)(address & 0xFF);
    command.write_flash.size8 = xferlen;

    if (xferlen==2)
      command.write_flash.flush = OLSFLAG_TWOBYTEXFER | OLSFLAG_FLUSHWRITE;
    else command.write_flash.flush = (count&1) ? OLSFLAG_FLUSHWRITE : OLSFLAG_NONE;

    memcpy (&(command.write_flash.data), &buffer[address], xferlen);

    result = transaction(&command, &response);
    if (result) {
      progress (result, last_percent);
      close(); 
      return result;
    }

    address += xferlen;

    int percent = ((address-startaddr)*100)/(endaddr-startaddr);
    if (percent>100) percent=100;
    if (percent != last_percent) {
      last_percent = percent;  
      progress (PICSTATUS_UPDATE_XFER, last_percent);
    }
  }

  close();
  progress (PICSTATUS_DONE_XFER, 100);
  return PICRESULT_NOERROR;
}



//
// Open USB channel to PIC...
//
int PicBootloader::open()
{
  if (m_hDevice != INVALID_HANDLE_VALUE) 
    close();

  GUID HidGuid;
  HidD_GetHidGuid( &HidGuid);

  HDEVINFO hDevInfo = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  if (hDevInfo == INVALID_HANDLE_VALUE)
    return PICRESULT_INVALID_HANDLE;

  SP_DEVICE_INTERFACE_DATA DevInterfaceData;

  bool done = false;
  unsigned long DevIndex = 0;
  while (!done) {
    memset (&DevInterfaceData,0,sizeof(DevInterfaceData));
    DevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &HidGuid, DevIndex, &DevInterfaceData)) break;

    unsigned long DetailsSize = 0;
    SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, NULL, 0, &DetailsSize, NULL);

    PSP_INTERFACE_DEVICE_DETAIL_DATA pDetails = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(DetailsSize);
    if (pDetails == NULL) {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return PICRESULT_MEMALLOCERROR_DEVICE_DETAIL;
    }

    memset (pDetails,0,DetailsSize);
    pDetails->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, pDetails, DetailsSize, NULL, NULL)) {
      free(pDetails);
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return PICRESULT_GETDEVICEDETAIL_FAIL;
    }

    CString path(pDetails->DevicePath);
    if (isValidDevice(path)) { // valid device found
      m_hDevice = CreateFile(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
      free(pDetails);
      break;
    }

    free(pDetails);
    DevIndex++;
  }

  SetupDiDestroyDeviceInfoList(hDevInfo);
  return (m_hDevice == INVALID_HANDLE_VALUE) ? PICRESULT_DEVICE_NOT_FOUND : PICRESULT_NOERROR;
}


//
// Close USB channel...
//
void PicBootloader::close()
{
  if (m_hDevice != INVALID_HANDLE_VALUE) CloseHandle(m_hDevice);
  m_hDevice = INVALID_HANDLE_VALUE;
}



//
// Make sure specified USB device has corrent vendor/product ID's...
//
bool PicBootloader::isValidDevice(const char *DevicePath)
{
  HANDLE hHidDevice = CreateFile(DevicePath,
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,        // no SECURITY_ATTRIBUTES structure
    OPEN_EXISTING,      // no special create flags
    0,        // no special attributes
    NULL);        // no template file

  if (hHidDevice == INVALID_HANDLE_VALUE)
    return NULL;

  HIDD_ATTRIBUTES Attr;
  memset (&Attr, 0, sizeof(Attr));

  BOOL result = HidD_GetAttributes (hHidDevice, &Attr);
  CloseHandle(hHidDevice);

  //printf ("%s\n",DevicePath);
  //printf ("device: vid=%x, pid=%x\n", Attr.VendorID, Attr.ProductID);

  return (result && (Attr.VendorID == m_vid) && (Attr.ProductID == m_pid));
}



//
// Send command, but don't wait for response.   Used for reset 
// command, which doesn't respond...
//
int PicBootloader::send_command(boot_cmd* pOut)
{
  int result = PICRESULT_NOERROR;
  unsigned char write_buf[sizeof(boot_cmd)+1];
  OVERLAPPED write_over;

  memset(&write_over, 0, sizeof(OVERLAPPED));
  write_over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  unsigned long written;
  memcpy(write_buf + 1, pOut, sizeof(boot_cmd));
  write_buf[0] = 0;

  BOOL write_res = WriteFile(m_hDevice, write_buf, sizeof(boot_cmd) + 1, &written, &write_over);
  if ((!write_res) && (GetLastError() != ERROR_IO_PENDING)) {
    result = PICRESULT_HID_WRITE_FAIL;
    goto done;
  }

  if (WaitForSingleObject(write_over.hEvent, 100) == WAIT_TIMEOUT) {
    result = PICRESULT_HID_WRITE_TIMEOUT;
    goto done;
  }

done:
  if (result) CancelIo(m_hDevice);
  CloseHandle(write_over.hEvent);
  return result;
}



//
// Perform transaction with PIC via the USB interface...
//
int PicBootloader::transaction(boot_cmd* pOut, boot_rsp* pIn)
{
  BOOL write_res, read_res;

  int result = PICRESULT_NOERROR;
  OVERLAPPED write_over, read_over;
  unsigned char read_buf[sizeof(boot_rsp)+1];
  unsigned char write_buf[sizeof(boot_cmd)+1];

  memset(&write_over, 0, sizeof(OVERLAPPED));
  memset(&read_over, 0, sizeof(OVERLAPPED));

  write_over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  read_over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  unsigned long readcount;
  if ((!ReadFile(m_hDevice, read_buf, sizeof(boot_rsp)+1, &readcount, &read_over)) && (GetLastError() != ERROR_IO_PENDING)) {
    result = PICRESULT_HID_READ_FAIL;
    goto done;
  }

  unsigned long written;
  memcpy(write_buf + 1, pOut, sizeof(boot_cmd));
  write_buf[0] = 0;

  write_res = WriteFile(m_hDevice, write_buf, sizeof(boot_cmd) + 1, &written, &write_over);
  if (!write_res && (GetLastError() != ERROR_IO_PENDING)) {
    CancelIo(m_hDevice);
    WaitForSingleObject(read_over.hEvent,INFINITE);
    result = PICRESULT_HID_WRITE_FAIL;
    goto done;
  }

  if (WaitForSingleObject(write_over.hEvent, 100) == WAIT_TIMEOUT) {
    CancelIo(m_hDevice);
    WaitForSingleObject(write_over.hEvent,INFINITE);
    WaitForSingleObject(read_over.hEvent,INFINITE);
    goto done;
  }

  if (WaitForSingleObject(read_over.hEvent, 5000) == WAIT_TIMEOUT) {
    CancelIo(m_hDevice);
    WaitForSingleObject(read_over.hEvent,INFINITE);
    goto done;
  }

  read_res = GetOverlappedResult(m_hDevice, &read_over, &readcount, FALSE);
  if (!read_res || (readcount != sizeof(boot_rsp) + 1)) {
    result = PICRESULT_HID_READ_FAIL;
    goto done;
  }

  memcpy(pIn, read_buf + 1, sizeof(boot_rsp));
  if ((pOut->header.cmd != pIn->header.cmd) || (pOut->header.echo != pIn->header.echo)) {
//    TRACE("pOut->header.cmd = %d,  pIn->header.cmd = %d", pOut->header.cmd, pIn->header.cmd);
//    TRACE("pOut->header.echo = %d,  pIn->header.echo = %d", pOut->header.echo, pIn->header.echo);
    result = PICRESULT_HID_RESPONSE_MISMATCH;
  }

done:
  CloseHandle(write_over.hEvent);
  CloseHandle(read_over.hEvent);
  return result;
}

