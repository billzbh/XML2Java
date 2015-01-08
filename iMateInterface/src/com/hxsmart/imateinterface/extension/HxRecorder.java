package com.hxsmart.imateinterface.extension;

import java.io.IOException;

import android.media.MediaPlayer;
import android.media.MediaRecorder;

public class HxRecorder {
    //语音操作对象  
    private MediaPlayer mPlayer = null;  
    private MediaRecorder mRecorder = null; 
    private String mFileName = null;
    private int mDuration = 0;
    
    /*
     * 启动后台录音
     * @audioFileName	录音文件名(包括路径)，不含扩展名，录音结束后将生成.amr为后缀的音频文件
     */
    public void startRecorder(String audioFileName) throws Exception
    {
    	if (mRecorder != null)
    		throw new Exception("后台录音已启动");
    		
    	mFileName = audioFileName;
    	try {
	        mRecorder = new MediaRecorder();  
	        mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);  
	        mRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT); 
	        mRecorder.setOutputFile(mFileName + ".amr");
	        mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);  
	        mRecorder.prepare();  
	        mRecorder.start();  
        } catch (IOException e) {  
        	throw new Exception("启动后台录音失败");
        }  	
    }
    
    /*
     * 终止后台录音
     */
    public void stopRecorder()
    {
    	if (mRecorder != null) {
    		mRecorder.stop();  
    		mRecorder.release();  
    		mRecorder = null;  
    	}
    }
    
    /*
     * 启动播放录音
     * @audioFileName	录音文件名(包括路径)，不含扩展名，如果为null，则播放刚录制的音频文件
     */
    public void startPlay(String audioFileName) throws Exception
    {
    	if (audioFileName == null && mFileName == null)
    		throw new Exception("无音频文件"); 
    	
    	if (mPlayer != null)
    		throw new Exception("音频文件正在播放中"); 

    	if (audioFileName != null)
    		mFileName = audioFileName;
    	
    	if (mPlayer != null)
    		stopPlay();
    	mPlayer = new MediaPlayer();  
        try{  
            mPlayer.setDataSource(mFileName + ".amr");  
            mPlayer.prepare(); 
            mPlayer.setLooping(false);
            mPlayer.start(); 

            //mDuration = mPlayer.getDuration();
        }catch(IOException e) {  
        	stopPlay();
        	throw new Exception("播放音频文件失败:" + e.getMessage());
        } 	
    }
    
    /*
     * 获取音频播放的进度
     * 返回码：
     * 	< 0 	：	无可用进度
     * 	其它		：	播放进度百分比
     */
    public float getPlayProgress()
    {
    	if (mDuration == 0 || mPlayer == null)
        	return -1; 
 
    	return ((float)mPlayer.getCurrentPosition() / (float)mDuration);
    }
    
    /*
     * 终止播放录音
     */
    public void stopPlay()
    {
    	if (mPlayer != null) {
    		mPlayer.release();  
            mPlayer = null;  
    	}
    }
}
