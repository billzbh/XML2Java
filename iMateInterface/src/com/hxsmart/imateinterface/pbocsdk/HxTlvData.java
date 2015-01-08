package com.hxsmart.imateinterface.pbocsdk;

public class HxTlvData {
	public byte[] tlvDataBytes;		//原始格式, 包括Tag、Length、Value
	public String tlvDataString;	//原始格式, 内容是tlvDataBytes进行oneTwo后的结果，包括Tag、Length、Value

	public int tag;					//数据标签, 例:0x9F79:电子现金余额
	
	public byte[] dataBytes;		//读出的数据(N数字型:转换成可读数字串, CN压缩数字型:去除尾部'F'后的数字串, A|AN|ANS|B|未知类型:原样返回),除B型外返回值后面会强制加一个结尾'\0', 
									//该结束符不包括在返回的长度之内。注意, 类似N3这样的数据, 返回的内容长度为N4
	public String dataString;		//读出的数据, 内容是dataBytes进行oneTwo后的结果									
}
