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
#define CHROMA_KEY_CONTROLLER_CLASS "com/askoropadsky/ChromaKey/ChromaKeyController"
#define MESSAGE_CALLBACK "nativeMessageReceived"
#define MESSAGE_CALLBACK_SIG "(I)V"

#define RED 	0
#define GREEN 	1
#define BLUE 	2

void* ChromaKeyRenderer::sDecodeAndRender(void* pUserData)
{
	ChromaKeyRenderer* instance = (ChromaKeyRenderer*)pUserData;
	instance->decodeAndRender();
	return NULL;
}

ChromaKeyRenderer::ChromaKeyRenderer(JavaVM* pJvm, JNIEnv* env, jobject controller)
{
	jvm = pJvm;

	window = NULL;

	pDecodedFrame = NULL;
	pFrameRGBA = NULL;

	pFormatContext = NULL;
	pCodecContext = NULL;
	pCodec = NULL;
	swsContext = NULL;
	buffer = NULL;

	bitmap = NULL;

	videoStreamIndex = AV_INVALID_STREAM_INDEX;
	width = 0;
	height = 0;

	keyColorRGB[0] = 128;
	keyColorRGB[1] = 128;
	keyColorRGB[2] = 128;
	keyChannel = 2;

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

	if(avformat_find_stream_info(pFormatContext, NULL) < 0)
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

	fileIsPrepared = false;
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
	jobject localBitmap = createBitmap(env, width, height);

	if(bitmap != NULL) env->DeleteGlobalRef(bitmap);
	bitmap = env->NewGlobalRef(localBitmap);

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

void ChromaKeyRenderer::setChromaKey(int red, int green, int blue, int keyColor)
{
	keyColorRGB[0] = red > 255 ? 255 : red ;
	keyColorRGB[1] = green > 255 ? 255 : green;
	keyColorRGB[2] = blue > 255 ? 255 : blue;

	keyChannel = keyColor;

	if(keyChannel > 2) keyChannel = 2;
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

//		if(*blue < 40 && *red < 40 && *green < 40) *alpha = 128;
//		if(*blue < 20 && *red < 20 && *green < 20) *alpha = 64;
//		if(*red < 5 && *green < 5 && *blue < 5 ) *alpha = 0;

		bool shouldBeTransparent = false;

		if(keyChannel == RED) 	shouldBeTransparent = (*red > keyColorRGB[0] && *green < keyColorRGB[1] && *blue < keyColorRGB[2]);
		if(keyChannel == GREEN) shouldBeTransparent = (*red < keyColorRGB[0] && *green > keyColorRGB[1] && *blue < keyColorRGB[2]);
		if(keyChannel == BLUE) 	shouldBeTransparent = (*red < keyColorRGB[0] && *green < keyColorRGB[1] && *blue > keyColorRGB[2]);

		if(shouldBeTransparent)
		{
			*red = 0;
			*green = 0;
			*blue = 0;
			*alpha = 0;
		}

//		if(*red < 128 && *green > 128 && *blue < 128)
//		{
//			*red = 0;
//			*green = 0;
//			*blue = 0;
//			*alpha = 0;
//		}
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

long ChromaKeyRenderer::getDuration()
{
	//TODO: needs to be implemented
	return 0;
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

	JNIEnv * g_env;
	// double check it's all ok
	int getEnvStat = jvm->GetEnv((void **)&g_env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED)
	{
		LOGD(LOG_TAG, "GetEnv: not attached");
		if (jvm->AttachCurrentThread(&g_env, NULL) != 0)
		{
			LOGE(LOG_TAG, "Failed to attach.");
		}
	}
	else if (getEnvStat == JNI_OK)
	{
		LOGD(LOG_TAG, "JNIEnv is valid");
	}
	else if (getEnvStat == JNI_EVERSION)
	{
		LOGE(LOG_TAG, "GetEnv: JNI version not supported.");
	}

	ANativeWindow_Buffer windowBuffer;
	AVPacket packet;
	int i=0;
	int frameFinished;
	int lineCnt;


	LOGD(LOG_TAG, "Enter render thread");

	while(av_read_frame(pFormatContext, &packet) >= 0 && !stopRendering)
	{
		//LOGD(LOG_TAG, "While start");
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
					//LOGD(LOG_TAG, "Render frame %d %d. WindowBuffer is %d %d, stride is %d, format is %d", width, height, windowBuffer.width, windowBuffer.height, windowBuffer.stride, windowBuffer.format);

					memcpy(windowBuffer.bits, buffer,  width * height * 4);
					// unlock the window buffer and post it to display
					ANativeWindow_unlockAndPost(window);
					// count number of frames
					i++;;
				}
			}
			//LOGD(LOG_TAG, "While end");
		}

		av_free_packet(&packet);
	}

	LOGD(LOG_TAG, "Total %d frames decoded and rendered", i);

	AndroidBitmap_unlockPixels(g_env, bitmap);
	releaseFile();

	jvm->DetachCurrentThread();
	isPlaying = false;
}

