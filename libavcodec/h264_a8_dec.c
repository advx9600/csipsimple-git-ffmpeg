/*
 * Copyright (c) 2012, Xidorn Quan
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

/**
 * @file
 * H.264 decoder via VDA
 * @author Xidorn Quan <quanxunzhen@gmail.com>
 */

#include <string.h>

#include "h264.h"
#include "avcodec.h"


#include "android_a8_4.0/a8_4_0_decoder.h"


//#define INPUT_BUFFER_SIZE   (204800)


#define YUVMALLOCSIZE (1024*1024*3)

static int vdadec_decode(AVCodecContext *avctx,
        void *data, int *got_frame, AVPacket *avpkt)
{
	A8MfcParam* param=avctx->priv_data;
	AVFrame *pic = data;
	int decodelen =0;

	decodelen=decoder_exe(param,avpkt->data,avpkt->size);
	if (decodelen > 0){
		avctx->codec_type=AVMEDIA_TYPE_VIDEO;
		avctx->codec_id = CODEC_ID_H264;
		avctx->width =param->output_info.img_width;
		avctx->height =param->output_info.img_height;
		avctx->coded_width=param->output_info.img_width;
		avctx->coded_height=param->output_info.img_height;
		avctx->pix_fmt=PIX_FMT_YUV420P;
		avctx->frame_size=avctx->coded_width*avctx->coded_height*3/2;
		avctx->coded_frame=pic;

		pic->width = param->output_info.img_width;
		pic->height = param->output_info.img_height;
		pic->linesize[0]=pic->width;
		pic->linesize[1]=pic->linesize[2]=pic->width/2;

		pic->format=PIX_FMT_YUV420P;
		pic->key_frame=0;
		pic->data[0]=param->outputY;
		pic->data[1]=param->outputU;
		pic->data[2]=param->outputV;

		*got_frame =1;
	}else *got_frame =0;
	return avpkt->size;
}

static av_cold int vdadec_close(AVCodecContext *avctx)
{
		A8MfcParam* param=avctx->priv_data;
		decoder_close(param);
    return 0;
}

static av_cold int vdadec_init(AVCodecContext *avctx)
{
	A8MfcParam* param=avctx->priv_data;
	param->codecType = H264_DEC;
	return decoder_init(param);
}

static void vdadec_flush(AVCodecContext *avctx)
{
}

AVCodec ff_h264_a8_decoder = {
    .name           = "h264 a8 hardware decoder",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H264,
    .priv_data_size = sizeof(A8MfcParam),
    .init           = vdadec_init,
    .close          = vdadec_close,
    .decode         = vdadec_decode,
    .capabilities   = CODEC_CAP_DELAY,
    .flush          = vdadec_flush,
    .long_name      = NULL_IF_CONFIG_SMALL("H.264 decoder only"),
};
