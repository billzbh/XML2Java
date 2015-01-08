package com.hxsmart.imateinterface.fingerprint;

public interface FingerprintInterface
{
	
	/**
	 * 登记3次指纹生成模板 (hexString)
	 * @throws  Exception	
	 */	
	public String GenerateFingerTemplate() throws Exception;
	
	/**
	 * 取消指纹仪的等待...
	 */
	public void cancel();
	
	/**
	 * 读取扩展的指纹特征(256字节)，目前支持FINGERPRINT_MODEL_ZHONGZHENG
	 * @return 
	 * @throws  Exception	 
	 */
	public String fingerExpInfo() throws Exception;
	
	/**
	 * 指纹模块上电 (缺省波特率连接，9600bps，COMM_NONE）
	 * @throws  Exception	 
	 */
	public void powerOn() throws Exception;
	
	
	/**
	 * 指纹模块下电
	 * @throws  Exception
	 * 			iMate通讯超时	 
	 */
	public void powerOff() throws Exception;
	
	/**
	 * 获取指纹模块的版本号信息
	 * @return	成功将返回指纹模块的版本号信息,详细格式请参考厂家文档。
	 * @throws  Exception 
	 */
	public String getVersion() throws Exception;
	
	/**
	 * 采集指纹特征值
	 * @throws  Exception	
	 */	
	public String takeFingerprintFeature() throws Exception;
}