/*
 * H.264 encoding using the x264 library
 * Copyright (C) 2005  Mans Rullgard <mans@mansr.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"
#include <x264.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "android_a8_4.0/a8_4_0_encoder.h"
const int x264_bit_depth = 8;

static void savefile(const char* filename,char* buf,int len)
{
  static FILE* fd = NULL;
	if (fd == NULL){
		fd=fopen(filename,"wb");
		if (fd == NULL) {
				LOGE("fopen %s failed!",filename);
				return ;
			}
	}
  if (fwrite(buf,1,len,fd)!=len){
    LOGE("fwrite file failed");
  } 
}

typedef struct X264Context {
    AVClass        *class;
    x264_param_t    params;
    x264_t         *enc;
    x264_picture_t  pic;
    uint8_t        *sei;
    int             sei_size;
    AVFrame         out_pic;
    char *preset;
    char *tune;
    char *profile;
    char *level;
    int fastfirstpass;
    char *wpredp;
    char *x264opts;
    float crf;
    float crf_max;
    int cqp;
    int aq_mode;
    float aq_strength;
    char *psy_rd;
    int psy;
    int rc_lookahead;
    int weightp;
    int weightb;
    int ssim;
    int intra_refresh;
    int b_bias;
    int b_pyramid;
    int mixed_refs;
    int dct8x8;
    int fast_pskip;
    int aud;
    int mbtree;
    char *deblock;
    float cplxblur;
    char *partitions;
    int direct_pred;
    int slice_max_size;
    char *stats;
} X264Context;


static int avfmt2_num_planes(int avfmt)
{
    switch (avfmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_YUVJ420P:
    case PIX_FMT_YUV420P9:
    case PIX_FMT_YUV420P10:
    case PIX_FMT_YUV444P:
        return 3;

    case PIX_FMT_BGR24:
    case PIX_FMT_RGB24:
        return 1;

    default:
        return 3;
    }
}


static int X264_frame(AVCodecContext *ctx, AVPacket *pkt, const AVFrame *frame,
                      int *got_packet)
{
	
	int i;
	A8MfcEncParam* param = ctx->priv_data;
	AVFrame* output=(AVFrame*)param->outputFrame;
	/* no delay */
	if (frame == NULL){
		*got_packet = 0;
		return 0;
	}
	for (i=0;i<param->height;i++)
	{
		memcpy(param->inputData+i*param->width,\
			frame->data[0]+i*frame->linesize[0],param->width);
	}
	
	for (i=0;i<param->height/2;i++)
	{
		memcpy(param->inputData+param->width*param->height+i*param->width/2,\
			frame->data[1]+i*frame->linesize[1],param->width/2);
		memcpy(param->inputData+param->width*param->height*5/4+i*param->width/2,\
			frame->data[2]+i*frame->linesize[2],param->width/2);
	}

//	LOGI("linesize[0]:%d,linesize[1]:%d,linesize[2]:%d",\
		frame->linesize[0],frame->linesize[1],frame->linesize[2]);
	//savefile("/sdcard/yuv.yuv",param->inputData,param->width*param->height*3/2);
	if (encoder_exe(param,NULL) <1){
		*got_packet =0;
		return -1;
	}
	
	if (param->outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME){
		memcpy(pkt->data,param->headerData,param->headerSize);
		memcpy(pkt->data+param->headerSize,param->data,param->dataSize);
		ctx->frame_size = param->dataSize+param->headerSize;
//		LOGI("this is I frame");
	}else{
		memcpy(pkt->data,param->data,param->dataSize);
		ctx->frame_size = param->dataSize;
	}

	*got_packet=1;
	pkt->size=ctx->frame_size;
	pkt->pts=AV_NOPTS_VALUE;
	pkt->dts=AV_NOPTS_VALUE;

	if (param->outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME){
		output->pict_type = AV_PICTURE_TYPE_I;
	}else if (param->outputInfo.frameType == MFC_FRAME_TYPE_P_FRAME){
		output->pict_type = AV_PICTURE_TYPE_P;
	}else if (param->outputInfo.frameType == MFC_FRAME_TYPE_B_FRAME){
		output->pict_type = AV_PICTURE_TYPE_B;
	}

//	LOGI("encode output adddr:0x%x,len:%d",pkt->data,ctx->frame_size);
	return 0;
}

static av_cold int X264_close(AVCodecContext *avctx)
{
		A8MfcEncParam* param = avctx->priv_data;
		encoder_close(param);
		if (param->outputFrame != NULL) av_free(param->outputFrame);
		if (avctx->extradata) av_freep(&avctx->extradata);
    return 0;
}


static av_cold int X264_init(AVCodecContext *avctx)
{
	int ret;
	A8MfcEncParam* param = avctx->priv_data;
	AVFrame* avFrame = avcodec_alloc_frame();

	avctx->coded_width = avctx->width;
	avctx->coded_height = avctx->height;
	avctx->codec_type=AVMEDIA_TYPE_VIDEO;
	avctx->codec_id = CODEC_ID_H264;

	param->width = avctx->width;
	param->height = avctx->height;
	param->codecType = H264_ENC;

	
	avctx->coded_frame=param->outputFrame=avFrame;
	avFrame->width=avctx->width;
	avFrame->height=avctx->height;
	avFrame->format=AV_SAMPLE_FMT_U8;

	if (avctx->pix_fmt != PIX_FMT_YUV420P){
		LOGE("not supported YUV format:%d",avctx->pix_fmt);
		return -1;
	}

	ret=encoder_init(param);

	avctx->extradata=av_malloc(param->headerSize);
	memcpy(avctx->extradata,param->headerData,param->headerSize);
	avctx->extradata_size = param->headerSize;
	avctx->log_level_offset =1;
	return ret;
}

static const enum PixelFormat pix_fmts_8bit[] = {
    PIX_FMT_YUV420P,
    PIX_FMT_YUVJ420P,
    PIX_FMT_YUV422P,
    PIX_FMT_YUV444P,
    PIX_FMT_NONE
};
static const enum PixelFormat pix_fmts_9bit[] = {
    PIX_FMT_YUV420P9,
    PIX_FMT_YUV444P9,
    PIX_FMT_NONE
};
static const enum PixelFormat pix_fmts_10bit[] = {
    PIX_FMT_YUV420P10,
    PIX_FMT_YUV422P10,
    PIX_FMT_YUV444P10,
    PIX_FMT_NONE
};
static const enum PixelFormat pix_fmts_8bit_rgb[] = {
#ifdef X264_CSP_BGR
    PIX_FMT_BGR24,
    PIX_FMT_RGB24,
#endif
    PIX_FMT_NONE
};

static av_cold void X264_init_static(AVCodec *codec)
{
    if (x264_bit_depth == 8)
        codec->pix_fmts = pix_fmts_8bit;
    else if (x264_bit_depth == 9)
        codec->pix_fmts = pix_fmts_9bit;
    else if (x264_bit_depth == 10)
        codec->pix_fmts = pix_fmts_10bit;
}

#define OFFSET(x) offsetof(X264Context, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
    { "preset",        "Set the encoding preset (cf. x264 --fullhelp)",   OFFSET(preset),        AV_OPT_TYPE_STRING, { .str = "medium" }, 0, 0, VE},
    { "tune",          "Tune the encoding params (cf. x264 --fullhelp)",  OFFSET(tune),          AV_OPT_TYPE_STRING, { 0 }, 0, 0, VE},
    { "profile",       "Set profile restrictions (cf. x264 --fullhelp) ", OFFSET(profile),       AV_OPT_TYPE_STRING, { 0 }, 0, 0, VE},
    { "fastfirstpass", "Use fast settings when encoding first pass",      OFFSET(fastfirstpass), AV_OPT_TYPE_INT,    { 1 }, 0, 1, VE},
    {"level", "Specify level (as defined by Annex A)", OFFSET(level), AV_OPT_TYPE_STRING, {.str=NULL}, 0, 0, VE},
    {"passlogfile", "Filename for 2 pass stats", OFFSET(stats), AV_OPT_TYPE_STRING, {.str=NULL}, 0, 0, VE},
    {"wpredp", "Weighted prediction for P-frames", OFFSET(wpredp), AV_OPT_TYPE_STRING, {.str=NULL}, 0, 0, VE},
    {"x264opts", "x264 options", OFFSET(x264opts), AV_OPT_TYPE_STRING, {.str=NULL}, 0, 0, VE},
    { "crf",           "Select the quality for constant quality mode",    OFFSET(crf),           AV_OPT_TYPE_FLOAT,  {-1 }, -1, FLT_MAX, VE },
    { "crf_max",       "In CRF mode, prevents VBV from lowering quality beyond this point.",OFFSET(crf_max), AV_OPT_TYPE_FLOAT, {-1 }, -1, FLT_MAX, VE },
    { "qp",            "Constant quantization parameter rate control method",OFFSET(cqp),        AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE },
    { "aq-mode",       "AQ method",                                       OFFSET(aq_mode),       AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE, "aq_mode"},
    { "none",          NULL,                              0, AV_OPT_TYPE_CONST, {X264_AQ_NONE},         INT_MIN, INT_MAX, VE, "aq_mode" },
    { "variance",      "Variance AQ (complexity mask)",   0, AV_OPT_TYPE_CONST, {X264_AQ_VARIANCE},     INT_MIN, INT_MAX, VE, "aq_mode" },
    { "autovariance",  "Auto-variance AQ (experimental)", 0, AV_OPT_TYPE_CONST, {X264_AQ_AUTOVARIANCE}, INT_MIN, INT_MAX, VE, "aq_mode" },
    { "aq-strength",   "AQ strength. Reduces blocking and blurring in flat and textured areas.", OFFSET(aq_strength), AV_OPT_TYPE_FLOAT, {-1}, -1, FLT_MAX, VE},
    { "psy",           "Use psychovisual optimizations.",                 OFFSET(psy),           AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE },
    { "psy-rd",        "Strength of psychovisual optimization, in <psy-rd>:<psy-trellis> format.", OFFSET(psy_rd), AV_OPT_TYPE_STRING,  {0 }, 0, 0, VE},
    { "rc-lookahead",  "Number of frames to look ahead for frametype and ratecontrol", OFFSET(rc_lookahead), AV_OPT_TYPE_INT, {-1 }, -1, INT_MAX, VE },
    { "weightb",       "Weighted prediction for B-frames.",               OFFSET(weightb),       AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE },
    { "weightp",       "Weighted prediction analysis method.",            OFFSET(weightp),       AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE, "weightp" },
    { "none",          NULL, 0, AV_OPT_TYPE_CONST, {X264_WEIGHTP_NONE},   INT_MIN, INT_MAX, VE, "weightp" },
    { "simple",        NULL, 0, AV_OPT_TYPE_CONST, {X264_WEIGHTP_SIMPLE}, INT_MIN, INT_MAX, VE, "weightp" },
    { "smart",         NULL, 0, AV_OPT_TYPE_CONST, {X264_WEIGHTP_SMART},  INT_MIN, INT_MAX, VE, "weightp" },
    { "ssim",          "Calculate and print SSIM stats.",                 OFFSET(ssim),          AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE },
    { "intra-refresh", "Use Periodic Intra Refresh instead of IDR frames.",OFFSET(intra_refresh),AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE },
    { "b-bias",        "Influences how often B-frames are used",          OFFSET(b_bias),        AV_OPT_TYPE_INT,    {INT_MIN}, INT_MIN, INT_MAX, VE },
    { "b-pyramid",     "Keep some B-frames as references.",               OFFSET(b_pyramid),     AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE, "b_pyramid" },
    { "none",          NULL,                                  0, AV_OPT_TYPE_CONST, {X264_B_PYRAMID_NONE},   INT_MIN, INT_MAX, VE, "b_pyramid" },
    { "strict",        "Strictly hierarchical pyramid",       0, AV_OPT_TYPE_CONST, {X264_B_PYRAMID_STRICT}, INT_MIN, INT_MAX, VE, "b_pyramid" },
    { "normal",        "Non-strict (not Blu-ray compatible)", 0, AV_OPT_TYPE_CONST, {X264_B_PYRAMID_NORMAL}, INT_MIN, INT_MAX, VE, "b_pyramid" },
    { "mixed-refs",    "One reference per partition, as opposed to one reference per macroblock", OFFSET(mixed_refs), AV_OPT_TYPE_INT, {-1}, -1, 1, VE },
    { "8x8dct",        "High profile 8x8 transform.",                     OFFSET(dct8x8),        AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE},
    { "fast-pskip",    NULL,                                              OFFSET(fast_pskip),    AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE},
    { "aud",           "Use access unit delimiters.",                     OFFSET(aud),           AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE},
    { "mbtree",        "Use macroblock tree ratecontrol.",                OFFSET(mbtree),        AV_OPT_TYPE_INT,    {-1 }, -1, 1, VE},
    { "deblock",       "Loop filter parameters, in <alpha:beta> form.",   OFFSET(deblock),       AV_OPT_TYPE_STRING, { 0 },  0, 0, VE},
    { "cplxblur",      "Reduce fluctuations in QP (before curve compression)", OFFSET(cplxblur), AV_OPT_TYPE_FLOAT,  {-1 }, -1, FLT_MAX, VE},
    { "partitions",    "A comma-separated list of partitions to consider. "
                       "Possible values: p8x8, p4x4, b8x8, i8x8, i4x4, none, all", OFFSET(partitions), AV_OPT_TYPE_STRING, { 0 }, 0, 0, VE},
    { "direct-pred",   "Direct MV prediction mode",                       OFFSET(direct_pred),   AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE, "direct-pred" },
    { "none",          NULL,      0,    AV_OPT_TYPE_CONST, { X264_DIRECT_PRED_NONE },     0, 0, VE, "direct-pred" },
    { "spatial",       NULL,      0,    AV_OPT_TYPE_CONST, { X264_DIRECT_PRED_SPATIAL },  0, 0, VE, "direct-pred" },
    { "temporal",      NULL,      0,    AV_OPT_TYPE_CONST, { X264_DIRECT_PRED_TEMPORAL }, 0, 0, VE, "direct-pred" },
    { "auto",          NULL,      0,    AV_OPT_TYPE_CONST, { X264_DIRECT_PRED_AUTO },     0, 0, VE, "direct-pred" },
    { "slice-max-size","Limit the size of each slice in bytes",           OFFSET(slice_max_size),AV_OPT_TYPE_INT,    {-1 }, -1, INT_MAX, VE },
    { "stats",         "Filename for 2 pass stats",                       OFFSET(stats),         AV_OPT_TYPE_STRING, { 0 },  0,       0, VE },
    { NULL },
};

static const AVClass class = {
    .class_name = "libxa8264_class",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};


AVCodec ff_libxa8264_encoder = {
    .name             = "libxa8264",
    .type             = AVMEDIA_TYPE_VIDEO,
    .id               = CODEC_ID_H264,
    .priv_data_size   = sizeof(X264Context),
    .init             = X264_init,
    .encode2          = X264_frame,
    .close            = X264_close,
    .capabilities     = CODEC_CAP_DELAY | CODEC_CAP_AUTO_THREADS,
    .long_name        = NULL_IF_CONFIG_SMALL("libx264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
    .priv_class       = &class,
    .init_static_data = X264_init_static,
};

