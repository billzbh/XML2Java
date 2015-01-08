package com.hxsmart.imateinterface.printer;


//用法：
//在onCreate中先于imate蓝牙线程 创建此对象（蓝牙连接动作）
//以后按需调用 blueprint 方法
// 在onDestroy中关闭连接

import com.android.print.sdk.PrinterConstants.Connect;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

public class DoPrint {
	private static DoPrint THIS = null;
	private Context context;
	private boolean isConnected;
	private boolean isCancel;
	BluetoothDevice device;
	private PrintUtil printUtil;
	
	private Handler mHandler = new Handler(){
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case Connect.SUCCESS:
				Toast.makeText(context, "connect OK", Toast.LENGTH_SHORT).show();
				printUtil.getPrinterAndRegister();
				isConnected = true;
				break;
			case Connect.FAILED:
				Toast.makeText(context, "请确保蓝牙打印机已经打开！", Toast.LENGTH_LONG).show();
				isConnected = false;
				break;
			case Connect.CLOSED:
				Toast.makeText(context, "connect closed", Toast.LENGTH_SHORT).show();
				isConnected = false ;
				break;
			default:
				break;
			}
		}
	};
	
	public DoPrint(Context from)
	{
		THIS=this;
		context = from;
		isConnected =false;
		isCancel =false;
		printUtil = new PrintUtil(context,mHandler,BluetoothAdapter.getDefaultAdapter());
		device = printUtil.getPrinterDevice();
	}
	
	public static DoPrint getInstance()
	{
		return THIS;
	}
	
	//停止打印机
	public void stopPrinter()
	{
		printUtil.closePrinter();
	}
	
	//打开打印机
	public void openPrinter()
	{
		printUtil.openPrinter();
	}
	
	//重新连接打印机
	public boolean reOpenPrinter()
	{
		device = printUtil.getPrinterDevice();
        if(device!=null)
		{
			printUtil.closePrinter();
			printUtil.openPrinter();
			isCancel = false;
			int i = 18;
			while( i > 0)
    		{
    			i--;
    			Log.i("wait to connect...", "等待连接...");
    			if(isCancel)
    			{
    				break;
    			}
    			if(isConnected)
    			{     
    				return true;
    			}
    			else
    			{
    				try {
    					Thread.sleep(500);
    				} catch (InterruptedException e) {
    					// TODO Auto-generated catch block
    					e.printStackTrace();
    				}
    				continue;
    			}		
    		}
    		if(i<=0)
    		{
    			return false;
    		}
    		
		}
		return false;
			
	}
	
	
	public void cancel()
	{
		isCancel = true;
	}
	
	//同样需要在多线程中运行
	//  闯入要打印的String	
	//	字符串
	//	成功返回1
	//	失败返回0
	public boolean bluePrint (String text)
	{
		if(!isConnected)
		{
	        device = printUtil.getPrinterDevice();
	        if(device!=null)
	        {
	        	openPrinter();
	        	isCancel = false;
	    		int i = 18;
	    		while( i > 0)
	    		{
	    			i--;
	    			Log.i("wait to connect...", "等待连接...");
	    			if(isCancel)
	    			{
	    				break;
	    			}
	    			if(isConnected)
	    			{
	    				Log.i("wait to print", "开始打印");
	    				printUtil.printText(text);      
	    				return true;
	    			}
	    			else
	    			{
	    				try {
	    					Thread.sleep(500);
	    				} catch (InterruptedException e) {
	    					// TODO Auto-generated catch block
	    					e.printStackTrace();
	    				}
	    				continue;
	    			}		
	    		}
	    		if(i<=0)
	    		{
	    			return false;
	    		}
	        }
	        else
	        {
	        	return false;
	        } 	
		}
		
		if(isConnected)
		{
			Log.e("wait to print", "this is not first print!");
			printUtil.printText(text);	
			return true;
		}	
		return false;
	}	
}