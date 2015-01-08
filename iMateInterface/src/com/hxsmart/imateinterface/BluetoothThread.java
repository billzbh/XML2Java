package com.hxsmart.imateinterface;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Set;
import java.util.UUID;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothSocket;
import android.util.Log;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BluetoothThread extends Thread 
{
	private final boolean DEBUG_MODE = false;
	private boolean mmIsConnected = false;
	private volatile boolean mmThreadPause = false;
	private volatile boolean mmThreadExit = false;
	private String deviceVersionString;
	private String mmBuleMAC;
	private static BluetoothThread THIS = null; 

	/**
	 * BluetoothThread构建
	 * @param bluetoothAdapter
	 */
	public BluetoothThread(BluetoothAdapter bluetoothAdapter) 
	{
		this.mmBluetoothAdapter = bluetoothAdapter;
		THIS = this;
	}
	
	public static BluetoothThread getInstance()
	{
		return THIS;
	}
	
	public String getBlueMAC()
	{
		if(mmIsConnected)
			return mmBuleMAC;
		return null;
	}
	
	/**
	 * 检测设备是否已经建立连接
	 * @return	true	已经连接
	 * 			false	未建立连接
	 */
	public boolean deviceIsConnecting()
	{
		return (BluetoothComplex.deviceIsConnecting());
	}
	
	/**
	 * 检测设备是否忙
	 * @return	true	设备忙
	 * 			false	设备空闲
	 */
	public boolean deviceIsWorking()
	{
		return (BluetoothComplex.isWorking());
	}
	
	/**
	 * 重新连接蓝牙
	 */
	public void reconnect() {
		bluetoothClose();
	}
	
	/**
	 * 取消操作
	 */
	public void cancel()
	{
		byte[] cancelBytes = new byte[1];
		cancelBytes[0] = 0x18;		
		bluetoothSend(cancelBytes);
	}
	
	/**
	 * 查询iMate系列号
	 * @return	null			不支持取系列号或通讯错误
	 * 			有值			iMate系列号，长度为24，由0~9或A~F组成
	 */
	public String deviceSerialNumber()
	{
		if (BluetoothComplex.isWorking())
			return null;
		if (!BluetoothComplex.deviceIsConnecting())
			return null;

		final byte[] receivedBytes = new byte[60];
		final byte[] sendBytes = new byte[1];
	    sendBytes[0] = 0x04;
	    
	    int receivedLength = sendReceive(sendBytes, 1, receivedBytes, 1);
	    if (receivedLength <= 0)
	    	return null;
	    
	    if(receivedBytes[0] != 0)
	    		return null;
	    
		byte[] dataBytes = new byte[24];
		for (int i=0; i<24; i++) {
			dataBytes[i] = receivedBytes[i+1];
		}	    
		return new String(dataBytes);
	}
	
	/**
	 * 查询iMate固件版本号
	 * @return	null			不支持取版本或通讯错误
	 * 			"A.A,B.B.B"		硬件和固件版本，其中A为硬件版本，B为固件版本
	 */
	public String deviceVersion()
	{
		if (deviceVersionString != null)
			return deviceVersionString;
		
		if (BluetoothComplex.isWorking())
			return null;
		if (!BluetoothComplex.deviceIsConnecting())
			return null;

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[1];
	    sendBytes[0] = 0x60;
	    
	    int receivedLength = sendReceive(sendBytes, 1, receivedBytes, 1);
	    if (receivedLength <= 0)
	    	return null;
	    
	    if (receivedBytes[0] != 0)
	    	return "NONE";
	    
		byte[] dataBytes = new byte[receivedLength -1];
		for (int i=0; i<receivedLength - 1; i++) {
			dataBytes[i] = receivedBytes[i+1];
		}	    
		return new String(dataBytes);
	}
	
	public void puseThread() {
		cancel();
		mmThreadPause = true;
	}
	
	public void pauseThread() {
		cancel();
		mmThreadPause = true;
	}
	
	public void exitThread() {
		cancel();
		mmThreadExit = true;
	}

	public void resumeThread() {
		Log.v("HxSmart_BluetoothThread", this + " resumed...");
		mmThreadPause = false;
	}
	
	
	/**
	 * iMate响一声
	 */
	public void buzzer()
	{
		final byte[] receivedBytes = new byte[20];
		final byte[] sendBytes = new byte[1];
	    sendBytes[0] = 0x03;
	    sendReceive(sendBytes, 1, receivedBytes, 1);
	}
	/**
	 * 部件检测。可检测的部件包括二代证模块，射频卡模块。（IMFC还包括指纹模块、SD模块）
	 * @param componentsMask：用componentsMask的低位bit来标识检测的部件， 可以用“|” 组合使用：：
	 * 								0x01 二代证模块
	 * 								0x02 射频模块
	 * 								0x04 IMFC 指纹模块（iMate不支持）	
	 * 								0x08 IMFC SD卡模块（iMate不支持）
	 * 								0xFF 全部部件检测
	 * @return	0			：所检测的部件全部正常
	 * 			其它    	：依照返回值的bit位标识对应的故障的部件，与componentsMask相对应
	 * @throws  Exception	：错误信息
	 */
	public int deviceTest(int componentsMask) throws Exception
	{
		if (BluetoothComplex.isWorking() )
			throw new Exception("设备忙");;
		if (!BluetoothComplex.deviceIsConnecting())
			throw new Exception("设备没有连接");

		final byte[] receivedBytes = new byte[80];
		final byte[] sendBytes = new byte[2];
		sendBytes[0] = 0x6C;
		sendBytes[1] = (byte)componentsMask;
		
	    int receivedLength = sendReceive(sendBytes, 2, receivedBytes, 4);
	    if (receivedLength <= 0)
			throw new Exception("通讯超时");
		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
		}
		return receivedBytes[1];
	}
	
	/**
	 * 等待事件，包括磁卡刷卡、Pboc IC插入、放置射频卡。timeout是最长等待时间(秒)
	 * @param eventMask : 用eventMask的低位bit来标识等待事件， 可以用“|” 组合使用：
	 * 					  		0x01 -- 等待刷卡事件
	 * 					  		0x02 -- 等待插卡事件
	 * 					  		0x04 -- 等待射频事件
	 * 					  		0xFF -- 等待所有事件
	 * @param timeout	:  超时时间（秒）
	 * @return  		:  第一个字节为事件类型：
	 * 					  		0x01 -- 检测到刷卡事件
	 * 					  		0x02 -- 检测到插卡事件
	 * 					  		0x04 -- 检测到射频事件
	 * 						后续的数据为：
	 * 							刷卡事件：二磁道和三磁道数据
	 * 							插卡事件：IC复位数据
	 * 							射频事件：卡片系列号					
	 * @throws  Exception：错误信息
	 */
	public byte[] waitEvent(int eventMask, int timeout) throws Exception
	{
		if (BluetoothComplex.isWorking() )
			throw new Exception("设备忙");;
		if (!BluetoothComplex.deviceIsConnecting())
			throw new Exception("设备没有连接");

		final byte[] receivedBytes = new byte[200];
		final byte[] sendBytes = new byte[3];
		sendBytes[0] = 0x6B;
		sendBytes[1] = (byte)eventMask;
		sendBytes[2] = (byte)timeout;

	    int receivedLength = sendReceive(sendBytes, 3, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
			throw new Exception("通讯超时");
		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
		}
		byte[] retData = new byte[receivedLength - 1];
		for (int i=0; i<receivedLength - 1; i++)
			retData[i] = receivedBytes[i + 1];
		return retData;
	}

	/**
	 * 刷卡
	 * @param cardData	磁条卡数据对象
	 * @param timeout	超时时间（秒）
	 * @return  0 		成功
	 * 			1		超时
	 * 			2		刷卡错误,错误信息可从cardData.errorString中获得
	 * 			8		设备忙
	 * 			9		设备没有连接
	 */
	public int swipeCard(MagCardData cardData, int timeout) 
	{
		if (BluetoothComplex.isWorking() )
			return 8;
		if (!BluetoothComplex.deviceIsConnecting())
			return 9;

		final byte[] receivedBytes = new byte[200];
		final byte[] sendBytes = new byte[2];
		sendBytes[0] = 0x61;
		sendBytes[1] = (byte)timeout;
		
	    int receivedLength = sendReceive(sendBytes, 2, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
	    	return 1;
		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			try {
				cardData.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				cardData.setErrorString("刷卡失败");
			}
			return 2;
		}
		
		byte[] cardNoBytes = new byte[21];

		for (int i=0; i<21; i++)
			cardNoBytes[i] = 0;
		
		for (int i=0; i<37; i++) {
			if (receivedBytes[i+1] == '=' || receivedBytes[i+1] == 0) {
				break;
			}
			cardNoBytes[i] = receivedBytes[i+1];
		}		
		try {
			cardData.setCardNoString(new String(cardNoBytes,"GBK").trim());
		} catch (UnsupportedEncodingException e) {}
		try {
			cardData.setTrack2String(new String(receivedBytes,1,37,"GBK").trim());
		} catch (UnsupportedEncodingException e) {}
		try {
			cardData.setTrack3String(new String(receivedBytes,38,104,"GBK").trim());
		} catch (UnsupportedEncodingException e) {}
		
		return 0;
	}
	
	/**
	 * 读二代证信息
	 * @param idInfo
	 * @param timeout
	 * @return	0		成功
	 * 			1		通讯超时
	 * 			2		读卡失败, 错误信息从idInfo.errorString中获取
	 * 			8		设备忙
	 * 			9		设备没有连接
	 */
	public int readIdInformation(IdInformationData idInfo, int timeout)
	{
		if (BluetoothComplex.isWorking() )
			return 8;
		if (!BluetoothComplex.deviceIsConnecting())
			return 9;

		final byte[] receivedBytes = new byte[1500];
		final byte[] sendBytes = new byte[3];
	    sendBytes[0] = 0x63;
	    sendBytes[1] = 0x02;
	    sendBytes[2] = (byte)timeout;
	    
	    int receivedLength = sendReceive(sendBytes, 3, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
	    	return 1;
		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			try {
				idInfo.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				idInfo.setErrorString("读二代证失败");
			}
			return 2;
		}
		
		byte[] dataBytes = new byte[256];
		for (int i=0;i<256;i++) {
			dataBytes[i] = receivedBytes[i+1];
		}
		byte[] pictureBytes = new byte[1024];
		for (int i=0;i<1024;i++) {
			pictureBytes[i] = receivedBytes[i+1+256];
		}
		processIdCardInfo(dataBytes,idInfo);
		idInfo.setPictureData(pictureBytes);
	    
		return 0;
	}
	
	/**
	 * 读金融IC卡
	 * @param icCardInfo
	 * @return	0		成功
	 * 			1		通讯超时
	 * 			2		读卡失败，错误信息可从icCardInfo.errorString中获得
	 * 			8		设备忙
	 * 			9		设备没有连接
	 */	
	public int readIcCard(IcCardData icCardInfo, int timeout)
	{
		if (BluetoothComplex.isWorking() )
			return 8;
		if (!BluetoothComplex.deviceIsConnecting())
			return 9;
		
		final byte[] apduIn = new byte[300];

		ApduExchangeData apdu = new ApduExchangeData();
		
		apdu.reset();    
		int retCode = resetCard(apdu, timeout);
		if (retCode != 0) {
			icCardInfo.setErrorString(apdu.getErrorString());
			return retCode;
		}
		
		apdu.reset();    
		apduIn[0] = 0x00;
		apduIn[1] = (byte)0xA4;
		apduIn[2] = 0x04;
		apduIn[3] = 0x00; //p2
		apduIn[4] = 0x05; //inLength
		apduIn[5] = (byte)0xA0;
		apduIn[6] = 0x00;
		apduIn[7] = 0x00;
		apduIn[8] = 0x03;
		apduIn[9] = 0x33;
		
		apdu.setAuduInBytes(apduIn);
		apdu.setInLength(10);
		retCode = audpExchange(apdu);
		if (retCode != 0) {
			icCardInfo.setErrorString(apdu.getErrorString());
			return retCode;
		}
	    
		if (apdu.getStatus()[0] != (byte)0x61 && apdu.getStatus()[0] != (byte)0x90) {
			icCardInfo.setErrorString("应用不支持");
			return 2;			
		}
		
		// 读记录文件
		for (int i=1; i<10; i++) {
			for (int j=1; j<10; j++) {
				apdu.reset();  
				apduIn[0] = 0x00;
				apduIn[1] = (byte)0xB2;
				apduIn[2] = (byte)i;
				apduIn[3] = (byte)(((j << 3)|0x04)&0xff); //p2
				apduIn[4] = 0x00; //inLength
			    
				apdu.setAuduInBytes(apduIn);
				apdu.setInLength(5);
				retCode = audpExchange(apdu);
				if (retCode != 0) {
					icCardInfo.setErrorString(apdu.getErrorString());
					return retCode;
				}			    
				if (apdu.getStatus()[0] != (byte)0x90 || apdu.getOutLength() == 0)
					break;
				
				/*
				for (int p=0; p<apdu.getOutLength(); p++) {
					System.out.format("%02x", apdu.getAuduOutBytes()[p]);
				}
				System.out.println();
				*/
				
				byte[] tagValue;
				if (icCardInfo.getCardNoString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x5a&0xff);
					if (tagValue != null) {
						String cardNoString = new String();
						for (int m=0; m<tagValue.length; m++) {
							cardNoString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						int point = cardNoString.indexOf('f');
						icCardInfo.setCardNoString("point = " + point);
						if (point > 0)
							cardNoString = cardNoString.substring(0, point);
						point = cardNoString.indexOf('F');
						if (point > 0)
							cardNoString = cardNoString.substring(0, point);
						icCardInfo.setCardNoString(cardNoString);
					}	
				}
				if (icCardInfo.getPanSequenceNoString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x5f34&0xffff);
					if (tagValue != null) {
						String dateString = Integer.toHexString((tagValue[0]&0x000000ff)|0xffffff00).substring(6);
						icCardInfo.setPanSequenceNoString(dateString.trim());
					}	
				}
				if (icCardInfo.getHolderNameString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x9f0b&0xffff);
					String name;
					if (tagValue != null) {
						try {
						name = new String(tagValue, "GBK");
						} catch (UnsupportedEncodingException e) {
							name = new String(tagValue);
						}
						icCardInfo.setHolderNameString(name.trim());	
					}	
				}
				if (icCardInfo.getHolderNameString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x5f20&0xffff);
					String name;
					if (tagValue != null) {
						try {
						name = new String(tagValue, "GBK");
						} catch (UnsupportedEncodingException e) {
							name = new String(tagValue);
						}
						icCardInfo.setHolderNameString(name.trim());	
					}	
				}
				tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x9f0b&0xffff);
				if (tagValue != null) {
					String name = new String(tagValue);
					icCardInfo.setHolderNameString(name.trim());	
				}
				
				if (icCardInfo.getHolderIdString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x9f61&0xffff);
					if (tagValue != null) {
						String holderId = new String(tagValue);
						icCardInfo.setHolderIdString(holderId.trim());	
						
					}	
				}
				
				if (icCardInfo.getExpireDateString() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x5f24&0xffff);
					if (tagValue != null) {
						String dateString = "";
						for (int m=0; m<3; m++) {
							dateString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						icCardInfo.setExpireDateString(dateString.trim());
					}	
				}
				if (icCardInfo.getTrack2String() == null) {
					tagValue = getTlvTagValue(apdu.getAuduOutBytes(), apdu.getOutLength(), 0x57&0xffff);
					if (tagValue != null) {
						String dateString = "";
						for (int m=0; m<tagValue.length; m++) {
							dateString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						int point = dateString.indexOf('f');
						if (point > 0)
							dateString = dateString.substring(0, point);
						point = dateString.indexOf('F');
						if (point > 0)
							dateString = dateString.substring(0, point);
						icCardInfo.setTrack2String(dateString.trim());
					}	
				}
				
				if (icCardInfo.getTrack2String() != null && icCardInfo.getCardNoString() != null && icCardInfo.getHolderNameString() != null && icCardInfo.getHolderIdString() != null && icCardInfo.getExpireDateString() != null) 
					return 0;
			}
		}
		return 0;
	}
	/**
	 * 获取iMated电池电量
	 * @return	!= null将返回iMate电池电量百分比。
	 * 			== null与iMate通讯错误
	 */
	public String getBatteryLeve()
	{
		if (BluetoothComplex.isWorking() )
			return null;
		if (!BluetoothComplex.deviceIsConnecting())
			return null;

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[1];
		sendBytes[0] = 0x65;
		
	    int receivedLength = sendReceive(sendBytes, 1, receivedBytes, 2);
	    if (receivedLength <= 0)
	    	return null;
		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			return null;
		}	
		int level = receivedBytes[1] & 0xFF;
		String levelString = "";
		levelString += level;
				
		return levelString;
	}
	/**
	 * Pboc卡复位
	 * @param apduData	卡片数据交换对象
	 * @param timeout	等待卡片插入的超时时间	
	 * @return	0		成功，卡复位数据通过apduData.getResetDataBytes()获取
	 * 			1		通讯超时
	 * 			2		读卡失败, 错误信息通过apduData.getErrorString()获取
	 * 			8		设备忙
	 * 			9		设备没有连接
	 * 
	 * 调用方法：
	 * 	ApduExchangeData apduData = new ApduExchangeData();
	 * 	apduData.reset(); 
	 * 	apduData.setSlot(ApduExchangeData.USER_SLOT); //设置卡座号，缺省为USER_SLOT    
	 *  int retCode = pbocReset(apduData, 20);
	 *  if (retCode == 0)
	 *  	.....apduData.getResetDataBytes()
	 */
	public int pbocReset(ApduExchangeData apduData, int timeout) {
		if (BluetoothComplex.isWorking() )
			return 8;
		if (!BluetoothComplex.deviceIsConnecting())
			return 9;
		//pduData.reset();    
		return resetCard(apduData, timeout);
	}
	
	/**
	 * Pboc卡APDU
	 * @param apduData	卡片数据交换对象
	 * @param timeout	等待卡片插入的超时时间	
	 * @return	0		成功 --- 卡复位数据通过apduData.getAuduOutBytes()获取
	 * 						 --- 卡片处理的状态码通过apduData.getStatus()获取
	 * 			1		通讯超时
	 * 			2		读卡失败, 错误信息通过apduData.getErrorString()获取
	 * 			8		设备忙
	 * 			9		设备没有连接
	 * 调用方法：
	 * 	ApduExchangeData apduData = new ApduExchangeData();
	 * 	apduData.reset(); 
	 *  apduData.setSlot(ApduExchangeData.USER_SLOT); //设置卡座号，缺省为USER_SLOT  
	 *  apduData.setCardType(ApduExchangeData.PBOC_CARD_TYPE); //设置卡类型，缺省为PBOC_CARD_TYPE
	 * 	apduIn[0] = 0x00;
	 * 	apduIn[1] = (byte)0xA4;
	 * 	apduIn[2] = 0x04;
	 * 	apduIn[3] = 0x00; //p2
	 * 	apduIn[4] = 0x05; //inLength
	 * 	apduIn[5] = (byte)0xA0;
	 * 	apduIn[6] = 0x00;
	 * 	apduIn[7] = 0x00;
	 * 	apduIn[8] = 0x03;
	 * 	apduIn[9] = 0x33;
	 * 
	 * 	apduData.setAuduInBytes(apduIn);
	 * 	apduData.setInLength(10);
	 * 	retCode = pbocApdu(apduData);
	 * 	if (retCode == 0) {
	 *  	.....apduData.getAuduOutBytes()
	 */
	public int pbocApdu(ApduExchangeData apduData) {
		if (BluetoothComplex.isWorking() )
			return 8;
		if (!BluetoothComplex.deviceIsConnecting())
			return 9;
		return audpExchange(apduData);
	}	

	public void run() 
	{
		Log.v("HxSmart_BluetoothThread", this + " runing...");
		while (true) {
			delay(10);

			if (mmThreadExit) {
				if (mmIsConnected)
					bluetoothClose();
				Log.v("HxSmart_BluetoothThread", this + " exited");
				return;
			}

			if (mmThreadPause) {
				if (mmIsConnected) {
					bluetoothClose();
					Log.v("HxSmart_BluetoothThread", this + " paused...");
				}
				delay(500);
				continue;
			}
			
			if (!mmIsConnected) {
				if (bluetoothConnect() != 0) {
					// 如果连接失败，等待2秒后重新连接
					delay(2000L);
					continue;
				}
				new Thread(new Runnable() {
					@Override
					public void run() {
						deviceVersionString = deviceVersion();
					}
				}).start();
			}
			bluetoothReceive();
		}
	} 
	
	private void delay(long m) {
	    try {
	    	Thread.sleep(m);
	    } catch (Exception e) {
	    	bluetoothClose();
	    }
	}
	
	public static byte[] getTlvTagValue(byte[] tlvBytes, int length, int tag)
	{
		int offset = 0;
		
		if (tlvBytes[offset++] != 0x70)
			return null;
		
		if (tlvBytes[offset++] < 0)
			offset ++;
			
		while (offset < length) {
			if (tlvBytes[offset] == 0) {
				offset++;
				continue;
			}
			if (tlvBytes[offset] == (byte)0xff) {
				offset++;
				continue;
			}
			int tmpTag = tlvBytes[offset++]&0xff;
			if ((tmpTag&0x1F) == 0x1f) 
				tmpTag = tmpTag*256 + (int)(tlvBytes[offset++]&0xff);
			int tmpLen = tlvBytes[offset++];
			if (tmpLen < 0)
				tmpLen = tlvBytes[offset++]&0xff;
			if (tmpTag == tag) {
				byte[] findTagValue = new byte[tmpLen];
				for (int i=0;i<tmpLen; i++)
					findTagValue[i] = tlvBytes[offset++];
				return findTagValue;
			}
			offset += tmpLen;
		}
		return null;		
	}
	
	private int resetCard(ApduExchangeData apdu, int timeout)
	{
		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[4];
		
		if (apdu.getSlot() == 1) {
			sendBytes[0] = (byte)0x81;
			sendBytes[1] = 1; //需激活
			sendBytes[2] = (byte)timeout;
			
		    int receivedLength = sendReceive(sendBytes, 3, receivedBytes, timeout+1);
		    if (receivedLength <= 0)
		    	return 1;
		    		
			int retCode = (int)receivedBytes[0];
			if ( retCode != 0 ) {
				try {
					apdu.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
				} catch (UnsupportedEncodingException e) {
					apdu.setErrorString("IC卡复位失败");
				}
				return 2;
			}
			byte[] resetDataBytes = {0x3B,(byte)0x8E,(byte)0x80,0x01,(byte)0x80,0x31,(byte)0x80,0x66,(byte)0xB0,(byte)0x84,0x0C,0x01,0x6E,0x01,(byte)0x83,0x00,(byte)0x90,0x00,0x1D};
			apdu.setResetDataBytes(resetDataBytes);
			
			return 0;			
		}
	    
	    sendBytes[0] = 0x62;
	    sendBytes[1] = 0x01;
	    sendBytes[2] = (byte)apdu.getSlot();
	    sendBytes[3] = (byte)timeout;
	    
	    int receivedLength = sendReceive(sendBytes, 4, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
	    	return 1;
	    		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			try {
				apdu.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				apdu.setErrorString("IC卡复位失败");
			}
			return 2;
		}
		byte[] resetDataBytes = new byte[receivedLength-1];
		for (int i=0; i<receivedLength-1; i++)
			resetDataBytes[i] = receivedBytes[i+1];		
		apdu.setResetDataBytes(resetDataBytes);
		
		return 0;
	}
	
	private int audpExchange(ApduExchangeData apdu)
	{
		final byte[] receivedBytes = new byte[300];
		final byte[] sendBytes = new byte[300];
		
	    sendBytes[0] = 0x62;
	    sendBytes[1] = 0x02;
	    sendBytes[2] = (byte)apdu.getSlot();
	    sendBytes[3] = (byte)apdu.getCardType();
	    
	    for (int i=0; i<apdu.getInLength();i++) {	    
	    	sendBytes[4+i] = apdu.getAuduInBytes()[i];
	    }
	    
	    int receivedLength = sendReceive(sendBytes, 4 + apdu.getInLength(), receivedBytes, 3);
	    if (receivedLength <= 0)
	    	return 1;
	    		
		int retCode = (int)receivedBytes[0];
		if ( retCode != 0 ) {
			try {
				apdu.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				apdu.setErrorString("卡片操作失败");
			}
			return 2;
		}
		byte[] status = new byte[2];
		status[0] = receivedBytes[receivedLength-2];
		status[1] = receivedBytes[receivedLength-1];
		apdu.setStatus(status);
		
		if (status[0] == (byte)0x61 || status[0] == (byte)0x9f) {
			byte[] apduIn = new byte[9];
			apduIn[0] = 0x62;
			apduIn[1] = 0x02;
			apduIn[2] = (byte)apdu.getSlot();
			apduIn[3] = (byte)apdu.getCardType();
			apduIn[4] = 0x00;
			apduIn[5] = (byte)0xc0;
			apduIn[6] = 0x00;
			apduIn[7] = 0x00;
			apduIn[8] = apdu.getStatus()[1];
		    receivedLength = sendReceive(apduIn, 9, receivedBytes, 3);
		    if (receivedLength <= 0)
		    	return 1;
			retCode = receivedBytes[0]&0xff;
			if ( retCode != 0 ) {
				try {
					apdu.setErrorString(new String(receivedBytes,1,receivedLength-1,"GBK"));
				} catch (UnsupportedEncodingException e) {
					apdu.setErrorString("卡片操作失败");
				}
				return 2;
			}
		}
		status[0] = receivedBytes[receivedLength-2];
		status[1] = receivedBytes[receivedLength-1];
		apdu.setStatus(status);
		apdu.setOutLength(0);
		apdu.setAuduOutBytes(null);
		if (receivedLength-3 > 0) {
			byte[] apduOutBytes = new byte[receivedLength-3];
			for (int i=0;i<receivedLength-3;i++)
				apduOutBytes[i] = receivedBytes[1+i];
			apdu.setOutLength(apduOutBytes.length);
			apdu.setAuduOutBytes(apduOutBytes);
		}
		return 0;
	}
	
	private int deviceConnect(BluetoothDevice device) {
		mmSocket = null;
		try {
			mmSocket = device.createRfcommSocketToServiceRecord(UUID
					.fromString("00001101-0000-1000-8000-00805F9B34FB"));
		} catch (IOException e) {
			mmSocket = null;
			Log.v("HxSmart_BluetoothThread", "device.createRfcommSocketToServiceRecord:" + e.getMessage());
		}
		if (mmSocket == null)
			return 1;

		// Connecting TPOS
		try {
			// Connect the device through the socket. This will block
			// until it succeeds or throws an exception
			mmSocket.connect();
		} catch (IOException connectException) {
			bluetoothClose();
			Log.v("HxSmart_BluetoothThread", "mmSocket.connect:" + connectException.getMessage());
			return 2;
		}
		mmInStream = null;
		mmOutStream = null;
		try {
			mmInStream = mmSocket.getInputStream();
			mmOutStream = mmSocket.getOutputStream();
		} catch (IOException e) {
			bluetoothClose();
			Log.v("HxSmart_BluetoothThread", e.getMessage());
			return 3;
		}
		mmIsConnected = true;
		BluetoothComplex.setDeviceConnected(true);
		BluetoothComplex.resetBuffer();


		Log.v("HxSmart_BluetoothThread", "bluetoothConnect succeed.");
		return 0;
	}
	
	private int bluetoothConnect() 
	{
		BluetoothComplex.setDeviceConnected(false);
		mmIsConnected = false;
		
		deviceVersionString = null;
		
		if (mmLastDevice != null ) {
			int ret = deviceConnect(mmLastDevice);
			if (ret != 0)
				mmLastDevice = null;
			return ret;
		}
		
		if (mmBluetoothAdapter == null)
			return -1;

		// Check bluetooth device enable
		if (!mmBluetoothAdapter.isEnabled()) {
			return -2;
		}

		// Cancel discovery because it will slow down the connection
		if (mmBluetoothAdapter.isDiscovering())
			mmBluetoothAdapter.cancelDiscovery();

		// Search Tpos device
		Set<BluetoothDevice> pairedDevices = mmBluetoothAdapter
				.getBondedDevices();
		// If there are paired devices
		if (pairedDevices.size() <= 0)
			return -3;
	
		for (BluetoothDevice device : pairedDevices) {		
			if (device.getName().contains("iMate") || device.getName().contains("IMFC") || device.getName().contains("iGAS")) {
				if (DEBUG_MODE)
					Log.v("HxSmart_BluetoothThread", device.getName());
				if (deviceConnect(device)!=0)
					continue;
				mmLastDevice = device;
				mmBuleMAC = device.getAddress();
				return 0;
			}
		}
		return -4;

	}

	synchronized private int bluetoothSend(byte[] bytes) 
	{
		if (!BluetoothComplex.deviceIsConnecting())
			return -1;
		try {
			mmOutStream.write(bytes, 0, bytes.length);
		} catch (Exception e) {
			Log.v("HxSmart_BluetoothThread", "mmOutStream.write:" + e.getMessage());
			bluetoothClose();
			return -1;
		}
		if (DEBUG_MODE) {
			System.out.format("bluetoothSended:");
			for (int i = 0; i < bytes.length; i++) {
				System.out.format("%02X ", bytes[i]);
			}
			System.out.println();
		}

		return bytes.length;
	}

	private int bluetoothReceive() 
	{
		final byte[] bytes;
		int readLength = 0;

		try {
			readLength = mmInStream.available();
			if (readLength == 0 )
				return 0;
			if (readLength < 0 || readLength == 65535) {
				bluetoothClose();
				System.out.println("bluetoothClose");
				return -1;
			}
				
			bytes = new byte[readLength];
			mmInStream.read(bytes, 0, readLength);
		} catch (Exception e) {
			bluetoothClose();
			Log.v("HxSmart_BluetoothThread", "mmInStream.read:" + e.getMessage());
			return -1;
		}
		BluetoothComplex.putInputData(bytes, readLength);

		if (DEBUG_MODE) {
			System.out.format("bluetoothReceived:");
			for (int i = 0; i < readLength; i++) {
				System.out.format("%02X ", bytes[i]);
			}
			System.out.println();
		}
		return readLength;
	}

	synchronized private void bluetoothClose() 
	{
		BluetoothComplex.resetBuffer();
		BluetoothComplex.setDeviceConnected(false);
		mmIsConnected = false;
		
		deviceVersionString = null;

		if (mmInStream != null) {
			try {
				mmInStream.close();
			} catch (IOException e) {
				Log.v("HxSmart_BluetoothThread", "mmInStream.close:" + e.getMessage());
			}
		}
		if (mmOutStream != null) {
			try {
				mmOutStream.close();
			} catch (IOException e) {
				Log.v("HxSmart_BluetoothThread", "mmOutStream.close:" + e.getMessage());
			}
		}
		if (mmSocket != null) {
			try {
				mmSocket.close();
			} catch (IOException e) {
				Log.v("HxSmart_BluetoothThread", "mmOutStream.close:" + e.getMessage());
			}
		}
		if (mmSocket != null) {
			try {
				mmSocket.close();
			} catch (IOException e) {
				Log.v("HxSmart_BluetoothThread", e.getMessage());
			}
		}

		mmInStream = null;
		mmOutStream = null;
		mmSocket = null;
	}
	
	public int sendReceive(byte[] sendBytes, int sendLength, byte[] receivedBytes, int timeout)
	{
		if (!BluetoothComplex.deviceIsConnecting())
			return -1;
		
		byte[] packBytes;
	    int offset = 0;

		if ( sendLength <= 255 ) {
			packBytes = new byte[sendLength+4];
		    packBytes[offset++] = 0x02;
	    	packBytes[offset++] = (byte)sendLength;
	    }
	    else {
			packBytes = new byte[sendLength+6];
		    packBytes[offset++] = 0x02;
	    	packBytes[offset++] = 0;
	    	packBytes[offset++] = (byte)(sendLength/256);
	    	packBytes[offset++] = (byte)(sendLength%256);
	    }
	    byte checkCode=0x03;
		for (int i=0;i<sendLength;i++) {
			packBytes[offset++] = sendBytes[i];
			checkCode ^= sendBytes[i];
		}
	    
    	packBytes[offset++] = 0x03;
    	packBytes[offset++] = checkCode;
    	
		BluetoothComplex.resetBuffer();
		
		for (int i=0;i<receivedBytes.length;i++)
			receivedBytes[i]= 0; 
    	
    	bluetoothSend(packBytes);
	    
		int receivedLength = 0;
		long timeMillis = System.currentTimeMillis() + timeout * 1000L;
		while (System.currentTimeMillis() < timeMillis) {
			if (mmThreadExit || mmThreadPause) {
				return -1;
			}
			receivedLength = BluetoothComplex.getReceivedData(receivedBytes);
			if (receivedLength > 0)
				break;
			try {
				Thread.sleep(5L);
			} catch (InterruptedException e) {
				break;
			}
		}
		if (receivedLength == 0) {
			Log.v("HxSmart_BluetoothThread", "sendReceive timeout");
			return -1;
		}
		
		return receivedLength;
	}
	
	public static String unicodeToString(String str) 
	{
		Pattern pattern = Pattern.compile("(\\\\u(\\p{XDigit}{4}))");
		Matcher matcher = pattern.matcher(str);
		char ch;
		while (matcher.find()) {
			ch = (char) Integer.parseInt(matcher.group(2), 16);
			str = str.replace(matcher.group(1), ch + "");
		}
		return str;
	}

	private void processIdCardInfo(byte[] idInfoBytes, IdInformationData idInfo)
	{
		final String nationName[] = {                               
				"汉族",
                "蒙古族", 
                "回族", 
                "藏族", 
                "维吾尔族", 
                "苗族", 
                "彝族", 
                "壮族", 
                "布依族",
                "朝鲜族",
                "满族",  
                "侗族", 
                "瑶族", 
                "白族", 
                "土家族", 
                "哈尼族", 
                "哈萨克族", 
                "傣族", 
                "黎族", 
                "傈僳族", 
                "佤族", 
                "畲族", 
                "高山族", 
                "拉祜族", 
                "水族", 
                "东乡族", 
                "纳西族", 
                "景颇族", 
                "柯尔克孜族", 
                "土族", 
                "达斡尔族", 
                "仫佬族", 
                "羌族", 
                "布朗族", 
                "撒拉族", 
                "毛难族", 
                "仡佬族", 
                "锡伯族", 
                "阿昌族", 
                "普米族", 
                "塔吉克族", 
                "怒族", 
                "乌孜别克族", 
                "俄罗斯族", 
                "鄂温克族", 
                "崩龙族", 
                "保安族", 
                "裕固族", 
                "京族", 
                "塔塔尔族", 
                "独龙族",
                "鄂伦春族",
                "赫哲族",
                "门巴族",
                "珞巴族",
                "基诺族"};
		final int segmentArray[] = {15,1,2,8,35,18,15,16,18};
		
		char[] uniChars = new char[idInfoBytes.length/2];
		for (int i=0; i<idInfoBytes.length/2; i++) {
			uniChars[i] = (char)((int)(idInfoBytes[i*2+1]&0xff)*256 + (int)(idInfoBytes[i*2]&0xff));
		}
		
	    String subString;
	    
	    // Name
	    int location = 0;
	    int length = segmentArray[0];
	    subString = new String(uniChars, location, length);
	    
	    idInfo.setNameString(subString.trim());
	    
	    // Sex
	    location += segmentArray[0];
	    length = segmentArray[1];
	    subString = new String(uniChars, location, length);
	    if (subString.equals("1"))
	    	idInfo.setSexString("男");
	    else
	    	idInfo.setSexString("女");
	    
	    // Nation
	    location += segmentArray[1];
	    length = segmentArray[2];
	    String indexString = new String(uniChars, location, length);
	    int index = Integer.parseInt(indexString);
	    if (index >=1 && index <= 56) 
	    	subString = nationName[index-1].trim();
	    else
	    	subString = "其他";
	    idInfo.setNationString(subString);
	    
	    // Birthday
	    location += segmentArray[2];
	    length = segmentArray[3];
	    subString = new String(uniChars, location, length);
	    idInfo.setBirthdayYearString(subString.substring(0, 4).trim());
	    idInfo.setBirthdayMonthString(subString.substring(4, 6).trim());
	    idInfo.setBirthdayDayString(subString.substring(6, 8).trim());
	    
	    // Address
	    location += segmentArray[3];;
	    length = segmentArray[4];
	    subString = new String(uniChars, location, length);
	    idInfo.setAddressString(subString.trim());

	    // Id numbers
	    location += segmentArray[4];;
	    length = segmentArray[5];
	    subString = new String(uniChars, location, length);
	    idInfo.setIdNumberString(subString.trim());
	    
	    // Issuer
	    location += segmentArray[5];;
	    length = segmentArray[6];
	    subString = new String(uniChars, location, length);
	    idInfo.setIssuerString(subString.trim());
	    
	    // Valid date
	    location += segmentArray[6];;
	    length = segmentArray[7];
	    subString = new String(uniChars, location, length);
	    String validDate = subString.substring(0, 4) + "." + subString.substring(4, 6) + "." + subString.substring(6, 8) + "-" + subString.substring(8, 12) + "." + subString.substring(12, 14) + "." + subString.substring(14, 16);
	    idInfo.setValidDateString(validDate.trim());
	    
	    // Others
	    location += segmentArray[7];
	    length = segmentArray[8];
	    subString = new String(uniChars, location, length);
	    idInfo.setOtherString(subString.trim());
	}

	final BluetoothAdapter mmBluetoothAdapter;
	private BluetoothSocket mmSocket = null;
	private InputStream mmInStream = null;
	private OutputStream mmOutStream = null;
	private BluetoothDevice mmLastDevice = null;
}

class BluetoothComplex 
{
	private static boolean 	isConnecting = false;
	private static byte[] 	inputBuffer = new byte[2048];
	private static int 		bytesLength = 0;
	private static int 		writeOffset = 0;
	private static boolean isWorking = false;
	
	public static boolean isWorking() 
	{
		synchronized(BluetoothComplex.class)
		{
			return isWorking;
		}
	}

	public static void setWorking(boolean isWorking) 
	{
		synchronized(BluetoothComplex.class)
		{
			BluetoothComplex.isWorking = isWorking;
		}
	}

	public static boolean deviceIsConnecting()
	{
		synchronized(BluetoothComplex.class)
		{
			return isConnecting;
		}
	}
	
	public static void setDeviceConnected(boolean connecting)
	{
		synchronized(BluetoothComplex.class)
		{
			isConnecting = connecting;
		}
	}
	
	public static void resetBuffer() 
	{
		synchronized(BluetoothComplex.class)
		{
			bytesLength = 0;
			writeOffset = 0;
		}
	}
	public static int getReceivedData(byte[] bytes)
	{
		synchronized(BluetoothComplex.class)
		{
			int offset = 0;
			int dataLen = 0;
			while (offset < bytesLength) {
				if (inputBuffer[offset] == 0x02) {
				    if ( inputBuffer[offset+1] == 0x00 ) {
				    	dataLen = (int)(inputBuffer[offset+2]&0xff)*256 + (int)(inputBuffer[offset+3]&0xff);
				        if ( dataLen + 6 + offset > bytesLength )
				            break;
				        offset = 4;
				    }
				    else {
				        dataLen = inputBuffer[offset+1]&0xFF;
				        if ( dataLen + 4 + offset > bytesLength )
				            break;
				        offset = 2;
				    }
				    if ( inputBuffer[offset+dataLen] != 0x03 ) {
				        break;
				    }
				    
				    byte checkCode=0;
				    for ( int i=0;i<dataLen+2;i++ )
				        checkCode^=inputBuffer[offset+i];
				    if ( checkCode == 0 ) {
				    	if ( bytes != null) {
				    		for (int i=0; i<dataLen; i++)
				    			bytes[i] = inputBuffer[offset+i];
				    	}
				    	return dataLen;
				    }
				    break;
				}
				offset ++;
			}
			return 0;
		}
	}
	
	public static void putInputData(byte[] bytes, int length)
	{
		if (length==0)
			return;
		synchronized(BluetoothComplex.class)
		{
			for (int i=0;i<length;i++)
				inputBuffer[i+writeOffset] = bytes[i];
			bytesLength += length;
			if (writeOffset+length <= inputBuffer.length)
				writeOffset += length;
		}
	}
}

