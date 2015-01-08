package com.hxsmart.imateinterface.pbocsdk;

public class HxCaPublicKey {
	public String key;        		// 公钥模数, 内容是keyBytes进行oneTwo后的结果,length <= 248 * 2	
	public int	lE;                	// 公钥指数，3或65537
	public String rid;         		// 该公钥所属的RID, length = 5 * 2
	public int index;         		// 公钥索引
	
	public HxCaPublicKey(int index, String ridString, String keyString, int lE) {
		this.key = keyString;
		this.lE = lE;
		this.rid = ridString;
		this.index = index;
	}
	
	public HxCaPublicKey(int index, byte[] ridBytes, byte[] keyBytes, int lE) {
		this.key = "";
		for (int i = 0; i < keyBytes.length; i++) {
			this.key += Integer.toHexString((keyBytes[i] & 0x000000ff) | 0xffffff00).substring(6);
		}
		this.lE = lE;
		this.rid = "";
		for (int i = 0; i < ridBytes.length; i++) {
			this.key += Integer.toHexString((ridBytes[i] & 0x000000ff) | 0xffffff00).substring(6);
		}
		this.index = index;
	}
}
