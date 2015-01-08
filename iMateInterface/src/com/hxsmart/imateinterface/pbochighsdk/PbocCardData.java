package com.hxsmart.imateinterface.pbochighsdk;

public class PbocCardData {
	public String field55;		// 组装好的55域内容, 十六进制可读格式, 预留513字节长度 
	public String pan;          // 主账号[19], 可读格式
	public int	panSeqNo;       // 主账号序列号, 0-99, -1表示不存在
	public String track2;       // 二磁道等效数据[37], 长度为0表示不存在
	public String extInfo;      // 其它数据, 保留
}
