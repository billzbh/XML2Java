package com.hxsmart.imateinterface.pbocissuecard;

import com.hxsmart.imateinterface.BluetoothThread;
import com.hxsmart.imateinterface.ApduExchangeData;

import android.os.Handler;
import android.os.Message;

public class PbocIssueCard {
	public int 		appVer = 0x20;			// 0x20,0x30分别表示2.0或3.0的应用
	public int  	kmcIndex = 1;
	public int  	appKeyBaseIndex = 3;
	public int  	issuerRsaKeyIndex = 2;
	
	public String 	panString;				// 卡号12-19, 需以88888开头
	public int  	panSerialNo = 1;		// 0-99, -1表示无
	public String 	expireDateString;		// YYMMDD 6
	public String 	holderNameString;		// Len>=2 <45
	public int  	holderIdType = 0;		// 0:身份证 1:军官证 2:护照 3:入境证 4:临时身份证 5:其它
	public String 	holderIdString; 		// <40
	
	public String 	defaultPinString;		// 4-12
	public String 	aidString;				// OneTwo之后的,32
	public String 	labelString;			// 1-16
	public int  	caIndex = 1;			// 0-255
	public int  	icRsaKeyLen = 128;		// 64-192
	public int 		icRsaE = 3;				// 3 or 65537
	public int  	countryCode = 156;		// 1-999
	public int  	currencyCode = 156;		// 1-999
	
	private String errorString;				// 发卡出错信息
	private Handler statusHandler;			// 发卡过程信息输出处理

	/**
	 * Pboc发卡接口初始化, 需要在发卡之前调用，多次发卡可只调用一次
	 * return 	0		初始化成功
	 * 			其它	初始化失败
	 */
	public int pbocIssueInit()
	{
		return issSetEnv();
	}
	
	/**
	 * Pboc发卡演示接口
	 * @param timeout	等待卡片插入的超时时间
	 */	
	public void pbocIssueCard(int timeout) throws Exception
	{
		if (BluetoothThread.getInstance() == null)
			throw new Exception("BluetoothThead.this为空");
		if (BluetoothThread.getInstance().deviceIsWorking() )
			throw new Exception("设备忙");
		if (!BluetoothThread.getInstance().deviceIsConnecting())
			throw new Exception("设备没有连接");
		
		int ret;
		ret = issueCard(timeout);
		if (ret != 0) {
			throw new Exception(errorString);
		}
	}
	
	/**
	 * Pboc卡复位
	 * @param resetData	复位数据缓冲区
	 * @param timeout	等待卡片插入的超时时间	
	 * @return	0		复位失败
	 * 			》0		复位成功，返回卡片复位数据长度
	 */
	public int pbocResetCallBack(byte[] resetData, int timeout) {	
		ApduExchangeData apduData = new ApduExchangeData();
		apduData.reset(); 
		
		int retCode;
		retCode = BluetoothThread.getInstance().pbocReset(apduData, timeout);
		if (retCode == 0) {
			byte[] data = apduData.getResetDataBytes();
			for (int i=0; i<data.length; i++) {
				resetData[i] = data[i];
			}
			return data.length;
		}
		return 0;
	}
	/**
	 * 蓝牙数据交换接口
	 * @param dataIn	输入数据缓冲
	 * @param inlength	输入数据长度	
	 * @param apduOut	输出数据缓冲
	 * @return	0		失败
	 * 			其他		成功，返回数据长度
	 */
	public int sendReceiveCallBack(byte[] dataIn, int inlength, byte[] dataOut, int timeOut) {	
	    return BluetoothThread.getInstance().sendReceive(dataIn, inlength, dataOut, timeOut);
	}

	
	public void showStatusCallBack(String statusString) {
		if (statusHandler != null) {
			Message msg = new Message();
			msg.obj = statusString;
			statusHandler.sendMessage(msg);
		}
	}
	
	public void setAppVer(int appVer) {
		this.appVer = appVer;
	}
	public int getAppVer() {
		return appVer;
	}
	
	public void setKmcIndex(int kmcIndex) {
		this.kmcIndex = kmcIndex;
	}
	public int getKmcIndex() {
		return kmcIndex;
	}
	
	public void setAppKeyBaseIndex(int appKeyBaseIndex) {
		this.appKeyBaseIndex = appKeyBaseIndex;
	}
	public int getAppKeyBaseIndex() {
		return appKeyBaseIndex;
	}
	
	public void setIssuerRsaKeyIndex(int issuerRsaKeyIndex) {
		this.issuerRsaKeyIndex = issuerRsaKeyIndex;
	}
	public int getIssuerRsaKeyIndex() {
		return issuerRsaKeyIndex;
	}
	
	public String getPanString() {
		return panString;
	}
	public void setPanString(String panString) {
		this.panString = panString;
	}
	public int getPanSerialNo() {
		return panSerialNo;
	}
	public void setPanSerialNo(int panSerialNo) {
		this.panSerialNo = panSerialNo;
	}
	public String getExpireDateString() {
		return expireDateString;
	}
	public void setExpireDateString(String expireDateString) {
		this.expireDateString = expireDateString;
	}
	public String getHolderNameString() {
		return holderNameString;
	}
	public void setHolderNameString(String holderNameString) {
		this.holderNameString = holderNameString;
	}
	public int getHolderIdType() {
		return holderIdType;
	}
	public void setHolderIdType(int holderIdType) {
		this.holderIdType = holderIdType;
	}
	public String getHolderIdString() {
		return holderIdString;
	}
	public void setHolderIdString(String holderIdString) {
		this.holderIdString = holderIdString;
	}
	public String getDefaultPinString() {
		return defaultPinString;
	}
	public void setDefaultPinString(String defaultPinString) {
		this.defaultPinString = defaultPinString;
	}
	public String getAidString() {
		return aidString;
	}
	public void setAidString(String aidString) {
		this.aidString = aidString;
	}
	public String getLabelString() {
		return labelString;
	}
	public void setLabelString(String labelString) {
		this.labelString = labelString;
	}
	public int getCaIndex() {
		return caIndex;
	}
	public void setCaIndex(int caIndex) {
		this.caIndex = caIndex;
	}
	public int getIcRsaKeyLen() {
		return icRsaKeyLen;
	}
	public void setIcRsaKeyLen(int icRsaKeyLen) {
		this.icRsaKeyLen = icRsaKeyLen;
	}
	public int getIcRsaE() {
		return icRsaE;
	}
	public void setIcRsaE(int icRsaE) {
		this.icRsaE = icRsaE;
	}
	public int getCountryCode() {
		return countryCode;
	}
	public void setCountryCode(int countryCode) {
		this.countryCode = countryCode;
	}
	public int getCurrencyCode() {
		return currencyCode;
	}
	public void setCurrencyCode(int currencyCode) {
		this.currencyCode = currencyCode;
	}
	public String getErrorString() {
		return errorString;
	}
	public void setErrorString(String errorString) {
		this.errorString = errorString;
	}
	public Handler getStatusHandler() {
		return statusHandler;
	}
	public void setStatusHandler(Handler statusHandler) {
		this.statusHandler = statusHandler;
	}
	
	private native int issSetEnv();
	private native int issueCard(int timeout);
	
	static {
		System.loadLibrary("pbocissuecard");
	}
}
