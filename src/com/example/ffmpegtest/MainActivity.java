package com.example.ffmpegtest;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;

import com.example.ffmpeg_test.R;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		JniConnector.initFfmpeg();
		
//		//JniConnector.openFile("/storage/sdcard0/adapter.mp4"); //TODO: add legal way to obtain absolute path
//		JniConnector.openFile("/storage/sdcard0/calibration.mp4");
//		JniConnector.renderFrame();
//		JniConnector.closeFile();

	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		new Thread(new Runnable() {
			
			@Override
			public void run() {
				JniConnector.openFile("/storage/sdcard0/calibration.mp4"); //TODO: add legal way to obtain absolute path
				JniConnector.renderFrame();
				JniConnector.closeFile();
			}
		}).start();
	}
	

}
