package com.hxsmart.imateinterface.extension;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.List;

import com.dtr.zbar.build.ZBarDecoder;

import android.graphics.Color;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class ZbarActivity extends Activity {

	private static final String TAG = "HXsmart";
    private static final int capture_crop_view_id=0x7f080002;
    private static final int capture_mask_top_id=0x7f080004;
    private static final int capture_mask_bottom_id = 0x7f080006;
    
	private Camera mCamera;
	private CameraPreview mPreview;
	private Handler autoFocusHandler;
	private CameraManager mCameraManager;

	private FrameLayout scanPreview;
	private Button scanCancel ;
	private Button flashswitch;
	private RelativeLayout scanContainer;
	private RelativeLayout scanCropView;
	private ImageView scanLine;

	private Rect mCropRect = null;
	private boolean previewing = true;
	private boolean isTrue = false;
	
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);  
        /*set it to be full screen*/
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,   
        WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		
		initUI();
		addEvents();
		initViews();
	}
	
    public final void initUI()
    {
    	//思路： 先创建父布局的参数对象，初始化上 子布局的属性，之后设置参数。
    	
    	//1. main 根布局
    	scanContainer = new RelativeLayout(this);
    	//设置参数
    	scanContainer.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
		
		
    	//2.第一层布局
    	//2.1 framelayout
    	scanPreview = new FrameLayout(this);
    	scanPreview.setLayoutParams(new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
    	//添加到父控件
    	scanContainer.addView(scanPreview);
    	
    	
    	//2.2 RelativeLayout
    	DisplayMetrics dm = new DisplayMetrics();
    	//获取屏幕信息
    	getWindowManager().getDefaultDisplay().getMetrics(dm);
    	int screenWidth = dm.widthPixels;
    	int screenHeigh = dm.heightPixels;
    	
    	Log.i("zbh","width="+screenWidth+",height = " +screenHeigh);
    	//根据屏幕指定扫描框的大小
    	int x=screenWidth/8*6;
    	int y=screenWidth/8*6;
    	
    	scanCropView = new RelativeLayout(this);
    	scanCropView.setId(capture_crop_view_id);
    	RelativeLayout.LayoutParams relativepaLayoutParams = new RelativeLayout.LayoutParams(x,y);
    	relativepaLayoutParams.addRule(RelativeLayout.CENTER_HORIZONTAL,RelativeLayout.TRUE);
    	relativepaLayoutParams.addRule(RelativeLayout.CENTER_VERTICAL,RelativeLayout.TRUE);
    	scanCropView.setBackgroundResource(getResourceId(HxBarcode.getMainActivityInstance(), "qr_code_bg", "drawable"));
    	//添加到父控件
    	scanContainer.addView(scanCropView, relativepaLayoutParams);
    	
    	
    	//2.2.1 image
    	scanLine = new ImageView(this);
    	RelativeLayout.LayoutParams imageParams = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.WRAP_CONTENT);
    	imageParams.bottomMargin=dip2px(this, 5);
    	imageParams.topMargin=dip2px(this, 5);
    	imageParams.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
    	scanLine.setImageResource(getResourceId(HxBarcode.getMainActivityInstance(), "scan_line", "drawable"));
    	
    	//添加到父控件
    	scanCropView.addView(scanLine, imageParams);
    	
    	
    	
    	//2.3    RelativeLayout
    	RelativeLayout capture_mask_top = new RelativeLayout(this);
    	capture_mask_top.setId(capture_mask_top_id);
    	capture_mask_top.setBackgroundResource(getResourceId(HxBarcode.getMainActivityInstance(), "shadow", "drawable"));
    	RelativeLayout.LayoutParams Params = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT);
    	Params.addRule(RelativeLayout.ABOVE, capture_crop_view_id);
    	Params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
    	
    	//添加到父控件
    	scanContainer.addView(capture_mask_top, Params);
    	
    	//2.3.1  button 
    	scanCancel = new Button(this);
    	scanCancel.setText("取消");
    	scanCancel.setTextSize(18);
    	scanCancel.setTextColor(Color.rgb(0xFF, 0xFA, 0xFA));
    	scanCancel.getBackground().setAlpha(0x00);
    	scanCancel.setGravity(Gravity.CENTER);
    	
    	RelativeLayout.LayoutParams cancelParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    	cancelParams.leftMargin=dip2px(this, 25);
    	cancelParams.topMargin = dip2px(this, 25);
    	cancelParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
    	
    	
    	//添加到父控件
    	scanCancel.setLayoutParams(cancelParams);
    	capture_mask_top.addView(scanCancel);
    	
    	//2.3.2  button
    	flashswitch = new Button(this);
    	flashswitch.setText("闪光灯");
    	flashswitch.setTextSize(18);
    	flashswitch.setTextColor(Color.rgb(0xFF, 0xFA, 0xFA));
    	flashswitch.getBackground().setAlpha(0x00);
    	flashswitch.setGravity(Gravity.CENTER);
    	
    	RelativeLayout.LayoutParams flashlightingParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    	flashlightingParams.topMargin = dip2px(this, 25);
    	flashlightingParams.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
    	
    	//添加到父控件
    	flashswitch.setLayoutParams(flashlightingParams);
    	capture_mask_top.addView(flashswitch);
    	
    	//2.4    RelativeLayout
    	RelativeLayout capture_mask_bottom = new RelativeLayout(this);
    	capture_mask_bottom.setId(capture_mask_bottom_id);
    	capture_mask_bottom.setBackgroundResource(getResourceId(HxBarcode.getMainActivityInstance(), "shadow", "drawable"));
    	RelativeLayout.LayoutParams bottomParams = new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.WRAP_CONTENT);
    	bottomParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
    	bottomParams.addRule(RelativeLayout.BELOW, capture_crop_view_id);
    	//添加到父控件
    	scanContainer.addView(capture_mask_bottom, bottomParams);
    	
    	//2.4.1  TextView
    	TextView tiptext = new TextView(this);
    	tiptext.setText("将二维码或条码放入框内，即可自动扫描");
    	tiptext.setTextColor(Color.rgb(0xff, 0xff, 0xff));//white
    	
    	RelativeLayout.LayoutParams textParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    	textParams.topMargin = dip2px(this, 5);
    	textParams.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
    	
    	//添加到父控件
    	capture_mask_bottom.addView(tiptext, textParams);
    	
    	//2.5    imageView
    	ImageView leftimage = new ImageView(this);
    	leftimage.setBackgroundResource(getResourceId(HxBarcode.getMainActivityInstance(), "shadow", "drawable"));
    	RelativeLayout.LayoutParams leftimageParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.MATCH_PARENT);
    	leftimageParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
    	leftimageParams.addRule(RelativeLayout.ABOVE, capture_mask_bottom_id);
    	leftimageParams.addRule(RelativeLayout.BELOW,capture_mask_top_id);
    	leftimageParams.addRule(RelativeLayout.LEFT_OF,capture_crop_view_id);
    	//添加到父控件
    	scanContainer.addView(leftimage, leftimageParams);
    	
    	//2.6    imageView
    	ImageView rightimage = new ImageView(this);
    	rightimage.setBackgroundResource(getResourceId(HxBarcode.getMainActivityInstance(), "shadow", "drawable"));
    	RelativeLayout.LayoutParams rightimageParams = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.MATCH_PARENT);
    	rightimageParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
    	rightimageParams.addRule(RelativeLayout.ABOVE, capture_mask_bottom_id);
    	rightimageParams.addRule(RelativeLayout.BELOW,capture_mask_top_id);
    	rightimageParams.addRule(RelativeLayout.RIGHT_OF,capture_crop_view_id);
    	//添加到父控件
    	scanContainer.addView(rightimage, rightimageParams);
    	
    	
    	setContentView(scanContainer);
    }

	private void addEvents() {
		flashswitch.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				if(isTrue)
				{
					turnLightOff(mCamera);
					isTrue=false;
					flashswitch.setTextColor(Color.rgb(0xFA, 0xFA, 0xFF));
				}
				else
				{
					turnLightOn(mCamera);
					isTrue = true;
					flashswitch.setTextColor(Color.rgb(0xFF, 0x00, 0x00));
				}
			}
		});
		
		scanCancel.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				Intent dataStr = new Intent();
				dataStr.putExtra("data", "Scan canceled!");
				setResult(1, dataStr);
				finish();
			}
		});
		
		scanCancel.setOnTouchListener(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub
				if(event.getAction()==MotionEvent.ACTION_DOWN){  
	                ((Button)v).setTextColor(Color.rgb(0xe1, 0x69, 0x41));
	            }else if(event.getAction()==MotionEvent.ACTION_UP){  
	            	((Button)v).setTextColor(Color.rgb(0xFA, 0xFA, 0xFF));  
	            }
				return false;
			}
		});
	}

	private void initViews() {
		autoFocusHandler = new Handler();
		mCameraManager = new CameraManager(this);
		try {
			mCameraManager.openDriver();
		} catch (IOException e) {
			e.printStackTrace();
		}

		mCamera = mCameraManager.getCamera();
		mPreview = new CameraPreview(this, mCamera, previewCb, autoFocusCB);
		scanPreview.addView(mPreview);

		TranslateAnimation animation = new TranslateAnimation(Animation.RELATIVE_TO_PARENT, 0.0f, Animation.RELATIVE_TO_PARENT, 0.0f, Animation.RELATIVE_TO_PARENT, 0.0f, Animation.RELATIVE_TO_PARENT,
				0.85f);
		animation.setDuration(3000);
		animation.setRepeatCount(-1);
		animation.setRepeatMode(Animation.REVERSE);
		scanLine.startAnimation(animation);
	}

	public void onPause() {
		super.onPause();
		previewing = false;
		mCamera.setPreviewCallback(null);
		mCamera.stopPreview();
		if(isTrue)
		{
			turnLightOff(mCamera);
			flashswitch.setTextColor(Color.rgb(0xFA, 0xFA, 0xFF));
		}
	}
	
	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
		mCamera.setPreviewCallback(previewCb);
		mCamera.startPreview();
		previewing = true;
		mCamera.autoFocus(autoFocusCB);	
	}
	
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		Log.i("zbh","onDestroy and release camera ");
		releaseCamera();
	}

	private void releaseCamera() {
		if (mCamera != null) {
			previewing = false;
			mCamera.setPreviewCallback(null);
			mCamera.release();
			mCamera = null;
		}
	}

	private Runnable doAutoFocus = new Runnable() {
		public void run() {
			if (previewing)
				mCamera.autoFocus(autoFocusCB);
		}
	};

	PreviewCallback previewCb = new PreviewCallback() {
		public void onPreviewFrame(byte[] data, Camera camera) {
			Size size = camera.getParameters().getPreviewSize();

			// 这里需要将获取的data翻转一下，因为相机默认拿的的横屏的数据
			byte[] rotatedData = new byte[data.length];
			for (int y = 0; y < size.height; y++) {
				for (int x = 0; x < size.width; x++)
					rotatedData[x * size.height + size.height - y - 1] = data[x + y * size.width];
			}

			// 宽高也要调整
			int tmp = size.width;
			size.width = size.height;
			size.height = tmp;

			initCrop();
			ZBarDecoder zBarDecoder = new ZBarDecoder();
			String result = zBarDecoder.decodeCrop(rotatedData, size.width, size.height, mCropRect.left, mCropRect.top, mCropRect.width(), mCropRect.height());

			if (!TextUtils.isEmpty(result)) {
				if(result.length()>4)
				{
					previewing = false;
					mCamera.setPreviewCallback(null);
					mCamera.stopPreview();
					//添加震动效果，提示用户扫描完成  
	                Vibrator mVibrator=(Vibrator)getSystemService(VIBRATOR_SERVICE);  
	                mVibrator.vibrate(100);
	                
					Intent dataStr = new Intent();
					dataStr.putExtra("data", result);
					setResult(Activity.RESULT_OK, dataStr);
					finish();
				}
				
			}
		}
	};

	// Mimic continuous auto-focusing
	AutoFocusCallback autoFocusCB = new AutoFocusCallback() {
		public void onAutoFocus(boolean success, Camera camera) {
			autoFocusHandler.postDelayed(doAutoFocus, 1000);
		}
	};

	/**
	 * 初始化截取的矩形区域
	 */
	private void initCrop() {
		int cameraWidth = mCameraManager.getCameraResolution().y;
		int cameraHeight = mCameraManager.getCameraResolution().x;

		/** 获取布局中扫描框的位置信息 */
		int[] location = new int[2];
		scanCropView.getLocationInWindow(location);

		int cropLeft = location[0];
		
//		int cropTop = location[1] - getStatusBarHeight();
		//全屏所以不用减
		int cropTop = location[1];
		
		int cropWidth = scanCropView.getWidth();
		int cropHeight = scanCropView.getHeight();

		/** 获取布局容器的宽高 */
		int containerWidth = scanContainer.getWidth();
		int containerHeight = scanContainer.getHeight();

		/** 计算最终截取的矩形的左上角顶点x坐标 */
		int x = cropLeft * cameraWidth / containerWidth;
		/** 计算最终截取的矩形的左上角顶点y坐标 */
		int y = cropTop * cameraHeight / containerHeight;

		/** 计算最终截取的矩形的宽度 */
		int width = cropWidth * cameraWidth / containerWidth;
		/** 计算最终截取的矩形的高度 */
		int height = cropHeight * cameraHeight / containerHeight;

		/** 生成最终的截取的矩形 */
		mCropRect = new Rect(x, y, width + x, height + y);
	}

	private int getStatusBarHeight() {
		try {
			Class<?> c = Class.forName("com.android.internal.R$dimen");
			Object obj = c.newInstance();
			Field field = c.getField("status_bar_height");
			int x = Integer.parseInt(field.get(obj).toString());
			return getResources().getDimensionPixelSize(x);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0;
	}
	
	 /** 
     * 通过设置Camera打开闪光灯 
     * @param mCamera 
     */  
    public static void turnLightOn(Camera mCamera) {  
        if (mCamera == null) {  
            return;  
        }  
        Parameters parameters = mCamera.getParameters();  
        if (parameters == null) {  
            return;  
        }  
    List<String> flashModes = parameters.getSupportedFlashModes();  
        // Check if camera flash exists  
        if (flashModes == null) {  
            // Use the screen as a flashlight (next best thing)  
            return;  
        }  
        String flashMode = parameters.getFlashMode();  
        Log.i(TAG, "Flash mode: " + flashMode);  
        Log.i(TAG, "Flash modes: " + flashModes);  
        if (!Parameters.FLASH_MODE_TORCH.equals(flashMode)) {  
            // Turn on the flash  
            if (flashModes.contains(Parameters.FLASH_MODE_TORCH)) {  
                parameters.setFlashMode(Parameters.FLASH_MODE_TORCH);  
                mCamera.setParameters(parameters);  
            } else {
            	Log.e(TAG, "FLASH_MODE_TORCH not supported"); 
            }  
        }  
    }
    
    /** 
     * 通过设置Camera关闭闪光灯 
     * @param mCamera 
     */  
    public static void turnLightOff(Camera mCamera) {  
        if (mCamera == null) {  
            return;  
        }  
        Parameters parameters = mCamera.getParameters();  
        if (parameters == null) {  
            return;  
        }  
        List<String> flashModes = parameters.getSupportedFlashModes();  
        String flashMode = parameters.getFlashMode();  
        // Check if camera flash exists  
        if (flashModes == null) {  
            return;  
        }  
        Log.i(TAG, "Flash mode: " + flashMode);  
        Log.i(TAG, "Flash modes: " + flashModes);  
        if (!Parameters.FLASH_MODE_OFF.equals(flashMode)) {  
            // Turn off the flash  
            if (flashModes.contains(Parameters.FLASH_MODE_OFF)) {  
                parameters.setFlashMode(Parameters.FLASH_MODE_OFF);  
                mCamera.setParameters(parameters);  
            } else {  
                Log.e(TAG, "FLASH_MODE_OFF not supported");  
            }  
        }  
    }
    
    /** 
     * 获取项目工程中的资源ID 
     * @param  主Activity的context
     * @param  资源文件的文件名
     * @param  资源的类型（如 id ， layou ， drawable等）
     */
    static int getResourceId(Context context,String name,String type){
    	 
        Resources themeResources=null;
        PackageManager pm=context.getPackageManager();
         try {
        	 String packageName = context.getApplicationContext().getPackageName();
        	 themeResources=pm.getResourcesForApplication(packageName);
            return themeResources.getIdentifier(name, type, packageName);
         } catch(NameNotFoundException e) {

           e.printStackTrace();
         }
         return 0;
   } 
    
    /** 
     * 根据手机的分辨率从 dp 的单位 转成为 px(像素) 
     */  
    public static int dip2px(Context context, float dpValue) {  
        final float scale = context.getResources().getDisplayMetrics().density;  
        return (int) (dpValue * scale + 0.5f);  
    }  
  
    /** 
     * 根据手机的分辨率从 px(像素) 的单位 转成为 dp 
     */  
    public static int px2dip(Context context, float pxValue) {  
        final float scale = context.getResources().getDisplayMetrics().density;  
        return (int) (pxValue / scale + 0.5f);  
    }
    
    @Override 
    public void onBackPressed() { 
    	
    	Intent dataStr = new Intent();
		dataStr.putExtra("data", "Scan canceled!");
		setResult(1, dataStr);
		finish();
    } 
}
