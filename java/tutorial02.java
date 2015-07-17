package com.example.jhjh.ndktest;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;

import java.io.File;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class tutorial02 extends AppCompatActivity {

    private native void nativeChanged(int w, int h);
    private native void nativeCreated();
    private native String ffmepgTest(String file, String path);


    class GLView extends GLSurfaceView implements GLSurfaceView.Renderer{

        public GLView(Context context) {
            super(context);

            this.setRenderer(this);
            this.requestFocus();
            this.setRenderMode(RENDERMODE_WHEN_DIRTY);
            this.setFocusableInTouchMode(true);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            nativeCreated();

        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            nativeChanged(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            String strFile = Environment.getExternalStorageDirectory()+ File.separator+"abc.mp4";
            String strPath = Environment.getExternalStorageDirectory()+File.separator;

            String str = ffmepgTest(strFile, strPath);
        }
    }


    static {
        System.loadLibrary("ndktest");
    }


    public GLSurfaceView mGLView;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mGLView = new GLView(this);
        setContentView(mGLView);

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
