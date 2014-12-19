#include "a8_4_0_encoder.h"
#include "../udpsend/udp_send.h"

//#define UDP_SEND_DATA

int gFrameCount=0;

int encoder_init(A8MfcEncParam* param) {
	int ret;

	#ifdef UDP_SEND_DATA
	create_udp_socket(0);
	#endif

	param->headerData = av_malloc(1024 * 1024);
	if (param->headerData == NULL) {
		LOGE("malloc headerData failed!");
		return -1;
	}
	param->inputData = av_malloc(1024 * 1024 * 3);
	if (param->inputData == NULL) {
		LOGE("malloc inputData failed");
		return -1;
	}
	switch (param->codecType) {
	case H264_ENC:
		memset(&param->param264, 0, sizeof(SSBSIP_MFC_ENC_H264_PARAM));
		param->param264.codecType = H264_ENC;
		param->param264.SourceWidth = param->width;
		param->param264.SourceHeight = param->height;
		param->param264.ProfileIDC = 2;
		param->param264.LevelIDC = 31;
		param->param264.IDRPeriod = 3; // gop
		param->param264.NumberReferenceFrames = 1;
		param->param264.NumberRefForPframes = 1;
		param->param264.SliceMode = 0;
		param->param264.SliceArgument = 0;
		param->param264.NumberBFrames = 0;
		param->param264.LoopFilterDisable = 1;
		param->param264.LoopFilterAlphaC0Offset = 0;
		param->param264.LoopFilterBetaOffset = 0;
		param->param264.SymbolMode = 0;
		param->param264.PictureInterlace = 0;
		param->param264.Transform8x8Mode = 0;
		param->param264.RandomIntraMBRefresh = 0;
		param->param264.PadControlOn = 0;
		param->param264.LumaPadVal = 0;
		param->param264.CbPadVal = 0;
		param->param264.CrPadVal = 0;
		param->param264.EnableFRMRateControl = 1;
		param->param264.EnableMBRateControl = 0;
		param->param264.FrameRate = 25;
		param->param264.Bitrate = 1000;
		param->param264.FrameQp = 20;
		param->param264.QSCodeMax = 30;
		param->param264.QSCodeMin = 10;
		param->param264.CBRPeriodRf = 100;
		param->param264.DarkDisable = 1;
		param->param264.SmoothDisable = 1;
		param->param264.StaticDisable = 1;
		param->param264.ActivityDisable = 1;
		param->param264.FrameMap = 3;
		break;
	}
	int bufType=NO_CACHE;
	param->mfcHandle = (_MFCLIB*) SsbSipMfcEncOpen(&bufType);
	if (param->mfcHandle == NULL) {
		LOGE("SsbSipMfcEncOpen failed!");
		return -1;
	}
	if (SsbSipMfcEncInit(param->mfcHandle, &param->paramAddr) != MFC_RET_OK) {
		LOGE("SsbSipMfcEncInit failed!");
		return -1;
	}
	if (SsbSipMfcEncSetSize(param->mfcHandle, param->codecType, param->width,
			param->height)!=MFC_RET_OK) {
		LOGE("SsbSipMfcEncSetSize failed!");
		return -1;
	}
	// get sps ssp information
	memset(&param->outputInfo, 0, sizeof(SSBSIP_MFC_ENC_OUTPUT_INFO));
	if ((ret = SsbSipMfcEncGetOutBuf(param->mfcHandle, &param->outputInfo))
			!= MFC_RET_OK) {
		LOGE("SsbSipMfcEncGetOutBuf failed:%d", ret);
		return -1;
	};
	LOGI("encode width:%d,height:%d,spsssp length:%d", param->width,
			param->height, param->outputInfo.headerSize);

	param->headerSize = param->outputInfo.headerSize;
	memcpy(param->headerData, param->outputInfo.StrmVirAddr, param->headerSize);

	#ifdef UDP_SEND_DATA
        send_udp_data(param->headerData,param->headerSize,"10.1.0.14",3303);
        #endif

	if (SsbSipMfcEncGetInBuf(param->mfcHandle, &param->inputInfo)
			!= MFC_RET_OK) {
		LOGE("SsbSipMfcEncGetInBuf failed!");
		return -1;
	}

	return 0;
}

int encoder_exe(A8MfcEncParam* param, char* buf) {
	int i, ret;
	char* p_nv12, *p_cb, *p_cr;

	#if 0
	gFrameCount++;
	if (gFrameCount%10 <8) return 0;
	#endif
	if (buf == NULL)
		buf = param->inputData;
	else
		memcpy(param->inputData, buf, param->width * param->height);

	memcpy(param->inputInfo.YVirAddr, buf, param->width * param->height);

	p_nv12 = param->inputInfo.CVirAddr;
	p_cb = param->inputData + param->width * param->height;
	p_cr = param->inputData + param->width * param->height * 5 / 4;
	for (i = 0; i < param->width * param->height >> 2; i++) {
		*p_nv12 = *p_cb;
		p_nv12++;
		*p_nv12 = *p_cr;
		p_nv12++;
		p_cb++;
		p_cr++;
	}

	if (SsbSipMfcEncExe(param->mfcHandle) != MFC_RET_OK) { // exe encode
		LOGE("SsbSipMfcEncExe failed!");
		return -1;
	}
	if ((ret = SsbSipMfcEncGetOutBuf(param->mfcHandle, &param->outputInfo))
			!= MFC_RET_OK) {
		LOGE("SsbSipMfcEncGetOutBuf failed:%d", ret);
		return -1;
	};
	//LOGI("encode size:%d",param->outputInfo.dataSize);
	param->data = param->outputInfo.StrmVirAddr;
	param->dataSize = param->outputInfo.dataSize;

	#ifdef UDP_SEND_DATA
	send_udp_data(param->data,param->dataSize,"10.1.0.14",3303);
	#endif

	return param->dataSize;
}

void encoder_close(A8MfcEncParam* param) {
	if (param->headerData != NULL)
		av_free(param->headerData);
	if (param->inputData != NULL)
		av_free(param->inputData);
	SsbSipMfcEncClose(param->mfcHandle);
	#ifdef UDP_SEND_DATA
	close_udp_socket();
	#endif
}

