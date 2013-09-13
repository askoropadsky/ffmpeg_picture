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

#define CHROMA_KEY_MESSAGE_VIDEO_DID_FINISHED	1

class ChromaKeyRenderer {
public:
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

	int	keyColorRGB[3];
	int keyChannel;

	bool fileIsPrepared;
	bool chromaKeyIsEnabled;
	bool isPlaying;
	bool stopRendering;

public:
	ChromaKeyRenderer(JavaVM* pJvm, JNIEnv* env, jobject controller);
	virtual ~ChromaKeyRenderer();

	bool prepare(const char* path);
	void releaseFile();

	void setSurface(JNIEnv* env, jobject surface);
	void setVideoScalingFactor(JNIEnv* env, int width, int height);
	void setChromaKey(int red, int green, int blue, int keyChannel);

	void fillVideoResolution(int* outWidth, int* outHeight);

	void enableChromaKey();
	void disableChromaKey();

	long getDuration();

	void play();
	void stop();

private:
	void decodeAndRender();
	jobject createBitmap(JNIEnv* env, int width, int height);
	void processBuffer(uint8_t* buffer, int width, int height);
};

#endif /* CHROMAKEYRENDERER_H_ */
