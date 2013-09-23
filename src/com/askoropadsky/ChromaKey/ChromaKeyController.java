package com.askoropadsky.ChromaKey;

import android.os.Handler;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

class ChromaKeyController implements SurfaceHolder.Callback {
	
	private Handler	mReceiver;
	
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
	
	public static void nativeMessageReceived(int message)
	{
		Log.d("qeqe", "GOT NATIVE MESSAGE " + message);
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
	public static native void setChromaKey(int red, int green, int blue, int keyChannel);
	public static native long getDuration();
	public static native boolean isPlaying();
	public static native void setLooped(boolean isLooped);
	public static native boolean isLooped();
	public static native void setPauseBetweenLoops(int seconds);
	
	static {
		System.loadLibrary("avutil-52");
		System.loadLibrary("avcodec-55");
		System.loadLibrary("avformat-55");
		System.loadLibrary("swscale-2");
		System.loadLibrary("ffmpeg_test");
	}
}
