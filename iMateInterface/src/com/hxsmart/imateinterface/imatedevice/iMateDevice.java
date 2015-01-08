package com.hxsmart.imateinterface.imatedevice;

import com.hxsmart.imateinterface.BluetoothThread;

public class iMateDevice {
	// SD ICC IO Type
	private final int SDSC_SCIO_UNKNOWN        =            0;         // The type of the SCIO is unknown.
	private final int SDSC_SCIO_7816           =            1;         // The type of the SCIO is 7816.
	private final int SDSC_SCIO_SPI_V1         =            2;         // The type of the SCIO is SPI V1.
	private final int SDSC_SCIO_SPI_V2         =            3;         // The type of the SCIO is SPI V2.
	private final int SDSC_SCIO_SPI_V3         =            4;         // The type of the SCIO is SPI V3
	
    public iMateDevice() {
        jniInit();
    }
    
    private native int jniInit();

    // 打开指纹模块电源，并检测是否连接成功。指纹模块的启动时间在2秒左右。
    // Device Mode采用A模式
    // 返回码： 
    //      0       :   成功
    //      99      :   不支持该功能
    //      其它    :   失败或未检测到指纹模块
    public native int fingerprintOpen();

    // 关闭指纹模块电源
    public native void fingerprintClose();

    // 检测指纹模块是否在线。该函数在fingerprintOpen后可不用调用。
    // 返回码： 
    //      0       :   成功
    //      其它    :   失败
    public native int fingerprintLink();

    // 向指纹模块发送命令, 不包含前导串和验证码
    // 输入参数：    
    //      in      :   发送的数据缓冲
    //      inLen   :   发送数据长度
    // 返回码：
    //          0   :   成功
    //         其它 :   失败
    public native  void fingerprintSend(byte[] in, int inLen);

    // 在超时时间内等待指纹模块返回的数据，接收到的数据不包含前导串和验证码。
    // 输入参数：
    //      timeOutMs   :   等待返回数据的超时时间（毫秒）
    // 输出参数：
    //      out         :   输出数据的缓冲区（不包括前导串和验证码等），需要预先分配空间，maxlength = 512
    //      outLen      :   接收到的数据长度（不包括前导串和验证码等），
    //                      需预先创建对象：Integer outLen = new Integer(0)
    // 返回码： 
    //      0           :   成功
    //      其它        :   失败
    public native int fingerprintRecv(byte[] out, Integer outLen, int timeOutMs);

    // TF ICC Functions
    
    // 打开SD卡电源，接口初始化
    // 返回码： 
    //      0       :   成功
    //      99      :   不支持该功能
    //      其它    :   失败
    public native int SD_Init();

    // 关闭SD卡电源
    public native void SD_DeInit();

    // 识别SD_ICC，冷复位
    // 返回码： 
    //      0       :   成功
    //      其它    :   失败
    public native int SDSCConnectDev();

    // 关闭SD_ICC
    // 返回码： 
    //      0       :   成功
    //      其它    :   失败
    public native int SDSCDisconnectDev();

    // 获取SD_ICC固件版本号
    // 输出参数 ：
    //      firmwareVer     :   固件版本缓冲区(需要预先分配空间，maxlength = 30）
    //      firmwareVerLen  :   固件版本数据的长度,需预先创建对象：Integer firmwareVerLen = new Integer(0)
    // 返回码： 
    //      0               :   成功
    //      其它            :   失败
    public native int SDSCGetFirmwareVer(byte[] firmwareVer, Integer firmwareVerLen);

    // SD_ICC热复位
    // 输出参数 : 
    //      atr     :   Atr数据缓冲区（需要预先分配空间，maxlength = 80）
    //      atrLen  :   返回复位数据长度,需预先创建对象：Integer atrLen = new Integer(0)
    // 返回码       ： 
    //      0       :   成功
    //      其它    :   失败
    public native int SDSCResetCard(byte[] atr, Integer atrLen);


    // 转换SD_ICC电源模式
    // 返回码： 
    //      0       :   成功
    //      其它    :   失败
    public native int SDSCResetController(int SCPowerMode);

    // SD_ICC APDU
    // 输入参数 :  
    //      command     :   ICC Apdu命令串
    //      commandLen  :   命令串长度
    //      timeOutMode :   超时模式，固定使用0.
    // 输出参数 : 
    //      outData     :   响应串缓冲区（不包括状态字）,需要预分配空间，maxlength = 300
    //      outDataLen  :   返回响应数据长度,需预先创建对象：Integer outDataLen = new Integer(0)
    //      cosState    :   卡片执行状态字, 需要预分配空间，maxlength = 2
    // 返回码： 
    //      0           :   成功
    //      其它        :   失败
    public native int SDSCTransmit(byte[] command, int commandLen, int timeOutMode, byte[] outData, Integer outDataLen, byte[] cosState);

    // SD_ICC APDU EX
    // 输入参数 :  
    //      command     :   apdu命令串
    //      commandLen  :   命令串长度
    //      timeOutMode :   超时模式，固定使用0x00.
    // 输出参数 : 
    //      outData     :   响应串缓冲区,需要预分配空间，maxlength = 300
    //      outDataLen  :   返回响应数据长度, 需预先创建对象：Integer outDataLen = new Integer(0)
    // 返回码： 
    //      0           :   成功
    //      其它        :   失败
    public native int SDSCTransmitEx(byte[] command, int commandLen, int timeOutMode, byte[] outData, Integer outDataLen);

    // 获取SD_ICC SDK版本号
    // 输出参数 ：
    //      version     :   SDK版本号数据缓冲，需要预先分配空间，maxlength = 30
    //      versionLen  :   SDK版本号数据长度(需预先创建对象：Integer versionLen = new Integer(0)）
    // 返回码： 
    //      0           :   成功
    //      其它        :   失败
    public native int SDSCGetSDKVersion(byte[] version, Integer versionLen);

    // 获取SD_ICC IO类型
    // 输出参数：
    //      SCIOType    :   IO类型,需预先创建对象：Integer SCIOType = new Integer(0)
    //                      请参考：SD ICC IO Type
    // 返回码： 
    //      0           :   成功
    //      其它        :   失败
    public native int SDSCGetSCIOType(Integer SCIOType);
    
    
    /**
     * 蓝牙数据交换接口
     * @param dataIn    输入数据缓冲
     * @param inlength  输入数据长度    
     * @param apduOut   输出数据缓冲
     * @return  0       失败
     *          其他        成功，返回数据长度
     */
    public int sendReceiveCallBack(byte[] dataIn, int inlength, byte[] dataOut, int timeOut) {  
        return BluetoothThread.getInstance().sendReceive(dataIn, inlength, dataOut, timeOut);
    }
	
	static {
		System.loadLibrary("imatedevice");
	}
}
