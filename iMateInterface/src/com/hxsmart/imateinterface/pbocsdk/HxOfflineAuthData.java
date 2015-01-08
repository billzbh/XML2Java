package com.hxsmart.imateinterface.pbocsdk;

public class HxOfflineAuthData {
	public int needCheckCrlFlag;	// 是否需要判断发卡行公钥证书为黑名单标志, 0:不需要判断 1:需要判断

	public byte[] ridBytes;         // RID[5], rid+caIndex+certSerNo这三项提供给应用层判断发卡行公钥证书是否在黑名单列表中. 任何一个指针传入为空都表示高层不需要
	public String ridString;        // RID, 内容是ridBytes进行oneTwo后的结果, length = 5 *2
	
	public int caIndex;				// CA公钥索引
	
	public byte[] certSerNoBytes;	// 发卡行公钥证书序列号[3]
	public String certSerNoString;	// 发卡行公钥证书序列号, , 内容是ridBytes进行oneTwo后的结果, length = 3*2
}
