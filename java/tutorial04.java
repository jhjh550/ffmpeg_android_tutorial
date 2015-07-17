
package com.example.jhjh.ndktest;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class tutorial04 extends ActionBarActivity {

    class Tutorial4Renerer implements GLSurfaceView.Renderer{

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            nativeInitRenderer();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            nativeChangeView(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            nativeDraw();
        }
    }



    class NdkGLSurf extends GLSurfaceView {

        private final Tutorial4Renerer mRenderer;

        public NdkGLSurf(Context context) {
            super(context);

            // create an opengl es 2.0 context
            setEGLContextClientVersion(2);

            // set the renderer for drawing on the glsurfaceview
            mRenderer = new Tutorial4Renerer();
            setRenderer(mRenderer);

            // render the view only when there is a change in the drawing data
            setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        }

        @Override
        public void onPause() {
            super.onPause();
            //mRenderer.onPause();
        }

        @Override
        public void onResume() {
            super.onResume();
            //mRenderer.onResume();
        }
    }

    private GLSurfaceView glSurfaceView;


    private final static String TAG ="video_sync";
    private AudioTrack track;
    int minBufSize = 0;
    String path = Environment.getExternalStorageDirectory()+ "/Movies/ab.mp4";

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeCleanupFFMPEG();
    }


    private void initializeAudio(){
        int sampleRate = nativeInitFFMPEG(path);
        if(sampleRate == -1){
            Log.d(TAG, "Failed in get Sample Rate");
            return;
        }


        minBufSize = AudioTrack.getMinBufferSize(sampleRate,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT);

        Log.d(TAG, "sampleRate: "+ sampleRate + " buffuer size :"+ minBufSize);

        track = new AudioTrack(AudioManager.STREAM_MUSIC,
                sampleRate,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                minBufSize,
                AudioTrack.MODE_STREAM
        );

        new Thread(new Runnable() {
            @Override
            public void run() {

                while(total - track.getPlaybackHeadPosition() < minBufSize) {
                    byte[] bytes = new byte[minBufSize];
                    nativeGetAudioData(bytes);
                }

            }
        }).start();


        new Thread(new Runnable() {
            @Override
            public void run() {

                nativeStartQueuePacket();


            }
        }).start();



    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_audio_test);
        glSurfaceView = new NdkGLSurf(this);
        setContentView(glSurfaceView);

        initializeAudio();

    }





    int total = 0;
    public void PlaySound(byte[] buf, int size){


        if(track.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
            track.play();
        }

        track.write(buf, 0, size);
        total += (size/4);
        //Log.d(TAG, "total buffuer size :"+total+ " head poisiton : "+ track.getPlaybackHeadPosition());
    }


    static {
        System.loadLibrary("ndktest");
    }


    private native int nativeInitFFMPEG(String path);
    private native void nativeCleanupFFMPEG();
    private native int nativeStartQueuePacket();
    private native void nativeGetAudioData(byte[] array);

    private native void nativeInitRenderer();
    private native void nativeChangeView(int w, int h);
    private native void nativeDraw();

}
