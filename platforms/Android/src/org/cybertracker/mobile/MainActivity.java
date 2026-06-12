package org.cybertracker.mobile;

import org.qtproject.qt.android.bindings.QtActivity;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.content.res.AssetFileDescriptor;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.AsyncTask;
import android.os.Process;
import android.os.Environment;
import android.os.PowerManager;
import android.net.Uri;
import android.net.ConnectivityManager;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.provider.MediaStore;
import android.telephony.TelephonyManager;
import android.media.AudioManager;
import android.util.Log;
import android.view.View;
import android.view.Window;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import androidx.core.app.ShareCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKeys;
import com.google.android.play.core.review.ReviewInfo;
import com.google.android.play.core.review.ReviewManager;
import com.google.android.play.core.review.ReviewManagerFactory;
import java.io.File;
import java.io.FileOutputStream;
import java.util.Objects;

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

    public static native void alarmFired();

    public static native boolean fileExists(String filePath);
    public static native void fileReceived(String filePathUrl);

    public static native void imagePickComplete(String fileUrl);

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

    public String getModel()
    {
        return android.os.Build.MODEL;
    }

    public String getManufacturer()
    {
        return android.os.Build.MANUFACTURER;
    }

    public String getDeviceUid()
    {
        String id = Settings.Secure.getString(getContentResolver(), Secure.ANDROID_ID);

        if (!id.isEmpty())
        {
            // Pad to uuid length.
            id = id + ("00000000000000000000000000000000".substring(id.length()));
        }

        return id;
    }

    public String saveMedia(String filePath, String filename, String mimeType, String directoryName)
    {
        File originalFile = new File(filePath);

        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.TITLE, filename);
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, filename);
        values.put(MediaStore.MediaColumns.DATE_ADDED, System.currentTimeMillis() / 1000);
        values.put(MediaStore.MediaColumns.MIME_TYPE, mimeType);
        Uri externalContentUri = externalContentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;

        // Android 10 restricts our access to the raw filesystem, use MediaStore to save media in that case
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
        {
            values.put(MediaStore.MediaColumns.RELATIVE_PATH, "DCIM/" + directoryName);
            values.put(MediaStore.MediaColumns.DATE_TAKEN, System.currentTimeMillis());
            values.put(MediaStore.MediaColumns.IS_PENDING, true);

            Uri uri = getContentResolver().insert(externalContentUri, values);
            if (uri != null)
            {
                try
                {
                    if (Utils.WriteFileToStream(originalFile, getContentResolver().openOutputStream(uri)))
                    {
                        values.put(MediaStore.MediaColumns.IS_PENDING, false);
                        getContentResolver().update(uri, values, null, null);
                    }
                }
                catch (Exception e)
                {
                    logMessage("Exception: " + e.getMessage());
                    getContentResolver().delete(uri, null, null);
                    return "";
                }

                return uri.toString();
            }
        }
        else
        {
            File directory = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM), directoryName);
            directory.mkdirs();

            File file = new File(directory, filename);
            try
            {
                if (Utils.WriteFileToStream(originalFile, new FileOutputStream(file)))
                {
                    values.put(MediaStore.MediaColumns.DATA, file.getAbsolutePath());
                    getContentResolver().insert(externalContentUri, values);

                    Utils.mediaScan(this, file.getPath());
                }
            }
            catch (Exception e)
            {
                logMessage("Exception: " + e.getMessage());
                return "";
            }

            return file.getAbsolutePath();
        }

        return "";
    }

    public boolean deleteMedia(String filePath)
    {
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
        {
            try
            {
                return getContentResolver().delete(Uri.parse(filePath), null, null) == 1;
            }
            catch (Exception e)
            {
                logMessage("Exception: " + e.getMessage());
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    public boolean hasContentUri(String uriString)
    {
        Uri uri = Uri.parse(uriString);

        try (AssetFileDescriptor afd = getContentResolver().openAssetFileDescriptor(uri, "r")) 
        {
            return afd != null;
        } 
        catch (Exception e) 
        {
            return false;
        }        
    }

    public String resolveContentUri(final String uri)
    {
        return Utils.getContentName(getContentResolver(), Uri.parse(uri));
    }

    public String copyToDownload(String filePath, String folder, String mimeType)
    {
        try
        {
            return Utils.copyToDownload(getContentResolver(), filePath, folder, mimeType);
        }
        catch (Exception e)
        {
            logMessage("copyToDownload exception: " + e.getMessage());
            return "";
        }
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

    private static final int PICK_IMAGE_REQUEST_CODE = 5123;
    public void startImagePicker()
    {
        Intent intent = new Intent(Intent.ACTION_PICK);
        intent.setType("image/*");
        startActivityForResult(intent, PICK_IMAGE_REQUEST_CODE);
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

    private static final String CHANNEL_ID = "ct_alert_channel_v1"; // Change when testing channel changes, since this is cached by the OS.
    private static final int NOTIFICATION_ID = 12345679; // Should be unique.

    public void showAlert(final String contentTitle, final String contentText, final String bigText)
    {
        NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
        if (!notificationManager.areNotificationsEnabled())
        {
            logMessage("showAlert: not enabled");
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Notifications", NotificationManager.IMPORTANCE_DEFAULT);
            notificationManager.createNotificationChannel(channel);
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_dialog_alert)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setContentTitle(contentTitle)
            .setAutoCancel(false);

        if (!contentText.isEmpty())
        {
            builder.setContentText(contentText);
        }

        if (!bigText.isEmpty())
        {
            builder.setStyle(new NotificationCompat.BigTextStyle().bigText(bigText));
        }

        notificationManager.notify(NOTIFICATION_ID, builder.build());
    }

    public void hideAlert()
    {
        NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
        notificationManager.cancel(NOTIFICATION_ID);
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
    public void startLocationUpdates(final String title, final String text, final String bigText)
    {
        logMessage("startLocationUpdates");

        SharedPreferences sharedPrefs = getApplicationContext().getSharedPreferences(getPackageName(), MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPrefs.edit();
        editor.putString(Constants.LOCATION_UPDATE_NOTIFY_TITLE, title);
        editor.putString(Constants.LOCATION_UPDATE_NOTIFY_TEXT, text);
        editor.putString(Constants.LOCATION_UPDATE_NOTIFY_BIG_TEXT, bigText);
        editor.commit();

        ContextCompat.startForegroundService(this, new Intent(this, LocationUpdateService.class));
    }

    public void stopLocationUpdates()
    {
        logMessage("stopLocationUpdates");

        stopService(new Intent(this, LocationUpdateService.class));
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

    // Google Play in-app review.
    public void requestReview()
    {
        runOnUiThread(new Runnable()
        {
            public void run()
            {
                final ReviewManager manager = ReviewManagerFactory.create(MainActivity.this);
                manager.requestReviewFlow().addOnCompleteListener(task ->
                {
                    if (task.isSuccessful())
                    {
                        ReviewInfo reviewInfo = task.getResult();
                        manager.launchReviewFlow(MainActivity.this, reviewInfo);
                        // The flow outcome is intentionally undetectable per the API.
                    }
                    else
                    {
                        logMessage("requestReview failed: " + task.getException());
                    }
                });
            }
        });
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

    // Encrypt and decrypt a string.
    private static final String VAULT_NAME = "vault";

    public void setVaultValue(String key, String value)
    {
        try
        {
            SharedPreferences prefs = EncryptedSharedPreferences.create(
                    VAULT_NAME,
                    MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC),
                    this,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM);

            prefs.edit().putString(key, value).apply();

        }
        catch (Exception e)
        {
            Log.e(TAG, "Error encrypting and storing string", e);
        }
    }

    public String getVaultValue(String key)
    {
        try
        {
            SharedPreferences prefs = EncryptedSharedPreferences.create(
                    VAULT_NAME,
                    MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC),
                    this,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM);

            return prefs.getString(key, "");
        }
        catch (Exception e)
        {
            Log.e(TAG, "Error retrieving encrypted string", e);
            return "";
        }
    }

    public void setDarkTheme(final boolean dark)
    {
        runOnUiThread(new Runnable()
        {
            public void run()
            {
                Window window = getWindow();
                View decorView = window.getDecorView();

                window.setStatusBarColor(dark ? Color.BLACK : Color.WHITE);

                WindowInsetsControllerCompat insetsController = WindowCompat.getInsetsController(window, decorView);
                if (insetsController != null)
                {
                    insetsController.setAppearanceLightStatusBars(!dark);
                }
            }
        });
    }

    // Activity.
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        s_instance = this;

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
        Log.d(TAG, "onDestroy");
        s_instance = null;

        // Terminate the activity process to ensure a clean start when the service is running.
        android.os.Process.killProcess(android.os.Process.myPid());

        super.onDestroy();
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        Log.d(TAG, "onStart");
    }

    @Override
    protected void onStop()
    {
        Log.d(TAG, "onStop");
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
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        super.onRestoreInstanceState(savedInstanceState);
        Log.d(TAG, "onRestoreInstanceState");
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        Log.d(TAG, "onResume");

        try
        {
            registerReceiver(m_batteryChangedReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        }
        catch (Exception e)
        {
            Log.d(TAG, e.getMessage());
        }
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        Log.d(TAG, "onPause");

        try
        {
            unregisterReceiver(m_batteryChangedReceiver);
        }
        catch (Exception e)
        {
            Log.d(TAG, e.getMessage());
        }

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

        // SEND or VIEW (see manifest).
        if (intent.getAction() == null)
        {
            return;
        }
        else if (intent.getAction().equals("android.intent.action.VIEW"))
        {
            intentUri = intent.getData();
        }
        else if (intent.getAction().equals("android.intent.action.SEND"))
        {
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

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == PICK_IMAGE_REQUEST_CODE)
        {
            if (resultCode == Activity.RESULT_OK && data != null && data.getData() != null)
            {
                imagePickComplete(data.getData().toString());
            }
            else
            {
                imagePickComplete("");
            }
        }
    }

    // For Bluetooth audio recording.
    private AudioManager m_audioManager;
    private boolean m_audioStarted = false;
    private boolean m_scoConnected = false;

    public void startSco() 
    {
        if (m_audioStarted)
        {
            stopSco();
        }

        if (m_audioManager == null)
        {
            m_audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

            IntentFilter filter = new IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED);

            registerReceiver(new BroadcastReceiver() 
            {
                @Override
                public void onReceive(Context context, Intent intent) 
                {
                    int state = intent.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, -1);
                    m_scoConnected = (state == AudioManager.SCO_AUDIO_STATE_CONNECTED);
                }
            }, filter);
        }

        m_audioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
        m_audioManager.startBluetoothSco();
        m_audioManager.setBluetoothScoOn(true);

        m_audioStarted = true;
    }

    public void stopSco() 
    {
        if (!m_audioStarted)
        {
            return;
        }

        m_audioManager.setBluetoothScoOn(false);
        m_audioManager.stopBluetoothSco();
        m_audioManager.setMode(AudioManager.MODE_NORMAL);

        m_audioStarted = m_scoConnected = false;
    }

    public boolean isScoConnected() 
    {
        return m_audioStarted && m_scoConnected;
    }
}
