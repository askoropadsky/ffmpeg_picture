/*
 * ChromaKeyRenderer.h
 *
 *  Created on: Sep 11, 2013
 *      Author: askoropadsky
 */

#ifndef CHROMAKEYRENDERER_H_
#define CHROMAKEYRENDERER_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <android/log.h>
#include <android/bitmap.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

class ChromaKeyRenderer {
public:
	//typedef void (*RENDER_CALLBACK)(void* userData);
	static void* sDecodeAndRender(void* pUserData);

private:
	JavaVM* jvm;
	ANativeWindow*	window;
	jobject bitmap;

	AVFrame*	pDecodedFrame;
	AVFrame*	pFrameRGBA;
	AVFormatContext* pFormatContext;
	AVCodecContext*	pCodecContext;
	AVCodec* pCodec;
	SwsContext*	swsContext;
	int videoStreamIndex;

	void* buffer;

	int width;
	int height;

	int	keyColorLowRGB[3];
	int keyColorHighRGB[3];

	bool fileIsPrepared;
	bool chromaKeyIsEnabled;
	bool isPlaying;
	bool stopRendering;

	void decodeAndRender();
	jobject createBitmap(JNIEnv* env, int width, int height);
	void processBuffer(uint8_t* buffer, int width, int height);

public:
	ChromaKeyRenderer(JavaVM* pJvm);
	virtual ~ChromaKeyRenderer();

	bool prepare(const char* path);
	void releaseFile();

	void setSurface(JNIEnv* env, jobject surface);
	void setVideoScalingFactor(JNIEnv* env, int width, int height);

	void fillVideoResolution(int* outWidth, int* outHeight);

	void enableChromaKey();
	void disableChromaKey();

	void play();
	void stop();
};

#endif /* CHROMAKEYRENDERER_H_ */
