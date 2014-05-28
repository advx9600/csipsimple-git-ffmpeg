#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "a8_4_0_decoder.h"
#include "../udpsend/udp_send.h"
#include "../filesave/filesave.h"
#include "../serial/my_serial.h"

#define INPUT_BUFFER_SIZE   (204800)

//#define DECODE_SEND_UDP 
//	#define READ_SERIAL_SEND_YUV  // before this DECODE_SEND_UDP must defined

//#define SAVE_DECODE_FILE 

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

static volatile int gtag=0;

static volatile int gstop =0;

static void* loop_read_serial_data(void* arg)
{
	create_my_serial("/dev/ttySAC2");
	while(gtag)
	{
		if (poll_my_serial(1000)){
		gstop = !gstop;
		}
	}
	close_my_serial();
	return NULL;
}

static void print_yuv_num(char* buf,int w,int h,int frame_at)
{
	int i,j;
	const int interval_line=30;

	memset(buf,0x0,w*h);
        for (i=0;i<h;i++)
        for (j=0;j<w;j++)
        {
                if(j/interval_line < frame_at  \
                        && j%interval_line==interval_line-1)
                        buf[i*w+j]=0xff;
        }
}

static int gframeat=0;

int decoder_init(A8MfcParam* param) {
	void* pPhyStrmBuf;
	gtag=1;
	gstop =0;

	#ifdef DECODE_SEND_UDP
	create_udp_socket(0);
		#ifdef READ_SERIAL_SEND_YUV
		pthread_t tid;
		pthread_create(&tid,NULL,loop_read_serial_data,NULL);
		#endif
	#endif

	#ifdef SAVE_DECODE_FILE 
	open_file("/sdcard/data/s5p_data.yuv");
	#endif
	gframeat=0;
	SSBIP_MFC_BUFFER_TYPE bufType = NO_CACHE;
	param->initialized = 0;
	param->mfcHandle = (_MFCLIB*) SsbSipMfcDecOpen((void*) &bufType);
	if (param->mfcHandle == NULL) {
		LOGE("SsbSipMfcDecOpen failed!");
		return -1;
	}
	param->mfcStrmBuf = SsbSipMfcDecGetInBuf(param->mfcHandle, &pPhyStrmBuf,
	INPUT_BUFFER_SIZE);
	if (param->mfcStrmBuf == NULL) {
		LOGE("SsbSipMfcDecGetInBuf failed!\n");
		return -1;
	}

	param->outputY = malloc(2048 * 2048*3/2);
	param->swapBuf = malloc(2048 * 2048 / 2);

	if (param->outputY == NULL || param->swapBuf == NULL) {
		LOGE("malloc yuv data failed!");
		return -1;
	}
	return 0;
}

int decoder_exe(A8MfcParam* param, char* buf, int len) {
	int i, ret;
	SSBSIP_MFC_DEC_OUTBUF_STATUS output_status;
	int isSpspps = 0;

#if 0
	LOGI("len:%d",len);
	if (len > 10){
	LOGI("front 5 data");
	LOGI("%02x,%02x,%02x,%02x,%02x",\
	buf[0]&0xff,buf[1]&0xff,buf[2]&0xff,buf[3]&0xff,buf[4]&0xff);
	LOGI("last 5 data");
	LOGI("%02x,%02x,%02x,%02x,%02x",\
	buf[len-5]&0xff,buf[len-4]&0xff,\
	buf[len-3]&0xff,buf[len-2]&0xff,buf[len-1]&0xff);
	}
#endif

#if 1
	if (!param->initialized) {
		for (i = 3; i < len && i < 5; i++) {
			if ((buf[i - 3] & 0xff) == 0 && (buf[i - 2] & 0xff) == 0
					&& (buf[i - 1] & 0xff) == 1
					&& ((buf[i] & 0xff) == 0x67 || (buf[i] & 0xff) == 0x27)) {
				isSpspps = 1;
				break;
			}
		}
		if (!isSpspps) {
			LOGW("decode 264 header not spspps data");
			return 0;
		}
	}
#endif

	memcpy(param->mfcStrmBuf, buf, len);
//	LOGI("decode size:%d",len);
//	if (gframeat >5) return 0;

	if (!param->initialized) {
		#if 0
		int nulheadcount=0;
		for (i=3;i<len;i++)
		{
			if ((buf[i-3]&0xff)==0x0 && (buf[i-2]&0xff)==0 && \
				(buf[i-1]&0xff)==0x0 &&(buf[i-0]&0xff)==0x1){
				nulheadcount++;
				if (nulheadcount==3){
				len=i-3;
				break;
				}
			}
		}
		#endif
		ret = SsbSipMfcDecInit(param->mfcHandle, param->codecType, len);

		if (ret != MFC_RET_OK) {
			LOGE("SsbSipMfcDecInit failed!\n");
			return -1;
		} else {
			param->initialized = 1;
			output_status = SsbSipMfcDecGetOutBuf(param->mfcHandle,
					&param->output_info);
			LOGV("out_status is 0x%x\n", output_status);
			LOGV("width:%d,height:%d\n", param->output_info.img_width,
					param->output_info.img_height);
			param->outputUV = (void*)( (int)param->outputY
					+ param->output_info.img_width
							* param->output_info.img_height );
			param->outputU = param->outputUV;
			param->outputV =(void*)( (int)param->outputUV
					+ param->output_info.img_width
							* param->output_info.img_height / 4);
			return 0;
		}
	}

	ret = SsbSipMfcDecExe(param->mfcHandle, len);
	if (ret != MFC_RET_OK) {
		LOGE("SsbSipMfcDecExe failed!%d\n", ret);
		goto failed;
	}
	memset(&param->output_info, 0, sizeof(SSBSIP_MFC_DEC_OUTPUT_INFO));
	if (SsbSipMfcDecGetOutBuf(param->mfcHandle, &param->output_info)
			!= MFC_GETOUTBUF_DISPLAY_DECODING) {
		LOGW(
				"SsbSipMfcDecGetOutBuf(mfcHandle, &output_info)!= MFC_GETOUTBUF_DISPLAY_DECODING");
		return 0;
	}
	#if 1
	csc_tiled_to_linear(param->outputY, param->output_info.YVirAddr,
			param->output_info.img_width, param->output_info.img_height);
	csc_tiled_to_linear_deinterleave(param->outputU, param->outputV,
			param->output_info.CVirAddr, param->output_info.img_width,
			param->output_info.img_height >> 1);

	#endif
	gframeat++;

	#ifdef SAVE_DECODE_FILE 
	write_file(param->outputY,param->output_info.img_width*param->output_info.img_height*3/2);
	#endif

	#ifdef DECODE_SEND_UDP
		#ifdef READ_SERIAL_SEND_YUV
		if (gstop){
		int i;
		int w=param->output_info.img_width,h=param->output_info.img_height;
		for (i=0;i<w*h*3/2;i+=1500)
		{
			int len = i+1500 >w*h*3/2 ? w*h*3/2%1500:1500;
			send_udp_data(param->outputY+i,len,"10.1.0.15",3303);
			usleep(1000*1);
		}
		memset(param->outputY,0,w*h*3/2);
		while(gstop) usleep(10000);
		}
		#endif
	send_udp_data(buf ,len,"10.1.0.14",3303);
	#endif


	#if 0
	memset(param->outputUV,0,param->output_info.img_width * \
		param->output_info.img_height/2);
	print_yuv_num(param->outputY,param->output_info.img_width,\
		param->output_info.img_height,gframeat%10);
	LOGI("frameat:%d",gframeat%10);
	#endif

	return param->output_info.img_width * param->output_info.img_height * 3 / 2;

	failed: return -1;
}

void decoder_close(A8MfcParam* param) {
	gtag =0;
	if (param->mfcHandle != NULL)
		SsbSipMfcDecClose(param->mfcHandle);
	if (param->outputY != NULL)
		free(param->outputY);
	if (param->swapBuf != NULL)
		free(param->swapBuf);
	#ifdef DECODE_SEND_UDP
	close_udp_socket();
	#endif

	#ifdef SAVE_DECODE_FILE 
	 close_file();
	#endif

}
