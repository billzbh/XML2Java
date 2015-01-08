package com.hxsmart.imateinterface;

import com.hxsmart.imateinterface.pbocsdk.*;

import android.os.Handler;
import android.os.Message;

public class PbocApiDemo {
	private Handler logHandler;
	private static PbocApi pbocApi;
	
	
	public PbocApiDemo() {
		pbocApi = new PbocApi();	
	}
	
	private String errorMessage(int retCode) {
		switch (retCode) {
		case PbocApi.HXEMV_OK:
			return null;
		case PbocApi.HXEMV_NA:
			return "不可用";
		case PbocApi.HXEMV_PARA:
			return "参数错误";
		case PbocApi.HXEMV_LACK_MEMORY:
			return "存储空间不足";
		case PbocApi.HXEMV_CORE:
			return "内部错误";
		case PbocApi.HXEMV_NO_SLOT:
			return "不支持的卡座";
		case PbocApi.HXEMV_NO_CARD:
			return "卡片不存在";
		case PbocApi.HXEMV_CANCEL:
			return "用户取消";
		case PbocApi.HXEMV_TIMEOUT:
			return "超时";
		case PbocApi.HXEMV_NO_APP:
			return "无支持的应用";
		case PbocApi.HXEMV_AUTO_SELECT:
			return "获取的应用可自动选择";
		case PbocApi.HXEMV_CARD_REMOVED:
			return "卡被取走";
		case PbocApi.HXEMV_CARD_OP:
			return "卡操作错";
		case PbocApi.HXEMV_CARD_SW:
			return "非法卡指令状态字";
		case PbocApi.HXEMV_NO_DATA:
			return "无数据";
		case PbocApi.HXEMV_NO_RECORD:
			return "无记录";
		case PbocApi.HXEMV_NO_LOG:
			return "卡片不支持交易流水记录";
		case PbocApi.HXEMV_TERMINATE:
			return "满足拒绝条件，交易终止";
		case PbocApi.HXEMV_USE_MAG:
			return "请使用磁条卡";
		case PbocApi.HXEMV_RESELECT:
			return "需要重新选择应用";
		case PbocApi.HXEMV_NOT_SUPPORTED:
			return "不支持";
		case PbocApi.HXEMV_DENIAL:
			return "交易拒绝";
		case PbocApi.HXEMV_DENIAL_ADVICE:
			return "交易拒绝, 有Advice";
		case PbocApi.HXEMV_NOT_ALLOWED:
			return "服务不允许";
		case PbocApi.HXEMV_TRANS_NOT_ALLOWED:
			return "交易不允许";
		case PbocApi.HXEMV_FLOW_ERROR:
			return "EMV流程错误";
		case PbocApi.HXEMV_CALLBACK_METHOD:
			return "回调与非回调核心接口调用错误";
		case PbocApi.HXEMV_NOT_ACCEPTED:
			return "不接受";
		}
		return "其它错误";
	}
	
	public void doPbocDemoInit() throws Exception {	
		// 初始化核心
		int ret = pbocApi.iHxEmvInit();
		if (ret != PbocApi.HXEMV_OK) {
			throw new Exception("初始化核心失败: " + errorMessage(ret));
		}
		
		HxEmvInfo emvInfo = new HxEmvInfo();
		ret = pbocApi.iHxEmvInfo(emvInfo);
		if (ret != PbocApi.HXEMV_OK) {
			throw new Exception("获取核心信息失败: " + errorMessage(ret));
		}
		
		showLog("核心名字  : "+ emvInfo.coreName);
		showLog("核心描述  : "+ emvInfo.coreDesc);
		showLog("核心版本  : "+ emvInfo.coreVer);
		showLog("核心日期  : "+ emvInfo.releaseDate);
		
		showLog("获取核心信息，成功");
		
		/*
		[TERM_PARAM]
		TermId              = 12345678
		MerchantId          = 123456789000001
		MerchantName        = 福田泰然212栋401
		# TermType用16进制表示
		TermType            = 22
		TermCatability      = A0E9C8
		TermCapabilityExt   = EF80F03000
		TermCountryCode     = 156
		AcquirerId          = 99999999999

		TermAppVer          = 0030
		FloorLimit          = 200000
		MaxTargetPercentage = 90
		TargetPercentage    = 40
		ThresholdValue      = 50000
		TacDefault          = FC70F8D800
		TacDenial           = 0000B80000
		TacOnline           = FFFF44FF00
		ECashSupport        = 1
		ECashTransLimit     = 50000
		 */
		
		HxTermParam termParam = new HxTermParam();	
		termParam.termType = 0x22;
		termParam.termCapability = "A0E9C8";
		termParam.additionalTermCapability = "EF80F03000";
		termParam.merchantId = "123456789000001";
		termParam.termId = "12345678";
		termParam.merchantNameLocation = "福田泰然212栋401";
		termParam.termCountryCode = 156;
		termParam.acquirerId = "99999999999";
		
		termParam.termAppVer = "0030";
		termParam.floorLimit = 200000;
		termParam.maxTargetPercentage = 90;
		termParam.targetPercentage = 40;
		termParam.thresholdValue = 50000;
		termParam.ecashSupport = 1;
		termParam.termECashTransLimit = "50000";
		termParam.tacDefault = "FC70F8D800";
		termParam.tacDenial = "0000B80000";
		termParam.tacOnline = "FFFF44FF00";
		
		ret = pbocApi.iHxEmvSetParam(termParam);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("设置终端参数失败：" + errorMessage(ret));
		showLog("设置终端参数，成功");

		HxTermAid termAids[] = new HxTermAid[9];
		termAids[0]=new HxTermAid("A000000333", 0);
		termAids[1]=new HxTermAid("A000000333010101", 1);
		termAids[2]=new HxTermAid("A000000333010102", 1);
		termAids[3]=new HxTermAid("A000000333010103", 1);
		termAids[4]=new HxTermAid("A000000333010104", 1);
		termAids[5]=new HxTermAid("A000000333010105", 1);
		termAids[6]=new HxTermAid("A000000333010106", 1);
		termAids[7]=new HxTermAid("A000000333010107", 1);
		termAids[8]=new HxTermAid("A000000333010108", 1);
		
		ret = pbocApi.iHxEmvLoadAid(termAids);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("装载终端支持的Aid失败：" + errorMessage(ret));
		showLog("装载终端支持的Aid，成功");
				
		HxCaPublicKey caPublicKey[] = new HxCaPublicKey[4];
		
		caPublicKey[0] = new HxCaPublicKey(0xF8, "A000000333", "ABFBB295F61AFAD608DB342756E1ECDB30C98E3A50189F308FBAB903E3F8E7E574CCE93FE80D619732F32E81262AD084F24540718F28AFB60C367888A2706C9968539B5F673A0A0CED46E1AB2B9FEAAA5D30945E5CC1E5C5D19C3A6B26198750756A7F4BEEDCA4B9982103001E87C05679A5D47D2F35F7E50B064244C23311116034C56BCF0E40F82425A6ED4AC71E97E2AAF8F45F543117A9B470FC3FAD67BF4F3091349414FC7E88BADC9CC045F50C82C3BE5A4E16939810174CC495D8DC23", 3);;
		caPublicKey[1] = new HxCaPublicKey(0x01, "A000000333", "ABFBB295F61AFAD608DB342756E1ECDB30C98E3A50189F308FBAB903E3F8E7E574CCE93FE80D619732F32E81262AD084F24540718F28AFB60C367888A2706C9968539B5F673A0A0CED46E1AB2B9FEAAA5D30945E5CC1E5C5D19C3A6B26198750756A7F4BEEDCA4B9982103001E87C05679A5D47D2F35F7E50B064244C23311116034C56BCF0E40F82425A6ED4AC71E97E2AAF8F45F543117A9B470FC3FAD67BF4F3091349414FC7E88BADC9CC045F50C82C3BE5A4E16939810174CC495D8DC23", 3);;
		caPublicKey[2] = new HxCaPublicKey(0x08, "A000000333", "B61645EDFD5498FB246444037A0FA18C0F101EBD8EFA54573CE6E6A7FBF63ED21D66340852B0211CF5EEF6A1CD989F66AF21A8EB19DBD8DBC3706D135363A0D683D046304F5A836BC1BC632821AFE7A2F75DA3C50AC74C545A754562204137169663CFCC0B06E67E2109EBA41BC67FF20CC8AC80D7B6EE1A95465B3B2657533EA56D92D539E5064360EA4850FED2D1BF", 3);;
		caPublicKey[3] = new HxCaPublicKey(0x09, "A000000333", "EB374DFC5A96B71D2863875EDA2EAFB96B1B439D3ECE0B1826A2672EEEFA7990286776F8BD989A15141A75C384DFC14FEF9243AAB32707659BE9E4797A247C2F0B6D99372F384AF62FE23BC54BCDC57A9ACD1D5585C303F201EF4E8B806AFB809DB1A3DB1CD112AC884F164A67B99C7D6E5A8A6DF1D3CAE6D7ED3D5BE725B2DE4ADE23FA679BF4EB15A93D8A6E29C7FFA1A70DE2E54F593D908A3BF9EBBD760BBFDC8DB8B54497E6C5BE0E4A4DAC29E5", 3);

		ret = pbocApi.iHxEmvLoadCaPublicKey(caPublicKey);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("装载CA公钥失败：" + errorMessage(ret));
		showLog("装载CA公钥，成功");
	}
	public void doPbocDemoTrans_1() throws Exception {		
		// 交易初始化
		int ret = pbocApi.iHxEmvTransInit(0);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("交易初始化：" + errorMessage(ret));
		showLog("交易初始化，成功");
				
		// 读取支持的应用
		HxAdfInfo adfInfo[] = new HxAdfInfo[10]; //本例子中最大支持的应用为10
		for (int i = 0; i< 10; i++)
			adfInfo[i] = new HxAdfInfo(); 
		ret = pbocApi.iHxEmvGetSupportedApp(1, adfInfo);
		if (ret != PbocApi.HXEMV_OK &&  ret != PbocApi.HXEMV_AUTO_SELECT)
			throw new Exception("读取支持的应用：" + errorMessage(ret));
		for (int i = 0; i < 10; i++) {
			if (adfInfo[i].adfNameString == null) 
				break;
			showLog("adfInfo[" + i +"].adfNameString = " + adfInfo[i].adfNameString);
			showLog("adfInfo[" + i +"].label = " + adfInfo[i].label);
			showLog("adfInfo[" + i +"].priority = " + adfInfo[i].priority);
			showLog("adfInfo[" + i +"].language = " + adfInfo[i].language);
			showLog("adfInfo[" + i +"].issuerCodeTableIndex = " + adfInfo[i].issuerCodeTableIndex);
			//showLog("adfInfo[" + i +"].preferredName = " + new String(adfInfo[i].preferredName));
		}
		showLog("读取支持的应用，成功");
		
		// 应用选择
		// 注意, 演示程序不管是否设置为自动选择, 总是自动选择第1个应用
		ret = pbocApi.iHxEmvAppSelect(1, adfInfo[0].adfNameString);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("应用选择：" + errorMessage(ret));
		showLog("应用选择，成功");
		
		// 选择应用以后，可以读卡日志
		Integer maxRecNum = 0;
		ret = pbocApi.iHxEmvGetLogInfo(0/*交易流水*/, maxRecNum);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("读卡片交易流水支持信息：" + errorMessage(ret));
		showLog("读卡片交易流水支持信息: " + maxRecNum);
		showLog("读卡片交易流水支持信息，成功");
		
		byte[] cardLog = new byte[100];
		Integer logLen = 0;
		int max = maxRecNum;
		for (int i = 0; i < max; i++) {
			ret = pbocApi.iHxEmvReadLog(0/*交易流水*/, i + 1, cardLog, logLen);
			if (ret != PbocApi.HXEMV_OK)
				break;
			String logString = "";
			for (int m=0; m<logLen; m++) {
				logString += Integer.toHexString((cardLog[m]&0x000000ff)|0xffffff00).substring(6);
			}
			showLog("日志内容(" + logLen +") : " + logString);
		}
		
		// GPO
		ret = pbocApi.iHxEmvGPO("20140327110723", 1/*ttc*/, PbocApi.TR_SALE, "5000000", 156);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("GPO：" + errorMessage(ret));
		showLog("GPO，成功");
		
		/*
		// 重新设置金额
		ret = pbocApi.iHxEmvSetAmount("1234500");
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("iHxEmvSetAmount：" + errorMessage(ret));
		showLog("传入或重新传入金额，成功");
		 */
		
		// 读应用记录
		ret = pbocApi.iHxEmvReadRecord();
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("GPO：" + errorMessage(ret));
		showLog("读应用记录，成功");

		// 读取应用数据(此时可以读卡号)
		HxTlvData outTlvData = new HxTlvData();
		ret = pbocApi.iHxEmvGetData(0x5a, outTlvData);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("读取应用数据：" + errorMessage(ret));
		showLog("卡号:" + new String(outTlvData.dataBytes));
		showLog("读取应用数据，成功");
		
		// 设置卡片黑名单及分开消费标志
		ret = pbocApi.iHxEmvSetPanFlag(0/*0:不是黑名单*/, 0/*0:分开消费没超限*/);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("设置卡片黑名单及分开消费标志：" + errorMessage(ret));
		showLog("设置卡片黑名单及分开消费标志，成功");
		
		// 脱机数据认证（SDA或DDA）
		HxOfflineAuthData offlineAuthData = new HxOfflineAuthData();
		ret = pbocApi.iHxEmvOfflineDataAuth(offlineAuthData);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("脱机数据认证（SDA或DDA）：" + errorMessage(ret));
		showLog("脱机数据认证（SDA或DDA），成功");
		
		// 测试程序, 不做CRL检查
		ret = pbocApi.iHxEmvSetIssuerCertCrl(0/*0:不再CRL列表中*/);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("设置发卡行公钥证书为黑名单：" + errorMessage(ret));
		showLog("设置发卡行公钥证书为黑名单，成功");
		
		// 处理限制
		ret = pbocApi.iHxEmvProcRistrictions();
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("处理限制：" + errorMessage(ret));
		showLog("处理限制，成功");
		
		// 终端风险管理
		ret = pbocApi.iHxEmvTermRiskManage();
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("终端风险管理：" + errorMessage(ret));
		showLog("终端风险管理，成功");
		
		// 获取持卡人验证
		// iCvm : HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
		Integer cvm = 0;
		Boolean bypassFlag = false;		
		ret = pbocApi.iHxEmvGetCvmMethod(cvm, bypassFlag);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("获取持卡人验证：" + errorMessage(ret));
		showLog("获取持卡人验证，成功");
		
		if (ret != PbocApi.HXEMV_NO_DATA) {
			byte[] vmData = new byte[6];
			vmData[0] = '1';
			vmData[1] = '2';
			vmData[2] = '3';
			vmData[3] = '4';
			vmData[4] = '5';
			vmData[5] = '6';

			Integer prompt = 0;
			
			// 执行持卡人验证
			// iCvmProc : HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
			ret = pbocApi.iHxEmvDoCvmMethod(PbocApi.HXCVM_BYPASS, vmData, prompt);
			if(prompt == 3)
				showLog("脱机密码验证,密码验证成功");
			else if(prompt == 2)
				showLog("脱机密码验证,密码已经被锁");
			else showLog("持卡人验证返回：" + ret);
		}
			
	 	// 获取CVM认证方法签字标志
		Boolean needSignFlag = false;
		ret = pbocApi.iHxEmvGetCvmSignFlag(needSignFlag);
		if (ret != PbocApi.HXEMV_OK) 
			throw new Exception("获取持卡人验证：" + errorMessage(ret));
		if(needSignFlag)
			showLog("本交易需要持卡人签字");

		// 终端行为分析
		ret = pbocApi.iHxEmvTermActionAnalysis();
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("终端行为分析：" + errorMessage(ret));
		showLog("终端行为分析，成功");
		

		// 第一次GAC
		Integer cardAction = 0;
		ret = pbocApi.iHxEmvGac1(0, cardAction);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("第一次GAC：" + errorMessage(ret));
		showLog("第一次GAC，成功");
		if(cardAction==PbocApi.GAC_ACTION_AAC || cardAction==PbocApi.GAC_ACTION_AAC_ADVICE) {
			// 交易拒绝
			showLog("交易被拒绝");
		}
		if(cardAction == PbocApi.GAC_ACTION_TC) {
			// 交易完成
			showLog("交易完成");
		}	
		showLog("doPbocDemoTrans_1 完成");
	}
	public void getTransData() throws Exception {
		HxTlvData outTlvData = new HxTlvData();
		int ret;
				
		// Pan(5A)		
		ret = pbocApi.iHxEmvGetData(0x5A, outTlvData);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("iHxEmvGetData(0x5A：" + errorMessage(ret));
		showLog("Pan(5A) = " + new String(outTlvData.dataBytes));
		
		// PanSeqNo(5F34)		
		ret = pbocApi.iHxEmvGetData(0x5F34, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x5F34：" + errorMessage(ret));
		if (ret == PbocApi.HXEMV_OK) {
			showLog("PanSeqNo(5F34) = " + outTlvData.dataString);
		}
		else {
			showLog("PanSeqNo(5F34),无数据");
		}

		// IssuerData(9F10)		
		ret = pbocApi.iHxEmvGetData(0x9F10, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F10：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("IssuerData(9F10) = " + outTlvData.dataString);
		}
		else {
			showLog("IssuerData(9F10),无数据");
		}
		
		// Arqc(9F26)		
		ret = pbocApi.iHxEmvGetData(0x9F26, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F26：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("Arqc(9F26) = " + outTlvData.dataString);
		}
		else {
			showLog("Arqc(9F26),无数据");
		}
		// TcHashValue(98)		
		ret = pbocApi.iHxEmvGetData(0x98, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x98：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TcHashValue(98) = " + outTlvData.dataString);
		}
		else {
			showLog("TcHashValue(98),无数据");
		}
		
		// Amount(9F02)		
		ret = pbocApi.iHxEmvGetData(0x9F02, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F02：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("Amount(9F02) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("Amount(9F02),无数据");
		}
		
		// Amount(9F03)		
		ret = pbocApi.iHxEmvGetData(0x9F03, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F03：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("Amount(9F03) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("Amount(9F03),无数据");
		}
		
		// TermCountryCode(9F1A)		
		ret = pbocApi.iHxEmvGetData(0x9F1A, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F1A：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TermCountryCode(9F1A) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("TermCountryCode(9F1A),无数据");
		}

		// CurrencyCode(5F2A)		
		ret = pbocApi.iHxEmvGetData(0x5F2A, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x5F2A：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("CurrencyCode(5F2A) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("CurrencyCode(5F2A),无数据");
		}
		
		// TransDate(9A)		
		ret = pbocApi.iHxEmvGetData(0x9A, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9A：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TransDate(9A) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("TransDate(9A),无数据");
		}
		
		// TransTypc(9C)		
		ret = pbocApi.iHxEmvGetData(0x9C, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9C：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TransTypc(9C) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("TransTypc(9C),无数据");
		}
		
		// TermRand(9F37)		
		ret = pbocApi.iHxEmvGetData(0x9F37, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F37：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TermRand(9F37) = " + outTlvData.dataString);
		}
		else {
			showLog("TermRand(9F37),无数据");
		}
		
		// Aip(82)		
		ret = pbocApi.iHxEmvGetData(0x82, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x82：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("Aip(82) = " + outTlvData.dataString);
		}
		else {
			showLog("Aip(82),无数据");
		}
		
		// Atc(9F36)	
		ret = pbocApi.iHxEmvGetData(0x9F36, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F36：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("Atc(9F36) = " + outTlvData.dataString);
		}
		else {
			showLog("Atc(9F36),无数据");
		}
		
		// EcBalance(9F79)	
		ret = pbocApi.iHxEmvGetCardNativeData(0x9F79, outTlvData);
		if (ret != PbocApi.HXEMV_OK && ret != PbocApi.HXEMV_NO_DATA)
			throw new Exception("iHxEmvGetData(0x9F79：" + errorMessage(ret));
		
		if (ret == PbocApi.HXEMV_OK) {
			showLog("EcBalance(9F79) = " + new String(outTlvData.dataBytes));
		}
		else {
			showLog("EcBalance(9F79),无数据");
		}
	}
	public void doPbocDemoTrans_2() throws Exception {
		// Gac2
		Integer cardAction = 0;
		byte[] field55 = new byte[100];
		// .....field55为后台返回的数据， 假定数组的长度为100
		
		int ret = pbocApi.iHxEmvGac2("00", null/*no auth code*/, field55, 100, cardAction);
		if (ret != PbocApi.HXEMV_OK)
			throw new Exception("Gac2：" + errorMessage(ret));
		showLog("Gac2，成功");
		if(cardAction==PbocApi.GAC_ACTION_AAC || cardAction==PbocApi.GAC_ACTION_AAC_ADVICE) {
			// 交易拒绝
			throw new Exception("Gac2, 交易被拒绝");
		}
		
		HxTlvData outTlvData = new HxTlvData();
		// 交易完成	
		ret = pbocApi.iHxEmvGetData(0xDFAA, outTlvData);
		if (ret == PbocApi.HXEMV_OK && outTlvData.dataBytes[0] == 2)
			showLog("GPO卡片返回EC授权码,但未走EC通道");
		showLog("交易成功");
		
		// 显示关键数据
		// 脚本执行情况
		ret = pbocApi.iHxEmvGetData(0xDF31, outTlvData);
		if (ret == PbocApi.HXEMV_OK) {
			if((outTlvData.dataBytes[0] & 0xF0) == 0x20)
				showLog("脚本执行成功");
			else
				showLog("脚本执行失败");
		}
		
		// TVR
		ret = pbocApi.iHxEmvGetData(0x95, outTlvData);
		if (ret == PbocApi.HXEMV_OK) {
			showLog("TVR : " + outTlvData.dataString);
		}
	}
	
	public void setLogHandler(Handler logHandler) {
		this.logHandler = logHandler;
	}
	
	private void showLog(String logString) {
		if (logHandler != null) {
			Message msg = new Message();
			msg.obj = logString;
			logHandler.sendMessage(msg);
		}
	}

}
