package com.hxsmart.imateinterface.pinpad;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

import com.hxsmart.imateinterface.BluetoothThread;

public class XydPinpad {
	private int workDirNum = 1;	
	private volatile boolean mmCancelFlag = false;
	
	private byte[] transferKey = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	private byte[] authCodeKey = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	private byte[] uid = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35};
	
	private byte keyMode = Pinpad.PIN_KEY_MODE;
	
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
	    
	    delay(500L);	
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
	 * 取消密码输入操作 
	 */
	public void cancel()
	{
		mmCancelFlag = true;
	}
	
	/**
	 * 设置密码键盘参数
	 * key 	:	参数名称
	 * 			"AuthCode"	:   认证密钥, 16字节长度, 缺省值为16个0x00
	 * 			"UID"		:	Pinpad UID, 缺省值为{'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5'}
	 * 			"WorkDirNum	:	子目录编号, 缺省值为1;
	 */
	public void pinpadSetup(String key, byte[] value)
	{
		if (key.equals("AuthCode") == true && value.length >= 16) {
			for (int i = 0; i < 16; i++) {
				authCodeKey[i] = value[i];
			}
		}
		if (key.equals("UID") == true && value.length >= 16) {
			for (int i = 0; i < 16; i++) {
				uid[i] = value[i];
			}				
		}
		if (key.equals("WorkDirNum") == true && value.length >= 1) {
			workDirNum = value[0];
		}
	}
	
	/**
	 * Pinpad复位自检
	 * @param   initFlag 	无意义
	 * @throws  Exception
	 */	
	public void reset(boolean initFlag) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	

		final byte[] sendBytes = new byte[3];
		
		sendBytes[0] = (byte) 0xD5;
		sendBytes[1] = (byte) 0x00;

		try {
			pinpadComm(sendBytes, sendBytes[1] + 2, null, 1);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * 获取Pinpad的终端序列号信息
	 * @return	成功将返回Pinpad的序列号信息（16个字节）
	 * @throws  Exception
	 */
	public byte[] getSerialNo() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		byte[] version = getVersion();
		
		byte[] retByte = new byte[16];
		for (int i=0; i < 16; i++)
			retByte[i] = version[i + 8];
		
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

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[3];
			    		
		sendBytes[0] = (byte) 0x90;
		sendBytes[1] = (byte) 0x00;
		
		int receivedLength = 0;		
		try {
			receivedLength = pinpadComm(sendBytes, 2, receivedBytes, 2);
			//System.out.println("version = " + new String(receivedBytes));
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
	 * 设置密钥类型， 用于下载masterkey或workingkey之前调用
	 * @param	mode		主密钥类型包括 : 
	 * 						Pinpad.DECRYPT_KEY_MODE, Pinpad.ENCRYPT_KEY_MODE, Pinpad.PIN_KEY_MODE
	 */	
	public void setKeyMode(int mode)
	{
		keyMode = (byte)mode;
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
		
		try {
			changeDir(workDirNum);
		}catch (Exception e) {
			throw e;
		}
		
		byte downloadMode = keyMode;
		if (keyLength > 8)
			downloadMode |= (byte)0x80;
		
		if (keyLength!=8 && keyLength != 16 && keyLength != 24)
			throw new Exception("密钥长度错误1");
		
		final byte[] sendBytes = new byte[50];
		final byte[] receivedBytes = new byte[20];
		
		int tmp = 0;
		//下载主密钥
		sendBytes[tmp++] = (byte)0x80;
		sendBytes[tmp++] = (byte) ((byte)0x0A + keyLength);
		sendBytes[tmp++] = (byte)index;
		sendBytes[tmp++] = (byte)downloadMode;
		
		
		byte[] data = new byte[8];
		byte[] cipherMasterKey = new byte[keyLength];
		
		for (int i=0; i< 8; i++)
			data[i] = masterKey[i];
		data = encryptMode(data, transferKey);
		
		for (int i = 0; i < 8; i++)
			cipherMasterKey[i] = data[i];
		
		
		if ( keyLength ==16 ) {
			for (int i=0; i< 8; i++)
				data[i] = masterKey[i + 8];
			data = encryptMode(data, transferKey);
			for (int i = 0; i < 8; i++)
				cipherMasterKey[i + 8] = data[i];
			
		}
		
		for(int i = 0; i < keyLength; i++){
			sendBytes[tmp++] = cipherMasterKey[i];
		}
		
		byte[] authCode;
		try{			
			authCode = getAuthCode(sendBytes, 2, sendBytes[1] - 8, (byte)0x01);
		}catch (Exception e){
			throw e;
		}
		
		for(int i = 0; i < 8; i ++){
			sendBytes[tmp++] = authCode[i];
		}
		
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, 2);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("下载主密钥出错");
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
		
		try {
			changeDir(workDirNum);
		}catch (Exception e) {
			throw e;
		}
		
		byte downloadMode = keyMode;
		if (is3des)
			downloadMode |= 0x10;
		if (keyLength > 8)
			downloadMode |= (byte)0x80;
		
		if (keyLength != 8 && keyLength != 16 && keyLength != 24)
			throw new Exception("密钥长度错误1");

		final byte[] sendBytes = new byte[50];
		final byte[] receivedBytes = new byte[20];
		
		int tmp = 0;
		//下载工作密钥
		sendBytes[tmp++] = (byte)0x81;
		sendBytes[tmp++] = (byte) (13 + keyLength);
		sendBytes[tmp++] = (byte)masterIndex;
		sendBytes[tmp++] = (byte)workingIndex;
		sendBytes[tmp++] = (byte)downloadMode;
		
		for(int i = 0; i < keyLength; i++){
			sendBytes[tmp++] = workingKey[i];
		}
		
		byte[] useNo = {(byte)0x7f, (byte)0xff};
		sendBytes[tmp++] = useNo[0];
		sendBytes[tmp++] = useNo[1];
				
		byte[] authCode;
		try{			
			authCode = getAuthCode(sendBytes, 2, sendBytes[1] - 8, (byte)0x01);
		}catch (Exception e){
			throw e;
		}
		
		for(int i = 0; i < 8; i ++){
			sendBytes[tmp++] = authCode[i];
		}
		
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, 2);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("下载工作密钥出错");
	}
	
	/**
	 * Pinpad输入密码（PinBlock）
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param	isAutoReturn	输入到约定长度时是否自动返回（不需要按Enter)
	 * @param   masterIndex		主密钥索引, 没有意义
	 * @param   workingIndex	用于Pin加密的工作密钥索引
	 * @param   cardNo			卡号/帐号（最少12位数字）
	 * @param   pinLength		需要输入PIN的长度
	 * @param   timeout			输入密码等待超时时间 <= 255 秒
	 * @return	成功返回输入的Pinblock
	 * @throws  Exception	
	 */	
	public byte[] inputPinblock(boolean is3des, boolean isAutoReturn, int masterIndex, int workingIndex, String cardNo, int pinLength, int timeout) 
			throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
		
		if (cardNo.length() < 13)
			throw new Exception("卡号/帐号长度错误");
		
		try {
			changeDir(workDirNum);
		}catch (Exception e) {
			throw e;
		}
		
		try {
			pinpadDisp(1, 1, 0, 1, "Entry the PIN");
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[50];
		
		int tmp = 0;
		
		//pin输入
		sendBytes[tmp++] = (byte)0x8A;
		
		//数据长度
		sendBytes[tmp++] = (byte)0x1e;
		
		//加密类型
		if (is3des)
			sendBytes[tmp++] = (byte)0x82;
		else
			sendBytes[tmp++] = (byte)0x02;
		
		//pin key id
		sendBytes[tmp++] = (byte)workingIndex;
		
		//use cnt
		sendBytes[tmp++] = (byte)(timeout / 255);
		sendBytes[tmp++] = (byte)(timeout % 255);
		
		//input min length
		sendBytes[tmp++] = (byte)pinLength;
		
		//input max length
		sendBytes[tmp++] = (byte)pinLength;
		
		//card no
		byte[] orgAccount = cardNo.getBytes();	
		int accLen = orgAccount.length;
		int accOffset = 0;
		if (accLen > 13)
			accOffset = accLen - 13;
		
		for(int i = 0; i < 16; i++) {
			if (i >= 4)
				sendBytes[tmp++] =  orgAccount[accOffset++];
			else
				sendBytes[tmp++] = '1';
		}
		
		byte[] authCode;
		try{			
			authCode = getAuthCode(sendBytes, 2, sendBytes[1] - 8, (byte)0x01);
		}catch (Exception e){
			throw e;
		}
		
		for(int i = 0; i < 8; i ++){
			sendBytes[i + sendBytes[1] - 8 + 2] = authCode[i];
		}
		
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, timeout + 1);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("密码键盘输入pin错误");
		
		if (receivedLength <= 8)
    		throw new Exception("取消");
		
		byte[] retByte = new byte[receivedLength - 8];
		for (int i=0; i<receivedLength - 8; i++)
			retByte[i] = receivedBytes[i];
		
		return retByte;	 
	}
	
	/**
	 * Pinpad加解密数据
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param	algo			算法，取值: Pinpad.ALGO_ENCRYPT, Pinpad.ALGO_DECRYPT, 以ECB方式进行加解密运算
	 * @param   masterIndex		主密钥索引, 没有意义
	 * @param   workingIndex	用于加解密的工作密钥索引
	 * @param   data			加解密数据，长度为8的倍数
	 * @param   dataLength		加解密数据的长度,长度为8的倍数
	 * @return	成功返回加解密的结果数据
	 * @throws  Exception	
	 */	
	public byte[] encrypt(boolean is3des, int algo, int masterIndex, 
			int workingIndex, byte[] data, int dataLength) throws Exception
	{
		if ((dataLength % 8) != 0 )
			throw new Exception("数据长度错误");
		
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
		
		try {
			changeDir(workDirNum);
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[50];
		byte[] retBytes = new byte[dataLength];
		
		byte encMode = 0;
		if (algo == Pinpad.ALGO_DECRYPT)
			encMode = (byte)0x80;
		if (is3des) {
			encMode |= 0x03;
		}
		else {
			encMode |= 0x01;			
		}
		
		// DES/3DES 加解密
		int retLength = 0;
		for(int j = 0; j < dataLength; j += 8){
			int tmp = 0;
			sendBytes[tmp++] = (byte) 0x87;
			sendBytes[tmp++] = 0x0A;
			sendBytes[tmp++] = (byte)workingIndex;
			sendBytes[tmp++] = encMode;
			for(int i = 0; i < 8; i++){
				sendBytes[tmp++] = data[j + i];
				
			}
			int recvLength = 0;
			try {
				recvLength = pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, 2);
			} catch (Exception e) {
				throw e;
			}
			if(encMode != (byte)0x82){
				if (recvLength % 8  != 0)
					throw new Exception("加解密数据失败");
			}else{
				if (recvLength % 18  != 0)
				throw new Exception("dukpt 加解密数据失败");
			}
			for (int i=0; i<recvLength; i++){
				retBytes[i + retLength] = receivedBytes[i];
			}
			retLength += recvLength;
		}
		if(retLength == 0){
			throw new Exception("加解密数据失败");
		}
		
		return retBytes;
	}

	/**
	 * Pinpad数据MAC运算（ANSIX9.9）
	 * @param	is3des			是否采用3DES算法，false表示使用DES算法
	 * @param   masterIndex		主密钥索引，没有意义
	 * @param   workingIndex	用于加解密的工作密钥索引
	 * @param   data			计算Mac原数据
	 * @param   dataLength		Mac原数据的长度,要求8的倍数并小于或等于246字节长度
	 * @return	成功返回8字节Mac数据
	 * @throws  Exception	
	 */	
	public byte[] mac(boolean is3des, int masterIndex, 
			int workingIndex, byte[] data, int dataLength) throws Exception
	{
		if ((dataLength % 8) != 0 || dataLength >= 246 )
			throw new Exception("数据长度错误");
		
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	
		
		try {
			changeDir(workDirNum);
		}catch (Exception e) {
			throw e;
		}
				
		final byte[] receivedBytes = new byte[300];
		final byte[] sendBytes = new byte[300];
		
		byte encMode = 0x01;
		if (is3des)
			encMode = 0x03;
		
		int tmp = 0;
		sendBytes[tmp++] = (byte) 0x85;
		sendBytes[tmp++] = (byte) (10 + dataLength);
		sendBytes[tmp++] = (byte) (workingIndex);
		sendBytes[tmp++] = encMode;
		
		for(int i = 0; i< dataLength; i++){
			sendBytes[tmp++] = data[i];
		}
		
		byte[] authCode;
		try{			
			authCode = getAuthCode(sendBytes, 2, sendBytes[1] - 8, (byte)0x01);
		}catch (Exception e){
			throw e;
		}
		
		for(int i = 0; i < 8; i ++){
			sendBytes[tmp++] = authCode[i];
		}
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, 2);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("密码键盘算MAC错");
		
		byte[] retByte = new byte[8];
		for (int i=0; i<8; i++)
			retByte[i] = receivedBytes[i];
		
		return retByte;	 
	}
	
	private void delay(long m) {
	    try {
	    	Thread.sleep(m);
	    } catch (InterruptedException e) {}
	}
	
	private void pininputCancel() 
	{
		final byte[] receivedBytes = new byte[20];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 3;		//发送数据报文命令
		
		sendBytes[3] = 0;
		sendBytes[4] = 3;	
		sendBytes[5] = (byte)0x8E; //cancel command
		sendBytes[6] = (byte)0x00; //data length
		sendBytes[7] = (byte)0x8E; //bcc

		if (BluetoothThread.getInstance().sendReceive(sendBytes, 8, receivedBytes, 1) < 0)
			return;
		
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 4;
	    BluetoothThread.getInstance().sendReceive(sendBytes, 3, receivedBytes, 1);
	}
	
	private int pinpadComm(byte[] in, int inLength, byte[] out, int timeout) throws Exception
	{
		mmCancelFlag = false;
		
		final byte[] receivedBytes = new byte[600];
		final byte[] sendBytes = new byte[600];
		byte chkCMD = in[0];
		byte bcc = 0x00;
		for (int i=0; i<inLength; i++)
			bcc ^= in[i];
		
		sendBytes[0] = 0x69;
		sendBytes[1] = 0x00;
		sendBytes[2] = 3;		//发送数据报文命令
		sendBytes[3] = (byte)((inLength + 1)/256);
		sendBytes[4] = (byte)((inLength + 1)%256);	
		for (int i=0; i<inLength; i++)
			sendBytes[i+5] = in[i];
		sendBytes[5 + inLength] = bcc;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 5 + inLength + 1, receivedBytes, timeout+1);
	    if (receivedLength < 1)
	    	throw new Exception("iMate通讯超时");
	    
	    long m = System.currentTimeMillis() + timeout*1000L+100;
	    receivedLength = 0;
	    boolean finished = false;
	    int receivePackLength = 0;
		byte[] tmpBytes = new byte[600];
	    while (System.currentTimeMillis() < m) {
	    	if (mmCancelFlag == true) {
	    		pininputCancel();
	    		mmCancelFlag = false;
	    		throw new Exception("取消Pin输入");
	    	}
	    		
			sendBytes[0] = 0x69;
			sendBytes[1] = 0x00;
			sendBytes[2] = 4;
			
		    int ret = BluetoothThread.getInstance().sendReceive(sendBytes, 3, tmpBytes, 1);
		    if (ret <= 0)
				throw new Exception("iMate通讯超时");
		    
		    if (tmpBytes[0] != 0)
				throw new Exception("iMate处理失败");
		    
		    if (ret <= 1) {
		    	continue;
		    }
		    //System.out.println("ret = " + (ret -1));		    
		    for (int i=0; i<ret-1; i++) {
		    	receivedBytes[receivedLength+i] = tmpBytes[i+1];
		    	//System.out.format("%02X ", tmpBytes[i +1]);
		    }
		    //System.out.println();
		    receivedLength += ret-1;
		    if((chkCMD >= (byte)0xB0 && chkCMD <= (byte)0xB5) || (chkCMD >= (byte)0xE0 && chkCMD <= (byte)0xE8)){
		    	if (receivedLength != receivedBytes[1] *256 + receivedBytes[2] + 4)
			    	continue;
		    }else{
		    	if (receivedLength != receivedBytes[1] + 3)
			    	continue;
		    }
		    
		    
		    //if (receivedBytes[0] == chkCMD ) {
		    	finished = true;
		    	break;
		   // }
	    }
		if (!finished)
			throw new Exception("通讯超时");
		/*
		System.out.print("receivedLength:" + receivedLength + "data: ");
		for (int i = 0; i < receivedLength; i++) {
			System.out.format("%02X ", receivedBytes[i]);
		}
		System.out.println();
		*/
		
		if(receivedBytes[0] != chkCMD){
			byte errorCode;
			if((chkCMD >= (byte)0xB0 && chkCMD <= (byte)0xB5) || (chkCMD >= (byte)0xE0 && chkCMD <= (byte)0xE8)){
				errorCode = receivedBytes[3];
				if(chkCMD == (byte)0xB4 || chkCMD == (byte)0xB3){
					throw new Exception(piccError(errorCode));
				}else{
					throw new Exception(pinpadError(errorCode));
				}
			}else{
				errorCode = receivedBytes[2];
				throw new Exception(pinpadError(errorCode));
			}
		}		
		
		if (out != null) {
			if((chkCMD >= (byte)0xB0 && chkCMD <= (byte)0xB5) || (chkCMD >= (byte)0xE0 && chkCMD <= (byte)0xE8)){
				receivePackLength = receivedBytes[1] * 256 + receivedBytes[2];
				for(int i = 0; i < receivePackLength; i++){
					out[i] = receivedBytes[i + 3];
				}	
			}else{
				receivePackLength = receivedBytes[1];
				for(int i = 0; i < receivePackLength; i++){
					out[i] = receivedBytes[i + 2];
				}	
			}
			
		}
		return receivePackLength;
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
	
	private void changeDir(int dirNum) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[3];
			    		
		sendBytes[0] = (byte) 0xD6;
		sendBytes[1] = (byte) 0x01;
		sendBytes[2] = (byte) dirNum;		
			
		try {
			pinpadComm(sendBytes, sendBytes[1] + 2, receivedBytes, 2);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * Pinpad屏幕行显示
	 * @param   clearScreen  0x01 清屏  0x00 不清屏
	 * @param   alignment    0x01 左对齐  0x02 中间对齐  0x03 右对齐
	 * @param   antiShow     0x00 正常显示  0x01 反显
	 * @param   lineNum      行号
	 * @param   displayString         显示数据
	 * @throws  Exception	
	 */
	private void pinpadDisp(int clearScreen, int alignment, int antiShow, int lineNum, String displayString) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	

		final byte[] sendBytes = new byte[60];
		
		int tmp = 0;
		sendBytes[tmp++] = (byte) 0xE3;
		sendBytes[tmp++] = (byte) 0x00;
		sendBytes[tmp++] = (byte) (0x05 + displayString.length());
		sendBytes[tmp++] = (byte) 0x00;
		sendBytes[tmp++] = (byte) clearScreen;
		sendBytes[tmp++] = (byte) alignment;
		sendBytes[tmp++] = (byte) antiShow;
		sendBytes[tmp++] = (byte) lineNum;
		
		for(int i = 0; i < displayString.length() ; i++){
			sendBytes[tmp++] = displayString.getBytes()[i];
		}

		try {
			pinpadComm(sendBytes, (sendBytes[1] * 256) + sendBytes[2] + 3, null, 2);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * Pinpad取随机数
	 * @param   
	 * @throws  Exception
	 */	
	private byte[] getRand() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	

		final byte[] sendBytes = new byte[2];
		final byte[] receivedBytes = new byte[20];
		
		sendBytes[0] = (byte) 0xD0;
		sendBytes[1] = (byte) 0x00;
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, 2, receivedBytes, 1);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("取随机数出错");
		
		byte[] retByte = new byte[receivedLength];
		for (int i=0; i<receivedLength; i++)
			retByte[i] = receivedBytes[i];
		
		return retByte;
	}
	
	private byte[] getAuthCode(byte[] data, int startIndex, int dataLength, byte mode) throws Exception{
		byte[] authMac = new byte[8];
		for(int i = 0; i < 8; i++){
			authMac[i] = (byte)0x00;
		}
		
		//异或 长度后到authcode前的数据
		for(int i = 0; i < dataLength; i++){
			authMac[i % 8] ^= data[startIndex + i];
		}
		
		/*
		System.out.print("authMac data: ");
		for(int i = 0; i < 8; i++){
			System.out.format("%02X", authMac[i]);
		}
		System.out.println();
		*/
		
		//byte[] random = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
		byte[] random;
		try {
			random = getRand();
		}catch (Exception e) {
			throw new Exception("取随机数失败");
		}
		
		//异或随机数 
		for(int i = 0; i < 8; i++){
			authMac[i] ^= random[i];
		}
		
		/*
		System.out.print("random data: ");
		for(int i = 0; i < 8; i++){
			System.out.format("%02X", random[i]);
		}
		System.out.println();
		*/
	
		//初始化时不需要异或uid
		if(mode == 0x01){			
			for(int i = 0; i < 16; i++){
				authMac[i % 8] ^= uid[i];
			}
		}
		/*
		System.out.print("EncData before : ");
		for(int i = 0; i < 8; i++){
			System.out.format("%02X", authMac[i]);
		}
		System.out.println();
		*/
		
		//3DES加密
		byte[] authCode = encryptMode(authMac, authCodeKey);
		
		if(authCode == null){
			throw new Exception("AuthCode 计算失败");
		}
		
		/*
		System.out.print("EncData after: ");
		for(int i = 0; i < 8; i++){
			System.out.format("%02X", authCode[i]);
		}
		System.out.println();
		*/
		
		return authCode;
		
	}
	private static final String Algorithm = "DESede"; //定义 加密算法,可用 DES,DESede,Blowfish
	private byte[] encryptMode(byte[] data, byte[] key){
		try{
			SecretKey deskey = new SecretKeySpec(key, Algorithm);
			
			//加密
			Cipher encC = Cipher.getInstance(Algorithm);
			encC.init(Cipher.ENCRYPT_MODE, deskey);
			return encC.doFinal(data);
		}catch (java.security.NoSuchAlgorithmException e1) {
            e1.printStackTrace();
        } catch (javax.crypto.NoSuchPaddingException e2) {
            e2.printStackTrace();
        } catch (java.lang.Exception e3) {
            e3.printStackTrace();
        }
        return null;
	}
	
	private String pinpadError(byte errorCode){
		String retStr = null;
		switch(errorCode){
		case (byte)0x01:
			retStr = "协议的长度错误";
			break;
		case (byte)0x02:
			retStr = "密钥校验错误";
			break;
		case (byte)0x03:
			retStr = "打开失败";
			break;
		case (byte)0x04:
			retStr = "关闭失败";
			break;
		case (byte)0x05:
			retStr = "设备操作失败";
			break;
		case (byte)0x06:
			retStr = "超时";
			break;
		case (byte)0x07:
			retStr = "参数错";
			break;
		case (byte)0x08:
			retStr = "认证失败";
			break;
		case (byte)0x09:
			retStr = "连续认证失败次数超过30次，密码键盘被锁定";
			break;
		case (byte)0x0A:
			retStr = "非法初始化";
			break;
		case (byte)0x0B:
			retStr = "非法探测保护";
			break;
		case (byte)0x0D:
			retStr = "EDC错误";
			break;
		case (byte)0x0E:
			retStr = "ESAM操作错误";
			break;
		case (byte)0x0F:
			retStr = "无卡";
			break;
		case (byte)0x21:
			retStr = "执行复位指令操作失败";
			break;	
		case (byte)0x31:
			retStr = "错误目录号";
			break;	
		case (byte)0x41:
			retStr = "随机数发生错误";
			break;
		case (byte)0x51:
			retStr = "非法主密钥ID或mode";
			break;
		case (byte)0x52:
			retStr = "当前目录错误";
			break;
		case (byte)0x53:
			retStr = "主密钥下载错误";
			break;
		case (byte)0x54:
			retStr = "写数据错";
			break;	
		case (byte)0x55:
			retStr = "读数据错";
			break;	
		case (byte)0x56:
		case (byte)0x57:
			retStr = "超出存储空间";
			break;	
		case (byte)0x61:
			retStr = "工作密钥ID错误";
			break;
		case (byte)0x62:
			retStr = "模式错误";
			break;
		case (byte)0x63:
			retStr = "指定的主密钥模式或ID错";
			break;
		case (byte)0x64:
			retStr = "密钥发散错";
			break;
		case (byte)0x65:
			retStr = "主密钥类型错误";
			break;
		case (byte)0x66:
			retStr = "主密钥截取错";
			break;
		case (byte)0x67:
			retStr = "密钥已经存在";
			break;
		case (byte)0x68:
			retStr = "密钥模式错";
			break;
		case (byte)0x71:
			retStr = "PIN加密密钥超过指定的使用次数";
			break;
		case (byte)0x72:
			retStr = "模式错误";
			break;
		case (byte)0x73:
			retStr = "PIN密钥ID非法";
			break;
		case (byte)0x74:
			retStr = "PIN的位数设置错";
			break;
		case (byte)0x75:
			retStr = "PIN加密错";
			break;
		case (byte)0x76:
			retStr = "PIN输入超时";
			break;
		case (byte)0x77:
			retStr = "用户取消PIN输入";
			break;
		case (byte)0x78:
			retStr = "第一次输入PIN与第二次输入PIN不相同";
			break;
		case (byte)0x79:
			retStr = "PINBLOCK算法设置错";
			break;
		case (byte)0x7A:
			retStr = "PIN输入相邻按键超时";
			break;
		case (byte)0x7B:
			retStr = "PIN 输入长度为0";
			break;
		case (byte)0x7C:
			retStr = "单位小时运算次数超过110次";
			break;
		case (byte)0x7D:
			retStr = "设置使用次数超出最大限制";
			break;
		case (byte)0x81:
			retStr = "加密ID或 mode非法";
			break;
		case (byte)0x82:
			retStr = "数据不是8的整数倍";
			break;
		case (byte)0x83:
			retStr = "MAC计算错误";
			break;
		case (byte) 0x91:
			retStr = "模式错误";
			break;
		case (byte) 0x92:
			retStr = "加解密失败";
			break;
		case (byte)0xA1:
			retStr = "DUKPT Load错";
			break;
		case (byte)0xA2:
			retStr = "计算器溢出";
			break;
		case (byte)0xA3:
			retStr = "更新DUKPT 21个新密钥错误";
			break;
		case (byte)0xA4:
			retStr = "存储DUKPT错误";
			break;
		case (byte)0xA5:
			retStr = "LoadinitKey 错误";
			break;
		case (byte)0xA9:
			retStr = "RTC 设置非法";
			break;
		case (byte)0xAA:
			retStr = "数据长度校验错";
			break;
		case (byte)0xAB:
			retStr = "接收数据内容校验错误";
			break;
		case (byte)0xB1:
			retStr = "RTC 设置失败";
			break;
		case (byte)0xD1:
			retStr = "LOG 操作越界";
			break;
		case (byte)0xE1:
			retStr = "UID 已经下载过了";
			break;
		case (byte) 0xFF:
			retStr = "执行错误";
			break;
		default:
			retStr = "未识别错误" + errorCode;
		}
		return retStr;
	}
	private String piccError(byte errorCode){
		String retStr = null;
		switch(errorCode){
			case 0x01:
				retStr = "协议的长度错误";
				break;
			case 0x02:
				retStr = "激活(上电)前未执行检测卡操作";
				break;
			case 0x03:
				retStr = "感应区中有多于一张的Type PICC卡";
				break;
			case 0x04:
				retStr = "Type A 卡 RATS 失败";
				break;
			case 0x05:
				retStr = "设备操作失败";
				break;
			case 0x06:
				retStr = "无卡";
				break;
			case 0x07:
				retStr = "B 卡激活失败";
				break;
			case 0x0A:
				retStr = "A 卡激活失败(可能多张卡存在)";
				break;
			case 0x0B:
				retStr = "B 卡冲突(可能多张卡存在)";
				break;
			case 0x0C:
				retStr = "A、B卡同时存在";
				break;
			case 0x0F:
				retStr = "不支持 ISO14443-4 协议的卡, 比如 Mifare-1卡";
				break;
			case (byte) 0xFF:
				retStr = "执行错误";
				break;
			default:
				retStr = "未识别错误" + errorCode;
		}
		return retStr;
	}
	/*
	public byte[] getIssue(byte[] encAuthCode, byte[] machineNum, byte[] encUid, byte dirNum, byte[] record) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}	

		final byte[] sendBytes = new byte[100];
		final byte[] receivedBytes = new byte[20];		
		
		int tmp = 0;
		sendBytes[tmp++] = (byte) 0xD4;
		sendBytes[tmp++] = (byte) 0x41;
		//认证密钥16字节
		for(int i = 0; i < 16; i++){
			sendBytes[tmp++] = 	encAuthCode[i];	
		}
		
		//机具序列号 16 字节
		for(int i = 0; i < 16; i++){
			sendBytes[tmp++] = machineNum[i];	
		}
		
		//uid 16  字节密文
		for(int i = 0; i < 16; i++){
			sendBytes[tmp++] = encUid[i];	
		}
		
		//初始化之后的目录分区数[1, 4];
		sendBytes[tmp++] = dirNum;
		
		//安装记录数 8 字节数据
		for(int i = 0; i < 8; i++){
			sendBytes[tmp++] = record[i];	
		}
		
		//计算authCode
		byte[] authCode;
		try{			
			authCode = getAuthCode(sendBytes, 2, sendBytes[1] - 8, (byte)0x00);
		}catch (Exception e){
			throw e;
		}
		
		for(int i = 0; i < 8; i++){
			sendBytes[tmp++] = authCode[i];
		}
		
		int receivedLength = 0;
		try {
			receivedLength = pinpadComm(sendBytes, sendBytes[1]  + 2, receivedBytes, 5);
		}catch (Exception e) {
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("密码键盘初始化失败");
		
		byte[] retByte = new byte[receivedLength];
		for (int i=0; i<receivedLength; i++)
			retByte[i] = receivedBytes[i];
		
		return retByte;
	}
	*/
}

