package com.jldevelopers.ffmpegremux;

import android.os.Bundle;
import android.util.Log;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        String path = " ";      //Input file path
        int audioTrack = 1;     //audio track no to extract
        String dest = "";       //output path

        String[] ffmpegCommand = new String[] {
                "ffmpeg",
                "-i", path,
                "-map", "0:v:0",
                "-map", "0:a:" + audioTrack,
                "-c", "copy",
                dest
        };

        FFmpegLoader.executeAsync(ffmpegCommand, new FFmpegLoader.Callback() {
            @Override
            public void onComplete(int i) {
                Log.d("FFmpegDebug", "Completed with code: " + i);
            }

            @Override
            public void onProgress(float v) {
                Log.d("FFmpegDebug", "Progress: " + v);
            }

            @Override
            public void onLog(String s) {
                Log.d("FFmpegDebug", "onLog: " + s);
            }

            @Override
            public void onError(String s) {
                Log.d("FFmpegDebug", "Progress: " + s);
            }
        });
    }
}