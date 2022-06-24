package org.cybertracker.mobile;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.os.PowerManager;

public class LocationWakeupService extends JobService
{
    @Override
    public boolean onStartJob(JobParameters params)
    {
        if (MainActivity.instance() != null && MainActivity.tracking())
        {
            MainActivity.logMessage("Location wakeup: waking device");

            Context context = MainActivity.instance();
            PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "LocationWakeupService:WakeLock");
            wakeLock.acquire(1500);
            start();
        }

        return true;
    }

    @Override
    public boolean onStopJob(JobParameters params)
    {
        return true;
    }

    private final static long ALARM_INTERVAL_MILLIS = 55 * 60 * 1000;   // 55 minutes in milliseconds.
    private final static long ALARM_MAX_DELAY_MILLIS = 1000;
    private final static int ALARM_SCHEDULE_JOB_ID = 123457;

    public static void start()
    {
        MainActivity.logMessage("Location wakeup: start");

        Context context = MainActivity.instance();
        ComponentName serviceComponent = new ComponentName(context, LocationWakeupService.class);
        JobInfo.Builder builder = new JobInfo.Builder(ALARM_SCHEDULE_JOB_ID, serviceComponent);
        builder.setMinimumLatency(ALARM_INTERVAL_MILLIS);                            // Wait at least.
        builder.setOverrideDeadline(ALARM_INTERVAL_MILLIS + ALARM_MAX_DELAY_MILLIS); // Maximum delay.
        builder.setRequiresDeviceIdle(false);                                        // Do not care if device is idle.
        builder.setRequiresCharging(false);                                          // Do not care if the device is charging or not.

        JobScheduler jobScheduler = context.getSystemService(JobScheduler.class);
        jobScheduler.schedule(builder.build());
    }

    public static void stop()
    {
        MainActivity.logMessage("Location wakeup: stop");

        Context context = MainActivity.instance();
        JobScheduler jobScheduler = context.getSystemService(JobScheduler.class);
        jobScheduler.cancel(ALARM_SCHEDULE_JOB_ID);
    }
}
