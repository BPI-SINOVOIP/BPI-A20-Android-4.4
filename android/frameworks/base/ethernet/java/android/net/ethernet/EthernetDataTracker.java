/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.net.ethernet;

//import android.net.*;
import android.net.NetworkUtils;
import android.net.DhcpResults;
import android.net.RouteInfo;
import android.net.ConnectivityManager;
import android.net.INetworkManagementEventObserver;
import android.net.NetworkStateTracker;
import android.net.LinkCapabilities;
import android.net.LinkProperties;
import android.net.NetworkInfo;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo.DetailedState;
import android.net.InterfaceConfiguration;
import android.net.LinkAddress;
import android.net.LinkQualityInfo;
import android.net.SamplingDataTracker;
//import android.net.ethernet.IEthernetManager;
//import android.net.ethernet.EthernetManager;
//import android.net.ethernet.EthernetDevInfo;
import android.os.Handler;
import android.os.IBinder;
import android.os.INetworkManagementService;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.util.Log;
import android.text.TextUtils;

import java.net.InetAddress;
import java.net.Inet4Address;
import java.net.UnknownHostException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.List;
import java.util.ArrayList;

/**
 * This class tracks the data connection associated with Ethernet
 * This is a singleton class and an instance will be created by
 * ConnectivityService.
 * @hide
 */
public class EthernetDataTracker implements NetworkStateTracker {
    private static final String NETWORKTYPE = "ETHERNET";
    private static final String TAG = "Ethernet";

    private AtomicBoolean mTeardownRequested = new AtomicBoolean(false);
    private AtomicBoolean mPrivateDnsRouteSet = new AtomicBoolean(false);
    private AtomicInteger mDefaultGatewayAddr = new AtomicInteger(0);
    private AtomicBoolean mDefaultRouteSet = new AtomicBoolean(false);

    private static boolean mLinkUp;
    private LinkProperties mLinkProperties;
    private LinkCapabilities mLinkCapabilities;
    private NetworkInfo mNetworkInfo;
    private InterfaceObserver mInterfaceObserver;
	private EthernetManager mEthManage;
	private INetworkManagementService mNMService;
    /* For sending events to connectivity service handler */
    private Handler mCsHandler;
    private Context mContext;
    private int prefixLength = 0;
    private static EthernetDataTracker sInstance;
    private static String sIfaceMatch = "";
    private static String mIface = "";

	/****  Begin of InterfaceObserver  ****/
    private static class InterfaceObserver extends INetworkManagementEventObserver.Stub {
        private EthernetDataTracker mTracker;

        InterfaceObserver(EthernetDataTracker tracker) {
            super();
            mTracker = tracker;
        }

        public void interfaceStatusChanged(String iface, boolean up) {
            //Log.d(TAG, "Interface status changed: " + iface + (up ? "up" : "down"));
        }

        public void interfaceLinkStateChanged(String iface, boolean up) {
			mTracker.interfaceLinkStateChanged(iface, up);
        }

        public void interfaceAdded(String iface) {
            mTracker.interfaceAdded(iface);
        }

        public void interfaceRemoved(String iface) {
            mTracker.interfaceRemoved(iface);
        }

        public void limitReached(String limitName, String iface) {
        }

        public void interfaceClassDataActivityChanged(String label, boolean active) {
            // Ignored.
        }
        
        public void addressUpdated(String address, String iface, int flags, int scope){
        }
        
        public void addressRemoved(String address, String iface, int flags, int scope){
        }
    }
/****  End of InterfaceObserver  ****/

    private EthernetDataTracker() {
        mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_ETHERNET,
													0, NETWORKTYPE, "");
        mLinkProperties = new LinkProperties();
        mLinkCapabilities = new LinkCapabilities();
        mLinkUp = false;

        mNetworkInfo.setIsAvailable(false);
        setTeardownRequested(false);
    }

    private void interfaceAdded(String iface) {
		/**  It will add an new interface to EthernetService and check it.  **/
		if(!mEthManage.addInterfaceToService(iface))
			return;

		/**  The first adding interface will be reconnect.  **/
        synchronized(mIface) {
            if(!mIface.isEmpty())
                return;
            mIface = iface;
        }

		NetworkUtils.enableInterface(mIface);
		reconnect();
    }

    private void interfaceRemoved(String iface) {

		mEthManage.removeInterfaceFormService(iface);
        if (!iface.matches(mIface))
            return;

		/*
        NetworkUtils.stopDhcp("eth_" + mIface);
        mLinkProperties.clear();
        mNetworkInfo.setIsAvailable(false);
        mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, null);

        Message msg = mCsHandler.obtainMessage(EVENT_CONFIGURATION_CHANGED,
													mNetworkInfo);
        msg.sendToTarget();
        msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
        msg.sendToTarget();
		*/
		teardown();

		synchronized(mIface){
			mIface = "";
		}
    }

	private void interfaceLinkStateChanged(String iface, boolean up) {
		int tmpStatus = 0;
		int i=0;
		if (mIface.matches(iface)) {
			for(i=0; i<3; i++){
				try{
					Thread.sleep(100);
				}catch(Exception e){ }
				tmpStatus = mEthManage.CheckLink(iface);
			}
			if(tmpStatus == -1){
				return;
			}
			up = (tmpStatus == 1) ? true : false;
			if(mLinkUp == up)
				return;

			mLinkUp = up;
			if (up) {
				reconnect();
			} else {
				NetworkUtils.stopDhcp("eth_" + mIface);
				mLinkProperties.clear();
				mNetworkInfo.setIsAvailable(false);
				mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, null);
				Message msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
				msg.sendToTarget();
				sendStateBroadcast(EthernetManager.EVENT_DISCONNECTED);
			}
		}
	}

    private void runDhcp() {
        Thread dhcpThread = new Thread(new Runnable() {
            public void run() {
				Log.d(TAG, "runDhcp enter");
		        DhcpResults dhcpResults = new DhcpResults();
                if (!NetworkUtils.runDhcp("eth_" + mIface, dhcpResults)) {
                    Log.e(TAG, "DHCP request error:" + NetworkUtils.getDhcpError());
					mNetworkInfo.setIsAvailable(false);
					mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, null);
					Message msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
					msg.sendToTarget();
					sendStateBroadcast(EthernetManager.EVENT_CONFIGURATION_FAILED);
                    return;
                }
                mLinkProperties = dhcpResults.linkProperties;
                mLinkProperties.setInterfaceName(mIface);

                mNetworkInfo.setDetailedState(DetailedState.CONNECTED, null, null);
				Log.d(TAG, "runDhcp setIsAvailable");
				mNetworkInfo.setIsAvailable(true);
                Message msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
                msg.sendToTarget();
				sendStateBroadcast(EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);
            }
        }, "ETH_DHCP");
        dhcpThread.start();
    }
    
    public DhcpResults getIpConfigure(EthernetDevInfo info){
        InetAddress netmask = null;
        InetAddress gw = null;
        RouteInfo routeAddress = null;
        DhcpResults mDhcpResult = new DhcpResults();
        
        if(info == null)
            return null;
        
        if(info.getNetMask() == null || info.getNetMask().matches("") ){
            netmask = NetworkUtils.numericToInetAddress("0.0.0.0");
        } else {
            netmask = NetworkUtils.numericToInetAddress(info.getNetMask());
        }
        prefixLength = NetworkUtils.netmaskIntToPrefixLength(NetworkUtils.inetAddressToInt((Inet4Address)netmask));
        mDhcpResult.addLinkAddress(info.getIpAddress(), prefixLength);
        if(info.getGateWay() != null && !info.getGateWay().matches(""))
            mDhcpResult.addGateway(info.getGateWay());
        else
            mDhcpResult.addGateway("0.0.0.0");
        
        mDhcpResult.addDns(info.getDnsAddr());
        return mDhcpResult;
	}
    
    public void ConnectNetwork(boolean up){
		//Log.d(TAG, "Up is " + up + " mLinkUp is " + mLinkUp +
		//			" On is " + mEthManage.isOn() + " mIface " + mIface);
		Log.d(TAG, "ConnectNetwork");
		if(!mEthManage.isConfigured()){
			return;
		}
		if(up && mEthManage.isOn()){
			EthernetDevInfo ifaceInfo = mEthManage.getSavedConfig();
			if(ifaceInfo == null)
				 return;
			synchronized(mIface){
				if(!mIface.equals(ifaceInfo.getIfName())){
					if(!mIface.isEmpty()){
						NetworkUtils.stopDhcp("eth_" + mIface);
						NetworkUtils.disableInterface(mIface);
					}
					mIface = ifaceInfo.getIfName();
				}
			}
			NetworkUtils.enableInterface(mIface);

			if(mLinkUp == false)
				return;

			if(mEthManage.isDhcp()){
				try{
					mNMService.clearInterfaceAddresses(mIface);
					NetworkUtils.resetConnections(mIface, 0);
				} catch (RemoteException e) {
					Log.e(TAG, "ERROR: " + e);
				}
				if(SystemProperties.get("dhcp." + mIface + ".result").equals("ok")){
					NetworkUtils.stopDhcp("eth_" + mIface);
					sendStateBroadcast(EthernetManager.EVENT_CONFIGURATION_FAILED);
				}
				runDhcp();
			}else{
				NetworkUtils.stopDhcp("eth_" + mIface);

				InterfaceConfiguration ifcg = new InterfaceConfiguration();
				DhcpResults mDhcpResult = getIpConfigure(ifaceInfo);
                mLinkProperties = mDhcpResult.linkProperties;
                mLinkProperties.setInterfaceName(mIface);
                LinkAddress linkAddress = new LinkAddress(NetworkUtils.numericToInetAddress(ifaceInfo.getIpAddress()),prefixLength);
                mDhcpResult.setInterfaceName(mIface);
				ifcg.setLinkAddress(linkAddress);
				ifcg.setInterfaceUp();
				try{
					mNMService.setInterfaceConfig(mIface, ifcg);
				} catch (Exception e) {
					Log.e(TAG, "ERROR: " + e);
					sendStateBroadcast(EthernetManager.EVENT_CONFIGURATION_FAILED);
					return;
				}
				Log.d(TAG, "ConnectNetwork setIsAvailable");
                mNetworkInfo.setDetailedState(DetailedState.CONNECTED, null, null);
				mNetworkInfo.setIsAvailable(true);
                Message msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
                msg.sendToTarget();
				sendStateBroadcast(EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);
			}
		} else if(isTeardownRequested()){
			NetworkUtils.stopDhcp("eth_" + mIface);
			mLinkProperties.clear();
			NetworkUtils.disableInterface(mIface);
			mNetworkInfo.setIsAvailable(false);
			mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, null);
			Message msg = mCsHandler.obtainMessage(EVENT_STATE_CHANGED, mNetworkInfo);
			msg.sendToTarget();
			sendStateBroadcast(EthernetManager.EVENT_DISCONNECTED);
		}
	}

    private void sendStateBroadcast(int event) {
        Intent intent = new Intent(EthernetManager.NETWORK_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
									| Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(EthernetManager.EXTRA_NETWORK_INFO, mNetworkInfo);
        intent.putExtra(EthernetManager.EXTRA_LINK_PROPERTIES,
							new LinkProperties (mLinkProperties));
		intent.putExtra(EthernetManager.EXTRA_ETHERNET_STATE, event);
        mContext.sendStickyBroadcast(intent);
    }

    public static synchronized EthernetDataTracker getInstance() {
        if (sInstance == null) sInstance = new EthernetDataTracker();
        return sInstance;
    }

    public Object Clone() throws CloneNotSupportedException {
        throw new CloneNotSupportedException();
    }

    public void setTeardownRequested(boolean isRequested) {
        mTeardownRequested.set(isRequested);
    }

    public boolean isTeardownRequested() {
        return mTeardownRequested.get();
    }

    /**
     * Begin monitoring connectivity
     */
	public void startMonitoring(Context context, Handler target) {
		mContext = context;
		mCsHandler = target;

		// register for notifications from NetworkManagement Service
		IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
		mNMService = INetworkManagementService.Stub.asInterface(b);

		mEthManage = EthernetManager.getInstance();

		if (mEthManage == null)
			Log.e("SHUGE", "mEthManage is nulllllllllllllllllllllllllllllllllllll");
		else
			Log.e("SHUGE", "mEthManage is OKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK");
		mInterfaceObserver = new InterfaceObserver(this);

		// enable and try to connect to an ethernet interface that
		// already exists
		sIfaceMatch = context.getResources().getString(
				com.android.internal.R.string.config_ethernet_iface_regex);
		//mEthManage.addInterfaceToService(sIfaceMatch);

		try {
			List<EthernetDevInfo> ethInfos = mEthManage.getDeviceNameList();
			EthernetDevInfo saveInfo = mEthManage.getSavedConfig();

			if(saveInfo != null && ethInfos != null){
				for (EthernetDevInfo info : ethInfos) {
					if (info.getIfName().matches(saveInfo.getIfName())){
						saveInfo.setIfName(info.getIfName());
						saveInfo.setHwaddr(info.getHwaddr());
						mEthManage.updateDevInfo(saveInfo);
					}
				}
			}

			final String[] ifaces = mNMService.listInterfaces();
			//for (String iface : ifaces) {
			//	if (iface.matches(sIfaceMatch)) {
			//		mIface = iface;
			//		mEthManage.setEnabled(mEthManage.isOn());
			//		break;
			//	}
			//}
		} catch (RemoteException e) {
			Log.e(TAG, "Could not get list of interfaces " + e);
		}

		try {
			mNMService.registerObserver(mInterfaceObserver);
		} catch (RemoteException e) {
			Log.e(TAG, "Could not register InterfaceObserver " + e);
		}
	}

    /**
     * Disable connectivity to a network
     * TODO: do away with return value after making MobileDataStateTracker async
     */
    public boolean teardown() {
        mTeardownRequested.set(true);
		ConnectNetwork(false);
        return true;
    }

    /**
     * Re-enable connectivity to a network after a {@link #teardown()}.
     */
    public boolean reconnect() {
        mTeardownRequested.set(false);
		ConnectNetwork(true);
        return true;
    }
    @Override
    public void captivePortalCheckComplete() {
        // not implemented
    }

    /**
     * Turn the wireless radio off for a network.
     * @param turnOn {@code true} to turn the radio on, {@code false}
     */
    public boolean setRadio(boolean turnOn) {
        return true;
    }

    /**
     * @return true - If are we currently tethered with another device.
     */
    public synchronized boolean isAvailable() {
        return mNetworkInfo.isAvailable();
    }

    /**
     * Tells the underlying networking system that the caller wants to
     * begin using the named feature. The interpretation of {@code feature}
     * is completely up to each networking implementation.
     * @param feature the name of the feature to be used
     * @param callingPid the process ID of the process that is issuing this request
     * @param callingUid the user ID of the process that is issuing this request
     * @return an integer value representing the outcome of the request.
     * The interpretation of this value is specific to each networking
     * implementation+feature combination, except that the value {@code -1}
     * always indicates failure.
     * TODO: needs to go away
     */
    public int startUsingNetworkFeature(String feature, int callingPid, int callingUid) {
        return -1;
    }

    /**
     * Tells the underlying networking system that the caller is finished
     * using the named feature. The interpretation of {@code feature}
     * is completely up to each networking implementation.
     * @param feature the name of the feature that is no longer needed.
     * @param callingPid the process ID of the process that is issuing this request
     * @param callingUid the user ID of the process that is issuing this request
     * @return an integer value representing the outcome of the request.
     * The interpretation of this value is specific to each networking
     * implementation+feature combination, except that the value {@code -1}
     * always indicates failure.
     * TODO: needs to go away
     */
    public int stopUsingNetworkFeature(String feature, int callingPid, int callingUid) {
        return -1;
    }

    @Override
    public void setUserDataEnable(boolean enabled) {
        Log.w(TAG, "ignoring setUserDataEnable(" + enabled + ")");
    }

    @Override
    public void setPolicyDataEnable(boolean enabled) {
        Log.w(TAG, "ignoring setPolicyDataEnable(" + enabled + ")");
    }

    /**
     * Check if private DNS route is set for the network
     */
    public boolean isPrivateDnsRouteSet() {
        return mPrivateDnsRouteSet.get();
    }

    /**
     * Set a flag indicating private DNS route is set
     */
    public void privateDnsRouteSet(boolean enabled) {
        mPrivateDnsRouteSet.set(enabled);
    }

    /**
     * Fetch NetworkInfo for the network
     */
    public synchronized NetworkInfo getNetworkInfo() {
        return mNetworkInfo;
    }

    /**
     * Fetch LinkProperties for the network
     */
    public synchronized LinkProperties getLinkProperties() {
        return new LinkProperties(mLinkProperties);
    }

   /**
     * A capability is an Integer/String pair, the capabilities
     * are defined in the class LinkSocket#Key.
     *
     * @return a copy of this connections capabilities, may be empty but never null.
     */
    public LinkCapabilities getLinkCapabilities() {
        return new LinkCapabilities(mLinkCapabilities);
    }

    /**
     * Fetch default gateway address for the network
     */
    public int getDefaultGatewayAddr() {
        return mDefaultGatewayAddr.get();
    }

    /**
     * Check if default route is set
     */
    public boolean isDefaultRouteSet() {
        return mDefaultRouteSet.get();
    }

    /**
     * Set a flag indicating default route is set for the network
     */
    public void defaultRouteSet(boolean enabled) {
        mDefaultRouteSet.set(enabled);
    }

    /**
     * Return the system properties name associated with the tcp buffer sizes
     * for this network.
     */
    public String getTcpBufferSizesPropName() {
        return "net.tcp.buffersize.wifi";
    }

    public void setDependencyMet(boolean met) {
        // not supported on this network
    }

    @Override
    public void addStackedLink(LinkProperties link) {
        mLinkProperties.addStackedLink(link);
    }

    @Override
    public void removeStackedLink(LinkProperties link) {
        mLinkProperties.removeStackedLink(link);
    }

    @Override
    public void supplyMessenger(Messenger messenger) {
        // not supported on this network
    }
    
    @Override
    public LinkQualityInfo getLinkQualityInfo(){
        return null;
    }
    
    @Override
    public void captivePortalCheckCompleted(boolean isCaptive){
    }
    
    @Override
    public String getNetworkInterfaceName(){
        if (mLinkProperties != null) {
            return mLinkProperties.getInterfaceName();
        } else {
            return null;
        }
    }
    
    @Override
    public void startSampling(SamplingDataTracker.SamplingSnapshot s){
    }
    
    @Override
    public void stopSampling(SamplingDataTracker.SamplingSnapshot s){
    }    
}
