/*
 * Michal Demin - 2010
 *
 * ----------------------------------------------------------------------------
 *
 * Modified Jan 8, 2011 - Ian Davis - Rewrote to return error codes instead of 
 *   reporting error conditions directly.  Added error result message handler.
 *
 */

#include "stdafx.h"

#ifndef WIN32
#include <stdio.h>
#include <memory.h>
#define TCHAR char
#define _T(a) a
#define wsprintf sprintf
#endif

#include "data_file.h"
#include "serial.h"
#include "ols.h"


const struct pump_flash_t PUMP_Flash[] = {
  {
    "\x1f\x24\x00\x00",
    264, // size of page
    2048, // number of pages
    "ATMEL AT45DB041D (4MBit)"
  },
  {
    "\x1f\x23\x00\x00",
    264, // size of page
    1024, // number of pages
    "ATMEL AT45DB021D (2MBit)"
  },
  {
    "\xef\x30\x13\x00",
    256, // size of page
    2048, // number of pages
    "WINBOND W25X40 (4MBit)"
  },
  {
    "\xef\x30\x12\x00",
    256, // size of page
    1024, // number of pages
    "WINBOND W25X20 (2MBit)"
  },
  {
    "\xef\x30\x14\x00",
    256, // size of page
    2048, // number of pages
    "WINBOND W25X40 (8MBit)"
  },
  {
    "\xef\x40\x14\x00",
    256, // size of page
    4096, // number of pages
    "WINBOND W25Q80 (8MBit)"
  },
};

#define PUMP_FLASH_NUM (sizeof(PUMP_Flash)/sizeof(struct pump_flash_t))



/*
 * Return readable message based on OLS error code...
 */
TCHAR *OLSResultErrorMsg(int errcode)
{
  static TCHAR temp[64];
  switch (errcode) {
    case OLSRESULT_SUCCESS : return _T("No error.");
    case OLSRESULT_TIMEOUT : return _T("Timeout while attempting to access OLS.");
    case OLSRESULT_CMDWRITE_ERROR : return _T("Problem writing command to OLS.");
    case OLSRESULT_1V2SUPPLY_BAD : return _T("1V2 supply failed self-test.");
    case OLSRESULT_2V5SUPPLY_BAD : return _T("2V5 supply failed self-test.");
    case OLSRESULT_PROGB_BAD : return _T("PROG_B pull-up failed self-test.");
    case OLSRESULT_DONE_BAD : return _T("DONE pull-up failed self-test.");
    case OLSRESULT_UNKNOWN_JEDICID : return _T("Unknown ROM JEDEC ID (this could be ok...)");
    case OLSRESULT_UPDATE_BAD : return _T("UPDATE button pull-up failed self-test.");
    case OLSRESULT_STATUS_READERROR : return _T("Unable to read status from OLS.");
    case OLSRESULT_ID_READERROR : return _T("Unable to read ID from OLS.");
    case OLSRESULT_FLASHID_READERROR : return _T("Unable to read Flash JEDIC ID from OLS.");
    case OLSRESULT_FLASHID_UNKNOWN : return _T("Unknown Flash JEDIC ID from OLS.");
    case OLSRESULT_ERASE_ERROR : return _T("Problem occured during Flash erase.");
    case OLSRESULT_INVALID_PAGE : return _T("Attempted to access invalid flash page number.");
    case OLSRESULT_PAGEREAD_ERROR : return _T("Unable to read flash page data.");
    case OLSRESULT_PAGEWRITE_ERROR : return _T("Unable to write flash page data.");
    case OLSRESULT_CHECKSUM_ERROR : return _T("Received checksum error writing OLS flash page data.");
  }
  wsprintf (temp,"Unknown error code %d.",errcode);
  return temp;
}



/*
 * Does OLS self-test
 * pump_fd - fd of pump com port
 */
int PUMP_selftest(int pump_fd) 
{
  static const uint8_t cmd[4] = {0x07, 0x00, 0x00, 0x00};
  uint8_t status;
  int res, retry;

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4) return OLSRESULT_CMDWRITE_ERROR;

  retry=0;
  while (1) {
    res = serial_read(pump_fd, &status, 1);
    if (res<1) retry++;

    if (res == 1)
      break;

    // 20 second timenout
    if (retry > 60) 
      return OLSRESULT_TIMEOUT;
  }

  if (status & 0x01) return OLSRESULT_1V2SUPPLY_BAD;
  if (status & 0x02) return OLSRESULT_2V5SUPPLY_BAD;
  if (status & 0x04) return OLSRESULT_PROGB_BAD;
  if (status & 0x08) return OLSRESULT_DONE_BAD;
  if (status & 0x10) return OLSRESULT_UNKNOWN_JEDICID;
  if (status & 0x20) return OLSRESULT_UPDATE_BAD;
  return OLSRESULT_SUCCESS;
}


/*
 * Reads OLS status
 * pump_fd - fd of pump com port
 */
int PUMP_GetStatus(int pump_fd, uint8_t *result) 
{
  static const uint8_t cmd[4] = {0x05, 0x00, 0x00, 0x00};
  uint8_t status;
  int res;

  if (result) *result = 0;

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  res = serial_read(pump_fd, &status, 1);
  if (res != 1)
    return OLSRESULT_STATUS_READERROR;

  if (result) *result = status;
  return OLSRESULT_SUCCESS;
}



/*
 * Reads OLS version
 * pump_fd - fd of pump com port
 */
int PUMP_GetID(int pump_fd, uint8_t *result, int result_maxsize) 
{
  static const uint8_t cmd[4] = {0x00, 0x00, 0x00, 0x00};
  uint8_t ret[OLS_ID_INFOSIZE];
  int res;

  if (result) memset (result,0,result_maxsize);

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  res = serial_read(pump_fd, ret, 7);
  if (res != 7)
    return OLSRESULT_ID_READERROR;

  if (result_maxsize>=sizeof(ret)) result_maxsize = sizeof(ret);
  if (result) memcpy (result,ret,result_maxsize);

  return OLSRESULT_SUCCESS;
}


/*
 * commands OLS to enter bootloader mode
 * pump_fd - fd of pump com port
 */
int PUMP_EnterBootloader(int pump_fd) 
{
  static const uint8_t cmd[4] = {0x24, 0x24, 0x24, 0x24};
  int res;

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  return OLSRESULT_SUCCESS;
}


/*
 * Switch the OLS to run mode
 * pump_fd - fd of pump com port
 */
int PUMP_EnterRunMode(int pump_fd) 
{
  static const uint8_t cmd[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  int res;

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  return OLSRESULT_SUCCESS;
}


/*
 * ask the OLS for JEDEC id
 * pump_fd - fd of pump com port
 */
int PUMP_GetFlashID(int pump_fd, int *flashid, uint8_t *flashjedic, int flashjedic_maxsize, int cmd_ignore_jedec) 
{
  static const uint8_t cmd[4] = {0x01, 0x00, 0x00, 0x00};
  uint8_t ret[OLD_FLASHID_INFOSIZE];
  int res;
  int i;

  if (flashid) *flashid=-1;
  if (flashjedic) memset(flashjedic,0,flashjedic_maxsize);

  if (cmd_ignore_jedec) { // ignore jedec if flag cmd_ignore_jedec = 1
    if (flashid) *flashid = 0;
    return OLSRESULT_SUCCESS;
  }

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  res = serial_read(pump_fd, ret, 4);
  if (res != 4)
    return OLSRESULT_FLASHID_READERROR;

  if (flashjedic) { // copy results to caller
    if (flashjedic_maxsize > sizeof(ret)) flashjedic_maxsize = sizeof(ret);
    memcpy (flashjedic, ret, flashjedic_maxsize);
  }

  // See if flash ID recognized...
  for (i=0; i< PUMP_FLASH_NUM; i++)
    if (memcmp(ret, PUMP_Flash[i].jedec_id, 4) == 0) {
      if (flashid) *flashid = i;      
      return OLSRESULT_SUCCESS;
    }

  return OLSRESULT_FLASHID_UNKNOWN;
}


/*
 * erases OLS flash
 * pump_fd - fd of pump com port
 */
int PUMP_FlashErase(int pump_fd, int flashid) {
  static const uint8_t cmd[4] = {0x04, 0x00, 0x00, 0x00};
  uint8_t status;
  int res;
  int retry = 0;

  if (flashid < 0)
    return OLSRESULT_FLASHID_UNKNOWN;

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  while (1) {
    res = serial_read(pump_fd, &status, 1);
    if (res < 1)
      retry++;

    if (res == 1)
      return (status == 0x01) ? OLSRESULT_SUCCESS : OLSRESULT_ERASE_ERROR;

    // 20 second timenout
    if (retry > 60)
      return OLSRESULT_TIMEOUT;
  }

  return OLSRESULT_SUCCESS;
}


/*
 * Reads data from flash
 * pump_fd - fd of pump com port
 * page - which page should be read
 * buf - buffer where the data will be stored
 */
int PUMP_FlashRead(int pump_fd, int flashid, int page, uint8_t *buf) 
{
  uint8_t cmd[4] = {0x03, 0x00, 0x00, 0x00};
  int res;

  if (flashid == -1)
    return OLSRESULT_FLASHID_UNKNOWN;

  if (page > PUMP_Flash[flashid].pages)
    return OLSRESULT_INVALID_PAGE;

  if (PUMP_Flash[flashid].page_size==264) {//ATMEL ROM with 264 byte pages
    cmd[1] = (page >> 7) & 0xff;
    cmd[2] = (page << 1) & 0xff;
  }
  else{ // most 256 byte page roms
    cmd[1] = (page >> 8) & 0xff;
    cmd[2] = (page) & 0xff;
  }

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  res = serial_read(pump_fd, buf, PUMP_Flash[flashid].page_size);
  if (res != PUMP_Flash[flashid].page_size)
    return OLSRESULT_PAGEREAD_ERROR;

  return OLSRESULT_SUCCESS;
}


/*
 * writes data to flash
 * pump_fd - fd of pump com port
 * page - where the data should be written to
 * buf - data to be written
 */
int PUMP_FlashWrite(int pump_fd, int flashid, int page, uint8_t *buf) 
{
  uint8_t cmd[4] = {0x02, 0x00, 0x00, 0x00};
  uint8_t status;
  uint8_t chksum;
  int res;

  if (flashid == -1)
    return OLSRESULT_FLASHID_UNKNOWN;

  if (page > PUMP_Flash[flashid].pages)
    return OLSRESULT_INVALID_PAGE;

  if (PUMP_Flash[flashid].page_size==264) {//ATMEL ROM with 264 byte pages
    cmd[1] = (page >> 7) & 0xff;
    cmd[2] = (page << 1) & 0xff;
  }
  else{ //most 256 byte page roms
    cmd[1] = (page >> 8) & 0xff;
    cmd[2] = (page) & 0xff;
  }

  chksum = Data_Checksum(buf, PUMP_Flash[flashid].page_size);

  res = serial_write(pump_fd, cmd, 4);
  if (res != 4)
    return OLSRESULT_CMDWRITE_ERROR;

  res = serial_write(pump_fd, buf, PUMP_Flash[flashid].page_size);
  res += serial_write(pump_fd, &chksum, 1);
  if (res != 1+PUMP_Flash[flashid].page_size)
    return OLSRESULT_PAGEWRITE_ERROR;

  res = serial_read(pump_fd, &status, 1);
  if (res != 1) 
    return OLSRESULT_TIMEOUT;

  if (status != 0x01) 
    return OLSRESULT_CHECKSUM_ERROR;

  return OLSRESULT_SUCCESS;
}

