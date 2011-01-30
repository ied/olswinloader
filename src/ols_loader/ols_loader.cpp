// ols_loader.cpp : Defines the entry point for the console application.
//
#ifdef WIN32
#include "stdafx.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#include "../ols.h"
#include "../serial.h"
#include "../data_file.h"

#define VERSION "v0.4"

#undef NO_PUMP
//#define NO_PUMP

uint8_t *bin_buf;
uint32_t bin_buf_size;

int verbose = 0;
int cmd_read = 0;
int cmd_erase = 0;
int cmd_write = 0;
int cmd_status = 0;
int cmd_boot = 0;
int cmd_run = 0;
int cmd_ignore_jedec = 0;
int cmd_selftest = 0;

int param_limit = -1;
unsigned long param_speed = 921600;
const char *param_port = NULL;
const char *param_read_hex = NULL;
const char *param_read_bin = NULL;
const char *param_write_hex = NULL;
const char *param_write_bin = NULL;

int PUMP_FlashID = -1;


int parseCommandLine(int argc, char** argv)
{
	int i = 0;

  int show_help=1;
	for (i=1; i<argc; i++)
	{
    show_help = 0;
		if ( !strncmp(argv[i], "-p", 2) ) 
      param_port = argv[i] + 3;
    else if ( !strnicmp(argv[i], "-l", 2) ) 
      param_limit = atoi(argv[i] + 3);
    else if ( !strnicmp(argv[i], "-t", 2) ) 
      param_speed = atol(argv[i] + 3);
    else if ( !strnicmp(argv[i], "-rH", 3) )
      param_read_hex = argv[i] + 4;
    else if ( !strnicmp(argv[i], "-rB", 3) )
      param_read_bin = argv[i] + 4;
    else if ( !strnicmp(argv[i], "-wH", 3) ) {
			if (param_write_hex != NULL) {
				printf("ERROR: Already specified HEX filename.\n");
        exit(-1);
      }
			if (param_write_bin != NULL) {
				printf("ERROR: Can write -either- HEX or BIN.  Not both.\n");
				exit(-1);
			}
			param_write_hex = argv[i] + 4;
		} 
    else if ( !strnicmp(argv[i], "-wB", 3) ) {
			if (param_write_bin != NULL) {
				printf("ERROR: Already specified BIN filename.\n");
        exit(-1);
      }
			if (param_write_hex != NULL) {
				printf("ERROR: Can write -either- HEX or BIN.  Not both.\n");
				exit(-1);
			}
			param_write_bin = argv[i] + 4;
		} 
    else if ( !stricmp(argv[i], "-verbose") ) 
      verbose = 1;
		else if ( !stricmp(argv[i], "-read") ) 
      cmd_read = 1;
		else if ( !stricmp(argv[i], "-erase") ) 
      cmd_erase = 1;
		else if ( !stricmp(argv[i], "-status") ) 
      cmd_status = 1;
		else if ( !stricmp(argv[i], "-write") ) 
      cmd_write = 1;
		else if ( !stricmp(argv[i], "-run") ) 
      cmd_run = 1;
		else if ( !stricmp(argv[i], "-boot") ) 
      cmd_boot = 1;
		else if ( !stricmp(argv[i], "-ignore_jedec") ) 
      cmd_ignore_jedec = 1;
    else if ( !stricmp(argv[i], "-selftest") ) 
      cmd_selftest = 1;
    else if ( !stricmp(argv[i], "--help") ) {
			show_help = 1;
			break;
		} 
    else {
			fprintf(stderr, "Unknown parameter '%s'\n", argv[i]);
			return -1;
		}
	}

	if (show_help) {
		printf("\nUsage:\n");
    printf("  %s [parameters] [commands]\n",argv[0]);

		printf("\nParameters:\n");
		printf("  -p:PORT   - port of Logic Sniffer, needs to be specified\n");
		printf("  -t:SPEED  - sets speed of the serial port\n");
		printf("  -wH:FILE  - HEX file to be uploaded to OLS\n");
		printf("  -wB:FILE  - BIN file to be uploaded to OLS\n");
		printf("  -rH:FILE  - HEX file to be downloaded from OLS\n");
		printf("  -rB:FILE  - BIN file to be downloaded from OLS\n");
		printf("  -l:X      - send only first X paged \n");

		printf("\nCommands:\n");
		printf("  -erase    - erases Flash\n");
		printf("  -write    - writes data to Flash\n");
		printf("  -read     - reads data from Flash\n");
		printf("  -run      - enter run mode after finished other commands\n");
		printf("  -status   - get OLS status\n");
		printf("  -boot     - enter bootloader mode - ignore other commands\n");
    printf("  -selftest - run self-test - ignore other commands\n");
		printf("  -ignore_jedec - ignore jedec id\n");

		printf("\nExamples:\n");
		printf("  Read flash from OLS on COM2 to HEX file 'OLS.hex':\n");
		printf("    ols-loader -p:COM2 -rH:OLS.hex -read\n\n");
		printf("  Erase and write flash to OLS on COM2, form data in BIN file 'OLS.bin':\n");
		printf("    ols-loader -p:COM2 -wB:OLS.hex -write -erase\n\n");
		printf("  Get status from OLS on COM2, and jump to run mode:\n");
		printf("    ols-loader -p:COM2 -run -status\n\n");
		printf("  If no command given, display FW version, and Flash ID\n");
		printf("    ols-loader -p:COM2\n");

		return 0;
	}

	return 1;
}



int main(int argc, char* argv[])
{
	int		dev_fd = -1, res = -1;
	uint32_t i;

  char m[32], d[32], y[32];
  sscanf (__DATE__,"%s %s %s",m,d,y);

  printf("Logic Sniffer ROM loader " VERSION " (%s %s, %s)\n",m,d,y);

	if ((res = parseCommandLine(argc, argv)) < 0) {
		return -1;
	} 
  else if ( res == 0 ) {
		return 0;
	}

	if( !param_port ) {
		fprintf(stderr, "Please specify serial port \n");
		return -1;
	}

	printf("Opening serial port '%s' @ %ld ... ", param_port, param_speed);
#ifndef NO_PUMP
	dev_fd = serial_open(param_port);
	if (dev_fd < 0) {
	    printf("Error opening :(\n");
			return -1;
	}

	if( serial_setup(dev_fd, param_speed) < 0 ) {
	    printf("Error configuring :(\n");
			return -1;
	}
#endif
	printf("OK\n");

#ifndef NO_PUMP
  uint8_t olsid[OLS_ID_INFOSIZE];
	int result = PUMP_GetID(dev_fd, olsid, sizeof(olsid));
  if (result) {
    printf("ERROR: Reading OLS ID failed!\n  %s\n", OLSResultErrorMsg(result));
    return -1;
  }
	printf("Found OLS HW: %d, FW: %d.%d, Boot: %d\n", olsid[1], olsid[3], olsid[4], olsid[6]);
#endif

#ifndef NO_PUMP
  const char *flashid_name = "Unknown Flash Type";
  uint8_t flashjedic[OLD_FLASHID_INFOSIZE];
  result = PUMP_GetFlashID(dev_fd, &PUMP_FlashID, flashjedic, sizeof(flashjedic), cmd_ignore_jedec);
  if (result) {
    printf("ERROR: Reading OLS Flash JEDIC ID failed!\n  %s\n", OLSResultErrorMsg(result));
		return -1;
	}
	if (PUMP_FlashID<0) {
		printf("Error - unknown flash type (%02x %02x %02x %02x)\n", flashjedic[0], flashjedic[1], flashjedic[2], flashjedic[3]);
		return -1;
	}
  else {
    flashid_name = PUMP_Flash[PUMP_FlashID].name;
    printf("Found flash: %s\n\n", flashid_name);
  }
#else
	PUMP_FlashID = 0;
#endif

	if (cmd_boot) {
#ifndef NO_PUMP
		int result = PUMP_EnterBootloader(dev_fd);
    if (result) {
      printf("ERROR! Switching OLS to bootloader mode failed!\n  %s\n", OLSResultErrorMsg(result));
      return -1;
    }
  	printf("OLS switched to bootloader mode\n");
#endif
		return 0;
	}

	// now the PUMP_FlashID contains offset into PUMP_Flash array
	bin_buf_size = PUMP_Flash[PUMP_FlashID].pages * PUMP_Flash[PUMP_FlashID].page_size * sizeof(uint8_t);
	bin_buf = (uint8_t*)malloc(bin_buf_size);

	if (bin_buf == NULL) {
		printf("Error allocating %ld bytes of memory\n", (long)bin_buf_size);
		return -1;
	}
	memset(bin_buf, 0, bin_buf_size);

	if (cmd_status) {
#ifndef NO_PUMP
    uint8_t status;
		int result = PUMP_GetStatus(dev_fd, &status);
    if (result) {
      printf("ERROR: Read status failed!\n  %s\n", OLSResultErrorMsg(result));
      return -1;
    }
  	printf("OLS status: %02x\n", status);
#endif
	}

	if (cmd_selftest){
    int result = PUMP_selftest(dev_fd);
    if (result) printf("ERROR: Self test failed!\n  %s\n", OLSResultErrorMsg(result));
    return -1;
	}

	if (cmd_erase) {
#ifndef NO_PUMP
    printf("Chip erase...\n");
		int result = PUMP_FlashErase(dev_fd, PUMP_FlashID);
    if (result) printf("ERROR: Flash erase failed!\n  %s\n", OLSResultErrorMsg(result));
#endif
	}

	if (cmd_write) {
		uint32_t pages = 0;
		uint32_t read_size;

		if (param_write_hex) {
			printf("Reading HEX file '%s' ... ", param_write_hex);
      int result = HEX_ReadFile(param_write_hex, bin_buf, bin_buf_size, &read_size);
      if (result) {
        printf("ERROR: Reading HEX file failed.\n  %s\n",
          FileResultErrorMsg(result, param_write_hex, flashid_name));
        return -1;
      }
		} 
    else if (param_write_bin) {
			printf("Reading BIN file '%s' ... ", param_write_bin);
			int result = BIN_ReadFile(param_write_bin, bin_buf, bin_buf_size, &read_size);
      if (result) {
        printf("ERROR: Reading BIN file failed.\n  %s\n",
          FileResultErrorMsg(result, param_write_bin, flashid_name));
        return -1;
      }
		} 
    else {
			printf("no input file specified ! \n");
			return -1;
		}

		if (read_size == 0) {
			printf("Error!\n");
			return -1;
		}

		printf("OK! (binary size = %ld)\n", (long int)read_size);

		if (param_limit <= 0) {
			// read_size / page_size .. round up when necessary
			pages = (read_size+PUMP_Flash[PUMP_FlashID].page_size-1)/PUMP_Flash[PUMP_FlashID].page_size;
		} else {
			pages = param_limit;
		}
		if (pages > PUMP_Flash[PUMP_FlashID].pages)
		   pages = PUMP_Flash[PUMP_FlashID].pages;

		printf("Writing flash chip...\n");

		for (i = 0; i < pages; i ++) {
#ifndef NO_PUMP
    	printf("\rPage %d of %d (%d%%)... ", i+1, pages, (100*(i+1))/pages);
			int result = PUMP_FlashWrite(dev_fd, PUMP_FlashID, i, bin_buf + (PUMP_Flash[PUMP_FlashID].page_size * i));
      if (result) {
        printf("ERROR: Writing flash page %d failed!\n  %s\n", i, OLSResultErrorMsg(result));
        return -1;
      }
#else
			printf("Writing from mem=0x%p to page=%ld\n", bin_buf + (PUMP_Flash[PUMP_FlashID].page_size * i), (long int)i);
#endif
		}
    printf ("\n");
	}

	if (cmd_read) {
		uint32_t pages;
		uint32_t temp_size;

		if (param_read_hex == NULL && param_read_bin == NULL) {
			printf("no output file specified ! \n");
			return -1;
		}
		if (param_limit <= 0) {
			pages = PUMP_Flash[PUMP_FlashID].pages;
		} else {
			pages = param_limit;
		}

		temp_size = pages * PUMP_Flash[PUMP_FlashID].page_size;

		printf("Reading flash chip...\n");

		for (i = 0; i < pages; i ++) {
#ifndef NO_PUMP
    	printf("\rPage %d of %d (%d%%)... ", i+1, pages, (100*(i+1))/pages);
			int result = PUMP_FlashRead(dev_fd, PUMP_FlashID, i, bin_buf + (PUMP_Flash[PUMP_FlashID].page_size * i));
      if (result) {
        printf("\nERROR: Reading flash page %d failed!\n  %s\n", i, OLSResultErrorMsg(result));
        return -1;
      }
#else
			printf("\nReading from page=%ld to mem=0x%p\n", (long int)i, bin_buf + (PUMP_Flash[PUMP_FlashID].page_size * i));
#endif
		}
    printf ("\n\n");

		if (param_read_hex) {
			printf("Writing HEX file: %s\n", param_read_hex);
			HEX_WriteFile(param_read_hex, bin_buf, temp_size);
			printf("Done!\n");
		}
		if (param_read_bin) {
			printf("Writing BIN file: %s\n", param_read_bin);
			BIN_WriteFile(param_read_bin, bin_buf, temp_size);
			printf("Done!\n");
		}
	}


	if (cmd_run) {
#ifndef NO_PUMP
		int result = PUMP_EnterRunMode(dev_fd);
    if (result) {
      printf("ERROR: Unable to switch to RUN mode!\n  %s\n",OLSResultErrorMsg(result));
      return -1;
    }
  	printf("OLS switched to RUN mode\n");
#endif
	}

	free(bin_buf);
	return 0;
}

