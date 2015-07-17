package com.example.jhjh.ndktest;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

import java.io.File;

public class tutorial01 extends AppCompatActivity {

    static {
        System.loadLibrary("ndktest");
    }


    public native String ffmepgTest(String file, String path);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView)findViewById(R.id.textView);

        String strFile = Environment.getExternalStorageDirectory()+File.separator+"abc.mp4";
        String strPath = Environment.getExternalStorageDirectory()+File.separator;


        String res = ffmepgTest(strFile, strPath);
        if(res != null){
            tv.setText(res);
        }




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
