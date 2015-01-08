package com.hxsmart.imateinterface;

public class ApduExchangeData {
	public final static int USER_SLOT = 0; 			//用户卡座
	public final static int MIF_SLOT = 1; 			//射频卡读卡器
	
	public final static int SAM1_SLOT = 4; 			//第一SAM卡座
	public final static int SAM2_SLOT = 5; 			//第二SAM卡座
	
	public final static int NORMAL_CARD_TYPE = 0;	//普通IC卡类型
	public final static int PBOC_CARD_TYPE = 1;		//PBOC IC卡类型
	
	private int slot = USER_SLOT; //卡座号，USER_SLOT，SAM1_SLOT，SAM2_SLOT; default : USER_SLOT
	private byte[] auduInBytes;
	private int inLength;
	private byte[] auduOutBytes;
	private int outLength;
	private byte[] status;
	private byte[] resetDataBytes;
	private String errorString;
	private int cardType = PBOC_CARD_TYPE; //IC卡类型：NORMAL_CARD_TYPE，PBOC_CARD_TYPE; default : PBOC_CARD_TYPE
	
	public void reset() {
		this.slot = USER_SLOT; 
		this.auduInBytes = null;
		this.inLength = 0;
		this.auduOutBytes = null;
		this.outLength = 0;
		this.status = null;
		this.resetDataBytes = null;
		this.errorString = null;
		this.cardType = PBOC_CARD_TYPE;
	}
	
	public int getSlot() {
		return slot;
	}
	
	public byte[] getAuduInBytes() {
		return auduInBytes;
	}
	public void setAuduInBytes(byte[] auduInBytes) {
		this.auduInBytes = auduInBytes;
	}
	public byte[] getAuduOutBytes() {
		return auduOutBytes;
	}
	public void setAuduOutBytes(byte[] auduOutBytes) {
		this.auduOutBytes = auduOutBytes;
	}
	public byte[] getStatus() {
		return status;
	}
	public void setStatus(byte[] status) {
		this.status = status;
	}
	public int getInLength() {
		return inLength;
	}
	public int getCardType() {
		return cardType;
	}
	
	public void setSlot(int slot) {
		this.slot = slot;
	}
	
	public void setInLength(int inLength) {
		this.inLength = inLength;
	}
	public int getOutLength() {
		return outLength;
	}
	public void setOutLength(int outLength) {
		this.outLength = outLength;
	}
	public String getErrorString() {
		return errorString;
	}
	public void setErrorString(String errorString) {
		this.errorString = errorString;
	}
	public byte[] getResetDataBytes() {
		return resetDataBytes;
	}
	public void setResetDataBytes(byte[] resetDataBytes) {
		this.resetDataBytes = resetDataBytes;
	}
	public void setCardType(int cardType) {
		this.cardType = cardType;
	}
}
