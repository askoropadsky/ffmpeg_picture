/*
 * ChromaKeyRenderer.cpp
 *
 *  Created on: Sep 11, 2013
 *      Author: askoropadsky
 */

#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <wchar.h>

#include "ChromaKeyRenderer.h"

#define LOG_TAG "NativeLib"

#define LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define AV_INVALID_STREAM_INDEX -1

ChromaKeyRenderer::ChromaKeyRenderer() {
	jvm = NULL;
	window = NULL;

	pDecodedFrame = NULL;
	pFrameRGBA = NULL;

	pFormatContext = NULL;
	pCodecContext = NULL;
	pCodec = NULL;
	swsContext = NULL;
	buffer = NULL;

	videoStreamIndex = AV_INVALID_STREAM_INDEX;
	width = 0;
	height = 0;

	stop = 0;
	fileIsPrepared = false;
	chromaKeyIsEnabled = false;
	isPlaying = false;

	av_register_all();
}

ChromaKeyRenderer::~ChromaKeyRenderer() {
}

bool ChromaKeyRenderer::prepare(const char* path)
{
	if(NULL == path)
	{
		LOGD(LOG_TAG, "File path is NULL");
		return false;
	}

	LOGD(LOG_TAG, "File path is: %s", path);

	pFormatContext = NULL;
	avformat_open_input(&pFormatContext, path, NULL, NULL);
	if(NULL == pFormatContext)
	{
		LOGE(LOG_TAG, "Can't avformat_open_input");
		return false;
	}
	LOGD(LOG_TAG, "File %s is opened.", path);

	if(av_find_stream_info(pFormatContext) < 0)
	{
		LOGE(LOG_TAG, "Can't find streams in the file %s", path);
		avformat_close_input(&pFormatContext);
		return false;
	}

	LOGD(LOG_TAG, "Number of found streams is %d", pFormatContext->nb_streams);

	int i;
	videoStreamIndex = -1;
	for(i = 0 ; i < pFormatContext->nb_streams ; i++)
	{
		if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = i;
			break;
		}
	}

	if(-1 == videoStreamIndex)
	{
		LOGE(LOG_TAG, "Can't find video stream in the file %s", path);
		avformat_close_input(&pFormatContext);
		return false;
	}
	LOGD(LOG_TAG, "Video stream is #%d", videoStreamIndex);

	pCodecContext = pFormatContext->streams[videoStreamIndex]->codec;

	pCodec = avcodec_find_decoder(pCodecContext->codec_id);

	if(NULL == pCodec)
	{
		LOGE(LOG_TAG, "Can't find codec.");
		avformat_close_input(&pFormatContext);
		return false;
	}

	int res = avcodec_open2(pCodecContext, pCodec, NULL);
	if(res < 0)
	{
		LOGE(LOG_TAG, "Can't open codec.");
		avformat_close_input(&pFormatContext);
		return false;
	}

	pDecodedFrame = avcodec_alloc_frame();
	pFrameRGBA = avcodec_alloc_frame();

	if(NULL == pDecodedFrame || NULL == pFrameRGBA)
	{
		LOGE(LOG_TAG, "Can't allocate frames.");
		avcodec_close(pCodecContext);
		avformat_close_input(&pFormatContext);
		return false;
	}

	fileIsPrepared = true;
	return true;
}

