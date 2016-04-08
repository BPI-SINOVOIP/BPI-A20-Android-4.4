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

package com.android.provision;

import android.app.Activity;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ResolveInfo;
import java.util.List;

/**
 * Application that sets the provisioned bit, like SetupWizard does.
 */
public class DefaultActivity extends Activity {

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        // Add a persistent setting to allow other apps to know the device has been provisioned.
        Settings.Global.putInt(getContentResolver(), Settings.Global.DEVICE_PROVISIONED, 1);
        Settings.Secure.putInt(getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE, 1);

        // remove this activity from the package manager.
        PackageManager pm = getPackageManager();
/*
		String examplePackageName = "com.google.android.googlequicksearchbox";
		String exampleActivityName = "com.google.android.launcher.GEL";
		String TAG = "DefaultActivity";
		   
		Intent intent=new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
				  
		List<ResolveInfo> resolveInfoList = pm.queryIntentActivities(intent, 0);
		if(resolveInfoList != null) {
			int size = resolveInfoList.size();
			for(int j=0; j<size; j++) {
			final ResolveInfo r = resolveInfoList.get(j);
			if(r.activityInfo.packageName.equals(this.getPackageName())) {
					resolveInfoList.remove(j);
					size -= 1;
					j--;
					//Log.d(TAG, ">>>>>>>>>>>>>>>>>>>>>>>remove:" + r.activityInfo.packageName);
				}
			}
			//Log.d(TAG,">>>>>>>>>>>>>>>>>>>>>>>>>>size:" + size);
			ComponentName[] set = new ComponentName[size];
			ComponentName defaultLauncher=new ComponentName(examplePackageName, exampleActivityName);
			int defaultMatch=0;
			for(int i=0; i<size; i++) {
				final ResolveInfo resolveInfo = resolveInfoList.get(i);
				//Log.d(TAG, ">>>>>>>>>>>>>>>>" + resolveInfo.toString());
				set[i] = new ComponentName(resolveInfo.activityInfo.packageName, resolveInfo.activityInfo.name);
				if(defaultLauncher.getClassName().equals(resolveInfo.activityInfo.name)) {
					defaultMatch = resolveInfo.match;
				}
			}
			Log.d(TAG,"defaultMatch="+Integer.toHexString(defaultMatch));
			IntentFilter filter=new IntentFilter();
			filter.addAction(Intent.ACTION_MAIN);
			filter.addCategory(Intent.CATEGORY_HOME);
			filter.addCategory(Intent.CATEGORY_DEFAULT);

			pm.clearPackagePreferredActivities(defaultLauncher.getPackageName());
			pm.addPreferredActivity(filter, defaultMatch,set,defaultLauncher);
		}*/


		
        ComponentName name = new ComponentName(this, DefaultActivity.class);
        pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);

		setDefaultBrowser(this);
        // terminate the activity.
        finish();
    }

	private void setDefaultBrowser(Context context) {
		PackageManager pm = context.getPackageManager();

		ComponentName defaultBrowser = new ComponentName("com.android.chrome", "com.google.android.apps.chrome.Main");
		ComponentName[] set = {new ComponentName("com.android.browser", "com.android.browser.BrowserActivity"), defaultBrowser };

		IntentFilter filter = new IntentFilter();
		filter.addAction(Intent.ACTION_VIEW);
		filter.addCategory(Intent.CATEGORY_DEFAULT);
		filter.addDataScheme("http");

		pm.clearPackagePreferredActivities(defaultBrowser.getPackageName());
		pm.addPreferredActivity(filter, IntentFilter.MATCH_CATEGORY_SCHEME, set, defaultBrowser);
	}
}

