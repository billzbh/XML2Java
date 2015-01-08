package com.hxsmart.imateinterface;

public class IdInformationData 
{
	private String nameString;
	private String sexString;
	private String nationString;
	private String birthdayYearString;
	private String birthdayMonthString;
	private String birthdayDayString;
	private String addressString;
	private String idNumberString;
	private String issuerString;
	private String validDateString;
	private String otherString;
	private byte[] pictureData;
	private String errorString;
	
	public IdInformationData() {
		this.nameString = null;
		this.sexString = null;
		this.nationString = null;
		this.birthdayYearString = null;
		this.birthdayMonthString = null;
		this.birthdayDayString = null;
		this.addressString = null;
		this.idNumberString = null;
		this.issuerString = null;
		this.validDateString = null;
		this.otherString = null;
		this.pictureData = null;
		this.errorString = null;
	}
	
	public String getNameString() {
		return nameString;
	}
	public void setNameString(String nameString) {
		this.nameString = nameString;
	}
	public String getSexString() {
		return sexString;
	}
	public void setSexString(String sexString) {
		this.sexString = sexString;
	}
	public String getNationString() {
		return nationString;
	}
	public void setNationString(String nationString) {
		this.nationString = nationString;
	}
	public String getBirthdayYearString() {
		return birthdayYearString;
	}
	public void setBirthdayYearString(String birthdayYearString) {
		this.birthdayYearString = birthdayYearString;
	}
	public String getBirthdayMonthString() {
		return birthdayMonthString;
	}
	public void setBirthdayMonthString(String birthdayMonthString) {
		this.birthdayMonthString = birthdayMonthString;
	}
	public String getBirthdayDayString() {
		return birthdayDayString;
	}
	public void setBirthdayDayString(String birthdayDayString) {
		this.birthdayDayString = birthdayDayString;
	}
	public String getAddressString() {
		return addressString;
	}
	public void setAddressString(String addressString) {
		this.addressString = addressString;
	}
	public String getIdNumberString() {
		return idNumberString;
	}
	public void setIdNumberString(String idNumberString) {
		this.idNumberString = idNumberString;
	}
	public String getIssuerString() {
		return issuerString;
	}
	public void setIssuerString(String issuerString) {
		this.issuerString = issuerString;
	}
	public String getValidDateString() {
		return validDateString;
	}
	public void setValidDateString(String validDateString) {
		this.validDateString = validDateString;
	}
	public String getOtherString() {
		return otherString;
	}
	public void setOtherString(String otherString) {
		this.otherString = otherString;
	}
	public byte[] getPictureData() {
		return pictureData;
	}
	public void setPictureData(byte[] pictureData) {
		this.pictureData = pictureData;
	}
	public String getErrorString() {
		return errorString;
	}
	public void setErrorString(String errorString) {
		this.errorString = errorString;
	}
}
