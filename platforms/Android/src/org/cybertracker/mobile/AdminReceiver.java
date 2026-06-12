package org.cybertracker.mobile;

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;

// Class implementing a device administration component that provides callbacks for lock state changes.
// For our purposes it can be left as it is. It must be declared in AndroidManifest.xml.
public class AdminReceiver extends DeviceAdminReceiver {

    @Override
    public void onEnabled(Context context, Intent intent)
    {
    }

    @Override
    public CharSequence onDisableRequested(Context context, Intent intent)
    {
        return context.getString(R.string.device_admin_disabled_warning);
    }

    @Override
    public void onDisabled(Context context, Intent intent)
    {
    }

    @Override
    public void onLockTaskModeEntering(Context context, Intent intent, String pkg)
    {
    }

    @Override
    public void onLockTaskModeExiting(Context context, Intent intent)
    {
    }
}
