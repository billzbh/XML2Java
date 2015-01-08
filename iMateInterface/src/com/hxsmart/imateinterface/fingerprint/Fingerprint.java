package com.hxsmart.imateinterface.fingerprint;

public class Fingerprint {	
	public final static int FINGERPRINT_MODEL_JSABC = 0;
	public final static int FINGERPRINT_MODEL_SHENGTENG = 1;
	public final static int FINGERPRINT_MODEL_ZHONGZHENG = 2;

	private FingerprintInterface BaseFingerprint=null; 
	
	
	public Fingerprint()
	{
		//默认是浙江维尔指纹仪
		BaseFingerprint = new FingerprintJSABC();
	}
	
	/**
	 * 设置支持的指纹仪类型，目前支持FINGERPRINT_MODEL_JSABC、FINGERPRINT_MODEL_SHENGTENG、FINGERPRINT_MODEL_ZHONGZHENG
	 * 
	 */
	public void fingerprintSetModel(int theFingerprintModel)
	{
		switch (theFingerprintModel) {
		case FINGERPRINT_MODEL_JSABC:
			BaseFingerprint = new FingerprintJSABC();
			break;
		case FINGERPRINT_MODEL_SHENGTENG:
			BaseFingerprint = new FingerprintSHENGTENG();
			break;
		case FINGERPRINT_MODEL_ZHONGZHENG:
			BaseFingerprint = new FingerprintZhongZheng();
			break;
		}
	}
			
	/**
	 * 指纹模块上电 (缺省波特率连接，9600bps，COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception 
	{
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		
		try {
			BaseFingerprint.powerOn();	
		} 
		catch (Exception e) {
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
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		
		try {
			BaseFingerprint.powerOff();	
		} 
		catch (Exception e) {
			throw e;
			}
	}	
	
	/**
	 * 获取指纹模块的版本号信息
	 * @return	成功将返回指纹模块的版本号信息,详细格式请参考厂家文档。
	 * @throws  Exception 
	 */
	public String getVersion() throws Exception
	{
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		try {
			return BaseFingerprint.getVersion();	
		} 
		catch (Exception e) {
			throw e;
			}
	}
	
	
	/**
	 * 采集指纹特征值
	 * @throws  Exception	
	 */	
	public String takeFingerprintFeature() throws Exception
	{
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		try {
			return BaseFingerprint.takeFingerprintFeature();	
		} 
		catch (Exception e) {
			throw e;
			}
	}
	
	/**
	 * 读取扩展的指纹特征(256字节)，目前支持FINGERPRINT_MODEL_ZHONGZHENG
	 * @return 
	 * @throws  Exception	 
	 */
	public String fingerExpInfo() throws Exception
	{
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		try {
			return BaseFingerprint.fingerExpInfo();
		} 
		catch (Exception e) {
			throw e;
			}
	}
	
	/**
	 * 取消指纹仪的等待...
	 */
	public void cancel()
	{
		if (BaseFingerprint == null)
			return;
		BaseFingerprint.cancel();	
	}
	
	/**
	 * 登记3次指纹生成模板 (hexString)
	 * @throws  Exception	
	 */	
	public String GenerateFingerTemplate() throws Exception
	{
		if (BaseFingerprint == null)
			throw new Exception("Fingerprint对象为空");
		
		try {
			return BaseFingerprint.GenerateFingerTemplate();
		} 
		catch (Exception e) {
			throw e;
			}
	}
	
	
}
