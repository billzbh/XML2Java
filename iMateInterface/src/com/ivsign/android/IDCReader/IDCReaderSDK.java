package com.ivsign.android.IDCReader;


public class IDCReaderSDK
{
	/**
	 * 照片解码，解码成功后，解码后的照片文件名为zp.bmp,文件保存在wltlibDirectory目录下
	 *
	 * @param wltlibDirectory	解密照片授权文件所在的目录，解码后的照片也保存在该目录下
	 * @param pictureData		读二代证得到的照片尚未解密的数据，长度为1024字节，传入的数据不正确会造成程序的异常终止
	 *
	 * @return	0		成功
	 * 			1		照片解码初始化失败，需要检查传入的wltlibDirectory以及base.dat文件
	 * 			2		授权文件错误，需要检查license.lic文件
	 * 			3		照片解码失败，其它错误
	 */	
	public static int decodingPictureData(String wltlibDirectory, byte[] pictureData)
	{
		int ret = wltInit(wltlibDirectory);
		if (ret != 0) {
			return 1;
		}
		byte[] datawlt = new byte[1384];
		byte[] byLicData = {(byte)0x05,(byte)0x00,(byte)0x01,(byte)0x00,(byte)0x5B,(byte)0x03,(byte)0x33,(byte)0x01,(byte)0x5A,(byte)0xB3,(byte)0x1E,(byte)0x00};
		
		for(int i = 0 ; i < 1024; i++) {
			datawlt[i+14+256] = pictureData[i];			
		}
		ret = wltGetBMP(datawlt, byLicData);
		if (ret == 2) {
			return 2;
		}
		if (ret != 1)
			return 3;
		
		return 0;
	}
	
	public static native int wltInit(String s);
	public static native int wltGetBMP(byte abyte0[], byte abyte1[]);

	static 
	{
		System.loadLibrary("wltdecode");
	}
}
