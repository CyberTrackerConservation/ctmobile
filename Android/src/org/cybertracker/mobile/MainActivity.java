package org.cybertracker.mobile;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.net.Uri;
import android.net.ConnectivityManager;
import android.os.PowerManager;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ShareCompat;
import android.support.v4.content.FileProvider;
import java.io.File;

public class MainActivity extends QtActivity
{
    private static final String TAG = MainActivity.class.getSimpleName();
    public static native void logMessage(String message);

    private static MainActivity s_instance;
    public static MainActivity instance()
    {
        return s_instance;
    }

    private boolean m_kioskMode = false;
    public static boolean kioskMode()
    {
        return s_instance.m_kioskMode;
    }

    private boolean m_tracking = false;
    public static boolean tracking()
    {
        return s_instance.m_tracking;
    }

    public static native void alarmFired();

    public static native boolean fileExists(String filePath);
    public static native void fileReceived(String filePathUrl);

    private static IntentFilter s_batteryChangedIntentFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
    private native void batteryStateChange(int level, boolean charging);

    private final BroadcastReceiver m_batteryChangedReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            final int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
            final int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
            final int currentLevel = (level * 100) / scale;
            final boolean charging = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1) != 0;
            batteryStateChange(level, charging);
        }
    };

    // Interface called from Qt.
    public String getFlavor()
    {
        return BuildConfig.FLAVOR;
    }

    public String getDeviceId()
    {
        String id = Settings.Secure.getString(getContentResolver(), Secure.ANDROID_ID);

        if (!id.isEmpty())
        {
            // Pad to uuid length.
            id = id + ("00000000000000000000000000000000".substring(id.length()));
        }

        return id;
    }

    public String getImei()
    {
        TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);

        try
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            {
                return telephonyManager.getImei();
            }
        }
        catch (SecurityException e)
        {
            logMessage("Couldn't read default IMEI: " + e.getMessage());
        }

        return null;
    }

    public String getLaunchUri()
    {
        Uri commandLine = getIntent().getData();
        return commandLine != null ? commandLine.toString() : "";
    }

    public boolean getWifiConnected()
    {
        final ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        final android.net.NetworkInfo networkInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        return networkInfo.isConnected();
    }

    public void requestDisableBatterySaver()
    {
        if (Build.VERSION.SDK_INT < 23)
        {
            return;
        }

        String appId = BuildConfig.APPLICATION_ID;
        PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
        if (powerManager != null && !powerManager.isIgnoringBatteryOptimizations(appId))
        {
            startActivity(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:" + getPackageName())));
        }
    }

    public void setKioskMode(final boolean enable)
    {
        if (enable == m_kioskMode)
        {
            return;
        }

        m_kioskMode = enable;
        final Activity activity = this;

        runOnUiThread(new Runnable()
        {
            public void run()
            {
                if (enable)
                {
                    Log.d(TAG, "KIOSK_MODE: Start");
                    logMessage("KIOSK_MODE: Start");
                    Utils.setKioskMode(activity, true);
                }
                else
                {
                    Utils.setKioskMode(activity, false);
                    Log.d(TAG, "KIOSK_MODE: Stop");
                    logMessage("KIOSK_MODE: Stop");
                }
            }
        });
    }

    public String getBestSDPath()
    {
        return Utils.getBestSDPath();
    }

    public static boolean s_qtReady = false;
    public static boolean s_intentPending = false;
    public static String s_tempFilePath;

    public void qtReady(String tempFilePath)
    {
        s_qtReady = true;
        s_tempFilePath = tempFilePath;

        if (s_intentPending)
        {
            s_intentPending = false;
            processIntent();
        }
    }

    // LocationUpdateService.
    public void startLocationUpdates()
    {
        m_tracking = true;
        LocationWakeupService.start();
        ContextCompat.startForegroundService(this, new Intent(this, LocationUpdateService.class));
    }

    public void stopLocationUpdates()
    {
        LocationWakeupService.stop();
        stopService(new Intent(this, LocationUpdateService.class));
        m_tracking = false;
    }

    // AlarmJobService.
    public void setAlarm(int seconds)
    {
        AlarmJobService.setAlarm(seconds);
    }

    public void killAlarm()
    {
        AlarmJobService.killAlarm();
    }

    // mediaScan.
    public void mediaScan(String filePath)
    {
        Utils.mediaScan(this, filePath);
    }

    // Share.
    public void share(String url, String title)
    {
        Intent sendIntent = ShareCompat.IntentBuilder.from(this).getIntent();
        sendIntent.setAction(Intent.ACTION_SEND);

        sendIntent.putExtra(Intent.EXTRA_TEXT, url);
        sendIntent.setType("text/plain");

        startActivity(Intent.createChooser(sendIntent, title));
    }

    // Send file.
    public void sendFile(String filePath, String title, String mimeType)
    {
        Intent sendIntent = ShareCompat.IntentBuilder.from(this).getIntent();
        sendIntent.setAction(Intent.ACTION_SEND);

        Uri fileUri;
        try
        {
            String authority = BuildConfig.APPLICATION_ID + ".FileProvider";
            fileUri = FileProvider.getUriForFile(this, authority, new File(filePath));
        }
        catch (IllegalArgumentException e)
        {
            Log.d(TAG, "sendFile error: " + e.getMessage());
            return;
        }

        sendIntent.putExtra(Intent.EXTRA_STREAM, fileUri);
        sendIntent.setType(mimeType);

        sendIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        sendIntent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        startActivity(Intent.createChooser(sendIntent, title));
    }

    // Activity.
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d(TAG,"OnCreate()");
        s_instance = this;

        // TODO(justin): Remove for Qt6.
        QtPositioning.setContext(this);

        // Pend launch intent for pickup when Qt is ready.
        Intent intent = getIntent();
        if (intent != null)
        {
            s_intentPending = intent.getAction() != null;
        }
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        s_instance = null;
    }

    @Override
    protected void onStart()
    {
        super.onStart();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
    }

    @Override
    protected void onUserLeaveHint()
    {
        super.onUserLeaveHint();

        if (m_kioskMode)
        {
            logMessage("KIOSK_MODE: Moving task to front");
            ((ActivityManager) getApplicationContext().getSystemService(Context.ACTIVITY_SERVICE)).moveTaskToFront(getTaskId(), ActivityManager.MOVE_TASK_NO_USER_ACTION);
        }
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        registerReceiver(m_batteryChangedReceiver, s_batteryChangedIntentFilter);
    }

    @Override
    protected void onPause()
    {
        super.onPause();

        unregisterReceiver(m_batteryChangedReceiver);

        if (m_kioskMode)
        {
            ((ActivityManager) getApplicationContext().getSystemService(Context.ACTIVITY_SERVICE)).moveTaskToFront(getTaskId(), ActivityManager.MOVE_TASK_NO_USER_ACTION);
        }
    }

    @Override
    public void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);

        setIntent(intent);

        if (!s_qtReady)
        {
            s_intentPending = true;
        }
        else
        {
            s_intentPending = false;
            processIntent();
        }
    }

    private void processIntent()
    {
        Intent intent = getIntent();

        Uri intentUri;
        String intentScheme;
        String intentAction;

        // SEND or VIEW (see manifest).
        if (intent.getAction().equals("android.intent.action.VIEW"))
        {
            intentAction = "VIEW";
            intentUri = intent.getData();
        }
        else if (intent.getAction().equals("android.intent.action.SEND"))
        {
            intentAction = "SEND";
            Bundle bundle = intent.getExtras();
            intentUri = (Uri)bundle.get(Intent.EXTRA_STREAM);
        }
        else
        {
            return;
        }

        if (intentUri == null)
        {
            return;
        }

        // Content or file.
        intentScheme = intentUri.getScheme();
        if (intentScheme == null)
        {
            return;
        }

        // Applink.
        if (intentScheme.equals("https"))
        {
            fileReceived(intentUri.toString());
            return;
        }

        // URI as encoded string or applink.
        if (intentScheme.equals("file") || intentScheme.equals("https"))
        {
            fileReceived(intentUri.toString());
            return;
        }

        if (!intentScheme.equals("content"))
        {
            return;
        }

        ContentResolver cR = this.getContentResolver();
        String filePath = Utils.getRealPathFromURI(this, intentUri);
        if (filePath != null)
        {
            if (fileExists(filePath))
            {
                fileReceived(filePath);
                return;
            }
        }

        // Copy to temp folder.
        filePath = Utils.createFile(cR, intentUri, s_tempFilePath);
        if (filePath == null)
        {
            return;
        }

        fileReceived(filePath);
    }
}
