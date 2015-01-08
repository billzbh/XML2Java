package com.hxsmart.imateinterface.printer;

import java.util.Set;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.util.Log;

import com.android.print.sdk.PrinterConstants.Command;
import com.android.print.sdk.PrinterInstance;

//设置handler
//先调用getPrinterDevice，判断是否不为null。
//open（不断查询是否为连接状态，才能打印）
//printText
//close


public class PrintUtil{

    private BluetoothDevice mmDevice;
    private PrinterInstance mmPrinter;
    private Context mmContext;
	private Handler mHandler;
	private boolean hasRegDisconnectReceiver;
	private IntentFilter filter;
	private BluetoothAdapter mmAdapter;
	
    
    public PrintUtil(Context context,Handler handler,BluetoothAdapter adapter) 
	{
    	mmContext =  context;
    	mHandler = handler;
    	mmAdapter = adapter;
    	hasRegDisconnectReceiver = false;
    	
		filter = new IntentFilter();
        //filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        //filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
	}
    
	public BluetoothDevice getPrinterDevice()
	{
		if(mmDevice!=null)
			return mmDevice;
		// Search Tpos device
		Set<BluetoothDevice> pairedDevices = mmAdapter
				.getBondedDevices();
		// If there are paired devices
		if (pairedDevices.size() <= 0)
			return null;

		//获取打印机的蓝牙设备
		for (BluetoothDevice device : pairedDevices) {		
			if (device.getName().contains("T7 BT Printer")) {
					Log.v("Printer_device", device.getName());
					mmDevice = device;	
					break;
			}	
		}
		return mmDevice;
	}
    
	public void printText(String text)
	{
		mmPrinter.init();
		//返回值 int 类型
		mmPrinter.printText(text);
		mmPrinter.setPrinter(Command.PRINT_AND_WAKE_PAPER_BY_LINE, 2); // 换2行		
	}
	
	public void openPrinter()
	{
		if(mmDevice !=null)
		{
			mmPrinter = new PrinterInstance(mmContext, mmDevice, mHandler);
			if(mmPrinter != null)
				mmPrinter.openConnection();
		}
	}
	
	public void closePrinter() {
		if (mmPrinter != null) {
			mmPrinter.closeConnection();
			mmPrinter = null;
		}
		if(hasRegDisconnectReceiver){
			mmContext.unregisterReceiver(myReceiver);
			hasRegDisconnectReceiver = false;
		}
	}
	
	

	public PrinterInstance getPrinterAndRegister() {
		if (mmPrinter != null && mmPrinter.isConnected()) {
			if(!hasRegDisconnectReceiver){
				mmContext.registerReceiver(myReceiver, filter);
				hasRegDisconnectReceiver = true;
			}
		}
		return mmPrinter;
	}
	
	private BroadcastReceiver myReceiver = new BroadcastReceiver() {
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
			if (action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED)) {
				if (device != null && mmPrinter != null && mmPrinter.isConnected() && device.equals(mmDevice)) {
					closePrinter();
				}
			}
		}
	};
	
	public boolean isPrinterConnected()
	{
		return hasRegDisconnectReceiver;
	}
}
