package com.hxsmart.imateinterface.extension;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Base64;
import android.util.Log;

public class CameraDataHandleActivity extends Activity implements MediaScannerConnectionClient{
	
    public static final int DATA_URL = 0;              // Return base64 encoded string
    public static final int FILE_URI = 1;              // Return file path
    private static final String LOG_TAG = "HxCamera";
    private static final int Requestcode = 0xFF;
    private int mQuality;                   // Compression quality hint (0-100: 0=low quality & high compression, 100=compress of max quality)
    private int targetWidth;                // desired width of the image
    private int targetHeight;               // desired height of the image
    private int UriType;                    // Type of encoding to use
    
    private File srcImage;      //原始图片文件
    private Uri imageUri;
    private Context context;
    
    private int numPics;
    private MediaScannerConnection conn;    // Used to update gallery app with newly-written files
    private Uri scanMe;                     // Uri of image to be added to content store
    
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	// TODO Auto-generated method stub
    	super.onCreate(savedInstanceState);
    	Intent it = getIntent();
    	Bundle bundle = it.getExtras();
    	mQuality = bundle.getInt("Quality");
    	targetWidth = bundle.getInt("Width");
    	targetHeight = bundle.getInt("Height");
    	UriType = bundle.getInt("UriType");
    	context = HxCamera.getMainActivityInstance();
    	
    	takePicture(Requestcode);
    }
	
	public void takePicture(int requestCode) {
		// Save the number of images currently on disk for later
        this.numPics = queryImgDB(whichContentStore()).getCount();
        
        
        // Display camera
        Intent intent = new Intent("android.media.action.IMAGE_CAPTURE");

        // Specify file so that large image is captured and returned
        srcImage = createCaptureFile();
        intent.putExtra(android.provider.MediaStore.EXTRA_OUTPUT, Uri.fromFile(srcImage));
        this.imageUri = Uri.fromFile(srcImage);

        startActivityForResult(intent, requestCode);
        
    }
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		// TODO Auto-generated method stub
		super.onActivityResult(requestCode, resultCode, data);
		System.out.printf("============ :%d,%d\n", requestCode,resultCode);
		switch(requestCode)
		{
		case Requestcode:
			if (resultCode == Activity.RESULT_OK) {
                try {
                    this.processResultFromCamera(UriType, data);
                } catch (IOException e) {
                    e.printStackTrace();
                    this.ResultPicture("Error capturing image.");
                }
            }

            // If cancelled
            else if (resultCode == Activity.RESULT_CANCELED) {
                this.ResultPicture("Camera cancelled.");
            }

            // If something else
            else {
                this.ResultPicture("Did not complete!");
            }
			break;
		}
	}
	
	public void processResultFromCamera(int destType, Intent intent) throws IOException 
	{
        int rotate = 0;

        // Create an ExifHelper to save the exif data that is lost during compression
        ExifHelper exif = new ExifHelper();
        try {
                exif.createInFile(getTempDirectoryPath() + "/.Pic.jpg");
                exif.readExifData();
                rotate = exif.getOrientation();
        } catch (IOException e) {
            e.printStackTrace();
        }

        Bitmap bitmap = null;
        Uri uri = null;

        // If sending base64 image back
        if (destType == DATA_URL) 
        {
            bitmap = getScaledBitmap(FileHelper.stripFileProtocol(imageUri.toString()));
            if (bitmap == null) {
                // Try to get the bitmap from intent.
                bitmap = (Bitmap)intent.getExtras().get("data");
            }
            
            // Double-check the bitmap.
            if (bitmap == null) {
                Log.d(LOG_TAG, "I either have a null image path or bitmap");
                this.ResultPicture("Unable to create bitmap!");
                return;
            }
            
            checkForDuplicateImage(DATA_URL);
            this.processPicture(bitmap);
//            checkForDuplicateImage(DATA_URL);
        }

        // If sending filename back
        else if (destType == FILE_URI) 
        {
            
//            uri = Uri.fromFile(new File(getTempDirectoryPath(), System.currentTimeMillis() + ".jpg"));
        	File oldfile = new File(getTempDirectoryPath(), "Modified_finaly" + ".jpg");
        	if(oldfile.exists())
        		oldfile.delete();
        	
        	uri = Uri.fromFile(new File(getTempDirectoryPath(), "Modified_finaly" + ".jpg"));

            if (uri == null)
            {
            	this.ResultPicture("Error capturing image - no media storage found.");
            }

            // If all this is true we shouldn't compress the image.
            if (this.targetHeight == -1 && this.targetWidth == -1 && this.mQuality == 100) 
            {
                writeUncompressedImage(uri);
//                Log.e("zbh",uri.toString());
                this.cleanup(FILE_URI, this.imageUri, uri, bitmap);
                bitmap = null;
                ResultPicture(uri.toString());
            } 
            else 
            {
                bitmap = getScaledBitmap(FileHelper.stripFileProtocol(imageUri.toString()));


                // Add compressed version of captured image to returned media store Uri
                OutputStream os = context.getContentResolver().openOutputStream(uri);
                bitmap.compress(Bitmap.CompressFormat.JPEG, this.mQuality, os);
                os.close();

                // Restore exif data to file
                
                String exifPath;
                    
                exifPath = uri.getPath();
                    
                exif.createOutFile(exifPath);
                exif.writeExifData();

//                Log.e("zbh",uri.toString());
                this.cleanup(FILE_URI, this.imageUri, uri, bitmap);
                bitmap = null;
                ResultPicture(uri.toString());
        
            }

        }
    }
	
	
	/**
     * In the special case where the default width, height and quality are unchanged
     * we just write the file out to disk saving the expensive Bitmap.compress function.
     *
     * @param uri
     * @throws FileNotFoundException
     * @throws IOException
     */
    private void writeUncompressedImage(Uri uri) throws FileNotFoundException,
            IOException {
        FileInputStream fis = new FileInputStream(FileHelper.stripFileProtocol(imageUri.toString()));
        OutputStream os = context.getContentResolver().openOutputStream(uri);
        byte[] buffer = new byte[4096];
        int len;
        while ((len = fis.read(buffer)) != -1) {
            os.write(buffer, 0, len);
        }
        os.flush();
        os.close();
        fis.close();
    }
    
    /**
     * Return a scaled bitmap based on the target width and height
     *
     * @param imagePath
     * @return
     * @throws IOException 
     */
    private Bitmap getScaledBitmap(String imageUrl) throws IOException {
        // If no new width or height were specified return the original bitmap
        if (this.targetWidth <= 0 && this.targetHeight <= 0) {
            return BitmapFactory.decodeStream(FileHelper.getInputStreamFromUriString(imageUrl, context));
        }

        // figure out the original width and height of the image
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeStream(FileHelper.getInputStreamFromUriString(imageUrl, context), null, options);
        
        //CB-2292: WTF? Why is the width null?
        if(options.outWidth == 0 || options.outHeight == 0)
        {
            return null;
        }
        
        // determine the correct aspect ratio
        int[] widthHeight = calculateAspectRatio(options.outWidth, options.outHeight);

        // Load in the smallest bitmap possible that is closest to the size we want
        options.inJustDecodeBounds = false;
        options.inSampleSize = calculateSampleSize(options.outWidth, options.outHeight, this.targetWidth, this.targetHeight);
        Bitmap unscaledBitmap = BitmapFactory.decodeStream(FileHelper.getInputStreamFromUriString(imageUrl, context), null, options);
        if (unscaledBitmap == null) {
            return null;
        }

        return Bitmap.createScaledBitmap(unscaledBitmap, widthHeight[0], widthHeight[1], true);
    }
    
    /**
     * Determine if we are storing the images in internal or external storage
     * @return Uri
     */
    private Uri whichContentStore() {
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            return android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
        } else {
            return android.provider.MediaStore.Images.Media.INTERNAL_CONTENT_URI;
        }
    }
    
    /**
     * Maintain the aspect ratio so the resulting image does not look smooshed
     *
     * @param origWidth
     * @param origHeight
     * @return
     */
    public int[] calculateAspectRatio(int origWidth, int origHeight) {
        int newWidth = this.targetWidth;
        int newHeight = this.targetHeight;

        // If no new width or height were specified return the original bitmap
        if (newWidth <= 0 && newHeight <= 0) {
            newWidth = origWidth;
            newHeight = origHeight;
        }
        // Only the width was specified
        else if (newWidth > 0 && newHeight <= 0) {
            newHeight = (newWidth * origHeight) / origWidth;
        }
        // only the height was specified
        else if (newWidth <= 0 && newHeight > 0) {
            newWidth = (newHeight * origWidth) / origHeight;
        }
        // If the user specified both a positive width and height
        // (potentially different aspect ratio) then the width or height is
        // scaled so that the image fits while maintaining aspect ratio.
        // Alternatively, the specified width and height could have been
        // kept and Bitmap.SCALE_TO_FIT specified when scaling, but this
        // would result in whitespace in the new image.
        else {
            double newRatio = newWidth / (double) newHeight;
            double origRatio = origWidth / (double) origHeight;

            if (origRatio > newRatio) {
                newHeight = (newWidth * origHeight) / origWidth;
            } else if (origRatio < newRatio) {
                newWidth = (newHeight * origWidth) / origHeight;
            }
        }

        int[] retval = new int[2];
        retval[0] = newWidth;
        retval[1] = newHeight;
        return retval;
    }

    /**
     * Figure out what ratio we can load our image into memory at while still being bigger than
     * our desired width and height
     *
     * @param srcWidth
     * @param srcHeight
     * @param dstWidth
     * @param dstHeight
     * @return
     */
    public static int calculateSampleSize(int srcWidth, int srcHeight, int dstWidth, int dstHeight) {
        final float srcAspect = (float)srcWidth / (float)srcHeight;
        final float dstAspect = (float)dstWidth / (float)dstHeight;

        if (srcAspect > dstAspect) {
            return srcWidth / dstWidth;
        } else {
            return srcHeight / dstHeight;
        }
      }
    
    /**
     * Cleans up after picture taking. Checking for duplicates and that kind of stuff.
     * @param newImage
     */
    private void cleanup(int imageType, Uri oldImage, Uri newImage, Bitmap bitmap) {
        if (bitmap != null) {
            bitmap.recycle();
        }

        // Clean up initial camera-written image file.
        (new File(FileHelper.stripFileProtocol(oldImage.toString()))).delete();

        checkForDuplicateImage(imageType);

        System.gc();
    }
    
    private void checkForDuplicateImage(int type) {
        int diff = 1;
        Uri contentStore = whichContentStore();
        Cursor cursor = queryImgDB(contentStore);
        int currentNumOfImages = cursor.getCount();

        if (type == FILE_URI ) {
            diff = 2;
        }

        // delete the duplicate file if the difference is 2 for file URI or 1 for Data URL
        if ((currentNumOfImages - numPics) == diff) {
            cursor.moveToLast();
            int id = Integer.valueOf(cursor.getString(cursor.getColumnIndex(MediaStore.Images.Media._ID)));
            if (diff == 2) {
                id--;
            }
            Uri uri = Uri.parse(contentStore + "/" + id);
            context.getContentResolver().delete(uri, null, null);
            cursor.close();
        }
    }
    
    /**
     * Creates a cursor that can be used to determine how many images we have.
     *
     * @return a cursor
     */
    private Cursor queryImgDB(Uri contentStore) {
        return context.getContentResolver().query(
                contentStore,
                new String[] { MediaStore.Images.Media._ID },
                null,
                null,
                null);
    }
	
    /**
     * Compress bitmap using jpeg, convert to Base64 encoded string, and return to JavaScript.
     *
     * @param bitmap
     */
    public void processPicture(Bitmap bitmap) {
        ByteArrayOutputStream jpeg_data = new ByteArrayOutputStream();
        try {
            if (bitmap.compress(CompressFormat.JPEG, mQuality, jpeg_data)) {
                byte[] code = jpeg_data.toByteArray();
                byte[] output = Base64.encode(code, Base64.NO_WRAP);
                String js_out = new String(output);

                ResultPicture(js_out);
            }
        } catch (Exception e) {
        	jpeg_data = null;
            this.ResultPicture("Error compressing image.");
        }
    }
	
	
    private String getTempDirectoryPath() {
        File cache = null;

        // SD Card Mounted
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            cache = new File(Environment.getExternalStorageDirectory().getAbsolutePath() +
                    "/Android/data/" + ((Activity) context).getPackageName() + "/cache/");
        }
        // Use internal storage
        else {
            cache = ((Activity) context).getCacheDir();
        }

        // Create the cache directory if it doesn't exist
        cache.mkdirs();
        return cache.getAbsolutePath();
    }
    
	/**
     * Create a file in the applications temporary directory based upon the supplied encoding.
     *
     * @param encodingType of the image to be taken
     * @return a File object pointing to the temporary picture
     */
    private File createCaptureFile() {
        return new File(getTempDirectoryPath(), "/.Pic.jpg");
    }
    
    /**
     * 
     *
     * @param err
     */
    public void ResultPicture(String msg) {
		Intent intent = new Intent();
		intent.putExtra("data", msg);
		setResult(533, intent);
		finish();
    }
	
	@Override
	public void onMediaScannerConnected() {
		// TODO Auto-generated method stub
		try{
            this.conn.scanFile(this.scanMe.toString(), "image/*");
        } catch (java.lang.IllegalStateException e){
            Log.e(LOG_TAG, "Can't scan file in MediaScanner after taking picture");
        }
	}

	@Override
	public void onScanCompleted(String path, Uri uri) {
		// TODO Auto-generated method stub
		this.conn.disconnect();
	}
}

