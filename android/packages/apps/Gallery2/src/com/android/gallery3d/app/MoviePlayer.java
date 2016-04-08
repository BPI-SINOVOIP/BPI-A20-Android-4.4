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

package com.android.gallery3d.app;

import java.io.File;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Resources;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.TimedText;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.Virtualizer;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
//import android.widget.VideoView;
import com.android.gallery3d.app.VideoView;
import com.android.gallery3d.app.SlipSwitchDialog.OnSwitchResultListener;
//import com.android.gallery3d.app.MovieViewControl.AwHdmiPluggedReceiver;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.content.IContentProvider;
import android.content.ContentResolver;
import android.provider.MediaStore;
import android.provider.MediaStore.Video;
import android.provider.Settings;

import java.util.Collections;
import java.util.List;
import java.util.ArrayList;

import com.android.gallery3d.R;
import com.android.gallery3d.common.ApiHelper;
import com.android.gallery3d.common.BlobCache;
import com.android.gallery3d.util.CacheManager;
import com.android.gallery3d.util.GalleryUtils;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.UnsupportedEncodingException;

import android.hardware.display.DisplayManager;
import android.view.View.OnTouchListener;
import android.view.IWindowManager;
import android.os.ServiceManager;
import android.hardware.input.InputManager;
//import android.media.MediaPlayer.SubInfo;
//import android.media.MediaPlayer.TrackInfoVendor;
import android.media.MediaPlayer.TrackInfo;
import java.util.Random;
import android.os.Debug;
import android.view.Window;
import android.view.WindowManager;
import android.view.Display;



public class MoviePlayer implements
        MediaPlayer.OnErrorListener, MediaPlayer.OnCompletionListener, /*GestureDetector.OnGestureListener,*/
        MediaPlayer.OnTimedTextListener, ControllerOverlay.Listener,VideoView.OnSubFocusItems{
    @SuppressWarnings("unused")
    private static final String TAG = "MoviePlayer";

    private static final String KEY_VIDEO_POSITION = "video-position";
    private static final String KEY_RESUMEABLE_TIME = "resumeable-timeout";

    // These are constants in KeyEvent, appearing on API level 11.
    private static final int KEYCODE_MEDIA_PLAY = 126;
    private static final int KEYCODE_MEDIA_PAUSE = 127;

    // Copied from MediaPlaybackService in the Music Player app.
    private static final String SERVICECMD = "com.android.music.musicservicecommand";
    private static final String CMDNAME = "command";
    private static final String CMDPAUSE = "pause";

    private static final String VIRTUALIZE_EXTRA = "virtualize";
    private final View mProgressView;
    private final ContentResolver mContentResolver;
    private Resources mRes;
    private final SharedPreferences sp;
    private final SharedPreferences.Editor editor;  
    private PowerManager.WakeLock mWakeLock;
	private static final String STORE_NAME = "SubtitleSetting";
	private SlipSwitchDialog mSlipSwitch;
    private TextView mDialogTitle;
	private static final String EDITOR_SUBGATE = "MovieViewControl.SUBGATE";
    private static final String EDITOR_SUBSELECT = "MovieViewControl.SUBSELECT";
    private static final String EDITOR_ZOOM = "MovieViewControl.MODEZOOM";
    private static final String EDITOR_MODE3D = "MovieViewControl.MODE3D";
    private static final String EDITOR_TRACK = "MovieViewControl.TRACK";
    private static final String EDITOR_REPEAT = "MovieViewControl.REPEAT";
    private static final String EDITOR_SUBCHARSET = "MovieViewControl.SUBCHARSET";

	
	//* Display type: Built-in display, HDMI display
	public static final int DISPLAY_3D_TYPE_BUILT_IN = 1;
    public static final int DISPLAY_3D_TYPE_HDMI = 2;
	
	private int mDisplay3DType =  DISPLAY_3D_TYPE_HDMI;
    
    private String mControlFocus = null;
    private int mListFocus = 0, m3DFocus = 2;
    private ListView mListView;
    private Dialog mListDialog;
    private static final int PLAYMODE_REPEAT_ALL = 0;
    private int mRepeatMode = PLAYMODE_REPEAT_ALL;
    private static final int PLAYMODE_SEQUENCE = 1;
    private static final int PLAYMODE_REPEAT_ONE = 2;
    private static final int PLAYMODE_RANDOM = 3;
    private int mCurrentIndex = 0;
    private String mPlayListType;
    private ArrayList<String> mPlayList;
	private boolean mOnPause = false;
    private View mDialogView;
    private static MoviePlayer mInstance = null;
    private boolean mToQuit = false;
    private int mCurrentTrackSave, mCurrentSubSave;		//used for video onPause/onResume
    private boolean mShowAble = false;
    //GestureDetector mGestureDetector = new GestureDetector(this);
    Activity mMovieActivity = null;
    //private AudioManager mAudiomanage = null;
    private final float mTolerenceY = 1.5f;
    private final float mTolerenceX = 2.0f;
    private float mAccumulateY = 0.0f;
	/*
    @Override
	  public boolean onDown(MotionEvent e) {
	   // TODO Auto-generated method stub
	    return true;
	  }
	  @Override
	  public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
	    float velocityY) {
	   // TODO Auto-generated method stub
	   return false;
	  }
	  @Override
	  public void onLongPress(MotionEvent e) {
	   // TODO Auto-generated method stub
	   
	  }
	  @Override
	  public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
	   // TODO Auto-generated method stub
		  Window win = mMovieActivity.getWindow();
		  Point outSize = new Point();
		  mMovieActivity.getWindowManager().getDefaultDisplay().getSize(outSize);
		  int screenWidth = outSize.x;
		  int screenHeight = outSize.y;
		  float pointDownX = e1.getRawX();
		  if (Math.abs(distanceY) > Math.abs(distanceX)) {
			  mHandler.removeCallbacks(mHideStatusBar);
			  if (pointDownX > 0.6 * screenWidth) {
				  if (distanceY > mTolerenceY || distanceY < -mTolerenceY) {
					  WindowManager.LayoutParams layoutParams = win.getAttributes();
					  Log.d(TAG, "");
					  if (layoutParams.screenBrightness < 0) {
						  try {
							  layoutParams.screenBrightness = Settings.System.getInt(mMovieActivity.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS) / 255.0f;  
						  } catch(Exception localException) {
							  layoutParams.screenBrightness = 0.5f;
						  }
						  Log.d(TAG, "<0#############screenBrightness###################:" + layoutParams.screenBrightness);
					  }
					  //Log.d(TAG, "1################screenBrightness:" + layoutParams.screenBrightness);
			          layoutParams.screenBrightness += (distanceY / 1000);
			          //Log.d(TAG, "2################screenBrightness:" + layoutParams.screenBrightness);
			          if (layoutParams.screenBrightness > 1.0) {
			        	  layoutParams.screenBrightness = 1.0f;
			          } else if (layoutParams.screenBrightness < 0.0f) {
			        	  layoutParams.screenBrightness = 0.0f;
			          }
			          
			          
			          float percent = layoutParams.screenBrightness * 100;
			          int percents = (int) percent;
			          String text = mMovieActivity.getString(R.string.Brightness);
			          mVolumeView.setVisibility(View.GONE);
			          mBrightView.setVisibility(View.VISIBLE);
			          mBrightView.setText(text + ":" + percents + "%");
			          win.setAttributes(layoutParams);
				  }
			  } else if (pointDownX < 0.3 * screenWidth) {
				  if (Math.abs(distanceY) > mTolerenceY) {
					  int maxVolume = mAudiomanage.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
					  String text = mMovieActivity.getString(R.string.Volume);
					  mBrightView.setVisibility(View.GONE);
					  mVolumeView.setVisibility(View.VISIBLE);
					  mAccumulateY += distanceY;
					  if (mAccumulateY > 60) {
						  mAudiomanage.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, 0);
						  mAccumulateY = 0;
						  int percents = 100 * mAudiomanage.getStreamVolume(AudioManager.STREAM_MUSIC) / maxVolume;
						  if (percents > 100) {
							  percents = 100;
						  } else if (percents < 0) {
							  percents = 0;
						  }
						  mVolumeView.setText(text + ":" + percents + "%");
					  } else if (mAccumulateY < -60){
						  mAudiomanage.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_LOWER, 0);
						  mAccumulateY = 0;
						  int percents = 100 * mAudiomanage.getStreamVolume(AudioManager.STREAM_MUSIC) / maxVolume;
						  if (percents > 100) {
							  percents = 100;
						  } else if (percents < 0) {
							  percents = 0;
						  }
						  mVolumeView.setText(text + ":" + percents + "%");
					  } else {
						  int AccumulateY = (int) mAccumulateY;
						  int currentVolume = mAudiomanage.getStreamVolume(AudioManager.STREAM_MUSIC);
						  int percents = 100 * currentVolume/maxVolume + AccumulateY/10;
						  if (percents > 100) {
							  percents = 100;
						  } else if (percents < 0) {
							  percents = 0;
						  }
						  mVolumeView.setText(text + ":" + percents + "%");
					  }
				  }
			  }
		  } else {
			  
		  }
		  
	      return true;
	  } 
	  @Override
	  public void onShowPress(MotionEvent e) {
	   // TODO Auto-generated method stub
	  }
	  @Override
	  public boolean onSingleTapUp(MotionEvent e) {
	   // TODO Auto-generated method stub
		Log.d(TAG, "################onSingleTapUp##############");
		mHandler.removeCallbacksAndMessages(null);
		if (mMediaController.isShowing()) {
      	mMediaController.hide();
      } else {
      	mMediaController.show();
      	mHandler.removeCallbacks(mHideStatusBar);
      }	
	   return true;
	  }
	  */
	private static final String[] SUB_EXTS = new String[] {
		".idx",
		".sub",  //.idx
		".srt",
		".smi",
		".rt",
		".txt",
		".ssa",
		".aqt",
		".jss",
		".js",
    	".ass",
    	".vsf",
    	".tts",
    	".stl",
    	".zeg",
    	".ovr",
    	".dks",
    	".lrc",
    	".pan",
    	".sbt",
    	".vkt",
    	".pjs",
    	".mpl",
    	".scr",
    	".psb",
    	".asc",
    	".rtf",
    	".s2k",
    	".sst",
    	".son",
    	".ssts"
	};

	private static final String[] MEDIA_MIMETYPE = new String[] {
		"application/idx-sub",
		"application/sub",  //.idx
		"application/x-subrip",
		"text/smi",
		"text/rt",
		"text/txt",
		"text/ssa",
		"text/aqt",
		"text/jss",
		"text/js",
    	"text/ass",
    	"text/vsf",
    	"text/tts",
    	"text/stl",
    	"text/zeg",
    	"text/ovr",
    	"text/dks",
    	"text/lrc",
    	"text/pan",
    	"text/sbt",
    	"text/vkt",
    	"text/pjs",
    	"text/mpl",
    	"text/scr",
    	"text/psb",
    	"text/asc",
    	"text/rtf",
    	"text/s2k",
    	"text/sst",
    	"text/son",
    	"text/ssts"
	};

    private static final long BLACK_TIMEOUT = 500;

    // If we resume the acitivty with in RESUMEABLE_TIMEOUT, we will keep playing.
    // Otherwise, we pause the player.
    private static final long RESUMEABLE_TIMEOUT = 3 * 60 * 1000; // 3 mins

	private static final int TEXTVIEW_UPDATE = 2;
    private Context mContext;
    private final VideoView mVideoView;
    private final View mRootView;
    private final Bookmarker mBookmarker;
    private Uri mUri;
    private final Handler mHandler = new Handler();
    //private final AudioBecomingNoisyReceiver mAudioBecomingNoisyReceiver;
    //private final MovieControllerOverlay mController;
    private MediaController mMediaController;
    

    private long mResumeableTime = Long.MAX_VALUE;
    private int mVideoPosition = 0;
    private boolean mHasPaused = false;
    private int mLastSystemUiVis = 0;

    // If the time bar is being dragged.
    private boolean mDragging;

    // If the time bar is visible.
    private boolean mShowing;

	private ArrayList<String> mSrtList = new ArrayList<String>();
	private ArrayList<String> mMediaTypeList = new ArrayList<String>();
    private Virtualizer mVirtualizer;
    /*******************************************************/
    //add for handling subtitle
	private ImageView mImageview;
	private Bitmap mBitmap;
	private int mBitmapSubtitleFlag = 0;
	private int mScreenWidth;
	private int mScreenHeight;

	private SubTitleInfoOps subTitleInfoOps = new SubTitleInfoOps();
	private SubTitleInfo mSubTitleInfo;
	private TextViewInfo[] mTextViewInfo = new TextViewInfo[10];
	private final Handler mSubTitleHandler = new MainHandler();   //handle text subtitle
    /*******************************************************/

	private static int position = 0;

    private final Runnable mPlayingChecker = new Runnable() {
        @Override
        public void run() {
            if (mVideoView.isPlaying()) {
                //mController.showPlaying();
                mProgressView.setVisibility(View.GONE);
            } else {
                mHandler.postDelayed(mPlayingChecker, 250);
            }
        }
    };
    private final Runnable mHideStatusBar = new Runnable() {
        @Override
        public void run() {
        	mRootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
    };
	private final Runnable mDubbleBack = new Runnable() {
			@Override
			public void run() {
				sendKeyIntent(KeyEvent.KEYCODE_BACK);
			}
		};

    private final Runnable mProgressChecker = new Runnable() {
        @Override
        public void run() {
            int pos = setProgress();
            mHandler.postDelayed(mProgressChecker, 1000 - (pos % 1000));
        }
    };

	private final Runnable mUpdateImageView = new Runnable() {
        @Override
        public void run() {
            mImageview.setImageBitmap(mBitmap);
        }
    };

	private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case TEXTVIEW_UPDATE: {
					for(int i = 0; i < mTextViewInfo.length; i++)
					{
						mTextViewInfo[i].textView.setText(mTextViewInfo[i].text);
					}
                    break;
                }
            }
        }
    }
	
    private Uri Uri2File2Uri(Uri videoUri) {
    	String path = null;
    	Cursor c = null;
        String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };
		ContentResolver cr = mContext.getContentResolver(); 
		c = cr.query(videoUri,VIDEO_PROJECTION,null,null,null);
        if(c != null)
        {
            try {
                while (c.moveToNext()) {
                    path = c.getString(0);
                    int idx = path.lastIndexOf(".");
                    int idx1 = path.lastIndexOf("/");   // position of last /
                    if(idx1 > 0 && idx > 0 && idx > idx1)
                    {
                        String folder = path.substring(0,idx1);   // storage/emulated/0/DCIM
                        String name = path.substring(idx1 + 1,idx);   // a
                        File directory = new File(folder);
                        if(directory.isDirectory())
                        {
                            File[] files = directory.listFiles();
                            if(files != null)
                            {
                                for(File f : files)
                                {
                                    if(f.exists())
                                    {
                                        String fileName = f.getName();
                                        int idx_a = fileName.indexOf(".");
                                        int idx_b = fileName.lastIndexOf(".");
                                        int idx_c = fileName.length();
                                        if(idx_b > 0 && idx_c > 0)
                                        {
                                            for(;idx_a > 0;)
                                            {
                                                String name1 = fileName.substring(0, idx_a);
                                                String name2 = fileName.substring(idx_b, idx_c);
                                                if(name1.equals(name))
                                                {
                                                    for(int i = 0; i < SUB_EXTS.length; i ++)
                                                    {
                                                        if(name2.equals(SUB_EXTS[i]))
                                                        {
                                                            mSrtList.add(folder + "/" + fileName);
                                                            mMediaTypeList.add(MEDIA_MIMETYPE[i]);
                                                        }
                                                    }
                                                }
                                                idx_a = fileName.indexOf(".",idx_a+1);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } finally {
                c.close();
                c = null;
            }
        }
        if((Build.VERSION.SDK_INT > 16) && (path != null))
        {
            return Uri.fromFile(new File(path));
        }
        else if((Build.VERSION.SDK_INT < 17) && (path != null))
        {
            return Uri.fromFile(new File(path));
        }
        else
        {
            path = videoUri.getPath();
            int idx = path.lastIndexOf(".");
            int idx1 = path.lastIndexOf("/");   // position of last /
            if(idx1 > 0 && idx > 0 && idx > idx1)
            {
                String folder = path.substring(0,idx1);   // storage/emulated/0/DCIM
                String name = path.substring(idx1 + 1,idx);   // a
                File directory = new File(folder);
                if(directory.isDirectory())
                {
                    File[] files = directory.listFiles();
                    if(files != null)
                    {
                        for(File f : files)
                        {
                            if(f.exists())
                            {
                                String fileName = f.getName();
                                int idx_a = fileName.indexOf(".");
                                int idx_b = fileName.lastIndexOf(".");
                                int idx_c = fileName.length();
                                if(idx_b > 0 && idx_c > 0)
                                {
                                    for(;idx_a > 0;)
                                    {
                                        String name1 = fileName.substring(0, idx_a);
                                        String name2 = fileName.substring(idx_b, idx_c);
                                        if(name1.equals(name))
                                        {
                                            for(int i = 0; i < SUB_EXTS.length; i ++)
                                            {
                                                if(name2.equals(SUB_EXTS[i]))
                                                {
                                                    mSrtList.add(folder + "/" + fileName);
                                                    mMediaTypeList.add(MEDIA_MIMETYPE[i]);
                                                }
                                            }
                                        }
                                        idx_a = fileName.indexOf(".",idx_a+1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return videoUri;
        }
    }

  //* ACTION_HDMISTATUS_CHANGED
  /*
  	private class AwHdmiPluggedReceiver extends BroadcastReceiver {
  		public void register() {
  			mContext.registerReceiver(this,
  					new IntentFilter(Intent.ACTION_HDMISTATUS_CHANGED));
  		}

  		public void unregister() {
  			mContext.unregisterReceiver(this);
  		}

  		@Override
  		public void onReceive(Context context, Intent intent) {
  			int wasPlugged = mCurHdmiPlugged?1:0;
  			mCurHdmiPlugged = intent.getIntExtra(DisplayManagerAw.EXTRA_HDMISTATUS, wasPlugged)>0?true:false;

  			if (mCurHdmiPlugged == true)
  		    {
  		        mVideoView.setHdmiPluggedStatus(true);
  		    }
  	        else
  	        {
  	            mVideoView.setHdmiPluggedStatus(false);
  	        }
  			
  			if (mHdmiPluggedFirstFlags)
  			{
  			    mPreHdmiPlugged = mCurHdmiPlugged;
  			    mHdmiPluggedFirstFlags = false;
  			}
  			if (mPreHdmiPlugged != mCurHdmiPlugged)
  			{
  			    int rotation = ((Activity)mContext).getWindowManager().getDefaultDisplay().getRotation();
  				if (rotation != 0)
  				{
  					onPause();
  					onResume();	
  				}
  	        }
  	
  			if (mCurHdmiPlugged == true)
  			{
  			    mVideoView.setParameter(500, 1);
  			}
  			else
  			{
  				mVideoView.setParameter(500, 0);
  			}
  			
  			mPreHdmiPlugged = mCurHdmiPlugged;
  		}
  	}*/
  	private void setImageButtonListener(MediaController mediaController) {
  		
    	mediaController.setSetListener(mSetListener);
    	mediaController.setSubGateListener(mSubGateListener);
    	mediaController.setSubSelectListener(mSubSelectListener);
    	mediaController.setSubCharSetListener(mSubCharSetListener);
    	//mediaController.setSubColorListener(mSubColorListener);
    	//mediaController.setSubCharSizeListener(mSubCharSizeListener);
    	//mediaController.setSubOffSetListener(mSubOffSetListener);
    	//mediaController.setSubDelayListener(mSubDelayListener);
    	mediaController.setZoomListener(mZoomListener);
    	mediaController.set3DListener(m3DListener);
    	mediaController.setTrackListener(mTrackListener);
    	mediaController.setRepeatListener(mRepeatListener);
    }
  	private View.OnClickListener mSubCharSetListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBCHARSET;
        	
        	mDialogTitle.setText(R.string.charset_title);
        	mListFocus = 0;
        	String currentCharset = mVideoView.getSubCharset();
        	String[] CharsetList = mRes.getStringArray(R.array.screen_charset_values);
        	for(int i = 0; i < CharsetList.length; i++) {
        		if(currentCharset.equalsIgnoreCase(CharsetList[i])) {
        			mListFocus = i;
        			break;
        		}
        	}
        	
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_charset_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
  	private View.OnClickListener mSetListener = new View.OnClickListener() {
        public void onClick(View v) {
        	mMediaController.setSetSettingsEnable();
        	TrackInfo[] subList = mVideoView.getSubList();
        	if(subList != null && subList.length > 0) {
    			mMediaController.setSubsetEnabled(true);
    		} else {
    			mMediaController.setSubsetEnabled(false);
    		}
        }
    };
    private View.OnClickListener mSubSelectListener = new View.OnClickListener() {
        public void onClick(View v) {
        	
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBSELECT;
        	mListFocus = 0;

        	mDialogTitle.setText(R.string.select_title);
        	TrackInfo[] subList = mVideoView.getSubList();
        	if(subList != null) {
        		int subCount = subList.length;
        		mListFocus =  mVideoView.getCurSub();
        		String[] transformSub = new String[subList.length];
        		Log.d(TAG, "£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦£¦subCount:" + subCount);
        		for(int i = 0;i < subCount;i++) {
        			Log.d(TAG, "language:" + subList[i].getLanguage());
        			transformSub[i] = new String(subList[i].getLanguage());
        			/*
        			try {
        				if(subList[i].charset.equals(MediaPlayer.CHARSET_UNKNOWN)) {
        					transformSub[i] = new String(subList[i].name, "UTF-8");
        				} else {
        					transformSub[i] = new String(subList[i].name, subList[i].charset);
        				}
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						Log.w(TAG, "*********** unsupported encoding: "+subList[i].charset);
						e.printStackTrace();
					}
        		*/}
               	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			transformSub);
               	mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
               	mListView.smoothScrollToPosition(mListFocus);
        	} else {
        		ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			mRes.getStringArray(R.array.screen_select_entries));
               	mListView.setAdapter(adapter);
                mListView.setItemChecked(mListFocus, true);
        	}
        	mListDialog.show();
        }
    };
    private View.OnClickListener mSubGateListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
            Log.d(TAG, "~~~~~~~~~~~~~~~mSubGateListener click");
            OnSwitchResultListener l = new OnSwitchResultListener(){

    			public void OnSwitchResult(boolean switchOn) {
    				// TODO Auto-generated method stub
    				Log.d(TAG, "~~~~~~~~~~~~~~~~~~on switch result");
    				if(mSlipSwitch.isShowing()){
    					mSlipSwitch.dismiss();
    				}
    				Log.d(TAG, "~~~~~~~~~~~~switchOn:" + switchOn);
    				mVideoView.setSubGate(switchOn); 
                    mMediaController.setHolding(false);
                    //mMediaController.setSubsetEnabled(switchOn);
    	        	editor.putBoolean(EDITOR_SUBGATE, switchOn);
    				editor.commit();
    			}
            };
            boolean curSwitch = mVideoView.getSubGate();
            //mVideoView.setSubGate(!curSwitch);
            Log.d(TAG,"~~~~~~~~~getSubGate ::" + curSwitch);
            mSlipSwitch = new SlipSwitchDialog(mContext, l, curSwitch);
            mSlipSwitch.setCancelListener(new OnCancelListener(){
    			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
    			}
            });
            mSlipSwitch.show();
        }
    };
    
    private View.OnClickListener mZoomListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_ZOOM;
        	
        	mDialogTitle.setText(R.string.zoom_title);
        	int currentMode = sp.getInt(EDITOR_ZOOM, 0);
        	int[] list = mRes.getIntArray(R.array.screen_zoom_values);
        	for(mListFocus = 0; mListFocus < list.length; mListFocus++) {
        		if(currentMode == list[mListFocus]) {
        			break;
        		}
        	}
        	mListFocus = mListFocus%(list.length);
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_zoom_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
    

    private View.OnClickListener m3DListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_MODE3D;
        	
        	mDialogTitle.setText(R.string.mode3d_title);
        	//int currentMode = mVideoView.getOutputDimensionType();
        	//int[] list = mRes.getIntArray(R.array.screen_3d_values);
        	//for(mListFocus = 0; mListFocus < list.length; mListFocus++) {
        	//	if(currentMode == list[mListFocus]) {
        	//		break;
        	//	}
        	//}
        	//mListFocus = mListFocus%(list.length);
        	mListFocus = m3DFocus;
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_3d_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
    
    private View.OnClickListener mTrackListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_TRACK;
        	mListFocus = 0;

        	mDialogTitle.setText(R.string.track_title);
        	TrackInfo[] trackList = mVideoView.getTrackList();
        	if(trackList != null) {
        		int trackCount = trackList.length;
        		mListFocus =  mVideoView.getCurTrack();
        		String[] transformTrack = new String[trackList.length];
        		for(int i = 0;i < trackCount;i++) {
        			transformTrack[i] = new String(trackList[i].getLanguage());
        			/*
        			try {
        				if(trackList[i].charset.equals(MediaPlayer.CHARSET_UNKNOWN)) {
        					transformTrack[i] = new String(trackList[i].name, "UTF-8");
        				} else {
        					transformTrack[i] = new String(trackList[i].name, trackList[i].charset);
        				}
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						Log.w(TAG, "*********** unsupported encoding: "+trackList[i].charset);
						e.printStackTrace();
					}
        		*/}
               	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			transformTrack);
               	mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
        	} else {
        		ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			mRes.getStringArray(R.array.screen_track_entries));
        		mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
        	}
        	mListDialog.show();
        }
    };
    
    private View.OnClickListener mRepeatListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_REPEAT;
        	mListFocus = mRepeatMode;

        	mDialogTitle.setText(R.string.repeat_title);
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_repeat_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
    
    private OnItemClickListener mItemClickListener = new OnItemClickListener() {

		public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
			if(position == mListFocus) {
				/* the same item */
				return;
			}
			
			int ret;	// mVideoView set the sub param's return value
			if(mControlFocus.equals(EDITOR_SUBSELECT)) {
					/* sub select */
				ret = mVideoView.switchSub(position); 
        		if(ret == 0) {
        			mListFocus = position;
					mCurrentSubSave = position;
        		} else {
        			Log.w(TAG, "*********** change the sub select failed !");
        		}
        	} else if(mControlFocus.equals(EDITOR_SUBCHARSET)) {
                /* sub charset */
                String[] listCharSet = mRes.getStringArray(R.array.screen_charset_values);
                ret = mVideoView.setSubCharset(listCharSet[position]);
                if(ret == 0) {
                    mListFocus = position;
                } else {
                    Log.w(TAG, "*********** change the sub charset failed !");
                }
             }
			else if(mControlFocus.equals(EDITOR_TRACK)) {
        		/* track */
        		ret = mVideoView.switchTrack(position); 
        		if(ret == 0) {
        			mListFocus = position;
					mCurrentTrackSave = position;
        		} else {
        			Log.w(TAG, "*********** change the sub track failed !");
        		}
        	}
        	else if(mControlFocus.equals(EDITOR_REPEAT)){
        		/* repeat */
				mRepeatMode = position;
	        	editor.putInt(EDITOR_REPEAT, mRepeatMode);
				editor.commit();
        	}
        	else if(mControlFocus.equals(EDITOR_MODE3D)) {
        		/* 3D mode */
    			mListFocus = position;
    			m3DFocus = position;
        		set3DMode(position);
        	}
        	else if(mControlFocus.equals(EDITOR_ZOOM)) {
        		/* zoom mode */
        		mListFocus = position;
        		mVideoView.setZoomMode(position); 
	        	editor.putInt(EDITOR_ZOOM, position);
				editor.commit();
        	}
		}
    };
    
    private void set3DMode(int position) {
    	int[] listValue = mRes.getIntArray(R.array.screen_3d_values);
    	int crrentValue = listValue[position];
		DisplayManager displayManager = (DisplayManager) mContext.getSystemService(Context.DISPLAY_SERVICE);
    	if(crrentValue == 0) {   
    		//mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
			//mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);
            if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
			    displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_2D_LEFT);
			else if (mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
				displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_2D_LEFT);
    	} else if(crrentValue == 1) {
    		//mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
			//mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);
			if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
			    displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_2D_TOP);
			else if (mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
				displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_2D_TOP);
    	} else if(crrentValue == 2) {
    		//mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_2D);
    		if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
    		    displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_2D_ORIGINAL);
			else if (mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
				displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_2D_ORIGINAL);
    	} else if(crrentValue == 3) {
    		//mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		//mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    		if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
    		    displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_3D_LEFT_RIGHT_HDMI);
			else if (mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
				displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_3D_LEFT_RIGHT_HDMI);
			mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    	} else if(crrentValue == 4) {
    		//mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		//mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
			if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
    		    displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_3D_TOP_BOTTOM_HDMI);
			else if (mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
				displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_3D_TOP_BOTTOM_HDMI);
			mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);	
    	}/* else if(crrentValue == 5) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_DOUBLE_STREAM);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
			Log.d(TAG, "********************6******************");
    	} else if(crrentValue == 6) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_LINE_INTERLEAVE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
			Log.d(TAG, "********************7******************");
    	} else if(crrentValue == 7) {
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_COLUME_INTERLEAVE);
    	}*/ else if(crrentValue == 8) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
			Log.d(TAG, "********************8******************");
    	} else if(crrentValue == 9) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
			Log.d(TAG, "********************9******************");
    	} else if(crrentValue == 10) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 11) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 12) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 13) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 14) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 15) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 16) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 17) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 18) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 19) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 20) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 21) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	}
    }
    private void playFile() {
        mWakeLock.acquire();

        m3DFocus = 2;
		mVideoView.setVideoURI(mUri);
		mMediaController.setFilePathTextView(mUri.getPath());
        mVideoView.requestFocus();
		mVideoView.start();
		set3DMode(m3DFocus);

        mWakeLock.release();        
    }
	private void sendKeyIntent(int keycode){
		final int keyCode = keycode;
		// to avoid deadlock, start a thread to perform operations
        Thread sendKeyDelay = new Thread(){   
            public void run() {
                try {
                    int count = 1;
                    if(keyCode == KeyEvent.KEYCODE_BACK)
                        count = 2;
                    
                    IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
                    for(int i = 0; i < count; i++){
                        Thread.sleep(100);
                        long now = SystemClock.uptimeMillis();
                        if(!mOnPause) {
	                        KeyEvent keyDown = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0);
        					InputManager.getInstance().injectInputEvent(keyDown, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
//	                        wm.injectKeyEvent(keyDown, false);   
	            
	                        KeyEvent keyUp = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0);
	                        InputManager.getInstance().injectInputEvent(keyUp, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);   
//	                        wm.injectKeyEvent(keyUp, false);
                        }
                    }  
                } catch (InterruptedException e) {
                    e.printStackTrace();   
                }   
            }   
        };
        sendKeyDelay.start();
    }
	private void createFolderDispList() {
    	String fileNameText, filePathText;
    	File filePath;
    	
    	String[] fileEndingVideo = mRes.getStringArray(R.array.fileEndingVideo);
    	fileNameText = mUri.getPath();
    	int index = fileNameText.lastIndexOf('/');
    	if(index >= 0) {
    		filePathText = fileNameText.substring(0, index);
    		filePath = new File(filePathText);
    		File[] fileList = filePath.listFiles();
    		if(fileList != null && filePath.isDirectory()) {
    			for(File currenFile : fileList) {
    				String fileName = currenFile.getName();
    				int indexPoint = fileName.lastIndexOf('.');
    				if(indexPoint > 0 && currenFile.isFile()) {
    					String fileEnd = fileName.substring(indexPoint+1);
    					for(int i = 0;i < fileEndingVideo.length; i++) {
        					if(fileEnd.equalsIgnoreCase(fileEndingVideo[i])) {
        						mPlayList.add(currenFile.getPath());
        						break;
        					}
        				}
    				}
    			}
    		}
    	}
    	Collections.sort(mPlayList);

        /* get current index */
        mCurrentIndex = 0;
        String mCurrentPath = mUri.getPath();
        for(int i = 0;i < mPlayList.size();i++) {
        	if( mCurrentPath.equalsIgnoreCase(mPlayList.get(i)) ) {
        		mCurrentIndex = i;
        		break;
        	}
        }
    }

    private void createMediaProviderDispList(Uri uri, Context mContext) {
        Cursor c = null;
        IContentProvider mMediaProvider = mContext.getContentResolver().acquireProvider("media");
        Uri mVideoUri = Video.Media.getContentUri("external");
        String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };
        
        /* get playlist */
        try {
			c = mMediaProvider.query(null, mVideoUri, VIDEO_PROJECTION, null, null, null, null);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        if(c != null)
        {
            try {
                while (c.moveToNext()) {
                    String path = c.getString(0);
                    if(new File(path).exists()){
                    	mPlayList.add(path);
                    }
                }
            } finally {
                c.close();
                c = null;
            }
            
            /* get current index */
            mCurrentIndex = 0;
            String mCurrentPath = mUri.getPath();
            for(int i = 0;i < mPlayList.size();i++) {
            	if( mCurrentPath.equalsIgnoreCase(mPlayList.get(i)) ) {
            		mCurrentIndex = i;
            		break;
            	}
            }
        }
    }
    private void SetListDialogParam() {
        mDialogView = View.inflate(mContext, R.layout.dialog_list, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.list_title);
        mListView = (ListView) mDialogView.findViewById(R.id.list);
        mListView.setItemsCanFocus(true);
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        mListView.setOnItemClickListener(mItemClickListener);
        Button confirm = (Button) mDialogView.findViewById(R.id.list_confirm);
        confirm.setOnClickListener(new View.OnClickListener(){
			public void onClick(View arg0) {
                mMediaController.setHolding(false);
				mListDialog.hide();
			}
        });
        
        mListDialog =  new Dialog(mContext,R.style.dialog);
    	mListDialog.setContentView(mDialogView);
    	mListDialog.setOnCancelListener(new OnCancelListener(){
			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
				mListDialog.hide();
			}
        });    	
    }
    public static MoviePlayer getInstance() {
    	return mInstance;
    }
    public void resetAllTextView() {
    	int i = 0;
    	for (i=0; i<10; i++) {
    		mTextViewInfo[i].text = "";
    		mTextViewInfo[i].textView.setText(mTextViewInfo[i].text);
    		mTextViewInfo[i].used = false;
    	}
		mImageview.setImageBitmap(null);
    }
    public void subFocusItems() {
    	/* sub gate */
    	boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
		mVideoView.setSubGate(true);
    	int ret = mVideoView.setSubGate(gate);
        Log.d(TAG, "ret:" + ret);
		/* zoom mode */
		int zoom = sp.getInt(EDITOR_ZOOM, 0);
		mVideoView.setZoomMode(zoom);
		
		/* play mode */
		mRepeatMode = sp.getInt(EDITOR_REPEAT, 0);
		
		/* resume the current sub & track */
		if(mOnPause) {
			mOnPause = false;
			mVideoView.switchSub(mCurrentSubSave);			
			mVideoView.switchTrack(mCurrentTrackSave);
		}
    }
    public MoviePlayer(View rootView, final MovieActivity movieActivity,
            Uri videoUri, Bundle savedInstance, boolean canReplay, 
            int screenWidth, int screenHeight, Intent intent) {
    	//mContext = movieActivity.getApplicationContext();
    	mMovieActivity = movieActivity;
//    	mAudiomanage = (AudioManager)mMovieActivity.getSystemService(Context.AUDIO_SERVICE);
    	mInstance = this;
    	mContext = (Context)movieActivity;
    	mContentResolver = mContext.getContentResolver();
        mRootView = rootView;
        mVideoView = (VideoView) rootView.findViewById(R.id.surface_view);
		        mProgressView = rootView.findViewById(R.id.progress_indicator);
        //mVolumeView = (TextView)rootView.findViewById(R.id.volume);
        //mBrightView = (TextView)rootView.findViewById(R.id.Brightness);
        mRes = mContext.getResources();
        sp = mContext.getSharedPreferences(STORE_NAME, Context.MODE_PRIVATE);
		editor = sp.edit();
		
		Log.v(TAG, " ## videoUri " +videoUri);
        mUri = Uri2File2Uri(videoUri);
        String scheme = mUri.getScheme();
		Log.v(TAG, " ## mUri " + mUri);
		Log.v(TAG, " ## scheme " + scheme);
		
		//init subtitle textview
		mTextViewInfo[0] = new TextViewInfo();
		mTextViewInfo[1] = new TextViewInfo();
		mTextViewInfo[2] = new TextViewInfo();
		mTextViewInfo[3] = new TextViewInfo();
		mTextViewInfo[4] = new TextViewInfo();
		mTextViewInfo[5] = new TextViewInfo();
		mTextViewInfo[6] = new TextViewInfo();
		mTextViewInfo[7] = new TextViewInfo();
		mTextViewInfo[8] = new TextViewInfo();
		mTextViewInfo[9] = new TextViewInfo();
		
		mTextViewInfo[0].textView = (TextView) rootView.findViewById(R.id.text_view_0);
		mTextViewInfo[0].used = false;
		mTextViewInfo[1].textView = (TextView) rootView.findViewById(R.id.text_view_1);
		mTextViewInfo[1].used = false;
		mTextViewInfo[2].textView = (TextView) rootView.findViewById(R.id.text_view_2);
		mTextViewInfo[2].used = false;
		mTextViewInfo[3].textView = (TextView) rootView.findViewById(R.id.text_view_3);
		mTextViewInfo[3].used = false;
		mTextViewInfo[4].textView = (TextView) rootView.findViewById(R.id.text_view_4);
		mTextViewInfo[4].used = false;
		mTextViewInfo[5].textView = (TextView) rootView.findViewById(R.id.text_view_5);
		mTextViewInfo[5].used = false;
		mTextViewInfo[6].textView = (TextView) rootView.findViewById(R.id.text_view_6);
		mTextViewInfo[6].used = false;
		mTextViewInfo[7].textView = (TextView) rootView.findViewById(R.id.text_view_7);
		mTextViewInfo[7].used = false;
		mTextViewInfo[8].textView = (TextView) rootView.findViewById(R.id.text_view_8);
		mTextViewInfo[8].used = false;
		mTextViewInfo[9].textView = (TextView) rootView.findViewById(R.id.text_view_9);
		mTextViewInfo[9].used = false;
		
		
		mImageview = (ImageView) rootView.findViewById(R.id.image_view);
		mBookmarker = new Bookmarker(movieActivity);
		
		
		PowerManager pm = (PowerManager)mContext.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK|PowerManager.ON_AFTER_RELEASE, TAG);
        mWakeLock.setReferenceCounted(false);
		
		//mAwHdmiPluggedReceiver = new AwHdmiPluggedReceiver();
		//mAwHdmiPluggedReceiver.register();
		
		//media_controller
		mMediaController = new MediaController(movieActivity, rootView);
		setImageButtonListener(mMediaController);
        mVideoView.setMediaController(mMediaController);
        mMediaController.setFilePathTextView(mUri.getPath());
        //setOnSystemUiVisibilityChangeListener();
		
        rootView.setOnSystemUiVisibilityChangeListener(
            new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                Log.d(TAG, "mLastSystemUiVis:" + mLastSystemUiVis);
				Log.d(TAG, "visibility:" + visibility);
                int diff = mLastSystemUiVis ^ visibility;
				Log.d(TAG, "diff:" + diff);
                mLastSystemUiVis = visibility;
                if ((diff & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0
                        && (visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
                    mMediaController.show();
                }
            }
        });
        rootView.setSystemUiVisibility(View.SYSTEM_UI_CLEARABLE_FLAGS | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);

        //mRootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        
        SetListDialogParam();
        
        /* create playlist */
		mPlayListType =  intent.getStringExtra(MediaStore.PLAYLIST_TYPE);
		mPlayList = new ArrayList<String>();
		Log.d(TAG, "**********************mPlayListType:" + mPlayListType);
		if (mPlayListType == null) {
			mPlayListType = MediaStore.PLAYLIST_TYPE_CUR_FOLDER;
		}
        if(mPlayListType != null) {
        	if(mPlayListType.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_CUR_FOLDER)) {
         		/* create playlist from current folder */
         		createFolderDispList();
        	} else if(mPlayListType.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_MEDIA_PROVIDER)) {
        		/* create playlist from mediaprovider */
        		createMediaProviderDispList(mUri, mContext);
        	}
        	
        	if(scheme != null && scheme.equalsIgnoreCase("file")) {
	         	mMediaController.setPrevNextVisible(true);		// make prev/next button visible
        	}
        	else{
        		Log.w(TAG,"############scheme = " + scheme);
        	}
        }        
        
        
        mMediaController.setVolumeIncListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_VOLUME_UP);
			}
    	});
        mMediaController.setVolumeDecListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_VOLUME_DOWN);
			}
    	});
        mMediaController.setBackListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_BACK);
				 mHandler.postDelayed(mDubbleBack, 30);
			}
        }); 
        mMediaController.setPrevNextListeners(new View.OnClickListener() {
            public void onClick(View v) {
            	int size = mPlayList.size();
            	if(mCurrentIndex >= 0 && size > 0)
            	{
            		mVideoView.clearCurrentStat();
            		mCurrentIndex = (mCurrentIndex+1)%size;
            		String path = mPlayList.get(mCurrentIndex);
            		mUri = Uri.fromFile(new File(path));
            		mUri = Uri2File2Uri(mUri);           		
            		if(mSrtList != null && mSrtList.size() > 0)
            		{
            			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
            		}
            		playFile();
            	}
            }
        }, 
        new View.OnClickListener() {
            public void onClick(View v) {
            	int size = mPlayList.size();
            	if(mCurrentIndex >= 0 && size > 0)
            	{
            		mVideoView.clearCurrentStat();
            		mCurrentIndex = (mCurrentIndex - 1 + size) % size;
            		String path = mPlayList.get(mCurrentIndex);
            		mUri = Uri.fromFile(new File(path));
            		mUri = Uri2File2Uri(mUri);
            		if(mSrtList != null && mSrtList.size() > 0)
            		{
            			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
            		}
            		playFile();
            	}
            }
        }); 
        
        
        
        mVideoView.setOnSubFocusItems(this);
        
        

        
        

        mVideoView.setOnErrorListener(this);
        mVideoView.setOnCompletionListener(this);
		mVideoView.setOnTimedTextListener(this);
        mVideoView.setVideoURI(mUri);

		if(mSrtList != null && mSrtList.size() > 0)
		{
			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
		}
        Intent ai = movieActivity.getIntent();
        /*boolean virtualize = ai.getBooleanExtra(VIRTUALIZE_EXTRA, false);
        if (virtualize) {
            int session = mVideoView.getAudioSessionId();
            if (session != 0) {
                mVirtualizer = new Virtualizer(0, session);
                mVirtualizer.setEnabled(true);
            } else {
                Log.w(TAG, "no audio session to virtualize");
            }
        }*/
			mRootView.setOnTouchListener(new OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent me) {
					Log.d(TAG, "*********mRootView onTouch************");
					if (!mMediaController.isShowing()) {
						mMediaController.show(2000);
					}
					return true;
				}
        });
        /*mVideoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer player) {
                if (!mVideoView.canSeekForward() || !mVideoView.canSeekBackward()) {
                    mController.setSeekable(false);
                } else {
                    mController.setSeekable(true);
                }
                setProgress();
            }
        });*/


        //mAudioBecomingNoisyReceiver = new AudioBecomingNoisyReceiver();
        //mAudioBecomingNoisyReceiver.register();

        mVideoView.requestFocus();
        Intent i = new Intent(SERVICECMD);
        i.putExtra(CMDNAME, CMDPAUSE);
        movieActivity.sendBroadcast(i);

        if (savedInstance != null) { // this is a resumed activity
            mVideoPosition = savedInstance.getInt(KEY_VIDEO_POSITION, 0);
            mResumeableTime = savedInstance.getLong(KEY_RESUMEABLE_TIME, Long.MAX_VALUE);
			if (null != mProgressView) {
				mProgressView.setVisibility(View.GONE);
			}
            mVideoView.start();
            mVideoView.suspend();
            mHasPaused = true;
        } else {
            final Integer bookmark = mBookmarker.getBookmark(mUri);
            if (bookmark != null) {
				mCurrentSubSave = mBookmarker.getSubSave();
				mCurrentTrackSave = mBookmarker.getTrackSave();
                showResumeDialog(movieActivity, bookmark);
				mVideoView.switchSub(mCurrentSubSave);
				mVideoView.switchTrack(mCurrentTrackSave);
            } else {
                startVideo();
            }
        }
		mScreenWidth = screenWidth;
		mScreenHeight = screenHeight;
    }
/*
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void setOnSystemUiVisibilityChangeListener() {
        //if (!ApiHelper.HAS_VIEW_SYSTEM_UI_FLAG_HIDE_NAVIGATION) return;

        // When the user touches the screen or uses some hard key, the framework
        // will change system ui visibility from invisible to visible. We show
        // the media control and enable system UI (e.g. ActionBar) to be visible at this point
        mRootView.setOnSystemUiVisibilityChangeListener(
                new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                int diff = mLastSystemUiVis ^ visibility;
                mLastSystemUiVis = visibility;
                if ((diff & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0
                        && (visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
                	//Log.d(TAG, "~~~~~~~~~~~~~~~~~~mMediaController.show()");
                    //mController.show();
                	mHandler.postDelayed(mHideStatusBar, 5000);
                }
            }
        });
    }*/

    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void showSystemUi(boolean visible) {
        if (!ApiHelper.HAS_VIEW_SYSTEM_UI_FLAG_LAYOUT_STABLE) return;

        int flag = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        if (!visible) {
            // We used the deprecated "STATUS_BAR_HIDDEN" for unbundling
            flag |= View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        }
        mVideoView.setSystemUiVisibility(flag);
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putInt(KEY_VIDEO_POSITION, mVideoPosition);
        outState.putLong(KEY_RESUMEABLE_TIME, mResumeableTime);
		if(mVideoPosition > 0)
		{
			position = mVideoPosition;
		}
    }

    private void showResumeDialog(Context context, final int bookmark) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(R.string.resume_playing_title);
        builder.setMessage(String.format(
                context.getString(R.string.resume_playing_message),
                GalleryUtils.formatDuration(context, bookmark / 1000)));
        builder.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
            	Log.d(TAG, "gone ************************");
            	mProgressView.setVisibility(View.GONE);
                onCompletion();
            }
        });
        builder.setPositiveButton(
                R.string.resume_playing_resume, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                mVideoView.seekTo(bookmark);
                startVideo();
            }
        });
        builder.setNegativeButton(
                R.string.resume_playing_restart, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                startVideo();
            }
        });
        builder.show();
    }

    public void onPause() {
        mHasPaused = true;
        mHandler.removeCallbacksAndMessages(null);
        mVideoPosition = mVideoView.getCurrentPosition();
        mBookmarker.setBookmark(mUri, mVideoPosition, mVideoView.getDuration(), mCurrentSubSave, mCurrentTrackSave);
        mVideoView.suspend();
        mResumeableTime = System.currentTimeMillis() + RESUMEABLE_TIMEOUT;
    }

    public void onResume() {
		resetAllTextView();
		if (mMediaController != null)
			mMediaController.show(2000);
        if (mHasPaused) {
           if(position > 0 && mVideoPosition == 0)
			{
				mVideoView.seekTo(position);
			}
			else
			{
            	mVideoView.seekTo(mVideoPosition);
			}
            mVideoView.resume();

            // If we have slept for too long, pause the play
            //if (System.currentTimeMillis() > mResumeableTime) {
                pauseVideo();
            //}
        }
        mHandler.post(mProgressChecker);
    }

    public void onDestroy() {
        if (mVirtualizer != null) {
            mVirtualizer.release();
            mVirtualizer = null;
        }
		//* reset to normal 2d
        DisplayManager displayManager = (DisplayManager) mContext.getSystemService(Context.DISPLAY_SERVICE);
		if (mDisplay3DType == DISPLAY_3D_TYPE_HDMI)
            displayManager.setDisplay3DMode(Display.TYPE_HDMI, DisplayManager.DISPLAY_2D_ORIGINAL);
		else if(mDisplay3DType == DISPLAY_3D_TYPE_BUILT_IN)
			displayManager.setDisplay3DMode(Display.TYPE_BUILT_IN, DisplayManager.DISPLAY_2D_ORIGINAL);
        mVideoView.stopPlayback();
        //mAudioBecomingNoisyReceiver.unregister();
    }

    // This updates the time bar display (if necessary). It is called every
    // second by mProgressChecker and also from places where the time bar needs
    // to be updated immediately.
    private int setProgress() {
        if (mDragging || !mShowing) {
            return 0;
        }
        int position = mVideoView.getCurrentPosition();
        int duration = mVideoView.getDuration();
        //mController.setTimes(position, duration, 0, 0);
        return position;
    }

	public void replayVideo() {
		mVideoView.setVideoURI(mUri);
		mVideoView.seekTo(0);

        String scheme = mUri.getScheme();
        if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
            //mController.showLoading();
            mHandler.removeCallbacks(mPlayingChecker);
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            //mController.showPlaying();
            //mController.hide();
            mProgressView.setVisibility(View.GONE);
        }

        //mController.showPaused();
        setProgress();
	}

    private void startVideo() {
        // For streams that we expect to be slow to start up, show a
        // progress spinner until playback starts.
        String scheme = mUri.getScheme();
        if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
            //mController.showLoading();
            mHandler.removeCallbacks(mPlayingChecker);
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            //mController.showPlaying();
            //mController.hide();
            mProgressView.setVisibility(View.GONE);
        }

        mVideoView.start();
        setProgress();
    }

    private void playVideo() {
        mVideoView.start();
        //mController.showPlaying();
        setProgress();
    }

    private void pauseVideo() {
        mVideoView.pause();
        //mController.showPaused();
    }

    // Below are notifications from VideoView
    @Override
    public boolean onError(MediaPlayer player, int arg1, int arg2) {
        mHandler.removeCallbacksAndMessages(null);
        // VideoView will show an error dialog if we return false, so no need
        // to show more message.
        //mController.showErrorMessage("");
        mProgressView.setVisibility(View.GONE);
		mToQuit = true;
        return false;
    }
    public boolean toQuit() {
        return mToQuit;
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        //mController.showEnded();
    	if(mListDialog != null && mListDialog.isShowing()) {
            mMediaController.setHolding(false);
    		mListDialog.hide();
    	}
    	if(mSlipSwitch != null && mSlipSwitch.isShowing()) {
            mMediaController.setHolding(false);
    		mSlipSwitch.dismiss();
    	}
        onCompletion();
    }

    public void onCompletion() {
    	 mVideoView.setOnErrorListener(this);
         int size = mPlayList.size();
         if(mToQuit == true)
         	return;
         
         if(mCurrentIndex >= 0 && size > 0){
         	switch(mRepeatMode){
         		case PLAYMODE_REPEAT_ALL:{
         			int index = (mCurrentIndex+1)%size;
         			File nextFile = new File(mPlayList.get(index));
         			if (!nextFile.exists()){
         				   mToQuit = true;
         			}else {
         				mVideoView.clearCurrentStat();
         				mCurrentIndex = index;
         				mUri = Uri.fromFile(nextFile);
         				mMediaController.setFilePathTextView(mUri.getPath());
         				mUri = Uri2File2Uri(mUri);           		
                		if(mSrtList != null && mSrtList.size() > 0)
                		{
                			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
                		}
         				playFile();
         			}
     				break;
     			}	
         		case PLAYMODE_SEQUENCE:{
         			int index;
         			if(mCurrentIndex < size - 1){
         				index = mCurrentIndex + 1;
         				File nextFile = new File(mPlayList.get(index));
         				if (!nextFile.exists()){
         				   	mToQuit = true;
         				}else {
         					mVideoView.clearCurrentStat();
         					mCurrentIndex = index;
         					mUri = Uri.fromFile(nextFile);
         					mMediaController.setFilePathTextView(mUri.getPath());
         					mUri = Uri2File2Uri(mUri);           		
                    		if(mSrtList != null && mSrtList.size() > 0)
                    		{
                    			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
                    		}
         					playFile();
         				}
         			}
         			else{
         				mToQuit = true;
         			}
         			break;
         		}
         		case PLAYMODE_REPEAT_ONE:{
         			playFile();
         			break;
         		}
         		case PLAYMODE_RANDOM:{
         			int index = (new Random()).nextInt(size);
         			File nextFile = new File(mPlayList.get(index));
         			if (!nextFile.exists()){
         				   mToQuit = true;
         			}else {
         				mVideoView.clearCurrentStat();
         				mCurrentIndex = index;
         				mUri = Uri.fromFile(nextFile);
         				mMediaController.setFilePathTextView(mUri.getPath());
         				mUri = Uri2File2Uri(mUri);           		
                		if(mSrtList != null && mSrtList.size() > 0)
                		{
                			mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
                		}
         				playFile();
         			}
         			break;
         		}
         		default:
         			break;
         	}
         }else{
         	mToQuit = true;
         }
    }

	public class SubTitleInfo {
        String text;
        int hideSubFlag;
        int subDispPos;
        Rect textScreenBound;
        Rect textBound;
        List<TimedText.Style> styleList;
		int textViewID;
    }

	public class TextViewInfo {
        TextView textView;
		boolean used;
		String text;
    }

	public class SubTitleInfoOps {
		private List<SubTitleInfo> mSubTitleInfoList = null;

		public SubTitleInfoOps()
		{
			mSubTitleInfoList = new ArrayList<SubTitleInfo>();
		}

		public void addSubTitleInfo(SubTitleInfo subTitleInfo)
		{
			mSubTitleInfoList.add(subTitleInfo);
		}

		public void removeSubTitleInfo(SubTitleInfo subTitleInfo)
		{
			for(int i = 0; i < mSubTitleInfoList.size(); i ++)
			{
				if(mSubTitleInfoList.get(i).text.equals(subTitleInfo.text))
				{
					mTextViewInfo[mSubTitleInfoList.get(i).textViewID].used = false;
					mTextViewInfo[mSubTitleInfoList.get(i).textViewID].text = null;
					subTitleNoDraw();
					mSubTitleInfoList.remove(mSubTitleInfoList.get(i));
				}
			}
		}

		public void removeAllSubTitleInfo()
		{
			mSubTitleInfoList.clear();
		}

		public int getNumOfSubTitle()
		{
			return mSubTitleInfoList.size();
		}

		public SubTitleInfo getSubTitleInfo(int index)
		{
			return mSubTitleInfoList.get(index);
		}
	}

	@Override
    public void onTimedText(MediaPlayer mp, TimedText text) {
        if (text != null) {
			mBitmapSubtitleFlag = text.AWExtend_getBitmapSubtitleFlag();
			if(mBitmapSubtitleFlag == 0)
			{
				SubTitleInfo subTitleInfo = new SubTitleInfo();
				//Log.v(TAG, " ** text.getText().trim() = " + text.getText());
				subTitleInfo.text = text.getText();
				subTitleInfo.hideSubFlag = text.AWExtend_getHideSubFlag();
				subTitleInfo.subDispPos = text.AWExtend_getSubDispPos();
				subTitleInfo.textScreenBound = text.AWExtend_getTextScreenBounds();
				subTitleInfo.textBound = text.getBounds();
				subTitleInfo.styleList = text.AWExtend_getStyleList();
				
				if(subTitleInfo.text != null)
				{
					//Log.v(TAG, " ** text:" + subTitleInfo.text + " hideSubFlag:" + subTitleInfo.hideSubFlag);
					if(subTitleInfo.hideSubFlag == 0)
					{
						for(int i = 0; i < mTextViewInfo.length; i++)
						{
							if(mTextViewInfo[i].used == false)
							{
								subTitleInfo.textViewID = i;
								mTextViewInfo[i].used = true;
								mTextViewInfo[i].text = subTitleInfo.text;
								subTitleDraw(i, subTitleInfo.subDispPos, subTitleInfo.textScreenBound, subTitleInfo.textBound, subTitleInfo.styleList);
								break;
							}
						}
						subTitleInfoOps.addSubTitleInfo(subTitleInfo);
					}
					else
					{
						subTitleInfoOps.removeSubTitleInfo(subTitleInfo);
					}
				}
			}
			else if(mBitmapSubtitleFlag == 1)
			{
				mBitmap = text.AWExtend_getBitmap();
				int width = mBitmap.getWidth();
				int height = mBitmap.getHeight();
				if (mBitmap != null) {
					mHandler.removeCallbacks(mUpdateImageView);
					mHandler.post(mUpdateImageView);
	            }
			}
        }
		else
		{
			subTitleInfoOps.removeAllSubTitleInfo();
			for(int i = 0; i < mTextViewInfo.length; i++)
			{
				if(mTextViewInfo[i].used == true)
				{
					mTextViewInfo[i].used = false;
					mTextViewInfo[i].text = null;
					subTitleNoDraw();
				}
			}

			mBitmap = null;
			mHandler.removeCallbacks(mUpdateImageView);
			mHandler.post(mUpdateImageView);
		}
    }

	private void subTitleDraw(int numOfTextViewDrawed, int subDispPos, Rect screenBound, Rect textBound, List<TimedText.Style> styleList)
	{
	    RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
		int screenleft = 0;
		int screentop = 0;
		int screenright = 0;
		int screenbottom = 0;
		int textleft = 0;
		int texttop = 0;
		int textright = 0;
		int textbottom = 0;
		boolean isBold = false;
		boolean isItalic = false;
		boolean isUnderlined = false;
		int fontSize = 40;
		int colorRGBA = -1;
		float zoom = 1;
		
		if(screenBound != null && textBound != null)
		{
			screenleft = screenBound.left;
			screentop = screenBound.top;
			screenright = screenBound.right;
			screenbottom = screenBound.bottom;

			zoom = (float)mScreenWidth / (float)(screenright - screenleft);
			
			screenleft = (int)(screenleft * zoom);
			screentop = (int)(screentop * zoom);
			screenright = (int)(screenright * zoom);
			screenbottom= (int)(screenbottom* zoom);

			textleft = (int)(textBound.left * zoom);
			texttop = (int)(textBound.top * zoom);
			textright = (int)(textBound.right * zoom);
			textbottom = (int)(textBound.bottom * zoom);
		}

		if(subDispPos == TimedText.SUB_DISPPOS_DEFAULT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
			params.setMargins(0,0,0,screenbottom - textbottom);
			
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_BOT_LEFT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE); 
			params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
			params.setMargins(textleft,0,0,screenbottom - textbottom);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_BOT_MID)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
			params.setMargins(0,0,0,screenbottom - textbottom);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_BOT_RIGHT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
			params.setMargins(0,0,screenright - textright,screenbottom - textbottom);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_MID_LEFT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
			params.setMargins(textleft,0,0,0);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_MID_MID)
		{
			params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
			params.setMargins(0,0,0,0);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_MID_RIGHT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
			params.setMargins(0,0,screenright - textright,0);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_TOP_LEFT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE); 
			params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
			params.setMargins(textleft,texttop,0,0);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_TOP_MID)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
			params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
			params.setMargins(0,texttop,0,0);
		}
		else if(subDispPos == TimedText.SUB_DISPPOS_TOP_RIGHT)
		{
			params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE); 
			params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
			params.setMargins(0,texttop,screenright - textright,0);
		}
		mTextViewInfo[numOfTextViewDrawed].textView.setLayoutParams(params);
		
		if(styleList != null)
		{
			TimedText.Style style = styleList.get(0);
			isBold = style.isBold;
			isItalic = style.isItalic;
			isUnderlined = style.isUnderlined;
			fontSize = style.fontSize;
			colorRGBA = style.colorRGBA;
		}
		mTextViewInfo[numOfTextViewDrawed].textView.setTextColor(colorRGBA);
		float textSize = (float)((float)fontSize * zoom / 2.0);
        //Log.d(TAG, "*************************textsize:" + textSize);
        if (textSize < 16) {
           textSize = 16;
         }
        mTextViewInfo[numOfTextViewDrawed].textView.setTextSize(textSize);
		
		if(isBold == true && isItalic == false)
		{
			mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
		}
		else if(isItalic == true && isBold == false)
		{
			mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.ITALIC));
		}
		else if(isItalic == true && isBold == true)
		{
			mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD_ITALIC));
		}
		else
		{
			mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.NORMAL));
		}

		mSubTitleHandler.sendEmptyMessage(TEXTVIEW_UPDATE);
	}

	private void subTitleNoDraw()
	{
		mSubTitleHandler.sendEmptyMessage(TEXTVIEW_UPDATE);
	}

    // Below are notifications from ControllerOverlay
    @Override
    public void onPlayPause() {
        if (mVideoView.isPlaying()) {
            pauseVideo();
        } else {
            playVideo();
        }
    }

    @Override
    public void onSeekStart() {
        mDragging = true;
    }

    @Override
    public void onSeekMove(int time) {
        mVideoView.seekTo(time);
    }

    @Override
    public void onSeekEnd(int time, int start, int end) {
        mDragging = false;
        mVideoView.seekTo(time);
        setProgress();
    }

    @Override
    public void onShown() {
        mShowing = true;
        setProgress();
        showSystemUi(true);
    }

    @Override
    public void onHidden() {
        mShowing = false;
        showSystemUi(false);
    }

    @Override
    public void onReplay() {
        startVideo();
    }

    // Below are key events passed from MovieActivity.
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        // Some headsets will fire off 7-10 events on a single click
        if (event.getRepeatCount() > 0) {
            return isMediaKey(keyCode);
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_HEADSETHOOK:
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                if (mVideoView.isPlaying()) {
                    pauseVideo();
                } else {
                    playVideo();
                }
                return true;
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
                if (mVideoView.isPlaying()) {
                    pauseVideo();
                }
                return true;
            case KeyEvent.KEYCODE_MEDIA_PLAY:
                if (!mVideoView.isPlaying()) {
                    playVideo();
                }
                return true;
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
            case KeyEvent.KEYCODE_MEDIA_NEXT:
                // TODO: Handle next / previous accordingly, for now we're
                // just consuming the events.
                return true;
        }
        return false;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return isMediaKey(keyCode);
    }

    private static boolean isMediaKey(int keyCode) {
        return keyCode == KeyEvent.KEYCODE_HEADSETHOOK
                || keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS
                || keyCode == KeyEvent.KEYCODE_MEDIA_NEXT
                || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
                || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY
                || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE;
    }

    // We want to pause when the headset is unplugged.
    private class AudioBecomingNoisyReceiver extends BroadcastReceiver {

        public void register() {
            mContext.registerReceiver(this,
                    new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
        }

        public void unregister() {
            mContext.unregisterReceiver(this);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
        	Log.d(TAG, "***************************onReceive");
            if (mVideoView.isPlaying()) {
				Log.d(TAG, "***************************onReceive:pause");
				pauseVideo();
			}
        }
    }
}

class Bookmarker {
    private static final String TAG = "Bookmarker";

    private static final String BOOKMARK_CACHE_FILE = "bookmark";
    private static final int BOOKMARK_CACHE_MAX_ENTRIES = 100;
    private static final int BOOKMARK_CACHE_MAX_BYTES = 10 * 1024;
    private static final int BOOKMARK_CACHE_VERSION = 1;

    private static final int HALF_MINUTE = 30 * 1000;
    private static final int TWO_MINUTES = 4 * HALF_MINUTE;

    private final Context mContext;

	private int mSubSave;
	private int mTrackSave;

    public Bookmarker(Context context) {
        mContext = context;
    }

    public void setBookmark(Uri uri, int bookmark, int duration, int subsave, int tracksave) {
        try {
            BlobCache cache = CacheManager.getCache(mContext,
                    BOOKMARK_CACHE_FILE, BOOKMARK_CACHE_MAX_ENTRIES,
                    BOOKMARK_CACHE_MAX_BYTES, BOOKMARK_CACHE_VERSION);

            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(bos);
            dos.writeUTF(uri.toString());
            dos.writeInt(bookmark);
            dos.writeInt(duration);
			dos.writeInt(subsave);
			dos.writeInt(tracksave);
            dos.flush();
            cache.insert(uri.hashCode(), bos.toByteArray());
        } catch (Throwable t) {
            Log.w(TAG, "setBookmark failed", t);
        }
    }

    public Integer getBookmark(Uri uri) {
        try {
            BlobCache cache = CacheManager.getCache(mContext,
                    BOOKMARK_CACHE_FILE, BOOKMARK_CACHE_MAX_ENTRIES,
                    BOOKMARK_CACHE_MAX_BYTES, BOOKMARK_CACHE_VERSION);

            byte[] data = cache.lookup(uri.hashCode());
            if (data == null) return null;

            DataInputStream dis = new DataInputStream(
                    new ByteArrayInputStream(data));

            String uriString = DataInputStream.readUTF(dis);
            int bookmark = dis.readInt();
            int duration = dis.readInt();
			mSubSave = dis.readInt();
			mTrackSave = dis.readInt();

            if (!uriString.equals(uri.toString())) {
                return null;
            }

            if ((bookmark < HALF_MINUTE) || (duration < TWO_MINUTES)
                    || (bookmark > (duration - HALF_MINUTE))) {
                return null;
            }
            return Integer.valueOf(bookmark);
        } catch (Throwable t) {
            Log.w(TAG, "getBookmark failed", t);
        }
        return null;
    }
	public int getSubSave()
    {
        return mSubSave;
    }

    public int getTrackSave()
    {
        return mTrackSave;
    }
}
