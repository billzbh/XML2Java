package com.hxsmart.imateinterface.extension;


import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import android.os.Bundle;

public class HxCamera{
    public static final int DATA_URL = 0;              // Return base64 encoded string
    public static final int FILE_URI = 1;              // Return file path
    private int targetWidth;                // desired width of the image
    private int targetHeight;               // desired height of the image
;
    private static Context mmcontext=null;
    
    
	public void takePhoto(Context context,int Quality,int UriType,int Width,int Height,int requestCode)
	{
		this.targetWidth = Width;
		this.targetHeight = Height;
		this.mmcontext = context;
		
		// If the user specifies a 0 or smaller width/height
        // make it -1 so later comparisons succeed
        if (this.targetWidth < 1) {
            this.targetWidth = -1;
        }
        if (this.targetHeight < 1) {
            this.targetHeight = -1;
        }
        
		Bundle bundle = new Bundle();
		bundle.putInt("Quality", Quality);
		bundle.putInt("UriType", UriType);//BASE64串 , 或者文件路径 带有 file：//
		bundle.putInt("Width", targetWidth);
		bundle.putInt("Height", targetHeight);
		
		Intent intent = new Intent(context,CameraDataHandleActivity.class);
		intent.putExtras(bundle);
		((Activity)context).startActivityForResult(intent, requestCode);
	}
	
	public static Context getMainActivityInstance()
	{
		return mmcontext;
	}

}

