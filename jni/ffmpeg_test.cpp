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

#include "ChromaKeyRenderer.h"
#include "ffmpeg_test.h"

#define LOG_TAG "NativeLib"

#define LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define JNI_CONNECTOR_CLASS "com/askoropadsky/ChromaKey/ChromaKeyController"

JavaVM* jvm;
ChromaKeyRenderer*	renderer;

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
	renderer = new ChromaKeyRenderer(jvm);
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

	renderer->prepare(path);
	env->ReleaseStringUTFChars(filePath, path);

	return true;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_closeFile(JNIEnv* env, jobject obj)
{
	LOGD(LOG_TAG,"closeFile");
	renderer->releaseFile();
}

JNIEXPORT jintArray JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_getVideoResolution(JNIEnv* env, jobject obj)
{
	jintArray lres;
	lres = env->NewIntArray(2);
	if(NULL == lres)
	{
		LOGE(LOG_TAG,"Cannot allocate memory for video resolution array.");
		return NULL;
	}

	jint resArray[2];
	renderer->fillVideoResolution(&resArray[0], &resArray[1]);
	env->SetIntArrayRegion(lres,0, 2, resArray);
	return lres;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setSurface(JNIEnv* env,jobject obj, jobject surface)
{
	LOGD(LOG_TAG,"SetSurface called.");
	renderer->setSurface(env, surface);
}

JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setup(JNIEnv* env, jobject obj, jint _width, jint _height)
{
	renderer->setVideoScalingFactor(env, _width, _height);
	return true;
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_play(JNIEnv* env, jobject obj)
{
	renderer->play();
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_stop(JNIEnv* env, jobject obj)
{
	renderer->stop();
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_enableChromaKey(JNIEnv* env, jobject obj)
{
	renderer->enableChromaKey();
}

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_disableChromaKey(JNIEnv* env, jobject obj)
{
	renderer->disableChromaKey();
}

