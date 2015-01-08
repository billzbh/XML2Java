package com.hxsmart.imateinterface.fingerprint;

import android.util.Base64;

import com.hxsmart.imateinterface.BluetoothThread;

public class FingerprintJSABC implements FingerprintInterface{	
	public final static int COMM_NONE = 0;
	public final static int COMM_EVEN = 1;
	public final static int COMM_ODD = 2;
	
	public int COMM_PORT = 3;	//指纹模块连接iMate内部端口号-通讯编号
	public int POWER_PORT = 2;	//指纹模块连接iMate内部端口号-电源编号
	
	private volatile boolean mmCancelFlag = false;
	
	/**
	 * 取消指纹仪的等待...
	 */
	public void cancel()
	{
		mmCancelFlag = true;
	}
	
	/**
	 * 判断设备是否为 iMate 或 iMateMini，由此决定指纹模块所连接的端口号以及上电编号
	 */
	private void setupComPort()
	{
		if (BluetoothThread.getInstance().deviceVersion().contains("IMATEMINI") ||
				(BluetoothThread.getInstance().deviceVersion().contains("IMATE") && BluetoothThread.getInstance().deviceVersion().getBytes()[5] >= '5')) {
			COMM_PORT = 4;
			POWER_PORT = 4;
		}
		else if (BluetoothThread.getInstance().deviceVersion().contains("IMATE")) {
			COMM_PORT = 3;
			POWER_PORT = 2;	
		}
	}
		
	/**
	 * 指纹模块上电 (传入通讯波特率和校验方式）
	 * @param   baud 	波特率
	 * @param   parity 	校验位,取值：COMM_NONE, COMM_EVEN, COMM_ODD
	 * @throws  Exception 
	 */
	private void powerOn(int comPort, int powerPort, long baud, int parity) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[9];
		
		sendBytes[0] = 0x6A;
		sendBytes[1] = (byte)comPort;
		sendBytes[2] = 1;		
		sendBytes[3] = (byte)((baud >> 24) % 256);
		sendBytes[4] = (byte)((baud >> 16) % 256);
		sendBytes[5] = (byte)((baud >> 8) % 256);
		sendBytes[6] = (byte)(baud % 256);
		sendBytes[7] = (byte)(parity);
		sendBytes[8] = (byte)(powerPort);
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 9, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");
	    
	    delay(800L);		
	}
	
	/**
	 * 指纹模块上电 (缺省波特率连接，9600bps，COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception 
	{
		setupComPort();
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
		setupComPort();
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}		

		final byte[] receivedBytes = new byte[50];
		final byte[] sendBytes = new byte[4];
		
		sendBytes[0] = 0x6A;
		sendBytes[1] = (byte)COMM_PORT;
		sendBytes[2] = 2;
		sendBytes[3] = (byte)POWER_PORT;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 4, receivedBytes, 1);
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

		final byte[] sendBytes = {0x02, 0x41, 0x41, 0x4E, 0x46, 0x30, 0x74, 0x6F, 0x2F, 0x7A, 0x67, 0x3D, 0x3D, 0x03};
		final byte[] receivedBytes = new byte[100];
		
		int receivedLength = 0;	
		try {
			receivedLength = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 2);
		}catch (Exception e) {
			throw e;
		}
		if (receivedLength <= 0)
			throw new Exception("无版本信息");
		
		return new String(receivedBytes, 0, receivedLength);
	}
	
	/**
	 * 采集指纹特征值
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
		
		final byte[] sendBytes = {0x02, 0x41, 0x41, 0x4d, 0x33, 0x62, 0x4e, 0x45, 0x76, 0x33, 0x67, 0x3d, 0x3d, 0x03};
		final byte[] receivedBytes = new byte[600];
				
		int length = 0;	
		try {
			length = fingerprintComm(sendBytes, sendBytes.length, receivedBytes, 6);
		}catch (Exception e) {
			throw e;
		}
		if (length <= 16)
			throw new Exception("采集指纹出错或超时");
		
		//处理 receiveBytes  by zbh
		
		//change to base64string
		String base64String = new String(receivedBytes, 0, length);
		//decode and  change to bytes
		byte[] decoded = Base64.decode(base64String, Base64.DEFAULT);
		
		System.out.println("lenth"+decoded.length);
		// get 256 bytes from decode bytes
//		byte[] resultbytes = new byte[256];
//		System.arraycopy(decoded, 6, resultbytes, 0, 256);
		
		//get 256 bytes from decode bytes and  encode to base64 again
		String resultBASE64String = Base64.encodeToString(decoded, 6, 256, Base64.DEFAULT);
		return resultBASE64String;
	}
	
	private void delay(long m) {
	    try {
	    	Thread.sleep(m);
	    } catch (InterruptedException e) {}
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
		/*
		System.out.format("sentBytes:");
		for (int i = 0; i < inLength + 5; i++) {
			System.out.format("%02X ", sendBytes[i]);
		}
		System.out.println();
		*/
		
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
		    if (ret <= 0)
				throw new Exception("iMate通讯超时");
		    
		    if (tmpBytes[0] != 0)
				throw new Exception("不支持指纹模块或iMate处理失败");
		    
		    if (ret <= 1) {
		    	continue;
		    }
		    		    
		    for (int i=0; i<ret; i++) {
		    	receivedBytes[receivedLength+i] = tmpBytes[i+1];
		    }
		    receivedLength += ret-1;

		    if (receivedLength < 2)
		    	continue;
		    
		    /*
			System.out.format("******receivedBytes:");
			for (int i = 0; i < receivedLength; i++) {
				System.out.format("%02X ", receivedBytes[i]);
			}
			System.out.println();
			*/
		    
		    if (receivedBytes[receivedLength - 1] == 0x03 ) {
		    	finished = true;
		    	receivedLength -= 2;
		    	break;
		    }
	    }
		if (!finished)
			throw new Exception("通讯超时");
				
		if (out != null) {
			for (int i = 0; i < receivedLength; i++)
				out[i] = receivedBytes[1 + i];
			/*
			System.out.format("receivedBytes:");
			for (int i = 0; i < receivedLength; i++) {
				System.out.format("%02X ", out[i]);
			}
			System.out.println();
			*/
		}	
		return receivedLength;
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

	@Override
	public String fingerExpInfo() throws Exception {
		throw new Exception("不支持此功能");
	}

	@Override
	public String GenerateFingerTemplate() throws Exception {
		throw new Exception("不支持此功能");
	}
}
