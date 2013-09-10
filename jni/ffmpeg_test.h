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

jobject createBitmap(JNIEnv *pEnv, int pWidth, int pHeight);
void SaveFrame(JNIEnv *pEnv, jobject pObj, jobject pBitmap, int width, int height, int iFrame);

#ifdef __cplusplus
}
#endif

#endif /* FFMPEG_TEST_H_ */
