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

	bool fileIsPrepared;
	bool chromaKeyIsEnabled;
	bool isPlaying;
	int stop;

public:
	ChromaKeyRenderer();
	virtual ~ChromaKeyRenderer();

	bool prepare(const char* path);
//	bool prepare();
//	void play();
//	void stop();
};

#endif /* CHROMAKEYRENDERER_H_ */
