/*
 * Michal Demin - 2010
 *
 * ----------------------------------------------------------------------------
 *
 * Modified Jan 8, 2011 - Ian Davis - Rewrote to return error codes instead of 
 *   reporting error conditions directly.  Added error result message handler.
 *
 */

#ifndef _OLS_H_
#define _OLS_H_

#ifndef WIN32
#include <stdint.h>
#endif

#define OLSRESULT_SUCCESS 0
#define OLSRESULT_TIMEOUT -1             // Timeout while attempting to access OLS
#define OLSRESULT_CMDWRITE_ERROR -2      // Error writing command to OLS
#define OLSRESULT_1V2SUPPLY_BAD -3       // 1V2 supply failed self-test
#define OLSRESULT_2V5SUPPLY_BAD -4       // 2V5 supply failed self-test
#define OLSRESULT_PROGB_BAD -5           // PROG_B pull-up failed self-test
#define OLSRESULT_DONE_BAD -6            // DONE pull-up failed self-test
#define OLSRESULT_UNKNOWN_JEDICID -7     // Unknown ROM JEDEC ID (this could be ok...)
#define OLSRESULT_UPDATE_BAD -8          // UPDATE button pull-up failed self-test
#define OLSRESULT_STATUS_READERROR -9    // Error reading status from OLS
#define OLSRESULT_ID_READERROR -10       // Error reading ID from OLS
#define OLSRESULT_FLASHID_READERROR -11  // Error reading Flash JEDIC ID from OLS
#define OLSRESULT_FLASHID_UNKNOWN -12    // Unknown Flash JEDIC ID from OLS
#define OLSRESULT_ERASE_ERROR -13        // Error occured during Flash erase.
#define OLSRESULT_INVALID_PAGE -14       // Attempted to access invalid flash page number
#define OLSRESULT_PAGEREAD_ERROR -15     // Error reading flash page data
#define OLSRESULT_PAGEWRITE_ERROR -16    // Error writing flash page data
#define OLSRESULT_CHECKSUM_ERROR -17     // Received checksum error while writing OLS flash page data

#define OLS_ID_INFOSIZE 7
#define OLD_FLASHID_INFOSIZE 4


struct pump_flash_t {
	const char *jedec_id;
	uint16_t page_size;
	uint16_t pages;
	const char *name;
};

extern const struct pump_flash_t PUMP_Flash[];

char *OLSResultErrorMsg(int errcode);

int PUMP_selftest(int pump_fd);
int PUMP_GetStatus(int pump_fd, uint8_t *result);
int PUMP_GetID(int pump_fd, uint8_t *result, int result_maxsize);
int PUMP_EnterBootloader(int pump_fd);
int PUMP_EnterRunMode(int pump_fd);
int PUMP_GetFlashID(int pump_fd, int *flashid, uint8_t *flashjedic, int flashjedic_maxsize, int cmd_ignore_jedec);
int PUMP_FlashErase(int pump_fd, int flashid);
int PUMP_FlashRead(int pump_fd, int flashid, int page, uint8_t *buf);
int PUMP_FlashWrite(int pump_fd, int flashid, int page, uint8_t *buf);

#endif // _OLS_H_

