package com.askoropadsky.ChromaKey;

import java.io.File;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceView;

public class ChromaKeyVideoView extends SurfaceView {
	
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
		ChromaKeyController.setup(width, height);
	}
	
	public void stop()
	{
		ChromaKeyController.stop();
	}
	
	public void enableChromaKey()
	{
		chromaKeyIsEnabled = true;
	}
	
	public void disableChromaKey()
	{
		chromaKeyIsEnabled = false;
	}
	
	
}
