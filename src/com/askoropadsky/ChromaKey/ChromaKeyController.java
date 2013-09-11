package com.askoropadsky.ChromaKey;

import android.view.Surface;
import android.view.SurfaceHolder;

class ChromaKeyController implements SurfaceHolder.Callback {
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		ChromaKeyController.setSurface(holder.getSurface());
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		ChromaKeyController.setSurface(null);
	}
	
	/* Native part */
	public static native void initFfmpeg();
	public static native boolean openFile(String path);
	public static native void renderFrame();
	public static native void closeFile();
	public static native int[] getVideoResolution();
	public static native void setSurface(Surface surface);
	public static native boolean setup(int width, int height);
	public static native void play();
	public static native void stop();
	public static native void enableChromaKey();
	public static native void disableChromaKey();
	
	static {
		System.loadLibrary("avutil-52");
		System.loadLibrary("avcodec-55");
		System.loadLibrary("avformat-55");
		System.loadLibrary("swscale-2");
		System.loadLibrary("ffmpeg_test");
	}
}
