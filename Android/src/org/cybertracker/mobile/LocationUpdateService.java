package org.cybertracker.mobile;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import androidx.core.app.NotificationCompat;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.util.Log;
import android.os.Build;
import android.os.PowerManager;

public class LocationUpdateService extends Service
{
    private static final String TAG = LocationUpdateService.class.getSimpleName();
    private static final int NOTIFICATION_ID = 12345678;
    private static final String CHANNEL_ID = "ct_location_channel_v1"; // Change when testing channel changes, since this is cached by the OS.
    private PowerManager.WakeLock m_wakeLock;

    @Override
    public void onCreate()
    {
        super.onCreate();
        Log.i(TAG, "onCreate called");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            String appName = getApplicationInfo().loadLabel(getPackageManager()).toString();
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, appName, NotificationManager.IMPORTANCE_DEFAULT);
            channel.setSound(null, null);
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(channel);
        }

        Context context = MainActivity.instance();
        if (context == null)
        {
            return;
        }

        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);

        m_wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.LOCATION_MODE_NO_CHANGE, "LocationUpdateService:WakeLock");
        m_wakeLock.acquire();
    }

    @Override
    public void onDestroy()
    {
        Log.i(TAG, "onDestroy called");

        if (m_wakeLock != null)
        {
            m_wakeLock.release();
        }

        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        Notification notification = new NotificationCompat.Builder(this, CHANNEL_ID)
            .setCategory(Notification.CATEGORY_SERVICE)
            .setOngoing(true)
            .setPriority(Notification.PRIORITY_HIGH)
            .setSmallIcon(R.drawable.track_service_icon)
            .setSound(null)
            .setWhen(System.currentTimeMillis())
            .build();

        startForeground(NOTIFICATION_ID, notification);

        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return null;
    }
}
