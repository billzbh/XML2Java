package com.hxsmart.imateinterface.pinpad;

public class Pinpad {
	public final static int KMY_MODEL = 0;
	public final static int XYD_MODEL = 1;
	
	public final static int ALGO_ENCRYPT = 1;
	public final static int ALGO_DECRYPT = 2;
	
	public final static int DECRYPT_KEY_MODE = 0x00;
	public final static int ENCRYPT_KEY_MODE = 0x01;
	public final static int PIN_KEY_MODE = 0x02;
	
	public final static int COMM_NONE = 0;
	public final static int COMM_EVEN = 1;
	public final static int COMM_ODD = 2;
		
	private KmyPinpad kmyPinpad;
	private XydPinpad xydPinpad;
	
	private int pinpadModel = KMY_MODEL;
	
	/*
	 * 设置Pinpad型号
	 * @param pinpadModel	Pinpad.KMY_MODEL ：凯明扬密码键盘
	 * 						Pinpad.XYD_MODEL : 信雅达密码键盘
	 */
	public void setPinpadModel(int pinpadModel)
	{
		if (pinpadModel != KMY_MODEL && pinpadModel != XYD_MODEL)
			return;
		this.pinpadModel = pinpadModel;
		kmyPinpad = null;
		xydPinpad = null;
	    if (pinpadModel == KMY_MODEL) {
	    	kmyPinpad = new KmyPinpad();
	    }else {
	    	xydPinpad = new XydPinpad();
	    }
	}
	
	/**
	 * 设置算法类型， 用于下载masterkey或workingkey之前调用
	 * @param	mode		主密钥类型包括 : 
	 * 						Pinpad.DECRYPT_KEY_MODE, Pinpad.ENCRYPT_KEY_MODE, Pinpad.PIN_KEY_MODE
	 */	
	public void setKeyMode(int mode)
	{
		if (xydPinpad != null)
			xydPinpad.setKeyMode(mode);
	}
	
	/**
	 * Pinpad上电 (缺省波特率连接，9600bps，Pinpad.COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception 
	{
		kmyPinpad = null;
		xydPinpad = null;
	    if (pinpadModel == KMY_MODEL && kmyPinpad == null) {
	    	kmyPinpad = new KmyPinpad();
	    }
	    if (pinpadModel == XYD_MODEL && xydPinpad == null) {
	    	xydPinpad = new XydPinpad();
	    }
		try {
			if (kmyPinpad != null)
				kmyPinpad.powerOn();
			if (xydPinpad != null)
				xydPinpad.powerOn();
		}catch (Exception e) {
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
			if (kmyPinpad != null)
				kmyPinpad.powerOff();
			if (xydPinpad != null)
				xydPinpad.powerOff();
		}catch (Exception e) {
			throw e;
		}
		kmyPinpad = null;
		xydPinpad = null;
	}
	
	/**
	 * 设置密码键盘参数, KMY密码键盘不支持
	 * key 	:	参数名称
	 * 			"AuthCode"	:   认证密钥, 16字节长度, 缺省值为16个0x00
	 * 			"UID"		:	Pinpad UID, 缺省值为{'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5'}
	 * 			"WorkDirNum	:	子目录编号, 缺省值为1;
	 */
	public void pinpadSetup(String key, byte[] value)
	{
		if (pinpadModel == XYD_MODEL && xydPinpad != null)
			xydPinpad.pinpadSetup(key, value);		
	}
	
	/**
	 * 取消密码输入操作
	 */
	public void cancel()
	{
		if (kmyPinpad != null)
			kmyPinpad.cancel();
		if (xydPinpad != null)
			xydPinpad.cancel();
	}
	
	/**
	 * 获取Pinpad的终端序列号信息
	 * @return	成功将返回Pinpad的序列号信息（8或16个字节）
	 * @throws  Exception
	 */
	public byte[] getSerialNo() throws Exception
	{
		if (kmyPinpad != null)
			return kmyPinpad.getSerialNo();
		if (xydPinpad != null)
			return xydPinpad.getSerialNo();
		
		return null;
	}
	
	/**
	 * Pinpad复位自检
	 * @param   initFlag 	true清除Pinpad中的密钥，false不清除密钥
	 * @throws  Exception
	 */	
	public void reset(boolean initFlag) throws Exception
	{
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				kmyPinpad.reset(initFlag);
			else
				xydPinpad.reset(initFlag);
		}catch (Exception e) {
			throw e;
		}
	}
	
	/**
	 * 获取Pinpad的版本号信息
	 * @return	成功将返回Pinpad的版本号信息,详细格式请参考厂家文档。
	 * @throws  Exception 
	 */
	public byte[] getVersion() throws Exception
	{
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				return kmyPinpad.getVersion();
			else
				return xydPinpad.getVersion();	
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
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				kmyPinpad.downloadMasterKey(is3des, index, masterKey, keyLength);
			else
				xydPinpad.downloadMasterKey(is3des, index, masterKey, keyLength);
		}catch (Exception e) {
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
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				kmyPinpad.downloadWorkingKey(is3des, masterIndex, workingIndex, workingKey, keyLength);
			if (xydPinpad != null)
				xydPinpad.downloadWorkingKey(is3des, masterIndex, workingIndex, workingKey, keyLength);
		}catch (Exception e) {
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
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				return kmyPinpad.inputPinblock(is3des, isAutoReturn, masterIndex, workingIndex, cardNo, pinLength, timeout);
			else
				return xydPinpad.inputPinblock(is3des, isAutoReturn, masterIndex, workingIndex, cardNo, pinLength, timeout);
		}catch (Exception e) {
			throw e;
		}
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
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				return kmyPinpad.encrypt(is3des, algo, masterIndex, workingIndex, data, dataLength);
			else
				return xydPinpad.encrypt(is3des, algo, masterIndex, workingIndex, data, dataLength);
		}catch (Exception e) {
			throw e;
		}
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
		if (kmyPinpad == null && xydPinpad == null)
			throw new Exception("Pinpad对象为null");
		try {
			if (kmyPinpad != null)
				return kmyPinpad.mac(is3des, masterIndex, workingIndex, data, dataLength);
			else
				return xydPinpad.mac(is3des, masterIndex, workingIndex, data, dataLength);
		}catch (Exception e) {
			throw e;
		}
	}
	

	private int toupper(int in) {
		if (in >= 'a' && in <= 'z')
			return in+32;
		return in;
	}
	
	public void twoOne(byte[] in, int offset, int length, byte[] out)
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
	
	public String oneTwo(byte[] in, int offset, int length)
	{
		String twoString = "";
		for (int m=0; m<length; m++) {
			twoString += Integer.toHexString((in[m + offset]&0x000000ff)|0xffffff00).substring(6);
		}
		return twoString;
	}
}
