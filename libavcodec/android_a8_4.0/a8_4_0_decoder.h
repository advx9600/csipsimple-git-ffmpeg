#ifndef _A8_4_0_decoder_H_
#define _A8_4_0_decoder_H_
#include <stdio.h>
#include "mfc_interface.h"
#include "SsbSipMfcApi.h"
#include "color_space_convertor.h"

#if  0
// define in the mfc_interface.h 
#define LOGV myprintf
#define LOGI myprintf
#define LOGW myprintf
#define LOGE myprintf

//#define my_sprintf(buf,format,...) sprintf(buf,format,__VA_ARGS__)
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define myprintf(format,...) \
do { \
  char print_buf[1024]; \
  static int fd_print =-1;\
  sprintf(print_buf,format,##__VA_ARGS__); \
  if (fd_print < 0) fd_print = open("/dev/ttySAC2",O_RDWR); \
	if (fd_print < 0) fd_print = open("/dev/s3c2410_serial2",O_RDWR); \
  strcat(print_buf,"\n"); \
  write(fd_print,print_buf,strlen(print_buf)); \
}while(0)
#endif

typedef struct a8MfcParam {
	_MFCLIB* mfcHandle;
	SSBSIP_MFC_CODEC_TYPE codecType;
	void* mfcStrmBuf;
	int initialized;

	SSBSIP_MFC_DEC_OUTPUT_INFO output_info;
	void* outputY;
	void* outputUV;
	void* outputU;
	void* outputV;
	void* swapBuf;
} A8MfcParam;

int decoder_init(A8MfcParam* param);

int decoder_exe(A8MfcParam* param, char* buf, int len);

void decoder_close(A8MfcParam* param);

#endif
