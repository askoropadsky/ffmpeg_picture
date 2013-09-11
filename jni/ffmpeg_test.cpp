#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <wchar.h>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "ffmpeg_test.h"

#define LOG_TAG "NativeLib"

#define LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define JNI_CONNECTOR_CLASS "com/askoropadsky/ChromaKey/ChromaKeyController"

JavaVM* jvm;
ANativeWindow*	window;
int width;
int height;

AVFrame*	pDecodedFrame;
AVFrame*	pFrameRGBA;

AVFormatContext* pFormatContext = NULL;
AVCodecContext*	pCodecContext = NULL;
int videoStreamIndex;
AVCodec*	pCodec = NULL;
bool fileWasOpened = false;
SwsContext*	swsContext = NULL;
static jobject bitmap;
void*	buffer = NULL;
bool	chromaKeyIsEnabled = false;
int stop;



jint JNI_OnLoad(JavaVM* pVm, void* reserved) {
	jvm = pVm;
	LOGD(LOG_TAG, "JNI_OnLoad: JavaVM is %p", jvm);

	JNIEnv* env;
	if (pVm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
		 return -1;
	}

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_initFfmpeg(JNIEnv* env, jobject obj)
{
	av_register_all();
	LOGD(LOG_TAG, "av_register_all() done");
}

JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_openFile(JNIEnv* env, jobject obj, jstring filePath)
{
	if(NULL == filePath)
	{
		LOGE(LOG_TAG, "Given filePath is NULL");
		return false;
	}

	const char* path = env->GetStringUTFChars(filePath, NULL);
	if(NULL == path)
	{
		LOGE(LOG_TAG, "Can't get path from jstring");
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

	fileWasOpened = true;

	env->ReleaseStringUTFChars(filePath, path);
	return true;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_closeFile(JNIEnv* env, jobject obj)
{
	LOGD(LOG_TAG,"closeFile");
	if(!fileWasOpened) return;
	av_free(pFrameRGBA);
	LOGD(LOG_TAG,"pFrameARGB released");
	av_free(pDecodedFrame);
	LOGD(LOG_TAG,"pFrame released");
	avcodec_close(pCodecContext);
	LOGD(LOG_TAG,"pCodecContext released");
	avformat_close_input(&pFormatContext);
	LOGD(LOG_TAG,"pFormatContext released");
}

JNIEXPORT jintArray JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_getVideoResolution(JNIEnv* env, jobject obj)
{
	jintArray lres;
	if(NULL == pCodecContext) return NULL;

	lres = env->NewIntArray(2);
	if(NULL == lres)
	{
		LOGE(LOG_TAG,"Cannot allocate memory for video resolution array.");
		return NULL;
	}

	jint resArray[2];
	resArray[0] = pCodecContext->width;
	resArray[1] = pCodecContext->height;
	env->SetIntArrayRegion(lres,0, 2, resArray);
	return lres;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setSurface(JNIEnv* env,jobject obj, jobject surface)
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

JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setup(JNIEnv* env, jobject obj, jint _width, jint _height)
{
	width = _width;
	height = _height;

	LOGD(LOG_TAG, "New W:H = %d : %d", width, height);
	bitmap = createBitmap(env, width, height);
	if(AndroidBitmap_lockPixels(env, bitmap, &buffer) < 0) return false;

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
	return true;
}

void finish(JNIEnv* env)
{
	AndroidBitmap_unlockPixels(env, bitmap);
	av_free(buffer);
	av_free(pFrameRGBA);
	av_free(pDecodedFrame);
	avcodec_close(pCodecContext);
	avformat_close_input(&pFormatContext);
}

void processBuffer(uint8_t* buffer, int width, int height)
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

		//*(buffer+i) = (uint8_t) rand() ;
		if(*blue < 40 && *red < 40 && *green < 40) *alpha = 128;
		if(*blue < 20 && *red < 20 && *green < 20) *alpha = 64;
		if(*blue < 5 && *red < 5 && *green < 5) *alpha = 0;
	}
}

void *decodeAndRender(void*)
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

	while(av_read_frame(pFormatContext, &packet) >= 0 && !stop)
	{
		if(packet.stream_index == videoStreamIndex)
		{
			avcodec_decode_video2(pCodecContext, pDecodedFrame, &frameFinished, &packet);

			if(frameFinished)
			{
				//LOGD(LOG_TAG, "Got new frame");
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

				//LOGD(LOG_TAG, "frame scaled");

				if(ANativeWindow_lock(window, &windowBuffer, NULL) < 0)
				{
					LOGE(LOG_TAG, "Can't lock window buffer");
				}
				else
				{
					//LOGD(LOG_TAG, "copy buffer %d:%d:%d", width, height, width*height*4);
					//LOGD(LOG_TAG, "window buffer: %d:%d:%d", windowBuffer.width, windowBuffer.height, windowBuffer.stride);

					if(chromaKeyIsEnabled)
					{
						processBuffer((uint8_t*)buffer, width, height);
					}

					memcpy(windowBuffer.bits, buffer,  width * height * 4);
					// unlock the window buffer and post it to display
					ANativeWindow_unlockAndPost(window);
					// count number of frames
					++i;
				}
			}
		}

		av_free_packet(&packet);
	}

	LOGD(LOG_TAG, "Total %d frames decoded and rendered", i);
	//finish(env); //TODO: implement finishing
	jvm->DetachCurrentThread();
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_play(JNIEnv* env, jobject obj)
{
	stop = 0;

	pthread_t decodeThread;
	pthread_create(&decodeThread, NULL, decodeAndRender,NULL);
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_stop(JNIEnv* env, jobject obj)
{
	stop = 1;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_enableChromaKey(JNIEnv* env, jobject obj)
{
	chromaKeyIsEnabled = true;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_disableChromaKey(JNIEnv* env, jobject obj)
{
	chromaKeyIsEnabled = false;
}

jobject createBitmap(JNIEnv *pEnv, int pWidth, int pHeight) {
	int i;
	LOGD(LOG_TAG, "create bitmap with w=%d, h=%d", pWidth, pHeight);
	//get Bitmap class and createBitmap method ID
	jclass javaBitmapClass = (jclass)pEnv->FindClass("android/graphics/Bitmap");
	jmethodID mid = pEnv->GetStaticMethodID(javaBitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
	//create Bitmap.Config
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
	//create the bitmap
	LOGD(LOG_TAG, "Calling java create bitmap method");
	return pEnv->CallStaticObjectMethod(javaBitmapClass, mid, pWidth, pHeight, javaBitmapConfig);
}
