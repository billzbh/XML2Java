package com.hxsmart.imateinterface;

public class MagCardData {
	private String cardNoString;
	private String track2String;
	private String track3String;
	private String errorString;
	
	public MagCardData() {
		this.cardNoString = null;
		this.track2String = null;
		this.track3String = null;
		this.errorString = null;
	}
	
	public String getCardNoString() {
		return cardNoString;
	}
	public void setCardNoString(String cardNoString) {
		this.cardNoString = cardNoString;
	}
	public String getTrack2String() {
		return track2String;
	}
	public void setTrack2String(String track2String) {
		this.track2String = track2String;
	}
	public String getTrack3String() {
		return track3String;
	}
	public void setTrack3String(String track3String) {
		this.track3String = track3String;
	}
	public String getErrorString() {
		return errorString;
	}
	public void setErrorString(String errorString) {
		this.errorString = errorString;
	}
}
