package com.hxsmart.imateinterface.pbochighsdk;

import java.security.PublicKey;

import com.hxsmart.imateinterface.BluetoothThread;

public class PbocHighApi {
	
	//EMV客户高层接口返回码
	public static final int HXPBOC_HIGH_OK			=	0;  // OK
	public static final int HXPBOC_HIGH_PARA		=	1;  // 参数错误
	public static final int HXPBOC_HIGH_NO_CARD		=	2;  // 无卡
	public static final int HXPBOC_HIGH_CARD_IO		=	4;  // 卡操作错
	public static final int HXPBOC_HIGH_CARD_SW		=	5;  // 非法卡指令状态字
	public static final int HXPBOC_HIGH_DENIAL		=	6;  // 交易被拒绝
	public static final int HXPBOC_HIGH_TERMINATE	=	7;  // 交易被终止
	public static final int HXPBOC_HIGH_OTHER		=	8;  // 其它错误
	
	{
		vHxPbocHighSetCardReaderType(0);
	}
	
	// 核心初始化
	// in  : merchantId      : 商户号[15]
	//		 termId          : 终端号[8]
	//		 merchantName    : 商户名字[40]
	//		 countryCode     : 终端国家代码, 1-999, 中国：156
	//		 currencyCode    : 交易货币代码, 1-999，人民币：156
	public native int iHxPbocHighInitCore(String merchantId, String termId, String merchantName, int countryCode, int currencyCode);

	// 设置读卡器类型, 在交易初始化之前设置
	// in  : cardReaderType  : IC读卡器类型，0：芯片读卡器；1：射频读卡器
	public native void vHxPbocHighSetCardReaderType(int cardReaderType);

	
	// 交易初始化
	// in  : dateTime     : 交易日期时间[14], YYYYMMDDhhmmss
	//       atc          : 终端交易流水号, 1-999999
	//       transType    : 交易类型, 0-256
	//       amount       : 交易金额, 分为单位
	// out : cardData     : 交易初始化返回的数据对象
	public native int iHxPbocHighInitTrans(String dateTime, int atc, int transType, int amount, PbocCardData cardData);
	
	// 完成交易
	// in  : issuerData     : 后台数据, 十六进制可读格式
	// out : field55        : 完成交易后返回的数据。组装好的55域内容, 十六进制可读格式，需要预分配空间，max = 513
	//		 dataLen        : 完成交易后返回的数据长度，需要预先创建对象
	// Note: 除了返回HXPBOC_HIGH_OK外, 返回HXPBOC_HIGH_DENIAL也会返回脚本结果
	public native int iHxPbocHighDoTrans(String issuerData, byte[] field55, Integer dataLen);

	// 读取卡的扩展信息
	// in  : infoType       :  指定info输出的格式，1表示XML，0表示 逗号隔开
	// out : outData        :  由这个参数带出数据，必须先new一个byte[] 数组，目前大小至少设置为1024.
	public native int iHxPbocHighReadInfoEx(byte[] outData,int infoType);
	
	// 获取TAG的值
	// in  : tag       		:  tag
	// ret ：tag值
	public native String szHxPbocHighGetTagValue(String tag);
	
	/**
	 * 蓝牙数据交换接口
	 * @param dataIn	输入数据缓冲
	 * @param inlength	输入数据长度	
	 * @param apduOut	输出数据缓冲
	 * @return	0		失败
	 * 			其他		成功，返回数据长度
	 */
	public int sendReceiveCallBack(byte[] dataIn, int inlength, byte[] dataOut, int timeOut) {	
	    return BluetoothThread.getInstance().sendReceive(dataIn, inlength, dataOut, timeOut);
	}
	
	static {
		System.loadLibrary("pbochighapi");
	}
}