package org.cybertracker.mobile;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.os.PowerManager;

public class AlarmJobService extends JobService
{
    @Override
    public boolean onStartJob(JobParameters params)
    {
        if (MainActivity.instance() == null)
        {
            return false;
        }

        MainActivity.logMessage("Alarm fired");

        Context context = MainActivity.instance();
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "AlarmJobService:WakeLock");
        wakeLock.acquire(1500);

        MainActivity.alarmFired();

        return true;
    }

    @Override
    public boolean onStopJob(JobParameters params)
    {
        return true;
    }

    private final static int ALARM_JOB_ID = 123458;

    public static void setAlarm(int seconds)
    {
        MainActivity.logMessage("Alarm started");

        Context context = MainActivity.instance();
        ComponentName serviceComponent = new ComponentName(context, AlarmJobService.class);
        JobInfo.Builder builder = new JobInfo.Builder(ALARM_JOB_ID, serviceComponent);
        builder.setMinimumLatency(seconds * 1000);          // Wait at least.
        builder.setOverrideDeadline(1000 + seconds * 1000); // Maximum delay.
        builder.setRequiresDeviceIdle(false);               // Do not care if device is idle.
        builder.setRequiresCharging(false);                 // Do not care if the device is charging or not.

        JobScheduler jobScheduler = context.getSystemService(JobScheduler.class);
        jobScheduler.cancel(ALARM_JOB_ID);
        jobScheduler.schedule(builder.build());
    }

    public static void killAlarm()
    {
        MainActivity.logMessage("Alarm cancelled");

        Context context = MainActivity.instance();
        JobScheduler jobScheduler = context.getSystemService(JobScheduler.class);
        jobScheduler.cancel(ALARM_JOB_ID);
    }
}
