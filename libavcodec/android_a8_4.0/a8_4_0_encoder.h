#ifndef _A8_4_0_ENCODER_H_
#define _A8_4_0_ENCODER_H_
#include <stdio.h>
#include <stdlib.h>
#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

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

typedef struct a8MfcEncParam {
	_MFCLIB* mfcHandle;
	int width;
	int height;
	SSBSIP_MFC_CODEC_TYPE codecType;
	union{
		int paramAddr;
		SSBSIP_MFC_ENC_H264_PARAM param264;
		SSBSIP_MFC_ENC_H263_PARAM param263;
	};

	void* headerData;
	int headerSize;

	SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo;
	SSBSIP_MFC_ENC_INPUT_INFO inputInfo;
	
	void* data;
	int dataSize;

	void* outputFrame; // for AVFRAME

	void* inputData;
} A8MfcEncParam;

int encoder_init(A8MfcEncParam* param);

int encoder_exe(A8MfcEncParam* param, char* buf);

void encoder_close(A8MfcEncParam* param);

#endif
