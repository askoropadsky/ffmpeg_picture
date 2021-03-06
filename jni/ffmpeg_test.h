/*
 * ffmpeg_test.h
 *
 *  Created on: Sep 9, 2013
 *      Author: askoropadsky
 */

#ifndef FFMPEG_TEST_H_
#define FFMPEG_TEST_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_initFfmpeg(JNIEnv *, jobject);
JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_openFile(JNIEnv *, jobject, jstring);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_closeFile(JNIEnv* env, jobject obj);
JNIEXPORT jintArray JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_getVideoResolution(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setSurface(JNIEnv* env, jobject obj, jobject surface);
JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setup(JNIEnv* env, jobject obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_play(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_stop(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_enableChromaKey(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_disableChromaKey(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setChromaKey(JNIEnv* env, jobject obj, jint red, jint green, jint blue, jint keyChannel);
JNIEXPORT jlong JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_getDuration(JNIEnv* env, jobject obj);
JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_isPlaying(JNIEnv* env, jobject obj);

JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setLooped(JNIEnv* env, jobject obj, jboolean isLooped);
JNIEXPORT jboolean JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_isLooped(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_askoropadsky_ChromaKey_ChromaKeyController_setPauseBetweenLoops(JNIEnv* env, jobject obj, jint seconds);


jobject createBitmap(JNIEnv *pEnv, int pWidth, int pHeight);

#ifdef __cplusplus
}
#endif

#endif /* FFMPEG_TEST_H_ */
