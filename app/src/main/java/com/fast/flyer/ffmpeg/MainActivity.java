package com.fast.flyer.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.fast.flyer.ffmpeg.utils.VideoUtils;

import java.io.File;
import java.io.FileInputStream;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private Button video_decode, video_encode,mixAudio,changeVolume;


    private String localPcmPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/PCM-隐形的翅膀.pcm";

    private String localPcmPath2 = Environment.getExternalStorageDirectory().getAbsolutePath() + "/PCM-老人与海.pcm";

    private String changeVolumePcmOutPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/PCM_改变音量.pcm";

    private String mixPcmOutPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/PCM_混音.pcm";



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        video_decode = findViewById(R.id.video_decode);
        video_encode = findViewById(R.id.video_encode);
        mixAudio = findViewById(R.id.mixAudio);
        changeVolume = findViewById(R.id.changeVolume);

        video_decode.setOnClickListener(this);
        video_encode.setOnClickListener(this);
        mixAudio.setOnClickListener(this);
        changeVolume.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v == video_decode) {

            VideoUtils.decodeVideoforH264();

            VideoUtils.decodeVideoforYUV();

        } else if (v == video_encode) {

        }else  if(v == mixAudio){


            Log.i("aaaaaa", "混音开启");

//            VideoUtils.mixAudio(localPcmPath,localPcmPath2,mixPcmOutPath);

            VideoUtils.mixAudioWithVolume(localPcmPath, localPcmPath2, mixPcmOutPath, 1+"", 0.2+"");

            Log.i("aaaaaa", "混音成功");

            playPCM(mixPcmOutPath);

        }else  if(v == changeVolume){

            VideoUtils.changeVolume(localPcmPath,changeVolumePcmOutPath,0.6f);

            Log.i("aaaaaa", "改变成功");

            playPCM(changeVolumePcmOutPath);
        }

    }

    private void playPCM(String pcmPath)
    {
        AudioTrack audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC, // 指定流的类型
                44100, // 设置音频数据的採样率 32k，假设是44.1k就是44100
                AudioFormat.CHANNEL_OUT_STEREO, // 设置输出声道为双声道立体声，而CHANNEL_OUT_MONO类型是单声道
                AudioFormat.ENCODING_PCM_16BIT, // 设置音频数据块是8位还是16位。这里设置为16位。
                AudioTrack.getMinBufferSize(44100,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_16BIT),
                AudioTrack.MODE_STREAM );// 设置模式类型，在这里设置为流类型，第二种MODE_STATIC貌似没有什么效果


        audioTrack.play(); // 启动音频设备。以下就能够真正開始音频数据的播放了
// 打开mp3文件，读取数据，解码等操作省略 ...
        byte[] buffer = new byte[4096];
        int length ;
        try{

            FileInputStream inputStream = new FileInputStream(new File(pcmPath));
            while((length = inputStream.read(buffer)) != -1)
            {
                // 最关键的是将解码后的数据，从缓冲区写入到AudioTrack对象中
                audioTrack.write(buffer, 0, length);
            }

        }catch (Exception e){

        }


    }

}
