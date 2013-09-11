package com.example.ffmpegtest;

import java.io.File;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.CheckBox;

import com.askoropadsky.ChromaKey.ChromaKeyVideoView;
import com.example.ffmpeg_test.R;

public class MainActivity extends Activity {

	private ChromaKeyVideoView chromaKeySurface;
	private CheckBox	chromaKeyCheckBox;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		chromaKeySurface = (ChromaKeyVideoView) findViewById(R.id.chromaKeySurface);
		chromaKeyCheckBox = (CheckBox) findViewById(R.id.checkBox1);
		File ext = getExternalFilesDir(null);
		
		Log.d("qeqe", "ext: " + ext.getAbsolutePath());
		
		//chromaKeySurface.setVideoPath("/storage/sdcard0/adapter.mp4");
		chromaKeySurface.setVideoPath(getExternalFilesDir(null) + "/adapter.mp4");
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	public void startPlayback(View v)
	{
		if(chromaKeyCheckBox.isChecked()) chromaKeySurface.enableChromaKey();
		else chromaKeySurface.disableChromaKey();
		
		try {
			chromaKeySurface.prepare();
			chromaKeySurface.play();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public void stopPlayback(View v)
	{
		chromaKeySurface.stop();
	}
	
	@Override
	public void onStop()
	{
		super.onStop();
		chromaKeySurface.stop();
	}
}
