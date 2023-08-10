package org.cybertracker.mobile;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.net.Uri;
import android.os.Bundle;

public class LinkActivity extends Activity
{
    private static final String TAG = LinkActivity.class.getSimpleName();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        finish();

        Uri commandLineData = getIntent().getData();
        String commandLine = commandLineData != null ? commandLineData.toString() : "";
        Log.d(TAG, "Command line: " + commandLine);

        Intent intent = new Intent(this, MainActivity.class);
        intent.setAction(getIntent().getAction());
        intent.setData(getIntent().getData());

        int flags = intent.getFlags();
        if (MainActivity.instance() != null)
        {
            flags |= Intent.FLAG_ACTIVITY_REORDER_TO_FRONT;
        }

        intent.setFlags(flags);
        startActivity(intent);
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
    }
}
