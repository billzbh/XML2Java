package com.hxsmart.imateinterface.jsbpbocsdk;

import com.hxsmart.imateinterface.BluetoothThread;

public class JsbPbocApi {
	
	//	生成ARQC
	//	in	:	txData				: 交易数据, 标签加长度内容格式
	//			appData				: ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
	//	out :	arqc				: 返回的ARQC, TLV格式, 转换为16进制可读形式
	//			arqcLen				: 返回的ARQC长度
	//	ret :	0					: 成功
	//			< 0					: 失败
	public native int ICC_GenARQC(String comNo, int termType, char bpNo, int icFlag, String txData, String appData, byte[] arqc, Integer arqcLen);
	
	//	读卡信息
	//	in	:	tagList				: 表示需要返回的用户数据的标签列表
	//	out :	userInfo			: 返回的用户数据, 缓冲区至少保留1024字节
	//			userInfoLen			: 返回的用户数据长度
	//			appData				: IC卡数据, 包括Tag8C、Tag8D等, 用于生成55域, 缓冲区至少保留4096字节
	//			icType				: 上电成功的卡类型, 1:接触卡 2:非接卡
	//								       用于函数ICC_GenARQC()和ICC_CtlScriptData()传入
	//	ret	:	0					: 成功
	//			< 0					: 失败
	public native int ICC_GetIcInfo(String comNo, int termType, char bpNo, int icFlag, String tagList, byte[] userInfo, Integer userInfoLen, byte[] appData, Integer appDataLen, Integer icType);

	//	获取卡片Pboc版本号, 必须在ICC_GetIcInfo之后执行
	//	ret	:	!= null				: 成功, "0002"标识为PBOC2.0卡，"0003"标识为PBOC3.0卡
	//			null				: 失败
	public native String ICC_GetPbocVersion();

	
	//	读取交易明细
	//	out	:	txDetail			: 交易明细, 格式为
	//  							       明细条数(2字节十进制)+每条明细的长度(3字节十进制) + 明细1+明细2+...
	//			txDetailLen			: 交易明细长度
	//	ret	:	0					: 成功
	//			< 0					: 失败
	// 	Note: 最多10条记录
	public native int ICC_GetTranDetail(String comNo, int termType, char bpNo, int icFlag, byte[] txDetail, Integer txDetailLen);
	
	//	交易结果处理
	//	in	:	pszTxData			: 交易数据, 标签加长度内容格式
	//			pszAppData			: ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
	//   		pszARPC				: 发卡行数据, TLV格式, 转换为16进制可读形式
	//	out : 	pnTCLen				: 返回的TC长度
	//			pszTC		        : 返回的TC, TLV格式, 转换为16进制可读形式
	//	ret : 	0   			    : 成功
	//  		<0		            : 失败
	public native int ICC_CtlScriptData(String comNo, int termType, char bpNo, int icFlag, String txData, String appData, String arpc,  byte[] tc, Integer tcLen,byte[] scriptResult, Integer scriptResultLen);
	
	
	//	获得EmvCore处理码，用于调试
	//	ret :	0					: 返回码的值
	public native int ICC_GetCoreRetCode();

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
		System.loadLibrary("jsbpbocapi");
	}
}