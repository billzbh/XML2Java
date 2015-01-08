package com.hxsmart.imateinterface.fingerprint;

import android.util.Log;

import com.hxsmart.imateinterface.BluetoothThread;

//中正指纹仪
//指令（复位，终止回空闲，获取描述符，获取序列号，读取指纹特征256/128字节，登记3-5次指纹生成模板）
public class FingerprintZhongZhengForUSB{
	public final static int COMM_NONE = 0;
	public final static int COMM_EVEN = 1;
	public final static int COMM_ODD = 2;
	
	public int COMM_PORT = 3;	//指纹模块连接iMate内部端口号-通讯编号
	public int POWER_PORT = 2;	//指纹模块连接iMate内部端口号-电源编号
	
	
	private volatile boolean mmCancelFlag = false;
	
	public void cancel()
	{
		mmCancelFlag = true;
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
	
	private void delay(long m) {
	    try {
	    	Thread.sleep(m);
	    } catch (InterruptedException e) {}
	}
	
	/**
	 * 指纹模块上电 (传入通讯波特率和校验方式）
	 * @param   baud 	波特率
	 * @param   parity 	校验位,取值：COMM_NONE, COMM_EVEN, COMM_ODD
	 * @throws  Exception 
	 */
	public void powerOn(int comPort, int powerPort, long baud, int parity) throws Exception
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
	    
	    delay(2000L);		
	}
	
	/**
	 * 指纹模块上电 (缺省波特率连接，9600bps，COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception 
	{
		try {
			powerOn(COMM_PORT, POWER_PORT, 9600L, COMM_NONE);
		} catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * 指纹模块下电
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
	 * 获取指纹模块的版本号信息
	 * @return	成功将返回指纹模块的版本号信息,详细格式请参考厂家文档。
	 * @throws  Exception 
	 */
	public String getVersion() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		//指纹仪指令： 读取模块描述符
		final byte[] sendBytes = {0x02,0x30,0x30,0x30,0x34,0x30,0x39,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3d,0x03};
		
		final byte[] receivedBytes = new byte[100];
		
		int receivedLength = 0;	
		try {
			//在fingerprintComm组装蓝牙的包头，接收的信息也解出真正的数据。
			receivedLength = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 1);
		}catch (Exception e) {
			//复位
			reset();
			throw e;
		}
		
		if (receivedLength <= 0)
			throw new Exception("无版本信息");
		
		return bytesToHexString(receivedBytes,receivedLength);
	}
	
	/**
	 * 采集指纹特征值  128字节 (hexString)
	 * @throws  Exception	
	 */	
	public String takeFingerprintFeature() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		BluetoothThread.getInstance().buzzer();
		
		final byte[] sendBytes = {0x02,0x30,0x30,0x30,0x34,0x31,0x3c,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x38,0x03};
		final byte[] receivedBytes = new byte[600];
				
		int length = 0;	
		try {
			length = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 5);
		}catch (Exception e) {
			reset();
			throw e;
		}
		return bytesToHexString(receivedBytes,length);
	}
	
	/**
	 * 登记3次指纹生成模板 (hexString)
	 * @throws  Exception	
	 */	
	public String GenerateFingerTemplate() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		BluetoothThread.getInstance().buzzer();
		
		final byte[] sendBytes = {0x02,0x30,0x30,0x30,0x34,0x30,0x3b,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3f,0x03};
		final byte[] receivedBytes = new byte[600];
				
		int length = 0;	
		try {
			length = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 15);
		}catch (Exception e) {
			reset();
			throw e;
		}
		return bytesToHexString(receivedBytes,length);
	}
	
	
	private int fingerprintComm(byte[] in, int inLength, byte[] out, int timeout) throws Exception
	{
		final byte[] receivedBytes = new byte[600];
		final byte[] sendBytes = new byte[600];
			    		
		sendBytes[0] = 0x6A;
		sendBytes[1] = (byte)COMM_PORT; 	// iMate与指纹模块相连的通讯端口
		sendBytes[2] = 3;					// 发送数据报文命令
		sendBytes[3] = (byte)(inLength/256);
		sendBytes[4] = (byte)(inLength%256);
				
		for (int i=0; i<inLength; i++) {
			sendBytes[5 + i] = in[i];
		}

	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, inLength + 5, receivedBytes, 1);
		
	    if (receivedLength < 1)
	    	throw new Exception("iMate通讯超时");
	    if (receivedBytes[0] != 0)
			throw new Exception("不支持指纹模块或iMate处理失败");
	    
	    long m = System.currentTimeMillis() + timeout*1000L+100;
	    receivedLength = 0;
	    boolean finished = false;
	    while (System.currentTimeMillis() < m) {
	    	if (mmCancelFlag) {
	    		mmCancelFlag = false;
	    		powerOff();
	    		throw new Exception("操作取消");	    		
	    	}
	    	
			sendBytes[0] = 0x6A;
			sendBytes[1] = (byte)COMM_PORT;
			sendBytes[2] = 4;
			
			byte[] tmpBytes = new byte[600];
		    int ret = BluetoothThread.getInstance().sendReceive(sendBytes, 3, tmpBytes, 1);
		    
		    
		    
		    System.out.println("ret = "+ret + "  tmpBytes[0] = " + tmpBytes[0]);
			System.out.format("receivedBytes:");
			for (int i = 0; i < ret; i++) {
				System.out.format("%02X ", tmpBytes[i]);
			}
			System.out.println();
		    
		    
		    if (ret <= 0)
				throw new Exception("iMate通讯超时");
		    
		    if (tmpBytes[0] != 0)
				throw new Exception("不支持指纹模块或iMate处理失败");
		    
		    		    
		    for (int i=0; i<ret-1; i++) {
		    	receivedBytes[receivedLength+i] = tmpBytes[i+1];
		    }
		    receivedLength += ret-1;
		    
		    if (ret < 2) {
		    	continue;
		    }
		     
		    if (tmpBytes[ret - 1] == 0x03 ) {
		    	finished = true;
		    	receivedLength -= 8;
		    	break;
		    }    
	    }
	    
	    int reallength =receivedLength/2-2;
	    
		if (!finished)
			throw new Exception("通讯超时");
				
		if (out != null) {
			
			//解析数据  将两个字节3X 3X 转换--》XX（一个字节）（例如0x31 0x3b ----》 0x1b ）
			byte[] retbyte = new byte[receivedLength/2];
			for(int i=0,j=0;i < receivedLength;j++,i +=2)
			{
				retbyte[j]=(byte) (((receivedBytes[5 + i] & 0x0000000f)<<4) | (receivedBytes[6 + i]&0x0000000f));
			}	
			
			//判断第一个字节，执行成功与否？
			if(retbyte[0]!=0x00)
				throw new Exception(returnErrorString(retbyte[0]));
			
			//截取数据（去掉前面两个字节）
			for(int i=0;i<reallength;i++)
			{
				out[i] = retbyte[i+2];
			}
			
			/*
			System.out.format("receivedBytes:");
			for (int i = 0; i < reallength; i++) {
				System.out.format("%02X ", out[i]);
			}
			System.out.println();
			*/
		}
		Log.i("zbh", "数据长度："+reallength);
		return reallength;
	}
	
    public static String bytesToHexString(byte[] src, int len) {
		StringBuilder stringBuilder = new StringBuilder("");
		if (src == null || len <= 0) {
			return null;		
		}
	    for (int i = 0; i < len; i++) {	
	    	int v = src[i] & 0xFF;	
	    	String hv = Integer.toHexString(v);	
	    	if (hv.length() < 2) {	
		    	stringBuilder.append(0);		
		    }	
	    	stringBuilder.append(hv);	
	    }
	    return stringBuilder.toString();
    }
    
    
    //复位，只发送指令，不关注结果
    private void reset()
    {
    	final byte[] receivedBytes = new byte[600];
		final byte[] sendBytes = new byte[100];
		final byte[] in= {0x02,0x30,0x30,0x30,0x34,0x30,0x34,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x03};
		
		sendBytes[0] = 0x6A;
		sendBytes[1] = (byte)COMM_PORT; 	// iMate与指纹模块相连的通讯端口
		sendBytes[2] = 3;					// 发送数据报文命令
		sendBytes[3] = (byte)(in.length/256);
		sendBytes[4] = (byte)(in.length%256);
				
		for (int i=0; i<in.length; i++) {
			sendBytes[5 + i] = in[i];
		}
		
	    BluetoothThread.getInstance().sendReceive(sendBytes, in.length + 5, receivedBytes, 1);	
    }
    
	
	private String returnErrorString(int retCode)
	{
	    switch (retCode) {
	        case 0x01:
	            return "指令执行失败";
	        case 0x0a:
	            return "设备忙";
	        case 0x0c:
	            return "未按好指纹";
	    }
	    return "指纹模块其它错误码:" + retCode;
	}

	//256字节 特征码(hexString)
	public String fingerExpInfo() throws Exception {
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		BluetoothThread.getInstance().buzzer();
		
		final byte[] sendBytes = {0x02,0x30,0x30,0x30,0x34,0x30,0x3c,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x38,0x03};
		final byte[] receivedBytes = new byte[600];
				
		int length = 0;	
		try {
			length = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 8);
		}catch (Exception e) {
			//复位
			reset();
			throw e;
		}
		return bytesToHexString(receivedBytes,length);
	}
	
}