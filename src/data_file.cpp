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
 *
 */

#include "stdafx.h"
#include "data_file.h"


/*
 * returns error message
 */
TCHAR *FileResultErrorMsg (int errcode, const char *targetfile, const char *romtype)
{
  static TCHAR temp[1024];
	switch (errcode) {
    case FILERESULT_SUCCESS :
      return _T("No error.");

    case FILERESULT_ERROR_NEEDFILENAME : 
      return _T("Filename not specified.");

    case FILERESULT_ERROR_FILENOTFOUND : 
      wsprintf (temp,_T("File '%s' not found."),targetfile);
      return temp;

    case FILERESULT_ERROR_CANNOTOPENFILE : 
      wsprintf (temp,_T("File '%s' cannot be opened."),targetfile);
      return temp;

    case FILERESULT_ERROR_FILETOOBIG :
      wsprintf (temp,_T("File too large for FPGA flash ROM."));
      return temp;

    case FILERESULT_ERROR_READERROR :
      return _T("Unable to read file.");

    case FILERESULT_ERROR_WRITEERROR :
      return _T("Unable to write file.");

    case FILERESULT_ERROR_BADFILEFORMAT :
      return _T("Unknown or bad file format detected.");
  }
  wsprintf (temp,"Unknown error code %d.",errcode);
  return temp;
}


/*
 * returns checksum of input buffer
 */
uint8_t Data_Checksum(uint8_t *buf, uint16_t size) 
{
	uint16_t i;
	uint8_t sum = 0;

	for (i=0; i<size; i++) {
		sum -= buf[i];
	}
	return sum;
}


/*
 * reads hex file
 *   file - name of hexfile
 *   buf - buffer where the data should be written to
 *   size - size of buffer
 */
int HEX_ReadFile(const char *targetfile, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes) 
{
	char raw_line[256];
	int line = 0;
	uint32_t base_addr;
	FILE *fp;
	uint32_t addr_max=0;

  if (read_bytes) *read_bytes=0;

  if (targetfile==NULL) return FILERESULT_ERROR_NEEDFILENAME;

	fp = fopen(targetfile, "r");
	if (fp == NULL) return FILERESULT_ERROR_FILENOTFOUND;

	while(fgets(raw_line, sizeof(raw_line), fp) != 0) {
		// read line header
		unsigned int tmp[3];
		uint8_t byte_count;
		uint32_t addr;
		uint8_t rec_type;

		if (raw_line[0] != ':') {
      fclose (fp);
      return FILERESULT_ERROR_BADFILEFORMAT;
		}
		line ++;

		sscanf(raw_line+1, "%2x%4x%2x", &tmp[0], &tmp[1], &tmp[2]);
		byte_count = tmp[0] & 0xff;
		addr = tmp[1] & 0xffff;
		rec_type = tmp[2] & 0xff;

		if (rec_type == 0x00) {
			// data record
			uint8_t chksum;
			uint16_t i;
			addr = base_addr | addr;

			if (out_buf_size < byte_count + addr) {
				fclose(fp);
				return FILERESULT_ERROR_FILETOOBIG;
			}

			for (i=0; i<byte_count; i++) {
				uint8_t byte;
				sscanf(raw_line+9+2*i, "%2x", &tmp[0]);
				byte = tmp[0];
				out_buf[addr+i] = byte;
				if (addr_max < addr+i) addr_max = addr+i;
			}
			sscanf(raw_line+9+2*i, "%2x", &tmp[0]);
			chksum = tmp[0];
			// TODO: check chksum
		} 
    else if (rec_type == 0x04) {
			// base addr
			sscanf(raw_line+9, "%4x", &base_addr);
			base_addr <<= 16;
		} 
    else if (rec_type == 0x01) {
			// end record
			break;
		} 
    else {
      fclose (fp);
      return FILERESULT_ERROR_BADFILEFORMAT;
		}
	}

	fclose(fp);
  if (read_bytes) *read_bytes=addr_max+1;
	return FILERESULT_SUCCESS;
}

/*
 * writes hex file
 * file - name of hexfile
 * buf - buffer where data is stored
 * size - size of buffer
 */
int HEX_WriteRec(FILE *fp, uint8_t rec_id, uint8_t byte_count, uint16_t addr, uint8_t *data, char *term) 
{
	int res;
	uint8_t bin_line[128];
	char raw_line[256];
	uint8_t i=0;
	uint8_t j;

  if (fp==NULL) return FILERESULT_ERROR_WRITEERROR;

//	uint8_t checksum = 0;

	// fill header
	bin_line[0] = byte_count;
	bin_line[1] = (addr >> 8) & 0xff;
	bin_line[2] = addr & 0xff;
	bin_line[3] = rec_id;

	// copy data
	for (i=0; i<byte_count; i++) {
		bin_line[4+i] = data[i];
	}

	// add checksum
	bin_line[4+i] = Data_Checksum(bin_line, 4+i);

	// rewrite bin data to hex
	res = wsprintf(raw_line, ":");
	for (j=0; j<4+i+1; j++) {
		res += wsprintf(raw_line+res, "%02X", bin_line[j]);
	}
	res += wsprintf(raw_line+res, term);

	// output to file
	int written = fwrite(raw_line, sizeof(char), res, fp);
  return (written==res) ? FILERESULT_SUCCESS : FILERESULT_ERROR_WRITEERROR;
}



/*
 * writes hex file
 * file - name of hexfile
 * buf - buffer which contains the data
 * size - size of buffer
 */
int HEX_WriteFile(const char *targetfile, uint8_t *in_buf, uint32_t in_buf_size) 
{
	uint32_t written = 0;
	uint16_t base = 0x0000;
	uint32_t addr = 0x0000;
	FILE *fp;

  if (targetfile==NULL) return FILERESULT_ERROR_NEEDFILENAME;

	fp = fopen(targetfile, "wb"); // avoid newline fudging by windows
	if (fp == NULL) return FILERESULT_ERROR_CANNOTOPENFILE;

	// ext address record
	if (HEX_WriteRec(fp, 0x04, 2, 0x0000, (uint8_t*)(&base), "\n") != FILERESULT_SUCCESS) {
    fclose(fp);
    return FILERESULT_ERROR_WRITEERROR;
  }

	while (written < in_buf_size) {
		uint8_t byte_count;
		if ((in_buf_size - written) > 0x10) {
			byte_count = 0x10;
		} 
    else {
			byte_count = in_buf_size - written;
		}

		// write data record
		if (HEX_WriteRec(fp, 0x00, byte_count, addr, &in_buf[written], "\n") != FILERESULT_SUCCESS) {
      fclose(fp);
      return FILERESULT_ERROR_WRITEERROR;
    }

		written += byte_count;
		addr += byte_count;

		// write ext addr record
		if (addr & 0x10000) {
			uint8_t tmp[2];

			base ++;

			tmp[0] = (base >> 8) & 0xff;
			tmp[1] = (base & 0xff);

			if (HEX_WriteRec(fp, 0x04, 2, 0x0000, tmp, "\n") != FILERESULT_SUCCESS) {
        fclose(fp);
        return FILERESULT_ERROR_WRITEERROR;
      }

			addr -= 0x10000;
		}
	}

	// end record
	if (HEX_WriteRec(fp, 0x01, 0x00, 0x0000, NULL,"\r\n") != FILERESULT_SUCCESS) {
    fclose(fp);
    return FILERESULT_ERROR_WRITEERROR;
  }

	fclose(fp);
	return FILERESULT_SUCCESS;
}



/*
 * reads bin targetfile
 * targetfile - name of file
 * buf - buffer where the data should be written to
 * size - size of buffer, returns actual size read
 */
int BIN_ReadFile(const char *targetfile, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes) 
{
	int res;
	long fsize;
	FILE *fp;

  if (read_bytes) *read_bytes=0;

  if (targetfile==NULL) return FILERESULT_ERROR_NEEDFILENAME;
	fp = fopen(targetfile, "rb");
	if (fp == NULL) return FILERESULT_ERROR_FILENOTFOUND;

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fsize > (long)out_buf_size) {
    fclose (fp);
    return FILERESULT_ERROR_FILETOOBIG;
	}

	res = fread(out_buf, sizeof(uint8_t), fsize, fp);
	if (res <= 0) {
    fclose (fp);
    return FILERESULT_ERROR_READERROR;
	}

	fclose (fp);
  if (read_bytes) *read_bytes=res;

	return FILERESULT_SUCCESS;
}



/*
 * writes bin targetfile
 * targetfile - name of hexfile
 * buf - buffer which contains the data
 * size - size of buffer
 */
int BIN_WriteFile(const char *targetfile, uint8_t *out_buf, uint32_t out_buf_size) 
{
	FILE *fp;
	int res;

  if (targetfile==NULL) return FILERESULT_ERROR_NEEDFILENAME;
	fp = fopen(targetfile, "wb");
	if (fp == NULL) return FILERESULT_ERROR_CANNOTOPENFILE;

	res = fwrite(out_buf, sizeof(uint8_t), out_buf_size, fp);
	if (res != (int)out_buf_size) {
    fclose (fp);
    return FILERESULT_ERROR_WRITEERROR;
	}

	fclose(fp);
	return FILERESULT_SUCCESS;
}



/*
 * reads Xilinx bit target directly
 * targetfile - name of file
 * buf - buffer where the data should be written to
 * size - size of buffer, returns actual size read
 */
uint8_t g_bitfile_header[] = {0x00, 0x09, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00, 0x00, 0x01};
typedef struct {uint8_t id; uint16_t len;} blocktype;

int BIT_ReadFile(const char *targetfile, uint8_t *out_buf, uint32_t out_buf_size, uint32_t *read_bytes) 
{
  static uint8_t temp[4096];
	int res, i, blockid, blocklen;

  if (read_bytes) *read_bytes=0;
  if (targetfile==NULL) return FILERESULT_ERROR_NEEDFILENAME;

	FILE *fp = fopen(targetfile, "rb");
	if (fp == NULL) return FILERESULT_ERROR_FILENOTFOUND;

  // Read first 4K of file.   Should easily contain entire Xilinx bitfile header...
	res = fread(temp, sizeof(uint8_t), sizeof(temp), fp);
	if (res <= 0) {
    fclose (fp);
    return FILERESULT_ERROR_READERROR;
	}

  // Verify bitfile header...
  for (i=0; i<sizeof(g_bitfile_header); i++)
    if (temp[i] != g_bitfile_header[i]) {
      fclose (fp);
      return FILERESULT_ERROR_BADFILEFORMAT;
    }

  // Parse the bitfile header...
  uint32_t datalen=0;
  bool hdrdone=false;
  while (!hdrdone) {
    if ((i>=res-16)) {
      fclose (fp);
      return FILERESULT_ERROR_BADFILEFORMAT;
    }

    blockid = temp[i++];
    switch (blockid) {
      case 0x61 : // design name
      case 0x62 : // target device
      case 0x63 : // build date
      case 0x64 : // build time
        blocklen = (temp[i]<<8) | temp[i+1];
        i += (blocklen+2);
        break;

      case 0x65 : // 4-byte length (msb first)
        datalen = (temp[i]<<24) | (temp[i+1]<<16) | (temp[i+2]<<8) | temp[i+3];
        i += 4;

        if (datalen > out_buf_size) {
          fclose (fp);
          return FILERESULT_ERROR_FILETOOBIG;
        }

        if (datalen<8*1024) {
          fclose (fp);
          return FILERESULT_ERROR_BADFILEFORMAT;
        }

        hdrdone = true;
        break;

      default :
        fclose (fp);
        return FILERESULT_ERROR_BADFILEFORMAT;
    }
  }

  // Copy whatever remains in temp buffer into outbuf...
  if (i>res) {
    fclose (fp);
    return FILERESULT_ERROR_BADFILEFORMAT;
  }
  uint32_t templen=res-i;
  if (templen > datalen) templen = datalen;
  memcpy (out_buf, &temp[i], templen);

  // Get balance from file...
  if (datalen>templen) {
	  res = fread(&out_buf[templen], sizeof(uint8_t), datalen-templen, fp);
	  if (res <= 0) {
      fclose (fp);
      return FILERESULT_ERROR_READERROR;
	  }
    if (res != (int)(datalen-templen)) {
      fclose (fp);
      return FILERESULT_ERROR_READERROR;
    }
  }

	fclose (fp);
  if (read_bytes) *read_bytes = datalen;

	return FILERESULT_SUCCESS;
}

