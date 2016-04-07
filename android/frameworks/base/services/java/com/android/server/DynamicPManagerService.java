/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.Binder;
import android.util.Log;

import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.FileWriter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.LinkedList;
import java.util.Iterator;

public class DynamicPManagerService extends Binder {
    private static final String TAG = "DynamicPManagerService";

    private final Context mContext;
    private final ActivityManager mActivityManager;
    private final PolicyChangedThread mMainThread;

    LinkedList<String> mAppList;

    /**
     * cpufreq governor
     */
    private final String CPUFREQ_GOVERNOR_FILE = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
    private final String FANTASYS = "fantasys";

    /**
     * powernow
     */
    private final String POWERNOW_MODE_FILE = "/sys/class/sw_powernow/mode";
    private final String EXTREMITY = "extremity";
    private final String MAX_POWER = "maxpower";
    private final String USER_EVENT = "userevent";
    private final String PERFORMANCE = "performance";

    private boolean loadAppList() {
        mAppList = new LinkedList<String>();

        try {
            BufferedReader br = new BufferedReader(new InputStreamReader(
                        mContext.getAssets().open("app_list.conf")));
            String line;
            while (true) {
                line = br.readLine();
                if (line == null)
                    break;

                mAppList.add(line);
            }

            return true;
        } catch (IOException e) {
            Log.e(TAG, "read app list error: " + e.getMessage());

            /* default app list */
            mAppList.add("com.antutu.ABenchMark");

            return false;
        }
    }

    private final String ACTION_WINDOW_ROTATION = "android.window.action.ROTATION";
    public DynamicPManagerService(Context context) {
        mContext = context;
        mActivityManager = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mMainThread = new PolicyChangedThread();

        Log.i(TAG, "loading app list");
        loadAppList();

        Log.i(TAG, "register boot completed receiver");
        mContext.registerReceiver(mBootCompletedReceiver,
                new IntentFilter(Intent.ACTION_BOOT_COMPLETED), null, null);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_WINDOW_ROTATION);
        mContext.registerReceiver(mWindowRotationReceiver, intentFilter);
    }


    /**
     * Boot completed receiver
     */
    private BroadcastReceiver mBootCompletedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
                Log.i(TAG, "boot completed");
                /* switch cpufreq governor*/
                setCpufreqGovernor(FANTASYS);
                setPowernowMode(PERFORMANCE);

                /* start policy changed thread */
                if (!mMainThread.isAlive())
                    mMainThread.start();
            }
        }
    };

    /**
     * Window rotation receiver
     */
    public static boolean mGlobleLauncherRotateState = false;
    private BroadcastReceiver mWindowRotationReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            /* Max Power */
            setPowernowMode(MAX_POWER);

            String launcherName = "com.android.launcher";
            if (getTopAppName().equals(launcherName)) {
                Log.i(TAG, "changing launcher cpu group");
                setAppTaskGroup(launcherName, "/dev/cpuctl/tasks");
            }
        }
    };

    public boolean setCpufreqGovernor(String governor) {
        try {
            FileOutputStream fos = new FileOutputStream(CPUFREQ_GOVERNOR_FILE);
            fos.write(governor.getBytes());
            fos.close();
            return true;
        } catch (Exception e) {
            Log.e(TAG, "set cpufreq governor error: " + e.getMessage());
            return false;
        }
    }

    public boolean setPowernowMode(String mode) {
        try {
            FileOutputStream fos = new FileOutputStream(POWERNOW_MODE_FILE);
            fos.write(mode.getBytes());
            fos.close();
            return true;
        } catch (Exception e) {
            Log.e(TAG, "set powernow mode error: " + e.getMessage());
            return false;
        }
    }

    public String getTopAppName() {
        return mActivityManager.getRunningTasks(1).get(0).topActivity.getPackageName();
    }

    public boolean getTopAppHit() {
        String topAppName = getTopAppName();

        for (int i = 0; i < mAppList.size(); i++) {
            if (mAppList.get(i).equals(topAppName))
                return true;
        }

        return false;
    }

    public boolean setAppTaskGroup(String appName, String taskFile) {
        Iterator appIter = mActivityManager.getRunningAppProcesses().iterator();

        try {
            String processName = mContext.getPackageManager().getApplicationInfo(appName, 0).processName;
            ActivityManager.RunningAppProcessInfo info = (ActivityManager.RunningAppProcessInfo)appIter.next();
            while (appIter.hasNext()) {
                if (info.processName.equals(processName)) {
                    FileWriter fw = new FileWriter(taskFile, true);
                    fw.write(String.valueOf(info.pid));
                    fw.close();
                    Log.i(TAG, "set " + appName + " task group done");
                    break;
                }
            }

            return true;
        } catch (Exception e) {
            Log.e(TAG, "set " + appName + " task group error: " + e.getMessage());
            return false;
        }
    }

    public class PolicyChangedThread extends Thread {
        private static final String TAG = "PolicyChangedThread";

        /**
         * Indicates policy whether need to changed or not.
         */
        private boolean mNeedChanged = false;

        /**
         * Dynamic power manager main thread.
         */
        public void run() {
            Log.i(TAG, "dynamic power manager main thread running");
            while (true) {
                boolean isHit = false;
                String topAppName = getTopAppName();

                for (int i = 0; i < mAppList.size(); i++) {
                    if (mAppList.get(i).equals(topAppName)) {
                        isHit = true;
                        break;
                    }
                }

                if (isHit && !mNeedChanged) {
                    /* Update powernow mode */
                    setPowernowMode(EXTREMITY);

                    if (setAppTaskGroup(topAppName, "/dev/cpuctl/tasks")) {
                        mNeedChanged = true;
                    }
                } else if (!isHit && mNeedChanged) {
                    mNeedChanged = false;
                    setPowernowMode(PERFORMANCE);
                }

                try {
                    Thread.sleep(1500);
                } catch (InterruptedException e) {
                    Log.e(TAG, "sleep at dynamic power manager main thread error: " + e.getMessage());
                }
            }
        }
    }
}
