package org.cybertracker.mobile;

import org.qtproject.qt.android.bindings.QtService;

import android.app.Notification;
import android.app.Service;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ServiceInfo;
import android.os.IBinder;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.NotificationCompat;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.util.Log;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

public class LocationUpdateService extends QtService implements LocationListener, SensorEventListener
{
    protected final String TAG = getClass().getSimpleName();
    protected final String CHANNEL_ID = "ct_location_channel_v1"; // Change when testing channel changes, since this is cached by the OS.
    protected final int NOTIFICATION_ID = 12345678; // Should be unique.

    private PowerManager.WakeLock m_wakeLock;
    private LocationManager m_locationManager;
    private SensorManager m_sensorManager;
    private Sensor m_accelerometer;
    private boolean m_movementDetectionEnabled = false;

    public static native void logMessage(String message);

    public native void locationUpdate(double x, double y, double z, long t, double accuracy, double speed, double direction, int satelliteCount, boolean stationary);

    public String getFlavor()
    {
        return BuildConfig.FLAVOR;
    }

    @Override
    public void onCreate()
    {
        super.onCreate();

        try
        {
            logMessage("Service: onCreate");
            PowerManager powerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);
            m_wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.LOCATION_MODE_NO_CHANGE, TAG + ":WakeLock");
            m_wakeLock.acquire();
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }
    }

    @Override
    public void onDestroy()
    {
        try
        {
            logMessage("Service: onDestroy");
            stopLocationUpdates();
            m_wakeLock.release();
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }

        super.onDestroy();
    }

    private void startLocationUpdates()
    {
        try
        {
            logMessage("Service: startLocationUpdates");
            m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 1000, 0, this);
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }
    }

    private void stopLocationUpdates()
    {
        try
        {
            logMessage("Service: stopLocationUpdates");
            m_locationManager.removeUpdates(this);
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        try
        {
            m_locationManager = (LocationManager)getSystemService(Context.LOCATION_SERVICE);
            m_sensorManager = (SensorManager)getSystemService(Context.SENSOR_SERVICE);
            m_accelerometer = m_sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);

            Context context = getApplicationContext();

            SharedPreferences sharedPrefs = context.getSharedPreferences(getPackageName(), MODE_PRIVATE);
            String notifyTitle = sharedPrefs.getString(Constants.LOCATION_UPDATE_NOTIFY_TITLE, "Location updates");
            String notifyText = sharedPrefs.getString(Constants.LOCATION_UPDATE_NOTIFY_TEXT, "Logging location");
            String notifyBigText = sharedPrefs.getString(Constants.LOCATION_UPDATE_NOTIFY_BIG_TEXT, "");

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            {
                NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
                NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Background location", NotificationManager.IMPORTANCE_HIGH);
                notificationManager.createNotificationChannel(channel);
            }

            Intent i = new Intent(context, MainActivity.class);
            i.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT | Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, i, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

            NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.track_service_icon)
                .setCategory(Notification.CATEGORY_SERVICE)
                .setOngoing(true)
                .setPriority(Notification.PRIORITY_HIGH)
                .setSound(null)
                .setContentTitle(notifyTitle)
                .setContentIntent(pendingIntent);

            if (!notifyText.isEmpty())
            {
                builder.setContentText(notifyText);
            }

            if (!notifyBigText.isEmpty())
            {
                builder.setStyle(new NotificationCompat.BigTextStyle().bigText(notifyBigText));
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            {
                startForeground(NOTIFICATION_ID, builder.build(), ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
            }
            else
            {
                startForeground(NOTIFICATION_ID, builder.build());
            }

            startLocationUpdates();
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }

        return START_STICKY;
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras)
    {
    }

    @Override
    public void onProviderEnabled(String provider)
    {
    }

    @Override
    public void onProviderDisabled(String provider)
    {
        logMessage("Service: onProviderDisabled: " + provider);
    }

    @Override
    public void onLocationChanged(Location location)
    {
        if (location == null)
        {
            logMessage("Service: onLocationChanged: null");
            return;
        }

        try
        {
            locationUpdate(location.getLongitude(), location.getLatitude(), location.hasAltitude() ? location.getAltitude() : Double.NaN, location.getTime(),
                           location.hasAccuracy() ? location.getAccuracy() : Double.NaN,
                           location.hasSpeed() ? location.getSpeed() : Double.NaN,
                           location.hasBearing() ? location.getBearing() : Double.NaN,
                           0, m_movementDetectionEnabled);

            if (location.hasAccuracy())
            {
                boolean stationary = location.hasSpeed() ? location.getSpeed() == 0 : true;
                if (!m_movementDetectionEnabled && stationary)
                {
                    //logMessage("Service: Speed is 0 -> starting movement detector");
                    m_movementDetectionEnabled = true;
                    m_sensorManager.registerListener(this, m_accelerometer, SensorManager.SENSOR_DELAY_NORMAL);
                }
                else if (m_movementDetectionEnabled && !stationary)
                {
                    //logMessage("Service: Speed is > 0 -> stopping movement detector");
                    m_movementDetectionEnabled = false;
                    m_sensorManager.unregisterListener(this);
                }
            }
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event)
    {
        try
        {
            if (event.sensor.getType() == Sensor.TYPE_LINEAR_ACCELERATION)
            {
                float x = event.values[0];
                float y = event.values[1];
                float z = event.values[2];

                float diff = (float) Math.sqrt(x * x + y * y + z * z);
                if (diff > 0.5) // 0.5 is a threshold.
                {
                    //logMessage("Service: Movement detected -> stopping movement detector");
                    m_movementDetectionEnabled = false;
                    m_sensorManager.unregisterListener(this);
                }
            }
        }
        catch (Exception e)
        {
            logMessage("Exception: " + e.getMessage());
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
        // Not implemented.
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return null;
    }
}
