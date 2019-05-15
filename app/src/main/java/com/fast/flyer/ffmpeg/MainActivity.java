package com.fast.flyer.ffmpeg;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.fast.flyer.ffmpeg.utils.VideoUtils;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private Button video_decode, video_encode;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        video_decode = findViewById(R.id.video_decode);
        video_encode = findViewById(R.id.video_encode);


        video_decode.setOnClickListener(this);
        video_encode.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v == video_decode) {

            VideoUtils.decodeVideoforH264();

            VideoUtils.decodeVideoforYUV();

        } else if (v == video_encode) {

        }
    }

}
