#include <stddef.h>
#include <wchar.h>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

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

static AVFrame*	pFrame;
static AVFrame*	pFrameARGB;

static AVFormatContext* pFormatContext;
static AVCodecContext*	pCodecContext;
static int videoStreamIndex;
static AVCodec*	pCodec;
static bool fileWasOpened = false;

static jobject bitmap;


void init_ffmpeg()
{
	av_register_all();
	LOGD(LOG_TAG, "av_register_all() done");
}

JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_initFfmpeg(JNIEnv* env, jobject obj)
{
	init_ffmpeg();
}

JNIEXPORT jboolean JNICALL Java_com_example_ffmpegtest_JniConnector_openFile(JNIEnv* env, jobject obj, jstring filePath)
{
	if(NULL == filePath)
	{
		LOGE(LOG_TAG, "Given filePath is NULL");
		return false;
	}

	LOGD(LOG_TAG , "jstring is %d", filePath);
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
		LOGE(LOG_TAG, "Cant avformat_open_input");
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
	pCodecContext;
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

	pFrame = avcodec_alloc_frame();
	pFrameARGB = avcodec_alloc_frame();

	if(NULL == pFrame || NULL == pFrameARGB)
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

JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_renderFrame(JNIEnv* env, jobject obj)
{
	int frameFinished;
	AVPacket packet;
	SwsContext* pSwsContext;
	pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height, PIX_FMT_ARGB, SWS_BILINEAR, NULL, NULL, NULL);

	void* buffer;
	bitmap = createBitmap(env, pCodecContext->width, pCodecContext->height);
	if (AndroidBitmap_lockPixels(env, bitmap, &buffer) < 0) return;

	avpicture_fill((AVPicture*)pFrameARGB, (uint8_t*)buffer, PIX_FMT_ARGB, pCodecContext->width, pCodecContext->height);

	int i=0;
	while(av_read_frame(pFormatContext, &packet) >= 0)
	{
		if(packet.stream_index == videoStreamIndex)
		{
			avcodec_decode_video2(pCodecContext, pFrame, &frameFinished, &packet);

			if(frameFinished)
			{
				sws_scale(pSwsContext, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, pCodecContext->height, pFrameARGB->data, pFrameARGB->linesize);
			}
		}

	}

//	void* p = pFrameARGB->data;
//	uint8_t* pp = (uint8_t*)p;
//	for(uint8_t i = 0 ; i < 16 ; i++)
//	{
//		//LOGD(LOG_TAG, "%p = %d", pFrameARGB->data + i, *(pFrameARGB->data + i));
//		LOGD(LOG_TAG, "%p = %d", pp+i, *(pp+i));
//	}

	SaveFrame(env, obj, bitmap, pFrameARGB->width, pFrameARGB->height, 99);
	//av_free_packet(&packet);
	AndroidBitmap_unlockPixels(env, bitmap);
}

JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_closeFile(JNIEnv* env, jobject obj)
{
	LOGD(LOG_TAG,"closeFile");
	if(!fileWasOpened) return;
	av_free(pFrameARGB);
	LOGD(LOG_TAG,"pFrameARGB released");
	av_free(pFrame);
	LOGD(LOG_TAG,"pFrame released");
	avcodec_close(pCodecContext);
	LOGD(LOG_TAG,"pCodecContext released");
	avformat_close_input(&pFormatContext);
	LOGD(LOG_TAG,"pFormatContext released");
}

void SaveFrame(JNIEnv *pEnv, jobject pObj, jobject pBitmap, int width, int height, int iFrame) {
	char szFilename[200];
	jmethodID sSaveFrameMID;
	jclass mainActCls;
	sprintf(szFilename, "/storage/sdcard0/frame%d.jpg", iFrame);
	//mainActCls = pEnv->GetObjectClass(pObj);
	//sSaveFrameMID = pEnv->GetStaticMethodID(mainActCls, "saveFrameToPath", "(Landroid/graphics/Bitmap;Ljava/lang/String;)V");

	jclass jniClass = pEnv->FindClass("com/example/ffmpegtest/JniConnector");
	jmethodID mid = pEnv->GetStaticMethodID(jniClass, "saveFrameToPath", "(Landroid/graphics/Bitmap;Ljava/lang/String;)V");

	LOGD(LOG_TAG,"call java method to save frame %d", iFrame);
	jstring filePath = pEnv->NewStringUTF(szFilename);
	//pEnv->CallVoidMethod(pObj, sSaveFrameMID, pBitmap, filePath);
	pEnv->CallStaticVoidMethod(jniClass, mid, pBitmap, filePath);
	LOGD(LOG_TAG, "call java method to save frame %d done", iFrame);
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
