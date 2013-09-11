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

#define LOG_TAG "ChromaKeyRenderer"

#define LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(LOG_TAG, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define AV_INVALID_STREAM_INDEX -1

void* ChromaKeyRenderer::sDecodeAndRender(void* pUserData)
{
	ChromaKeyRenderer* instance = (ChromaKeyRenderer*)pUserData;
	instance->decodeAndRender();
}

ChromaKeyRenderer::ChromaKeyRenderer(JavaVM* pJvm) {
	jvm = pJvm;
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

	keyColorLowRGB[0] = 0;
	keyColorLowRGB[1] = 0;
	keyColorLowRGB[2] = 0;

	keyColorHighRGB[0] = 5;
	keyColorHighRGB[1] = 5;
	keyColorHighRGB[2] = 5;

	stopRendering = false;
	fileIsPrepared = false;
	chromaKeyIsEnabled = false;
	isPlaying = false;

	av_register_all();
}

ChromaKeyRenderer::~ChromaKeyRenderer() {
// TODO: this should be called at the end of render thread
//	AndroidBitmap_unlockPixels(env, bitmap);
//	av_free(buffer);
//	av_free(pFrameRGBA);
//	av_free(pDecodedFrame);
//	avcodec_close(pCodecContext);
//	avformat_close_input(&pFormatContext);
}

bool ChromaKeyRenderer::prepare(const char* path)
{
	if(isPlaying)
	{
		LOGW(LOG_TAG, "Error. Can't prepare in playing state.");
		return false;
	}

	if(NULL == path)
	{
		LOGE(LOG_TAG, "File path is NULL");
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

void ChromaKeyRenderer::releaseFile()
{
	LOGD(LOG_TAG,"closeFile");
	if(!fileIsPrepared) return;
	av_free(pFrameRGBA);
	pFrameRGBA = NULL;
	LOGD(LOG_TAG,"pFrameARGB released");

	av_free(pDecodedFrame);
	pDecodedFrame = NULL;
	LOGD(LOG_TAG,"pFrame released");

	avcodec_close(pCodecContext);
	pCodecContext = NULL;
	LOGD(LOG_TAG,"pCodecContext released");

	avformat_close_input(&pFormatContext);
	pFormatContext = NULL;
	LOGD(LOG_TAG,"pFormatContext released");
}

void ChromaKeyRenderer::setSurface(JNIEnv* env, jobject surface)
{
	if(NULL != surface)
	{
		window = ANativeWindow_fromSurface(env, surface);
		ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);
		LOGD(LOG_TAG, "SetSurface OK");
	}
	else
	{
		LOGD(LOG_TAG, "SetSurface FAILED");
		ANativeWindow_release(window);
	}
}

void ChromaKeyRenderer::setVideoScalingFactor(JNIEnv* env, int _width, int _height)
{
	if(isPlaying)
	{
		LOGW(LOG_TAG,"Can't set videoScaling factor in playing state.");
		return;
	}
	width = _width;
	height = _height;

	ANativeWindow_Buffer windowBuffer;
	if(ANativeWindow_lock(window, &windowBuffer, NULL) >= 0)
	{
		LOGD(LOG_TAG, "passed W:H is %d:%d, windows W:H is %d:%d, stride = %d, format = %d", _width, _height, windowBuffer.width, windowBuffer.height, windowBuffer.stride, windowBuffer.format);
		if(width < windowBuffer.stride)	{ width = windowBuffer.stride; }
		ANativeWindow_unlockAndPost(window);
	}

	LOGD(LOG_TAG, "Scale video to W:H = %d : %d", width, height);
	bitmap = createBitmap(env, width, height);
	if(AndroidBitmap_lockPixels(env, bitmap, &buffer) < 0) return;

	swsContext = sws_getContext (
		        pCodecContext->width,
		        pCodecContext->height,
		        pCodecContext->pix_fmt,
		        width,
		        height,
		        AV_PIX_FMT_RGBA,
		        SWS_BILINEAR,
		        NULL,
		        NULL,
		        NULL);

	avpicture_fill((AVPicture *)pFrameRGBA, (uint8_t*)buffer, AV_PIX_FMT_RGBA, width, height);
}

void ChromaKeyRenderer::fillVideoResolution(int* outWidth, int* outHeight)
{
	if(NULL == pCodecContext || outWidth == NULL || outHeight == NULL) return;

	*outWidth = pCodecContext->width;
	*outHeight = pCodecContext->height;
}

jobject ChromaKeyRenderer::createBitmap(JNIEnv* pEnv, int pWidth, int pHeight)
{
	int i;
	LOGD(LOG_TAG, "create bitmap with w=%d, h=%d", pWidth, pHeight);
	jclass javaBitmapClass = (jclass)pEnv->FindClass("android/graphics/Bitmap");
	jmethodID mid = pEnv->GetStaticMethodID(javaBitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

	//reference: https://forums.oracle.com/thread/1548728
	const wchar_t* configName = L"ARGB_8888";
	int len = wcslen(configName);
	jstring jConfigName;
	if (sizeof(wchar_t) != sizeof(jchar)) {
		//wchar_t is defined as different length than jchar(2 bytes)
		jchar* str = (jchar*)malloc((len+1)*sizeof(jchar));
		for (i = 0; i < len; ++i) {
			str[i] = (jchar)configName[i];
		}
		str[len] = 0;
		jConfigName = pEnv->NewString((const jchar*)str, len);
	} else {
		//wchar_t is defined same length as jchar(2 bytes)
		jConfigName = pEnv->NewString((const jchar*)configName, len);
	}

	jclass bitmapConfigClass = pEnv->FindClass("android/graphics/Bitmap$Config");
	jobject javaBitmapConfig = pEnv->CallStaticObjectMethod(bitmapConfigClass,
			pEnv->GetStaticMethodID(bitmapConfigClass, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;"), jConfigName);

	LOGD(LOG_TAG, "Calling java create bitmap method");
	return pEnv->CallStaticObjectMethod(javaBitmapClass, mid, pWidth, pHeight, javaBitmapConfig);
}

void ChromaKeyRenderer::processBuffer(uint8_t* buffer, int width, int height)
{
	int pixelsCount = width * height;

	uint8_t* red;
	uint8_t* green;
	uint8_t* blue;
	uint8_t* alpha;

	for(int i = 0 ; i < pixelsCount ; i++)
	{
		red 	= buffer + i*4;
		green 	= buffer + i*4 + 1;
		blue 	= buffer + i*4 + 2;
		alpha	= buffer + i*4 + 3;

		if(*blue < 40 && *red < 40 && *green < 40) *alpha = 128;
		if(*blue < 20 && *red < 20 && *green < 20) *alpha = 64;
		if(*blue < 5 && *red < 5 && *green < 5) *alpha = 0;
	}
}

void ChromaKeyRenderer::enableChromaKey()
{
	chromaKeyIsEnabled = true;
}

void ChromaKeyRenderer::disableChromaKey()
{
	chromaKeyIsEnabled = false;
}

void ChromaKeyRenderer::play()
{
	if(!fileIsPrepared) return;

	LOGD(LOG_TAG, "Play: isPlaying = %d.", isPlaying);
	if(isPlaying) return;

	stopRendering = false;

	pthread_t decodeThread;
	pthread_create(&decodeThread, NULL, sDecodeAndRender,(void*)this);
	isPlaying = true;
}

void ChromaKeyRenderer::stop()
{
	stopRendering = true;
	isPlaying = false;
}

void ChromaKeyRenderer::decodeAndRender()
{
	LOGD(LOG_TAG, "Render thread enter.");
	LOGD(LOG_TAG, "Render thread: JavaVM is %p", jvm);
	JNIEnv* env = NULL;
	jvm->GetEnv((void**)&env, JNI_VERSION_1_6);


	ANativeWindow_Buffer windowBuffer;
	AVPacket packet;
	int i=0;
	int frameFinished;
	int lineCnt;


	LOGD(LOG_TAG, "Enter render thread");

	while(av_read_frame(pFormatContext, &packet) >= 0 && !stopRendering)
	{
		if(packet.stream_index == videoStreamIndex)
		{
			avcodec_decode_video2(pCodecContext, pDecodedFrame, &frameFinished, &packet);

			if(frameFinished)
			{
				sws_scale
				(
					swsContext,
					(uint8_t const * const *)pDecodedFrame->data,
					pDecodedFrame->linesize,
					0,
					pCodecContext->height,
					pFrameRGBA->data,
					pFrameRGBA->linesize
				);

				if(ANativeWindow_lock(window, &windowBuffer, NULL) < 0)
				{
					LOGE(LOG_TAG, "Can't lock window buffer");
				}
				else
				{
					if(chromaKeyIsEnabled)
					{
						processBuffer((uint8_t*)buffer, width, height);
					}
					LOGD(LOG_TAG, "Render frame %d %d. WindowBuffer is %d %d, stride is %d, format is %d", width, height, windowBuffer.width, windowBuffer.height, windowBuffer.stride, windowBuffer.format);

					memcpy(windowBuffer.bits, buffer,  width * height * 4);
					// unlock the window buffer and post it to display
					ANativeWindow_unlockAndPost(window);
					// count number of frames
					i++;;
				}
			}
		}

		av_free_packet(&packet);
	}

	LOGD(LOG_TAG, "Total %d frames decoded and rendered", i);
	isPlaying = false;
	//finish(env); //TODO: implement finishing
	//jvm->DetachCurrentThread();
}

