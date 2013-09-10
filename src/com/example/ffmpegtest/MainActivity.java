package com.example.ffmpegtest;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;

import com.example.ffmpeg_test.R;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

	private SurfaceView surface;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		surface = (SurfaceView) findViewById(R.id.surface);
		surface.getHolder().addCallback(this);
		//surface.getHolder().setFormat(PixelFormat.RGBA_8888);
		
		JniConnector.initFfmpeg();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	public void startPlayback(View v)
	{
		JniConnector.openFile("/storage/sdcard0/1.mp4");
		int[] videoRes = JniConnector.getVideoResolution();
		int[] screenRes = getScreenRes();
		Log.d("TEST", " Screen res is " + screenRes[0] + " , " + screenRes[1]);
		Log.d("TEST", " Video res is " + videoRes[0] + " , " + videoRes[1]);
		
		
		int width;
		int height;
		
		float widthScaleRatio = screenRes[0] * 1.0f / videoRes[0];
		float heightScaleRatio = screenRes[1] * 1.0f / videoRes[1];
		
		if (widthScaleRatio > heightScaleRatio) {
			//use heightScaledRatio
			width = (int) (videoRes[0]*heightScaleRatio);
			height = screenRes[1];
		} else {
			//use widthScaledRatio
			width = screenRes[0];
			height = (int) (videoRes[1]*widthScaleRatio);
		}
		Log.d("TEST", " prefinal View res width " + width + ",height:" + height);
		updateSurfaceView(width, height);
		screenRes = getSurfaceRes();
		
		widthScaleRatio = screenRes[0] * 1.0f / videoRes[0];
		heightScaleRatio = screenRes[1] * 1.0f / videoRes[1];
		
		if (widthScaleRatio > heightScaleRatio) {
			//use heightScaledRatio
			width = (int) (videoRes[0]*heightScaleRatio);
			height = screenRes[1];
		} else {
			//use widthScaledRatio
			width = screenRes[0];
			height = (int) (videoRes[1]*widthScaleRatio);
		}
		Log.d("TEST", " prefinal View res width " + width + ",height:" + height);
		JniConnector.setup(width, height);
		Log.d("TEST", " current Surface res width: " + screenRes[0] + ",height:" + screenRes[1]);
		
		JniConnector.play();
	}
	
	@SuppressLint("NewApi")
	private int[] getScreenRes() {
		int[] res = new int[2];
		Display display = getWindowManager().getDefaultDisplay();
		if (Build.VERSION.SDK_INT >= 13) {
			Point size = new Point();
			display.getSize(size);
			res[0] = size.x;
			res[1] = size.y;
		} else {
			res[0] = display.getWidth();  // deprecated
			res[1] = display.getHeight();  // deprecated
		}
		return res;
	}
	
	private int[] getSurfaceRes()
	{
		int[] res = new int[2];
		res[0] = surface.getWidth();
		res[1] = surface.getHeight();
		return res;
	}
	
	private void updateSurfaceView(int pWidth, int pHeight) {
		//update surfaceview dimension, this will cause the native window to change
		RelativeLayout.LayoutParams params = (LayoutParams) surface.getLayoutParams();
		params.width = pWidth;
		params.height = pHeight;
		surface.setLayoutParams(params);
		Log.d("TEST", " updateSurfaceView res: " + pWidth + ", " + pHeight);
	}
	
	@Override
	public void onStop()
	{
		super.onStop();
		JniConnector.stop();
	}
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		JniConnector.setSurface(holder.getSurface());
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		JniConnector.setSurface(null);
	}
	

}
