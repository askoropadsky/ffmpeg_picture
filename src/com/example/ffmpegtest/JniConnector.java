package com.example.ffmpegtest;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.view.Surface;

public class JniConnector {

	public static void saveFrameToPath(Bitmap bitmap, String pPath) {
		int BUFFER_SIZE = 1024 * 8;
	    try {
	    	File file = new File(pPath);
	        file.createNewFile();
	        FileOutputStream fos = new FileOutputStream(file);
	        final BufferedOutputStream bos = new BufferedOutputStream(fos, BUFFER_SIZE);
	        bitmap.compress(CompressFormat.JPEG, 100, bos);
	        bos.flush();
	        bos.close();
	        fos.close();
	    } catch (FileNotFoundException e) {
	        e.printStackTrace();
	    } catch (IOException e) {
	    	e.printStackTrace();
	    }
	}
	
	public static native void initFfmpeg();
	public static native boolean openFile(String path);
	public static native void renderFrame();
	public static native void closeFile();
	public static native int[] getVideoResolution();
	public static native void setSurface(Surface surface);
	public static native boolean setup(int width, int height);
	public static native void play();
	public static native void stop();
	
	static {
		System.loadLibrary("avutil-52");
		System.loadLibrary("avcodec-55");
		System.loadLibrary("avformat-55");
		System.loadLibrary("swscale-2");
		System.loadLibrary("ffmpeg_test");
	}
}
