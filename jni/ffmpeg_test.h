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

JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_initFfmpeg(JNIEnv *, jobject);

JNIEXPORT jboolean JNICALL Java_com_example_ffmpegtest_JniConnector_openFile(JNIEnv *, jobject, jstring);
JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_renderFrame(JNIEnv *, jobject);
JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_closeFile(JNIEnv* env, jobject obj);
JNIEXPORT jintArray JNICALL Java_com_example_ffmpegtest_JniConnector_getVideoResolution(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_setSurface(JNIEnv* env, jobject obj, jobject surface);
JNIEXPORT jboolean JNICALL Java_com_example_ffmpegtest_JniConnector_setup(JNIEnv* env, jobject obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_play(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_example_ffmpegtest_JniConnector_stop(JNIEnv* env, jobject obj);


jobject createBitmap(JNIEnv *pEnv, int pWidth, int pHeight);
void SaveFrame(JNIEnv *pEnv, jobject pObj, jobject pBitmap, int width, int height, int iFrame);

#ifdef __cplusplus
}
#endif

#endif /* FFMPEG_TEST_H_ */
