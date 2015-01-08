package com.hxsmart.imateinterface.pbocsdk;

public class HxAdfInfo {
	public byte[] adfNameBytes;				// ADF名字, length <= 16, null无效 
	public String adfNameString;            // ADF名字, 内容是adfNameBytes进行oneTwo后的结果，length <= 16*2, null或为空则无效
	
	public String label;               		// 应用标签, length <= 16
	public int	priority;                   // 应用优先级, -1表示该应用没有提供
	public String language;              	// 语言指示, 应用如没提供则置为空串, length <= 8;
	public int	issuerCodeTableIndex;       // 字符代码表, -1表示该应用没有提供
	public byte[] preferredName;        	// 应用首选名称, 应用如没提供则置为空串,length <= 16
}
