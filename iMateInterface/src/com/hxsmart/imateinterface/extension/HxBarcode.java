package com.hxsmart.imateinterface.extension;

import android.content.Context;
import android.content.Intent;
import android.app.Activity;

public class HxBarcode{
	
    private static Context mmcontext=null;
	
	public void scan(Context context,int requestCode)
	{
		this.mmcontext = context;
		
		Intent intent = new Intent();
		intent.setClass(context,ZbarActivity.class);
		((Activity)context).startActivityForResult(intent, requestCode);
	}
	
	public static Context getMainActivityInstance()
	{
		return mmcontext;
	}
	
}