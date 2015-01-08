package com.hxsmart.imateinterface;

import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BluetoothReceiver extends BroadcastReceiver {
	public void onReceive(Context context, Intent intent) {
		if (BluetoothThread.getInstance() == null)
			return;
		
        String action = intent.getAction();

        if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
            //Do something if connected
            Log.v("HxSmart_BluetoothReceiver", "BT Connected");
        }
        else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
            //Do something if disconnected
            Log.v("HxSmart_BluetoothReceiver","BT Disconnected");
            BluetoothThread.getInstance().reconnect();
        }
	}
}
