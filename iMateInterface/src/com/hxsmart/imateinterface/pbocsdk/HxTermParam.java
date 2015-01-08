package com.hxsmart.imateinterface.pbocsdk;

public class HxTermParam {
	public int termType;                   		// T9F35, 终端类型, 例:0x21
	public String termCapability;           	// T9F33, 终端能力, length = 3*2
	public String additionalTermCapability; 	// T9F40, 终端能力扩展, length = 5*2
	public String merchantId;           		// T9F16, 商户号, length = 15
	public String termId;                		// T9F1C, 终端号, length = 8
	public String merchantNameLocation;			// T9F4E, 商户名字地址, byte length <= 254
	public int termCountryCode;            		// T9F1A, 终端国家代码, 156=中国
	public String acquirerId;           		// T9F01, 收单行标识符, length >= 6 & length <=11

    public String termAppVer;              		// T9F09, 终端应用版本号, length = 2*2,"FFFF"表示不存在
    public int floorLimit;						// T9F1B, 终端限额, 单位为分, -1表示不存在
    public int	maxTargetPercentage;        	// 随机选择最大百分比，-1:不存在
    public int	targetPercentage;           	// 随机选择目标百分比，-1:不存在
    public int thresholdValue;             		// 随机选择阈值, -1表示不存在
    public int ecashSupport;					// 1:支持电子现金 0:不支持电子现金, -1表示不存在
    public String termECashTransLimit;  	    // T9F7B, 终端电子现金交易限额, 空表示不存在
    public String tacDefault;					// TAC-Default, length = 5*2, 参考TVR结构, null表示不存在
    public String tacDenial;					// TAC-Denial, length = 5*2, 参考TVR结构, null表示不存在
    public String tacOnline;					// TAC-Online, length = 5*2, 参考TVR结构, null表示不存在
	
	public HxTermParam() {
		termType = 0;
		termCapability = null;
		additionalTermCapability = null;
		merchantId = null;
		termId = null;
		merchantNameLocation = null;
		termCountryCode = 156;
		acquirerId = null;
		
		termAppVer = null;
		floorLimit = -1;
		maxTargetPercentage = -1;
		targetPercentage = -1;
		thresholdValue = -1;
		ecashSupport = -1;
		termECashTransLimit = null;
		tacDefault = null;
		tacDenial = null;
		tacOnline = null;
	}
}
