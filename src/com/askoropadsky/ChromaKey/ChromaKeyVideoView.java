package com.askoropadsky.ChromaKey;

import java.io.File;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceView;

public class ChromaKeyVideoView extends SurfaceView {
	
	public static final int KEY_CHANNEL_RED = 0;
	public static final int KEY_CHANNEL_GREEN = 1;
	public static final int KEY_CHANNEL_BLUE = 2;
	
	private final String LOG_TAG = "ChromaKeyVideoView";
	
	private String filePath; 
	private boolean chromaKeyIsEnabled = false;
	
	public ChromaKeyVideoView(Context context) {
		super(context);
		configure();
	}
	
	public ChromaKeyVideoView(Context context, AttributeSet attrs) {
		super(context, attrs);
		configure();
	}
	
	public ChromaKeyVideoView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		configure();
	}

	private void configure()
	{
		setZOrderOnTop(true);
		getHolder().setFormat(PixelFormat.TRANSLUCENT);
		getHolder().addCallback(new ChromaKeyController());
		
		ChromaKeyController.initFfmpeg();
	}
	
	public void setVideoPath(String path)
	{
		filePath = path;
	}
	
	public void prepare() throws Exception
	{
		if(!fileIsExists()) throw new Exception("File: " + filePath + " is not exist.");
		
		ChromaKeyController.openFile(filePath);
	}
	
	public void releaseVideoFile()
	{
		ChromaKeyController.closeFile();
	}
	
	private boolean fileIsExists()
	{
		File fd = new File(filePath);
		return fd.exists();
	}
	
	public void play()
	{
		if(chromaKeyIsEnabled) ChromaKeyController.enableChromaKey();
		else ChromaKeyController.disableChromaKey();
		
		setVideoScaleFactor();
		ChromaKeyController.play();
	}
	
	private void setVideoScaleFactor()
	{
		int width = getWidth();
		int height = getHeight();
		Log.d(LOG_TAG, "setVideoScaleFactor W : H = " + width + " " + height);
		ChromaKeyController.setup(width, height);
	}
	
	public void stop()
	{
		ChromaKeyController.stop();
	}
	
	public void setChromaKey(int red, int green, int blue, int keyChannel)
	{
		ChromaKeyController.setChromaKey(red, green, blue, keyChannel);
	}
	
	public void enableChromaKey()
	{
		chromaKeyIsEnabled = true;
	}
	
	public void disableChromaKey()
	{
		chromaKeyIsEnabled = false;
	}
	
	public long getDuration()
	{
		return ChromaKeyController.getDuration();
	}
	
	public boolean isPlaying()
	{
		return ChromaKeyController.isPlaying();
	}
	
	public float getVideoAspectRatio()
	{
		int[] videoRes = ChromaKeyController.getVideoResolution();
		float aspectRatio = (float) videoRes[0] / (float) videoRes[1];
		Log.d("qeqe", "video is " + videoRes[0] + " x " + videoRes[1]);
		return aspectRatio;
	}
	
	
}
