/*
 * HEX/BIN/BIT Data file parser and writer
 *
 * 2010 - Michal Demin
 *
 * ----------------------------------------------------------------------------
 *
 * Modified Jan 8, 2011 - Ian Davis - Rewrote to return error codes instead of 
 *   reporting error conditions directly.  Added error result message handler.
 *   Added code for reading Xilinx BIT files.
 */

#ifndef DATA_FILE_H_
#define DATA_FILE_H_

#ifdef WIN32
#else
#include <stdint.h>
#endif

#define FILERESULT_SUCCESS 0
#define FILERESULT_ERROR_NEEDFILENAME -1   // Filename not specified.
#define FILERESULT_ERROR_FILENOTFOUND -2   // File '%s' not found.
#define FILERESULT_ERROR_CANNOTOPENFILE -3 // File '%s' not opened.  Read only already exists?
#define FILERESULT_ERROR_FILETOOBIG -4     // Data won't fit into buffer
#define FILERESULT_ERROR_READERROR -5      // Error while reading file.
#define FILERESULT_ERROR_WRITEERROR -6     // Error while writing file.
#define FILERESULT_ERROR_BADFILEFORMAT -7  // File '%s' is unknown format (ie: bad hex file).

char *FileResultErrorMsg (int errcode, const char *targetfile, const char *romtype);
uint8_t Data_Checksum(uint8_t *buf, uint16_t size);
int HEX_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes);
int HEX_WriteFile(const char *file, uint8_t *in_buf, uint32_t in_buf_size);
int BIN_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes);
int BIN_WriteFile(const char *file, uint8_t *in_buf, uint32_t in_buf_size);
int BIT_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes);


#endif

