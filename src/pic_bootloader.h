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
 ***************************************************************************
 *                                                                         *
 * Feb 5 2011 - Ian Davis - Removed dependency on cstdlib, convoluted      *
 *                          buffer library, etc...  much simpler now.      *
 *                                                                         *
 ***************************************************************************/


#ifndef PIC_BOOTLOADER_H_INCLUDED
#define PIC_BOOTLOADER_H_INCLUDED

#include "boot_if.h"
#include <windows.h>

#define WM_PICBOOTLOADER_STATUS WM_APP+1234

#define OLS_FLASH_SIZE 0x4000                    // PIC flash memory size.
#define OLS_WRITEADDR_START 0x800                // Protect bootloader in first two pages.
#define OLS_WRITEADDR_END (OLS_FLASH_SIZE-0x400) // Protect flags page.

//#define OLS_WRITE_XFERLEN 2                    // Two byte xfer mode
#define OLS_WRITE_XFERLEN 32                     // one-half PIC page size (since HID xfers can't handle 64 bytes)

#define OLSFLAG_NONE 0x00                        // Flags for directing bootloader
#define OLSFLAG_FLUSHWRITE 0x01                  // Write data in buffer to flash.
#define OLSFLAG_TWOBYTEXFER 0x02                 // Perform two byte write.  If not set, assumes 32 bytes.

#define OLS_VID 0x04D8                           // Values for Logic Sniffer
#define OLS_PID 0xFC90


typedef unsigned char *BUFPTR;


//
// Error results...
//
#define PICRESULT_NOERROR 0
#define PICRESULT_FIRST_ERROR 100
#define PICRESULT_INVALID_HANDLE  100
#define PICRESULT_INVALID_BUFFER 101
#define PICRESULT_MEMALLOCERROR_DEVICE_DETAIL 102 // eTrace0("Couldn't allocate memory for device interface detail data");
#define PICRESULT_GETDEVICEDETAIL_FAIL 103
#define PICRESULT_DEVICE_NOT_FOUND 104
#define PICRESULT_HID_READ_FAIL 105 // GetLastError()
#define PICRESULT_HID_WRITE_FAIL 106 // GetLastError()
#define PICRESULT_HID_WRITE_TIMEOUT 107
#define PICRESULT_HID_READ_TIMEOUT 108
#define PICRESULT_HID_RESPONSE_MISMATCH 109
#define PICRESULT_VERIFY_FAIL 110
#define PICRESULT_LAST_ERROR 199

#define PICSTATUS_INIT_XFER 200
#define PICSTATUS_START_XFER 201
#define PICSTATUS_UPDATE_XFER 202
#define PICSTATUS_DONE_XFER 203


//
// Instantiate the class...
//
class PicBootloader
{
public:
  typedef void progress_callback_type(int percentage, int status);

  PicBootloader() {
    m_command_id = 0;
    m_hDevice = INVALID_HANDLE_VALUE;
    m_vid = OLS_VID;
    m_pid = OLS_PID;
    m_progress_callback = NULL;
    m_progress_hWnd = NULL;
  }

  ~PicBootloader() {
    if (m_hDevice != INVALID_HANDLE_VALUE)
      CloseHandle(m_hDevice);
  }

  // Attributes...
  unsigned int m_vid;         // Target device VID (Vendor ID)
  unsigned int m_pid;         // Target device PID (Product ID)
  HANDLE m_hDevice;           // Handle to USB device.
  unsigned char m_command_id; // A USB transaction counter.  Used to make sure USB device received command.

  // Progress/status callback support...
  progress_callback_type *m_progress_callback;
  HWND m_progress_hWnd;

  // Return page size of device memory...
  inline size_t page_size() {return 2;} // page size

  // Send reset command to device...
  int reset();

  // Erase device memory...
  int erase();

  // Read device memory.  Allocates a buffer & returns pointer in parameter...
  int read(BUFPTR *buffer=NULL, size_t *bufsize=NULL);

  // Verify flash memory on PIC against buffer...
  int verify(BUFPTR bufptr=NULL, size_t bufsize=0);

  // Program device memory...
  int write(BUFPTR buffer=NULL, size_t bufsize=0);

  // Read bootloader version info...
  int getversion(boot_rsp * response);

  // For reporting progress.  Either a callback, or post Windows message... or both.
  virtual void progress(int percentage, int status) {
    if (m_progress_callback) (*m_progress_callback)(percentage,status);
    if (m_progress_hWnd) ::PostMessage(m_progress_hWnd, WM_PICBOOTLOADER_STATUS, percentage, status);
  };

private:
  // Open USB channel to PIC...
  int open();

  // Close USB channel...
  void close();

  // Perform request & wait for response from PIC...
  int transaction(boot_cmd* pOut, boot_rsp* pIn);

  // the same as transaction but doesn't wait for response
  // useful for reset command (when device sends no response)
  int send_command(boot_cmd* pOut);

  // Make sure specified USB device has corrent vendor/product ID's...
  bool isValidDevice(char *DevicePath);
};

#endif // PIC_BOOTLOADER_H_INCLUDED
