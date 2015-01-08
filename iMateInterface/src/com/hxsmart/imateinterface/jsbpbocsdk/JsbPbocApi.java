package com.hxsmart.imateinterface.jsbpbocsdk;

import com.hxsmart.imateinterface.BluetoothThread;

public class JsbPbocApi {
	
	//	����ARQC
	//	in	:	txData				: ��������, ��ǩ�ӳ������ݸ�ʽ
	//			appData				: ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
	//	out :	arqc				: ���ص�ARQC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
	//			arqcLen				: ���ص�ARQC����
	//	ret :	0					: �ɹ�
	//			< 0					: ʧ��
	public native int ICC_GenARQC(String comNo, int termType, char bpNo, int icFlag, String txData, String appData, byte[] arqc, Integer arqcLen);
	
	//	������Ϣ
	//	in	:	tagList				: ��ʾ��Ҫ���ص��û����ݵı�ǩ�б�
	//	out :	userInfo			: ���ص��û�����, ���������ٱ���1024�ֽ�
	//			userInfoLen			: ���ص��û����ݳ���
	//			appData				: IC������, ����Tag8C��Tag8D��, ��������55��, ���������ٱ���4096�ֽ�
	//			icType				: �ϵ�ɹ��Ŀ�����, 1:�Ӵ��� 2:�ǽӿ�
	//								       ���ں���ICC_GenARQC()��ICC_CtlScriptData()����
	//	ret	:	0					: �ɹ�
	//			< 0					: ʧ��
	public native int ICC_GetIcInfo(String comNo, int termType, char bpNo, int icFlag, String tagList, byte[] userInfo, Integer userInfoLen, byte[] appData, Integer appDataLen, Integer icType);

	//	��ȡ��ƬPboc�汾��, ������ICC_GetIcInfo֮��ִ��
	//	ret	:	!= null				: �ɹ�, "0002"��ʶΪPBOC2.0����"0003"��ʶΪPBOC3.0��
	//			null				: ʧ��
	public native String ICC_GetPbocVersion();

	
	//	��ȡ������ϸ
	//	out	:	txDetail			: ������ϸ, ��ʽΪ
	//  							       ��ϸ����(2�ֽ�ʮ����)+ÿ����ϸ�ĳ���(3�ֽ�ʮ����) + ��ϸ1+��ϸ2+...
	//			txDetailLen			: ������ϸ����
	//	ret	:	0					: �ɹ�
	//			< 0					: ʧ��
	// 	Note: ���10����¼
	public native int ICC_GetTranDetail(String comNo, int termType, char bpNo, int icFlag, byte[] txDetail, Integer txDetailLen);
	
	//	���׽������
	//	in	:	pszTxData			: ��������, ��ǩ�ӳ������ݸ�ʽ
	//			pszAppData			: ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
	//   		pszARPC				: ����������, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
	//	out : 	pnTCLen				: ���ص�TC����
	//			pszTC		        : ���ص�TC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
	//	ret : 	0   			    : �ɹ�
	//  		<0		            : ʧ��
	public native int ICC_CtlScriptData(String comNo, int termType, char bpNo, int icFlag, String txData, String appData, String arpc,  byte[] tc, Integer tcLen,byte[] scriptResult, Integer scriptResultLen);
	
	
	//	���EmvCore�����룬���ڵ���
	//	ret :	0					: �������ֵ
	public native int ICC_GetCoreRetCode();

	/**
	 * �������ݽ����ӿ�
	 * @param dataIn	�������ݻ���
	 * @param inlength	�������ݳ���	
	 * @param apduOut	������ݻ���
	 * @return	0		ʧ��
	 * 			����		�ɹ����������ݳ���
	 */
	public int sendReceiveCallBack(byte[] dataIn, int inlength, byte[] dataOut, int timeOut) {	
	    return BluetoothThread.getInstance().sendReceive(dataIn, inlength, dataOut, timeOut);
	}
	
	static {
		System.loadLibrary("jsbpbocapi");
	}
}