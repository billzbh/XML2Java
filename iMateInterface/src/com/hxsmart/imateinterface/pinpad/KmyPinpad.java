package com.hxsmart.imateinterface.pinpad;

import android.util.Log;

import com.hxsmart.imateinterface.BluetoothThread;

public class KmyPinpad {	
	private volatile boolean mmCancelFlag = false;
	
	public void cancel()
	{
		mmCancelFlag = true;
	}
	
	/**
	 * Pinpad上电 (传入通讯波特率和校验方式）
	 * @param   baud 	波特率
	 * @param   parity 	校验位,取值：Pinpad.COMM_NONE, Pinpad.COMM_EVEN,Pinpad.COMM_ODD
	 * @throws  Exception 
	 */
	public void powerOn(long baud, int parity) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 1;		
		sendBytes[3] = (byte)(baud/256);
		sendBytes[4] = (byte)(baud%256);
		sendBytes[5] = (byte)(parity);
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 6, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");
	    
	    delay(200L);		
	}
	
	/**
	 * Pinpad上电 (缺省波特率连接，9600bps，Pinpad.COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception 
	{
		try {
			powerOn(9600L, Pinpad.COMM_NONE);
		} catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * Pinpad下电
	 * @throws  Exception
	 * 			iMate通讯超时	 
	 */
	public void powerOff() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}		

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[3];
		
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 2;		
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 3, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");
	}
	
	/**
	 * Pinpad复位自检
	 * @param   initFlag 	true清除Pinpad中的密钥，false不清除密钥
	 * @throws  Exception
	 */	
	public void reset(boolean initFlag) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	

		final byte[] sendBytes = new byte[2];
		
		sendBytes[0] = 0x31;
		sendBytes[1] = 0x38;
		int len = 1;
		if (initFlag)
			len = 2;
		try {
			pinpadComm(sendBytes, len, null, 1);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * 获取Pinpad的终端序列号信息
	 * @return	成功将返回Pinpad的序列号信息（8个字节）
	 * @throws  Exception
	 */
	public byte[] getSerialNo() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[1];
			    		
		sendBytes[0] = 0x38;
		
		int receivedLength = 0;		
		try {
			receivedLength = pinpadComm(sendBytes, 1, receivedBytes, 1);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("无序列号信息");
		
		byte[] retByte = new byte[8];
		for (int i = 0 ; i < 8 ; i++)
			retByte[i] = 0;
		
		//System.out.println("receivedLength = " + receivedLength);
		
		if (receivedLength < 8) {
			for (int i = 0 ; i < receivedLength ; i++)
				retByte[i] = receivedBytes[i];
		}
		else {	
			for (int i = 0; i < 8; i++)
				retByte[i] = receivedBytes[receivedLength - 8 + i];
		}
		return retByte;
	}
	
	/**
	 * 获取Pinpad的版本号信息
	 * @return	成功将返回Pinpad的版本号信息,详细格式请参考厂家文档。
	 * @throws  Exception 
	 */
	public byte[] getVersion() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[1];
			    		
		sendBytes[0] = 0x30;
		
		int receivedLength = 0;		
		try {
			receivedLength = pinpadComm(sendBytes, 1, receivedBytes, 1);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("无版本信息");
		
		byte[] retByte = new byte[receivedLength];
		for (int i=0; i<receivedLength; i++)
			retByte[i] = receivedBytes[i];
		
		return retByte;
	}
	
	/**
	 * Pinpad键盘输入MasterKey
	 * 键盘上输入工作密钥，按“确认”键结束，并保存密钥。不足位用 0x00 填补
	 * @param	is3des		是否采用3DES算法，false表示使用DES算法
	 * @param   index		主密钥索引
	 * @throws  Exception	
	 */	
	public void inputMasterKey(boolean is3des, int index) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] sendBytes = new byte[3];
		
		// 设置算法
		sendBytes[0] = 0x46;
		sendBytes[1] = 0;
		if (!is3des)
			sendBytes[2] = 0x20;
		else
			sendBytes[2] = 0x30;

		sendBytes[0] = 0x23;
		sendBytes[1] = (byte)index;
			
		try {
			pinpadComm(sendBytes, 2, null, 1);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * Pinpad键盘输入WorkingKey
	 * 键盘上输入工作密钥，按“确认”键结束，并保存密钥。不足位用 0x00 填补
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param   masterIndex		主密钥索引
	 * @param   workingIndex	工作密钥索引
	 * @throws  Exception	
	 */	
	public void inputWorkingKey(boolean is3des, int masterIndex, int workingIndex) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] sendBytes = new byte[3];
		
		// 设置算法
		sendBytes[0] = 0x46;
		sendBytes[1] = 0;
		if (!is3des)
			sendBytes[2] = 0x20;
		else
			sendBytes[2] = 0x30;

		sendBytes[0] = 0x24;
		sendBytes[1] = (byte)masterIndex;
		sendBytes[2] = (byte)workingIndex;
			
		try {
			pinpadComm(sendBytes, 3, null, 1);
		}catch (Exception e) {
			throw e;
		}
	}
			
	/**
	 * Pinpad下装主密钥
	 * @param	is3des		是否采用3DES算法，false表示使用DES算法
	 * @param   index		主密钥索引
	 * @param   mastKey		主密钥
	 * @param   keyLength	主密钥长度
	 * @throws  Exception	
	 */	
	public void downloadMasterKey(boolean is3des, int index, byte[] masterKey, int keyLength) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (keyLength!=8 && keyLength != 16 && keyLength != 24)
			throw new Exception("密钥长度错误1");
		
		if (!is3des && keyLength!=8)
			throw new Exception("密钥长度错误2");
		
		final byte[] sendBytes = new byte[50];
		
		// 设置算法
		sendBytes[0] = 0x46;
		sendBytes[1] = 0;
		if (!is3des)
			sendBytes[2] = 0x20;
		else
			sendBytes[2] = 0x30;

		try {
			pinpadComm(sendBytes, 3, null, 1);
		} catch (Exception e) {
			throw e;
		}
				
		// 下载主密钥
		sendBytes[0] = 0x32;
		sendBytes[1] = (byte)index;
		for (int i=0; i < keyLength; i++)
			sendBytes[2+i] = masterKey[i];		
		try {
			pinpadComm(sendBytes, keyLength+2, null, 2);
		} catch (Exception e) {
			throw e;
		}
	}

	/**
	 * Pinpad下装工作密钥(主密钥加密）
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param   masterIndex		主密钥索引
	 * @param   workingIndex	工作密钥索引
	 * @param   workingKey		工作密钥
	 * @param   keyLength		工作密钥长度
	 * @throws  Exception	
	 */	
	public void downloadWorkingKey(boolean is3des, int masterIndex, int workingIndex, byte[] workingKey, int keyLength)
			throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
		
		if (keyLength != 8 && keyLength != 16 && keyLength != 24)
			throw new Exception("密钥长度错误1");
				
		final byte[] sendBytes = new byte[50];

		// 设置算法
		sendBytes[0] = 0x46;
		sendBytes[1] = 0;
		if (!is3des)
			sendBytes[2] = 0x20;
		else
			sendBytes[2] = 0x30;
		try {
			pinpadComm(sendBytes, 3, null, 1);
		} catch (Exception e) {
			throw e;
		}
			    
		// 下载主密钥
		sendBytes[0] = 0x33;
		sendBytes[1] = (byte)masterIndex;
		sendBytes[2] = (byte)workingIndex;
		for (int i=0; i < keyLength; i++)
			sendBytes[3+i] = workingKey[i];		
		try {
			pinpadComm(sendBytes, keyLength+3, null, 2);
		} catch (Exception e) {
			throw e;
		}	    
	}
	
	/**
	 * Pinpad输入密码（PinBlock）
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param	isAutoReturn	输入到约定长度时是否自动返回（不需要按Enter)
	 * @param   masterIndex		主密钥索引
	 * @param   workingIndex	工作密钥索引
	 * @param   cardNo			卡号/帐号（最少12位数字）
	 * @param   pinLength		需要输入PIN的长度
	 * @param   timeout			输入密码等待超时时间 <= 255 秒
	 * @return	成功返回输入的Pinblock
	 * @throws  Exception	
	 */	
	public byte[] inputPinblock(boolean is3des, boolean isAutoReturn, int masterIndex, int workingIndex, String cardNo, int pinLength, int timeout) 
			throws Exception
	{
		boolean finished = false;
		
		mmCancelFlag = false;
		
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
		
		if (cardNo != null && cardNo.length() < 13)
			throw new Exception("卡号/帐号长度错误");
		
		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[50];
	
		// 复位自检
		sendBytes[0] = 0x31;
		try {
			pinpadComm(sendBytes, 1, receivedBytes, 1);
		} catch (Exception e) {
			throw e;
		}
		
		if (cardNo != null) {
			//下装帐号
			byte[] orgAccount = cardNo.getBytes();		
			sendBytes[0] = 0x34;
			for (int i=0; i<12; i++)
				sendBytes[i+1] = orgAccount[orgAccount.length+i+3-16];		
			try {
				pinpadComm(sendBytes, 13, receivedBytes, 2);
			} catch (Exception e) {
				throw e;
			}
		}
		
		// 设置加密方式
		sendBytes[0] = 0x46;
		sendBytes[1] = 0x01;
		if (!is3des) {
			if (workingIndex < 0)
				sendBytes[2] = 0x60; //DES
			else
				sendBytes[2] = 0x20; //DES
		}
		else {
			if (workingIndex < 0)
				sendBytes[2] = 0x70; //3DES
			else
				sendBytes[2] = 0x30; //3DES				
		}
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 1);
		} catch (Exception e) {
			throw e;
		}
		
		// 不自动加回车
		sendBytes[0] = 0x46;
		sendBytes[1] = 0x05;
		if (isAutoReturn)
			sendBytes[2] = 0x01;
		else
			sendBytes[2] = 0x00;
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 1);
		} catch (Exception e) {
			throw e;
		}

		// 激活工作密钥
		sendBytes[0] = 0x43;
		sendBytes[1] = (byte)masterIndex;
		
		if (workingIndex < 0)
			sendBytes[2] = 0;
		else
			sendBytes[2] = (byte)workingIndex;
		
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 2);
		} catch (Exception e) {
			throw e;
		}
		
		// 启动密码键盘
		sendBytes[0] = 0x35;
		sendBytes[1] = (byte)pinLength;
		sendBytes[2] = 0x01;	//显示*
		if (cardNo == null)
			sendBytes[3] = 2; 		//不与CardNo一起运算后加密，直接bcd加密
		else
			sendBytes[3] = 1; 		//与CardNo一起运算后加密
		sendBytes[4] = 0; 		//不提示
		sendBytes[5] = (byte)(timeout);
	
		try {
			pinpadComm(sendBytes, 6, receivedBytes, 2);
		} catch (Exception e) {
			closePinpad();
			throw e;
		}
		
		long tm = System.currentTimeMillis() + (timeout+1)*1000L;
	    long tm2 = 0;
		while (System.currentTimeMillis() < tm) {
	    	if (mmCancelFlag) {
	    		mmCancelFlag = false;
	    		closePinpad();
	    		throw new Exception("操作取消");	    		
	    	}
			
			sendBytes[0] = 0x69;
			sendBytes[1] = 0x00;
			sendBytes[2] = 4;		
			
		    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 3, receivedBytes, 1);
		    if (receivedLength <= 0) {
		    	closePinpad();
				throw new Exception("iMate通讯超时");
		    }

		    if (receivedLength > 1) {
		    	tm2 = System.currentTimeMillis() + 5000L;
		    }
		    if (tm2 > 0 && System.currentTimeMillis() > tm2) {
				closePinpad();
				throw new Exception("密码输入超时");
		    }
		    for (int i=1; i < receivedLength; i++) {
		    	if (receivedBytes[i] == 0x1b ) {
		    		closePinpad();
		    		throw new Exception("取消");
		    	}
		    	if (receivedBytes[i] == 8 ) {
		    		tm2 = 0;
		    		continue;
		    	}
		    	if (receivedBytes[i] == 0x0d ) {
		    		finished = true;
		    		break;
		    	}
		    }
		    if ( finished)
		    	break;
		}
		if (!finished) {
			closePinpad();
			throw new Exception("密码输入超时");
		}
		
		// 获取密码密文
		sendBytes[0] = 0x42;
		int retLen = 0;
		try {
			retLen = pinpadComm(sendBytes, 1, receivedBytes, 1);
		} catch (Exception e) {
			closePinpad();
			throw e;
		}
		if (retLen < 8)
			throw new Exception("Pinblock长度错误");
		
		byte[] pinBlock = new byte[8];
		for (int i=0; i<8; i++)
			pinBlock[i] = receivedBytes[i];
		
		return pinBlock;
	}
	
	/**
	 * Pinpad加解密数据
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param	algo			算法，取值: Pinpad.ALGO_ENCRYPT, Pinpad.ALGO_DECRYPT, 以ECB方式进行加解密运算
	 * @param   masterIndex		主密钥索引
	 * @param   workingIndex	工作密钥索引，如果工作密钥索引取值-1，使用主密钥索引指定的主密钥进行加解密
	 * @param   data			加解密数据
	 * @param   dataLength		加解密数据的长度,要求8的倍数并小于或等于248字节长度
	 * @return	成功返回加解密的结果数据
	 * @throws  Exception	
	 */	
	public byte[] encrypt(boolean is3des, int algo, int masterIndex, 
			int workingIndex, byte[] data, int dataLength) throws Exception
	{
		if (dataLength%8 != 0 || dataLength > 248 )
			throw new Exception("数据长度错误");
		
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
				
		final byte[] receivedBytes = new byte[300];
		final byte[] sendBytes = new byte[300];
		
		// 设置算法
		sendBytes[0] = 0x46;
		sendBytes[1] = 1;
		if (!is3des) {
			if (workingIndex < 0)
				sendBytes[2] = 0x60;
			else 
				sendBytes[2] = 0x20;
		}
		else {
			if (workingIndex < 0)
				sendBytes[2] = 0x70;
			else
				sendBytes[2] = 0x30;
		}
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 2);
		} catch (Exception e) {
			throw e;
		}
		
		// 激活密钥
		sendBytes[0] = 0x43;
		sendBytes[1] = (byte)masterIndex;
		if (workingIndex < 0)
			sendBytes[2] = 0x00;
		else
			sendBytes[2] = (byte)workingIndex;
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 2);
		} catch (Exception e) {
			throw e;
		}
				
		// 启动加解密
		if (algo == Pinpad.ALGO_ENCRYPT)
			sendBytes[0] = 0x36;
		else
			sendBytes[0] = 0x37;
		for (int i=0; i<dataLength; i++) {
			sendBytes[i+1] =  data[i];
		}	
		int retLength = 0;
		try {
			retLength = pinpadComm(sendBytes, 1+dataLength, receivedBytes, 4);
		} catch (Exception e) {
			throw e;
		}
		if (retLength < 8 )
			throw new Exception("加解密数据失败");
		
		byte[] retBytes = new byte[retLength];
		for (int i=0; i<retLength; i++)
			retBytes[i] = receivedBytes[i];
		
		return retBytes;
	}

	/**
	 * Pinpad数据MAC运算（ANSIX9.9）
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param   masterIndex		主密钥索引
	 * @param   workingIndex	工作密钥索引，如果工作密钥索引取值-1，使用主密钥索引指定的主密钥进行加解密
	 * @param   data			计算Mac原数据
	 * @param   dataLength		Mac原数据的长度,要求8的倍数并小于或等于246字节长度
	 * @return	成功返回8字节Mac数据
	 * @throws  Exception	
	 */	
	public byte[] mac(boolean is3des, int masterIndex, 
			int workingIndex, byte[] data, int dataLength) throws Exception
	{
		if (dataLength <4 || dataLength > 246 )
			throw new Exception("数据长度错误");
		
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
				
		final byte[] receivedBytes = new byte[300];
		final byte[] sendBytes = new byte[300];
		
		sendBytes[0] = 0x46;
		sendBytes[1] = 1;
		if (!is3des) {
			if (workingIndex < 0)
				sendBytes[2] = 0x60;
			else 
				sendBytes[2] = 0x20;
		}
		else {
			if (workingIndex < 0)
				sendBytes[2] = 0x70;
			else
				sendBytes[2] = 0x30;
		}
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 1);
		} catch (Exception e) {
			throw e;
		}
		
		// 激活密钥
		sendBytes[0] = 0x43;
		sendBytes[1] = (byte)masterIndex;
		if (workingIndex < 0)
			sendBytes[2] = 0x00;
		else
			sendBytes[2] = (byte)workingIndex;
		try {
			pinpadComm(sendBytes, 3, receivedBytes, 1);
		} catch (Exception e) {
			throw e;
		}
		
		// 启动计算Mac
		sendBytes[0] = 0x41;
		for (int i=0; i<dataLength; i++) {
			sendBytes[i+1] =  data[i];
		}	
		int retLength = 0;
		try {
			retLength = pinpadComm(sendBytes, 1+dataLength, receivedBytes, 4);
		} catch (Exception e) {
			throw e;
		}
		if (retLength < 8)
			throw new Exception("计算Mac失败");
		
		byte[] retBytes = new byte[8];
		for (int i=0; i<8; i++)
			retBytes[i] = receivedBytes[i];
		
		return retBytes;
	}
	
	private void closePinpad() 
	{
		final byte[] sendBytes = new byte[10];
		// 关闭密码键盘
		sendBytes[0] = 0x45;
		sendBytes[1] = 0x00;
		try {
			pinpadComm(sendBytes, 2, null, 1);
		} catch (Exception e) {
		}
	}
	
	private void delay(long m) {
	    try {
	    	Thread.sleep(m);
	    } catch (InterruptedException e) {}
	}
	
	private int pinpadComm(byte[] in, int inLength, byte[] out, int timeout) throws Exception
	{
		final byte[] receivedBytes = new byte[600];
		final byte[] sendBytes = new byte[600];
		
		byte bcc = (byte)inLength;
		for (int i=0; i<inLength; i++)
			bcc ^= in[i];
		
		String packDataString = "";
		packDataString += Integer.toHexString((inLength&0x000000ff)|0xffffff00).substring(6);
		for (int i=0; i<inLength; i++) {
			packDataString += Integer.toHexString((in[i]&0x000000ff)|0xffffff00).substring(6);
		}
		packDataString += Integer.toHexString((bcc&0x000000ff)|0xffffff00).substring(6);
		
		//Log.e("zbh", packDataString);
		int packLength = 1+(inLength+2)*2;
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 3;		//发送数据报文命令
		sendBytes[3] = (byte)(packLength/256);
		sendBytes[4] = (byte)(packLength%256);
		sendBytes[5] = 0x02;		
		for (int i=0; i<(inLength+2)*2; i++)
			sendBytes[i+6] = packDataString.getBytes()[i];
				
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 5+packLength, receivedBytes, timeout+1);
	    if (receivedLength < 1)
	    	throw new Exception("iMate通讯超时");
	    
	    long m = System.currentTimeMillis() + timeout*1000L+100;
	    receivedLength = 0;
	    boolean finished = false;
	    int receivePackLength = 0;
	    while (System.currentTimeMillis() < m) {
			sendBytes[0] = 0x69;
			sendBytes[1] = 0x00;
			sendBytes[2] = 4;
			
			byte[] tmpBytes = new byte[600];
		    int ret = BluetoothThread.getInstance().sendReceive(sendBytes, 3, tmpBytes, 1);
		    if (ret <= 0)
				throw new Exception("iMate通讯超时");
		    
		    if (tmpBytes[0] != 0)
				throw new Exception("iMate处理失败");
		    
		    if (ret <= 1) {
		    	continue;
		    }
		    		    
		    for (int i=0; i<ret; i++) {
		    	receivedBytes[receivedLength+i] = tmpBytes[i+1];
		    }
		    receivedLength += ret-1;

		    if (receivedLength < 3)
		    	continue;
		    
		    if (receivedBytes[0] == 0x02 ) {
		    	int length = twoOneInt(receivedBytes, 1);
		    	if ((length+2)*2+1 == receivedLength) {
		    		receivePackLength = length;
		    		finished = true;
		    		break;
		    	}
		    }
	    }
		if (!finished)
			throw new Exception("通讯超时");
		
		int pinpadRet = twoOneInt(receivedBytes, 3);
		if (pinpadRet != 4 && pinpadRet != 164) { //0xa4
			throw new Exception("Pinpad处理失败");
		}
		int retLength = receivePackLength-1;
		
		if (retLength >0 && out != null) {
			twoOne(receivedBytes, 5, retLength*2, out);
	
			/*
			System.out.format("receivedBytes:");
			for (int i = 0; i < receivePackLength; i++) {
				System.out.format("%02X ", out[i]);
			}
			System.out.println();
			*/
		}
		return receivePackLength-1;
	}
	
	private int toupper(int in) {
		if (in >= 'a' && in <= 'z')
			return in+32;
		return in;
	}
	
	private int twoOneInt(byte[] in, int offset)
	{
		int tmp, ret;

		tmp = in[offset];
		if (tmp > '9')
			tmp = toupper(tmp) - 'A' + 0x0a;
		else
			tmp &= 0x0f;
			
		ret = (byte)(tmp << 4);

		tmp = in[1+offset];
		if (tmp > '9')
			tmp = toupper(tmp) - 'A' + 0x0a;
		else
			tmp &= 0x0f;
		ret += tmp;
		
		return ret;
	}
	private void twoOne(byte[] in, int offset, int length, byte[] out)
	{
		int tmp;

		for (int i=0; i<length; i+=2)
		{
			tmp = in[i+offset];
			if (tmp > '9')
				tmp = toupper(tmp) - 'A' + 0x0a;
			else
				tmp &= 0x0f;
			
			out[i/2] = (byte)(tmp << 4);

			tmp = in[i+1+offset];
			if (tmp > '9')
				tmp = toupper(tmp) - 'A' + 0x0a;
			else
				tmp &= 0x0f;
			out[i/2] += tmp;
		} // for(i=0; i<uiLength; i+=2) {
	}
	
	private void deviceTest() throws Exception
	{
		if (BluetoothThread.getInstance().deviceIsWorking() )
			throw new Exception("设备忙");
		if (!BluetoothThread.getInstance().deviceIsConnecting())
			throw new Exception("设备没有连接");
		
		if (BluetoothThread.getInstance() == null)
			throw new Exception("BluetoothThread未创建");			
	}
}

