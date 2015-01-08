package com.hxsmart.imateinterface.pbocsdk;

public class HxTermAid {
    public String aid;						// AID, length <= 16*2
    public int ASI;							// 应用选择指示器, 0:部分名字匹配，1:全部名字匹配
    public int OnlinePinSupport = 1;		// 1:该Aid支持联机密码 0:该Aid不支持联机密码, -1表示无
    
    public HxTermAid(String aidString, int ASI) {
    	this.aid = aidString;
    	this.ASI = ASI;
    }
    
    public HxTermAid(byte[] aidBytes, int ASI) {
    	this.aid = "";
		for (int i = 0; i < aidBytes.length; i++) {
			this.aid += Integer.toHexString((aidBytes[i] & 0x000000ff) | 0xffffff00).substring(6);
		}
    	this.ASI = ASI;
    }
}
