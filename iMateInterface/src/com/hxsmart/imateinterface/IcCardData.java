package com.hxsmart.imateinterface;

public class IcCardData {
	private String cardNoString;
	private String panSequenceNoString;
	private String holderNameString;
	private String holderIdString;
	private String expireDateString;
	private String track2String;
	private String errorString;
	
	public IcCardData() {
		this.cardNoString = null;
		this.panSequenceNoString = null;
		this.holderNameString = null;
		this.holderIdString = null;
		this.expireDateString = null;
		this.track2String = null;
		this.errorString = null;
	}

	public String getCardNoString() {
		return cardNoString;
	}

	public void setCardNoString(String cardNoString) {
		this.cardNoString = cardNoString;
	}
	
	public String getPanSequenceNoString() {
		return panSequenceNoString;
	}

	public void setPanSequenceNoString(String panSequenceNoString) {
		this.panSequenceNoString = panSequenceNoString;
	}

	public String getHolderNameString() {
		return holderNameString;
	}

	public void setHolderNameString(String holderNameString) {
		this.holderNameString = holderNameString;
	}

	public String getHolderIdString() {
		return holderIdString;
	}

	public void setHolderIdString(String holderIdString) {
		this.holderIdString = holderIdString;
	}

	public String getExpireDateString() {
		return expireDateString;
	}

	public void setExpireDateString(String expireDateString) {
		this.expireDateString = expireDateString;
	}

	public String getTrack2String() {
		return track2String;
	}

	public void setTrack2String(String track2String) {
		this.track2String = track2String;
	}
	
	public String getErrorString() {
		return errorString;
	}

	public void setErrorString(String errorString) {
		this.errorString = errorString;
	}
}
