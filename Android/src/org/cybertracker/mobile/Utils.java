package org.cybertracker.mobile;

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.ContentUris;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Build;
import android.util.Log;
import android.database.Cursor;
import android.os.Environment;
import android.os.StatFs;
import android.support.v4.content.ContextCompat;
import android.text.TextUtils;
import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;
import java.util.Locale;
import java.util.UUID;
import java.util.regex.Pattern;
import java.lang.String;
import java.io.File;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileOutputStream;

import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_GLOBAL_ACTIONS;

public class Utils
{
    private static final String TAG = Utils.class.getSimpleName();

    // mediaScan.
    private static final String s_dummy_file = "/_dummy_";
    public static void mediaScan(Activity activity, String filePath)
    {
        try
        {
            File file = new File(filePath);
            if (file.isDirectory())
            {
                filePath = filePath + s_dummy_file;
                File tempFile = new File(filePath);
                tempFile.createNewFile();
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return;
        }

        MediaScannerConnection.scanFile(activity, new String[] { filePath }, null, new MediaScannerConnection.OnScanCompletedListener()
        {
            public void onScanCompleted(String path, Uri uri)
            {
                Log.i("mediaScan", "Scanned " + path + ":");
                Log.i("mediaScan", "-> uri=" + uri);

                if (path.endsWith(s_dummy_file))
                {
                    File file = new File(path);
                    file.delete();
                    MediaScannerConnection.scanFile(activity, new String[] { path }, null, null);
                }
            }
        });
    }

    // The public method that changes operation mode (Normal/Kiosk).
    // Activity object is the one that requests a change in Kiosk mode state.
    public static boolean setKioskMode(Activity activity, boolean mode)
    {
        if (Build.VERSION.SDK_INT < 21)
        {
            Log.d(TAG, "Kiosk mode not supported on this device");
            return true;
        }

        try
        {
            if (mode)
            {
                return enableKioskMode(activity);
            }
            else
            {

                return disableKioskMode(activity);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    //Private method for enabling Kiosk mode
    private static boolean enableKioskMode(Activity activity)
    {
        // Remove previous lock, if any.
        try
        {
            activity.stopLockTask();
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }

        try
        {
            ComponentName deviceAdmin = new ComponentName(activity, AdminReceiver.class);
            DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);

            // If app is set as device owner.
            if (dpm.isDeviceOwnerApp(activity.getPackageName()))
            {
                // Add app package to the list of packages that may enter lock task mode.
                dpm.setLockTaskPackages(deviceAdmin, new String[]{activity.getPackageName()});

                // Make MainActivity the default home activity so it will be immediately started after device boot.
                ComponentName component = new ComponentName(activity, MainActivity.class);
                IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MAIN);
                intentFilter.addCategory(Intent.CATEGORY_DEFAULT);
                intentFilter.addCategory(Intent.CATEGORY_HOME);
                intentFilter.addCategory(Intent.CATEGORY_LAUNCHER);
                dpm.addPersistentPreferredActivity(deviceAdmin, intentFilter, component);

                // Disable device keyguard, so the user will not have access to features that can bypass Kiosk mode.
                dpm.setKeyguardDisabled(deviceAdmin, true);
                
                //Allow access only to some functions of physical buttons: Shut down / Restart & Sound volume. Only available for API 28 and higher
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
                {
                    dpm.setLockTaskFeatures(deviceAdmin, LOCK_TASK_FEATURE_GLOBAL_ACTIONS );
                }
            }

            if (dpm.isLockTaskPermitted(activity.getPackageName()))
            {
                // Lock device in Kiosk mode.
                activity.startLockTask();
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    // Private method for disabling Kiosk mode.
    private static boolean disableKioskMode(Activity activity)
    {
        try
        {
            // Remove all intent handler preferences associated with the app package.
            ComponentName deviceAdmin = new ComponentName(activity, AdminReceiver.class);
            DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);

            // If app is set as device owner.
            if (dpm.isDeviceOwnerApp(activity.getPackageName()))
            {
                // Remove default status (as home activity).
                dpm.clearPackagePersistentPreferredActivities(deviceAdmin, activity.getPackageName());

                // Re-enable keyguard.
                dpm.setKeyguardDisabled(deviceAdmin, false);
            }

            // Remove app lock (disable Kiosk mode).
            activity.stopLockTask();
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    /**
     * Returns all available SD-Cards in the system (include emulated)
     *
     * Warning: Hack! Based on Android source code of version 4.3 (API 18)
     * Because there is no standart way to get it.
     * TODO: Test on future Android versions 4.4+
     *
     * @return paths to all available SD-Cards in the system (include emulated)
     */
    private static final Pattern DIR_SEPARATOR = Pattern.compile("/");

    private static String[] getStorageDirectories()
    {
        // Final set of paths
        final Set<String> rv = new HashSet<String>();
        // Primary physical SD-CARD (not emulated)
        final String rawExternalStorage = System.getenv("EXTERNAL_STORAGE");
        // All Secondary SD-CARDs (all exclude primary) separated by ":"
        final String rawSecondaryStoragesStr = System.getenv("SECONDARY_STORAGE");
        // Primary emulated SD-CARD
        final String rawEmulatedStorageTarget = System.getenv("EMULATED_STORAGE_TARGET");
        if (TextUtils.isEmpty(rawEmulatedStorageTarget))
        {
            // Device has physical external storage; use plain paths.
            if (TextUtils.isEmpty(rawExternalStorage))
            {
                // EXTERNAL_STORAGE undefined; falling back to default.
                rv.add("/storage/sdcard0");
            }
            else
            {
                rv.add(rawExternalStorage);
            }
        }
        else
        {
            // Device has emulated storage; external storage paths should have
            // userId burned into them.
            final String rawUserId;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1)
            {
                rawUserId = "";
            }
            else
            {
                final String path = Environment.getExternalStorageDirectory().getAbsolutePath();
                final String[] folders = DIR_SEPARATOR.split(path);
                final String lastFolder = folders[folders.length - 1];
                boolean isDigit = false;
                try
                {
                    Integer.valueOf(lastFolder);
                    isDigit = true;
                }
                catch (NumberFormatException ignored)
                {
                }
                rawUserId = isDigit ? lastFolder : "";
            }

            // /storage/emulated/0[1,2,...]
            if(TextUtils.isEmpty(rawUserId))
            {
                rv.add(rawEmulatedStorageTarget);
            }
            else
            {
                rv.add(rawEmulatedStorageTarget + File.separator + rawUserId);
            }
        }

        // Add all secondary storages
        if (!TextUtils.isEmpty(rawSecondaryStoragesStr))
        {
            // All Secondary SD-CARDs splited into array
            final String[] rawSecondaryStorages = rawSecondaryStoragesStr.split(File.pathSeparator);
            Collections.addAll(rv, rawSecondaryStorages);
        }

        return rv.toArray(new String[rv.size()]);
    }

    public static String[] getExternalMounts()
    {
        final HashSet<String> out = new HashSet<String>();
        String reg = "(?i).*vold.*(vfat|ntfs|exfat|fat32|ext3|ext4).*rw.*";
        String s = "";
        try
        {
            final Process process = new ProcessBuilder().command("mount").redirectErrorStream(true).start();
            process.waitFor();
            final InputStream is = process.getInputStream();
            final byte[] buffer = new byte[1024];
            while (is.read(buffer) != -1)
            {
                s = s + new String(buffer);
            }
            is.close();
        }
        catch (final Exception e)
        {
            e.printStackTrace();
        }

        // parse output
        final String[] lines = s.split("\n");
        for (String line : lines)
        {
            if (!line.toLowerCase(Locale.US).contains("asec"))
            {
                if (line.matches(reg))
                {
                    String[] parts = line.split(" ");
                    for (String part : parts)
                    {
                        if (part.startsWith("/"))
                            if (!part.toLowerCase(Locale.US).contains("vold"))
                                out.add(part);
                    }
                }
            }
        }

        return out.toArray(new String[out.size()]);
    }

    @SuppressWarnings("deprecation")
    public static String getBestSDPath()
    {
        try
        {
            ArrayList<String> folders = new ArrayList<String>();

            // Get all storage dirs
            {
                String folderStrings[] = getStorageDirectories();
                for (int i = 0; i < folderStrings.length; i++)
                {
                    folders.add(folderStrings[i]);
                }
            }

            // Get external mounts
            {
                String folderStrings[] = getExternalMounts();
                for (int i = 0; i < folderStrings.length; i++)
                {
                    folders.add(folderStrings[i]);
                }
            }

            // Get external files
            {
                File[] externalFiles = ContextCompat.getExternalFilesDirs(MainActivity.instance(), null);
                for (int i = 0; i < externalFiles.length; i++)
                {
                    if (externalFiles[i] != null)
                    {
                        folders.add(externalFiles[i].toString());
                    }
                }
            }

            class CandidatePath
            {
                public String path;
                public long freeSpace;
                public boolean emulated;
                public boolean writable;
            };

            ArrayList<CandidatePath> candidates = new ArrayList<CandidatePath>();

            // Look for the biggest.
            for (int i = 0; i < folders.size(); i++)
            {
                // Skip where cybertracker is installed
                File f = new File(folders.get(i) + "/cybertracker/version.txt");
                if (f.exists())
                {
                    continue;
                }

                // Skip non-directories
                f = new File(folders.get(i));
                if (!f.exists() || !f.isDirectory())
                {
                    continue;
                }

                // Check if this is a candidate.
                CandidatePath candidatePath = new CandidatePath();
                candidatePath.path = folders.get(i);
                candidatePath.emulated = candidatePath.path.contains("emulated");

                try
                {
                    // Get size
                    StatFs stat = new StatFs(candidatePath.path);
                    candidatePath.freeSpace = (long)(stat.getBlockSize()) * (long)(stat.getBlockCount());

                    // Check if writeable.
                    String testFileName = candidatePath.path + "/cybertrackerwritetest_" + UUID.randomUUID().toString();
                    File newDir = new File(testFileName);
                    candidatePath.writable = newDir.mkdir();
                    if (candidatePath.writable)
                    {
                        newDir.delete();
                    }
                }
                catch(Exception e)
                {
                }

                // Ignore non-candidates.
                if (!candidatePath.writable)
                {
                    continue;
                }

                // This is a candidate!
                candidates.add(candidatePath);
            }

            // Look for a big external SD.
            long maxFreeSpace = 0;
            int maxFreeIndex = -1;
            for (int i = candidates.size() - 1; i >= 0; i--)
            {
                CandidatePath c = candidates.get(i);
                if (!c.emulated && (c.freeSpace > maxFreeSpace))
                {
                    maxFreeSpace = c.freeSpace;
                    maxFreeIndex = i;
                }
            }

            if (maxFreeIndex != -1)
            {
                return candidates.get(maxFreeIndex).path;
            }

            // Look for a big emulated SD.
            maxFreeSpace = 0;
            maxFreeIndex = -1;
            for (int i = candidates.size() - 1; i >= 0; i--)
            {
                CandidatePath c = candidates.get(i);
                if (c.emulated && (c.freeSpace > maxFreeSpace))
                {
                    maxFreeSpace = c.freeSpace;
                    maxFreeIndex = i;
                }
            }

            if (maxFreeIndex != -1)
            {
                return candidates.get(maxFreeIndex).path;
            }
        }
        catch(Exception e)
        {
        }

        return "";
    }

    public static String getRealPathFromURI(final Context context, final Uri uri)
    {
        final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        // DocumentProvider
        if (isKitKat && DocumentsContract.isDocumentUri(context, uri))
        {
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri))
            {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                if ("primary".equalsIgnoreCase(type))
                {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                }
            }
            // DownloadsProvider
            else if (isDownloadsDocument(uri))
            {
                final String id = DocumentsContract.getDocumentId(uri);
                long longId = 0;
                try
                {
                    longId = Long.valueOf(id);
                }
                catch (NumberFormatException nfe)
                {
                    return getDataColumn(context, uri, null, null);
                }

                final Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downloads/public_downloads"), longId);

                return getDataColumn(context, contentUri, null, null);
            }
            // MediaProvider
            else if (isMediaDocument(uri))
            {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                Uri contentUri = null;
                if ("image".equals(type))
                {
                    contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                }
                else if ("video".equals(type))
                {
                    contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                }
                else if ("audio".equals(type))
                {
                    contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                }

                final String selection = "_id=?";
                final String[] selectionArgs = new String[]{ split[1] };

                return getDataColumn(context, contentUri, selection, selectionArgs);
            }
            else if ("content".equalsIgnoreCase(uri.getScheme()))
            {
                // Return the remote address
                if (isGooglePhotosUri(uri))
                {
                    return uri.getLastPathSegment();
                }

                return getDataColumn(context, uri, null, null);
            }
            // Other Providers
            else
            {
                try
                {
                    InputStream attachment = context.getContentResolver().openInputStream(uri);
                    if (attachment != null)
                    {
                        String filename = getContentName(context.getContentResolver(), uri);
                        if (filename != null)
                        {
                            File file = new File(context.getCacheDir(), filename);
                            FileOutputStream tmp = new FileOutputStream(file);
                            byte[] buffer = new byte[1024];
                            while (attachment.read(buffer) > 0)
                            {
                                tmp.write(buffer);
                            }
                            tmp.close();
                            attachment.close();
                            return file.getAbsolutePath();
                        }
                    }
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                    return null;
                }
            }
        }
        // MediaStore (and general)
        else if ("content".equalsIgnoreCase(uri.getScheme()))
        {
            if (isGooglePhotosUri(uri))
            {
                return uri.getLastPathSegment();
            }

            return getDataColumn(context, uri, null, null);
        }
        // File
        else if ("file".equalsIgnoreCase(uri.getScheme()))
        {
            return uri.getPath();
        }

        return null;
    }

    public static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs)
    {
        Cursor cursor = null;
        String result = null;
        final String column = "_data";
        final String[] projection = { column };

        try
        {
            cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs,null);
            if (cursor != null && cursor.moveToFirst())
            {
                final int index = cursor.getColumnIndexOrThrow(column);
                result = cursor.getString(index);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
        finally
        {
            if (cursor != null)
            {
                cursor.close();
            }
        }

        return result;
    }

    public static boolean isExternalStorageDocument(Uri uri)
    {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    public static boolean isDownloadsDocument(Uri uri)
    {
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    public static boolean isMediaDocument(Uri uri)
    {
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    public static boolean isGooglePhotosUri(Uri uri)
    {
        return "com.google.android.apps.photos.content".equals(uri.getAuthority());
    }

    public static String getContentName(ContentResolver cR, Uri uri)
    {
        Cursor cursor = cR.query(uri, null, null, null, null);
        cursor.moveToFirst();

        int nameIndex = cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME);
        if (nameIndex >= 0)
        {
            String name = cursor.getString(nameIndex);
            cursor.close();
            return name;
        }

        cursor.close();
        return null;
    }

    public static String createFile(ContentResolver cR, Uri uri, String fileLocation)
    {
        String filePath = null;
        try
        {
            InputStream iStream = cR.openInputStream(uri);
            if (iStream != null)
            {
                String name = getContentName(cR, uri);
                if (name != null)
                {
                    filePath = fileLocation + "/" + name;
                    File f = new File(filePath);
                    FileOutputStream tmp = new FileOutputStream(f);

                    byte[] buffer = new byte[1024];
                    while (iStream.read(buffer) > 0)
                    {
                        tmp.write(buffer);
                    }

                    tmp.close();
                    iStream.close();
                    return filePath;
                }
            }
        }
        catch (FileNotFoundException e)
        {
            e.printStackTrace();
            return filePath;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return filePath;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return filePath;
        }

        return filePath;
    }
}
