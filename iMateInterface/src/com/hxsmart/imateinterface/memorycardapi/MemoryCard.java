package com.hxsmart.imateinterface.memorycardapi;

public class MemoryCard {
	public static final int SLE4442_TYPE		= 	0x01;
	public static final int AT102_TYPE			= 	0x02;
	public static final int AT1608_TYPE			= 	0x04;
	public static final int AT1604_TYPE			= 	0x05;
	public static final int SLE4428_TYPE		= 	0x06;
	public static final int AT24Cxx_TYPE		= 	0x10;
	
	public static final int CARD_VOLTAGE_3V0	=	1;
	public static final int CARD_VOLTAGE_5V0	=	2;
	
	// 检测插卡
	// 返回码：
	//	    0       :   未插卡
	//		1		:	已插卡
	//	    99      :   不支持该功能
	public native int TestCard();
	
	// 功能：设置卡片电压
	// voltageTag :	CARD_VOLTAGE_3V0, CARD_VOLTAGE_5V0
	// 				表示3V卡和5V卡
	public native int SetCardVoltage(int voltageTag);
	
	// 功能：检测存储卡类型(4442\102\1604\1608\4428)
	// 返回: 	0xff	: 	不识别的卡
	//			其它		:	SLE4442_TYPE		0x01
	//						AT102_TYPE			0x02
	//						AT1608_TYPE			0x04
	//						AT1604_TYPE			0x05
	//						SLE4428_TYPE		0x06
	//						AT24Cxx_TYPE		0x10	
	public native int TestCardType();

	// 打开SLE4442卡
	public native void SLE4442_Open();
	
	// 打开SLE4442卡, 自动检测电压和卡类型
	// 返回: 	0		: 	打开成功
	//			其它		:	打开失败
	public native int SLE4442_OpenAuto();

	// 关闭SLE4442卡
	public native void SLE4442_Close();

	// 验证SLE4442卡密码
	// 输入参数：
	//	      securityCode  :   密码数据缓冲，3字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int SLE4442_ChkCode(byte[] securityCode);
	
	// 验证SLE4442卡密码(安全方式）
	// 输入参数：
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int SLE4442_ChkCodeEx(byte[] securityCode);

	// SLE4442读数据
	// 输入参数：
	//			offset	:	数据偏移量，0~255
	//			dataLen	:	数据长度
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength=255
	public native void SLE4442_Read(int offset, int dataLen, byte[] dataBuff);

	// SLE4442写数据
	// 输入参数：
	//			offset	:	数据偏移量，0~255
	//			dataLen	:	数据长度
	//			dataBuff:	写数据缓冲区地址, maxlength=255
	public native void SLE4442_Write(int offset, int dataLen, byte[] dataBuff);


	// 打开SLE4428卡
	public native void SLE4428_Open();
	
	// 打开SLE4428卡, 自动检测电压和卡类型
	// 返回: 	0		: 	打开成功
	//			其它		:	打开失败
	public native int SLE4428_OpenAuto();

	// 关闭SLE4428卡
	public native void SLE4428_Close();

	// 验证SLE4428卡密码
	// 输入参数：
	//	      securityCode  :   密码数据缓冲，2字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int SLE4428_ChkCode(byte[] securityCode);
	
	// 验证SLE4428卡密码（安全方式）
	// 输入参数：
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int SLE4428_ChkCodeEx(byte[] securityCode);

	// SLE4428读数据
	// 输入参数：
	//			offset	:	数据偏移量，0~1024
	//			dataLen	:	数据长度
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength = 1024
	public native void SLE4428_Read(int offset, int dataLen, byte[] dataBuff);

	// SLE4428写数据
	// 输入参数：
	//			offset	:	数据偏移量，0~1024
	//			dataLen	:	数据长度
	//			dataBuff:	写数据缓冲区地址, maxlength = 1024
	public native void SLE4428_Write(int offset, int dataLen, byte[] dataBuff);

	// 打开AT102卡
	public native void AT102_Open();
	
	// 打开AT102卡, 自动检测电压和卡类型
	// 返回: 	0		: 	打开成功
	//			其它		:	打开失败
	public native int AT102_OpenAuto();

	// 关闭AT102卡
	public native void AT102_Close();

	// 验证AT102卡密码
	// 输入参数：
	//	      securityCode  :   密码数据缓冲，2字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_ChkCode(byte[] securityCode);
	
	// 验证AT102卡密码
	// 输入参数：
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_ChkCodeEx(byte[] securityCode);

	// AT102读数据(按双字节读取）
	// 输入参数：
	//			wordOffset :	数据偏移量，<= 128
	//			wordNum	:	数据长度（字长度）, <= 128
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength = 256
	public native void AT102_ReadWords(int wordOffset, int wordNum, byte[] dataBuff);

	// AT102写数据(按双字节写卡）
	// 输入参数：
	//			wordOffset 	:	数据偏移量, <= 128
	//			wordNum		:	数据长度（字长度）<= 128
	//			dataBuff:	写数据缓冲区地址, maxlength = 256
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_WriteWords(int wordOffset, int wordNum, byte[] dataBuff);

	// AT102擦除非应用区的数据
	// 输入参数：
	//			wordOffset 	:	数据偏移量
	//			wordNum		:	删除字长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_EraseNonApp(int wordOffset, int wordNum);

	// AT102擦除应用区的数据
	// 输入参数：
	//			area  	: 应用区	1:	应用区1	2:	应用区2
	//			limited : 指出第2应用区的擦除次数是否受限制。此参数当且仅当area= 2时起作用。
	//			eraseKey: 擦除密码, 应用区1的擦除密码长度为6bytes，应用区2的擦除密码为4bytes，
	//				  Security Level 1 方式下，eraseKey=NULL则不需要提供EraseKey方式进行擦除
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_EraseApp(int area, int limited, byte[] eraseKey);
	
	// AT102擦除应用区的数据（安全方式）
	// 输入参数：
	//			area  	: 应用区	1:	应用区1	2:	应用区2
	//			limited : 指出第2应用区的擦除次数是否受限制。此参数当且仅当area= 2时起作用。
	//			eraseKey: 擦除密码, 加密后的应用区1的擦除密码，8字节
	//				  Security Level 1 方式下，eraseKey=NULL则不需要提供EraseKey方式进行擦除
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_EraseAppEx(int area, int limited, byte[] eraseKey);

	// AT102读应用区数据
	// 输入参数：
	//		area  	: 应用区	1:	应用区1	2:	应用区2
	// 输出参数：
	//		dataBuff:	读数据缓冲区地址, length = 64
	public native void AT102_ReadAZ(int area, byte[] dataBuff);

	// AT102写应用区数据
	// 输入参数：
	//		area  	: 应用区	1:	应用区1	2:	应用区2
	//		dataBuff:	读数据缓冲区地址, maxlength = 64
	// 返回码：
	//	    0   :   成功
	//	    其它 :   失败
	public native int AT102_WriteAZ(int area, byte[] dataBuff);


	// AT102读测试区数据
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, 获取2字节数据
	public native void AT102_ReadMTZ(byte[] dataBuff);

	// AT102写测试区数据
	// 输入参数：
	//			dataBuff:	读数据缓冲区地址, 2字节数据
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT102_UpdateMTZ(byte[] dataBuff);


	// 打开AT1604卡
	public native void AT1604_Open();
	
	// 打开AT1604卡, 自动检测电压和卡类型
	// 返回: 	0		: 	打开成功
	//			其它		:	打开失败
	public native int AT1604_OpenAuto();
	

	// 关闭AT1604卡
	public native void AT1604_Close();

	// 验证AT1604卡主密码
	// 输入参数：
	//	      securityCode  :   密码数据缓冲，2字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkCode(byte[] securityCode);
	
	// 验证AT1604卡主密码（安全方式）
	// 输入参数：
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkCodeEx(byte[] securityCode);

	// 验证AT1604卡分区密码
	// 输入参数：
	//			area		  :   分区号1、2、3、4
	//	      securityCode  :   密码数据缓冲，2字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkAreaCode(int area, byte[] securityCode);
	
	// 验证AT1604卡分区密码（安全方式）
	// 输入参数：
	//			area		  :   分区号1、2、3、4
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkAreaCodeEx(int area, byte[] securityCode);
	
	// 验证AT1604卡分区删除密码
	// 输入参数：
	//			area		  :   分区号1、2、3、4
	//	      securityCode  :   密码数据缓冲，2字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkAreaEraseCode(int area, byte[] securityCode);
	
	// 验证AT1604卡分区删除密码（安全方式）
	// 输入参数：
	//			area		  :   分区号1、2、3、4
	//	      securityCode  :   加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_ChkAreaEraseCodeEx(int area, byte[] securityCode);

	// AT1604读数据
	// 输入参数：
	//			offset	:	数据偏移量，0~2047
	//			dataLen	:	数据长度
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength = 2048
	public native void AT1604_Read(int offset, int dataLen, byte[] dataBuff);

	// AT1604写数据
	// 输入参数：
	//			offset	:	数据偏移量，0~2047
	//			dataLen	:	数据长度
	//			dataBuff:	写数据缓冲区地址, maxlength = 2048
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_Write(int offset, int dataLen, byte[] dataBuff);

	// AT1604读分区数据
	// 输入参数：
	//			area	:   分区号1、2、3、4
	//			offset	:	数据偏移量，0~511
	//			dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength = 512
	public native void AT1604_ReadAZ(int area, int offset, int dataLen, byte[] dataBuff);

	// AT1604写分区数据
	// 输入参数：
	//			area	:   分区号1、2、3、4
	//			offset	:	数据偏移量，0~511
	//			dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
	//			dataBuff:	写数据缓冲区地址, maxlength = 512
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_WriteAZ(int area, int offset, int dataLen, byte[] dataBuff);

	// AT1604擦除数据
	// 输入参数：
	//			offset	:	数据偏移量，0~2047
	//			dataLen	:	数据长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_Erase(int offset, int dataLen);

	// AT1604擦除分区数据
	// 输入参数：
	//			area	:   分区号1、2、3、4
	//			offset	:	数据偏移量，0~511
	//			dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_EraseAZ(int area, int offset, int dataLen);


	// AT1604读测试区数据
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, length = 2
	public native void AT1604_ReadMTZ(byte[] dataBuff);

	// AT1604写测试区数据
	// 输入参数：
	//			dataBuff:	写数据缓冲区地址, length = 2
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1604_WriteMTZ(byte[] dataBuff);


	// 打开AT1608卡
	// 输出参数：
	//		resetData:	复位数据, length = 4bytes
	public native void AT1608_Open(byte[] resetData);
	
	// 打开AT1608卡, 自动检测电压和卡类型
	// 返回: 	0		: 	打开成功
	//			其它		:	打开失败
	public native int AT1608_OpenAuto(byte[] resetData);
	

	// 关闭AT1608卡
	public native void AT1608_Close();
	
	// 读AT88SC1608熔丝状况
	// 返 回 值:	0--OK	1--Error
	public native int AT1608_ReadFuse(byte[] fuse);

	// AT1608卡设置应用区
	// 输入参数：
	//	      az	: 应用区，0~7
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_SetAZ(int az);

	// 验证AT1608卡读写密码
	// 输入参数：
	//	 		index		: 密码索引号 写密码索引号(0--7) 读密码索引号(0x80--0x87)
	//	      securityCode: 密码数据缓冲，3字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_ChkCode(int index, byte[] securityCode);
	
	// 验证AT1608卡读写密码（安全方式）
	// 输入参数：
	//	 	index		: 密码索引号 写密码索引号(0--7) 读密码索引号(0x80--0x87)
	//	    securityCode: 加密后的密码数据缓冲，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_ChkCodeEx(int index, byte[] securityCode);

	// AT1608卡安全认证
	// 输入参数：
	//	      gc	: 密钥，8字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_Auth(byte[] gc);
	
	// AT1608卡安全认证（安全方式）
	// 输入参数：
	//	      gc	: 加密后的密钥，16字节长度
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_AuthEx(byte[] gc);

	// AT1608读数据
	// 输入参数：
	//			level	:   0:应用区，1：设置区
	//			offset	:	设置区(0--127)	应用区(0--255)
	//			dataLen	:
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, maxlength = 256
	public native int AT1608_Read(int level, int offset, int dataLen, byte[] dataBuff);

	// AT1608写数据
	// 输入参数：
	//			level	:   0:应用区，1：设置区
	//			offset	:	设置区(0--127)	应用区(0--255)
	//			dataLen	:
	//			dataBuff:	读数据缓冲区地址, maxlength = 256
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT1608_Write(int level, int offset, int dataLen, byte[] dataBuff);


	// 打开AT24Cxx卡
	public native void AT24Cxx_Open();
	
	// 打开AT24Cxx卡, 自动检测检测卡类型的正确性
	public native int AT24Cxx_OpenAuto();

	// 关闭AT24Cxx卡
	public native void AT24Cxx_Close();

	// AT24Cxx读数据
	// 输入参数：
	//			offset	:	数据偏移量
	//			dataLen	:	数据长度
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, 根据具体情况确定
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT24Cxx_Read(int offset, int dataLen, byte[] dataBuff);

	// AT24Cxx写数据
	// 输入参数：
	//			offset	:	数据偏移量
	//			dataLen	:	数据长度
	//			dataBuff:	写数据缓冲区地址, 根据具体情况确定
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT24Cxx_Write(int offset, int dataLen, byte[] dataBuff);

	// AT24C32读数据
	// 输入参数：
	//			offset	:	数据偏移量
	//			dataLen	:	数据长度
	// 输出参数：
	//			dataBuff:	读数据缓冲区地址, 根据具体情况确定
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT24C32_Read(int offset, int dataLen, byte[] dataBuff);

	// AT24C32写数据
	// 输入参数：
	//			offset	:	数据偏移量
	//			dataLen	:	数据长度
	//			dataBuff:	写数据缓冲区地址, 根据具体情况确定
	// 返回码：
	//	      0   :   成功
	//	      其它 :   失败
	public native int AT24C32_Write(int offset, int dataLen, byte[] dataBuff);
	
	// 产生通讯密钥，用户存储卡密码验证时使用安全方式，密钥产生成功后，密钥存放在iGAS内存中
	// 输入参数：
	//		masterKeyId	: 主密钥ID, 一般使用6
	//		random		: 传入的随机数，8字节
	// 返回码：
	//		0	:	成功
	//		其它 ：	失败
	public native int GenCommKey(int masterKeyId, byte[] random);
	
	static {
		System.loadLibrary("memorycardapi");
	}
}
