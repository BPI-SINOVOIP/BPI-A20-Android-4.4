/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.settings;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.backup.IBackupManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.PowerManager;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

/**
 * Gesture lock pattern settings.
 */
public class PrivacySettings extends SettingsPreferenceFragment implements
        DialogInterface.OnClickListener {

    // Vendor specific
    private static final String GSETTINGS_PROVIDER = "com.google.settings";
    private static final String BACKUP_CATEGORY = "backup_category";
    private static final String BACKUP_DATA = "backup_data";
    private static final String AUTO_RESTORE = "auto_restore";
    private static final String CONFIGURE_ACCOUNT = "configure_account";
    private static final String SYSTEM_RECOVERY = "system_recovery";
    private IBackupManager mBackupManager;
    private CheckBoxPreference mBackup;
    private CheckBoxPreference mAutoRestore;
    private PreferenceScreen mRecovery;
    private Dialog mConfirmDialog;
    private PreferenceScreen mConfigure;

    private static final int DIALOG_ERASE_BACKUP = 2;
    private int mDialogType;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.privacy_settings);
        final PreferenceScreen screen = getPreferenceScreen();

        mBackupManager = IBackupManager.Stub.asInterface(
                ServiceManager.getService(Context.BACKUP_SERVICE));

        mBackup = (CheckBoxPreference) screen.findPreference(BACKUP_DATA);
        mAutoRestore = (CheckBoxPreference) screen.findPreference(AUTO_RESTORE);
        mConfigure = (PreferenceScreen) screen.findPreference(CONFIGURE_ACCOUNT);
        mRecovery = (PreferenceScreen) screen.findPreference(SYSTEM_RECOVERY);

        // Vendor specific
        if (getActivity().getPackageManager().
                resolveContentProvider(GSETTINGS_PROVIDER, 0) == null) {
            screen.removePreference(findPreference(BACKUP_CATEGORY));
        }
        updateToggles();
    }

    @Override
    public void onResume() {
        super.onResume();

        // Refresh UI
        updateToggles();
    }

    @Override
    public void onStop() {
        if (mConfirmDialog != null && mConfirmDialog.isShowing()) {
            mConfirmDialog.dismiss();
        }
        mConfirmDialog = null;
        mDialogType = 0;
        super.onStop();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        if (preference == mBackup) {
            if (!mBackup.isChecked()) {
                showEraseBackupDialog();
            } else {
                setBackupEnabled(true);
            }
        } else if (preference == mAutoRestore) {
            boolean curState = mAutoRestore.isChecked();
            try {
                mBackupManager.setAutoRestore(curState);
            } catch (RemoteException e) {
                mAutoRestore.setChecked(!curState);
            }
        } else if(preference == mRecovery) {
						showRecoveryDialog();
						}
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    private void showEraseBackupDialog() {
        mBackup.setChecked(true);

        mDialogType = DIALOG_ERASE_BACKUP;
        CharSequence msg = getResources().getText(R.string.backup_erase_dialog_message);
        // TODO: DialogFragment?
        mConfirmDialog = new AlertDialog.Builder(getActivity()).setMessage(msg)
                .setTitle(R.string.backup_erase_dialog_title)
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setPositiveButton(android.R.string.ok, this)
                .setNegativeButton(android.R.string.cancel, this)
                .show();
    }

		private void showRecoveryDialog() {	
				mConfirmDialog = new AlertDialog.Builder(getActivity()).setTitle(R.string.system_recovery_title)
								.setMessage(getActivity().getString(R.string.system_recovery_dialog_massage))
								.setPositiveButton(android.R.string.ok,new DialogInterface.OnClickListener()
				{
								@Override
								public void onClick(DialogInterface dialog, int which)
								{
												Intent intent = new Intent();
												intent.setAction("android.need.alarm.reset");
												getActivity().sendBroadcast(intent);
				
												try
												{
																Thread.sleep(2);
												}
												catch(Exception e)
												{}

												rebootRecovery();
									}
				})
				.setNegativeButton(android.R.string.cancel,this)
				.show();
			}

			private void rebootRecovery()
			{
							File RECOVERY_DIR = new File("/cache/recovery");
							File COMMAND_FILE = new File(RECOVERY_DIR, "command");
							String SHOW_TEXT="--show_text";
							RECOVERY_DIR.mkdirs();
							// In case we need it         		 COMMAND_FILE.delete();
							// In case it's not writable
							try
							{
											FileWriter command = new FileWriter(COMMAND_FILE);
											command.write(SHOW_TEXT);
											command.write("\n");
											command.close();
							}
							catch (IOException e)
							{
											// TODO Auto-generated catch block
											e.printStackTrace();
							}

							PowerManager pm = (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);
							pm.reboot("recovery");
					}

    /*
     * Creates toggles for each available location provider
     */
    private void updateToggles() {
        ContentResolver res = getContentResolver();

        boolean backupEnabled = false;
        Intent configIntent = null;
        String configSummary = null;
        try {
            backupEnabled = mBackupManager.isBackupEnabled();
            String transport = mBackupManager.getCurrentTransport();
            configIntent = mBackupManager.getConfigurationIntent(transport);
            configSummary = mBackupManager.getDestinationString(transport);
        } catch (RemoteException e) {
            // leave it 'false' and disable the UI; there's no backup manager
            mBackup.setEnabled(false);
        }
        mBackup.setChecked(backupEnabled);

        mAutoRestore.setChecked(Settings.Secure.getInt(res,
                Settings.Secure.BACKUP_AUTO_RESTORE, 1) == 1);
        mAutoRestore.setEnabled(backupEnabled);

        final boolean configureEnabled = (configIntent != null) && backupEnabled;
        mConfigure.setEnabled(configureEnabled);
        mConfigure.setIntent(configIntent);
        setConfigureSummary(configSummary);
}

    private void setConfigureSummary(String summary) {
        if (summary != null) {
            mConfigure.setSummary(summary);
        } else {
            mConfigure.setSummary(R.string.backup_configure_account_default_summary);
        }
    }

    private void updateConfigureSummary() {
        try {
            String transport = mBackupManager.getCurrentTransport();
            String summary = mBackupManager.getDestinationString(transport);
            setConfigureSummary(summary);
        } catch (RemoteException e) {
            // Not much we can do here
        }
    }

    public void onClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            //updateProviders();
            if (mDialogType == DIALOG_ERASE_BACKUP) {
                setBackupEnabled(false);
                updateConfigureSummary();
            }
        }
        mDialogType = 0;
    }

    /**
     * Informs the BackupManager of a change in backup state - if backup is disabled,
     * the data on the server will be erased.
     * @param enable whether to enable backup
     */
    private void setBackupEnabled(boolean enable) {
        if (mBackupManager != null) {
            try {
                mBackupManager.setBackupEnabled(enable);
            } catch (RemoteException e) {
                mBackup.setChecked(!enable);
                mAutoRestore.setEnabled(!enable);
                return;
            }
        }
        mBackup.setChecked(enable);
        mAutoRestore.setEnabled(enable);
        mConfigure.setEnabled(enable);
    }

    @Override
    protected int getHelpResource() {
        return R.string.help_url_backup_reset;
    }
}
