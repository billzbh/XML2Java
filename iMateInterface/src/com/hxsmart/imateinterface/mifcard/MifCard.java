package com.hxsmart.imateinterface.mifcard;

import java.io.UnsupportedEncodingException;

import com.hxsmart.imateinterface.BluetoothThread;

public class MifCard {
	
	public final static int Mifare_KeyA = 0;
	public final static int Mifare_KeyB = 1;

	/**
	 * MIF寻卡(Polling + AntiCollision) 
	 * @param   timeout 等待卡片超时时间(0~255秒)
	 * @return	成功返回UID，否则抛出异常
	 * @throws  Exception
	 */
	public byte[] waitCard(int timeout) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (timeout < 0 || timeout > 255)
			throw new Exception("等待时间参数错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = (byte)0x81;
		sendBytes[1] = 0; //无需激活
		sendBytes[2] = (byte)timeout;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 3, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("寻卡失败");
			}
	    }
	    /*
	    if (receivedLength-1 > 10) {
			throw new Exception("返回的UID长度错误，>10");	    	
	    }
	    */
	    
	    byte[] uids = new byte[receivedLength-1];
	    for (int i = 0; i < receivedLength-1; i++) {
	    	uids[i] = receivedBytes[i+1];
	    }
	    
	    return uids;
	}
	
	/**
	 * MIF CPU卡激活 (Activation) 
	 * @throws  Exception
	 */
	public void activeCard() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = (byte)0x82;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 1, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("卡激活失败");
			}
	    }
	}
	
	/**
	 * MIF APDU 
	 * @param   apduIn  	  apdu指令
	 * @param   apduInLength  指令长度
	 * @return	成功返回卡片响应，否则抛出异常
	 * @throws  Exception
	 */
	public byte[] apdu(byte[] apduIn, int apduInLength) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[300];
		final byte[] sendBytes = new byte[300];
		
		sendBytes[0] = (byte)0x83;
		for (int i=0; i < apduInLength; i++)
			sendBytes[i+1] = apduIn[i];
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, apduInLength+1, receivedBytes, 2);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("apdu失败");
			}
	    }
	    
	    byte[] response = new byte[receivedLength-1];
	    for (int i = 0; i < receivedLength-1; i++) {
	    	response[i] = receivedBytes[i+1];
	    }
	    return response;
	}
	
	/**
	 * MIF 在指定的时间内，等待移除卡片(卡片移除后，自动关闭射频)
	 * @param   timeout 等待卡片移除的超时时间(0~255秒)
	 * @return	返回true，检测到已经移除卡片，返回false，卡片尚未移除
	 * @throws  Exception
	 */
	public Boolean waitRemoval(int timeout) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (timeout < 0 || timeout > 255)
			throw new Exception("等待时间参数错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = (byte)0x84;
		sendBytes[1] = (byte)timeout;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 2, receivedBytes, timeout+1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
	    	if (receivedBytes[0] == 1)
	    		return false;
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("卡片移除操作失败");
			}
	    }	    
	    return true;
	}
	
	/**
	 * 关闭射频信号 
	 * @throws  Exception
	 */
	public void fieldOff() throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = (byte)0x85;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 1, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	}
	
	/**
	 * Mifare 扇区认证 
	 * @param   keyType 	密钥类型，MifCard.Mifare_KeyA 或 MifCard.Mifare_KeyB
	 * @param   sectorNo 	扇区号
	 * @param   key 		密钥（6字节长度）
	 * @return	返回true，认证成功，返回false，认证失败
	 * @throws  Exception
	 */
	public void mifareAuth(int keyType, int sectorNo, byte[] key) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (keyType != Mifare_KeyA && keyType != Mifare_KeyB)
			throw new Exception("密钥类型参数错误");
		if (sectorNo < 0)
			throw new Exception("扇区号参数错误");
		
		if (key.length < 6)
			throw new Exception("密钥长度参数错误");
		
		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = (byte)0x86;
		sendBytes[1] = (byte)keyType;
		sendBytes[2] = (byte)(sectorNo*4);
		for (int i=0; i < 6; i++)
			sendBytes[i+3] = key[i];
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 13, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("扇区认证操作失败");
			}
	    }	    
	}
	
	/**
	 * Mifare 读块数据 
	 * @param   Mifare  	  块号
	 * @return	成功返回块数据(16字节)，否则抛出异常
	 * @throws  Exception
	 */
	public byte[] mifareRead(int blockNo) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = (byte)0x87;
		sendBytes[1] = (byte)blockNo;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 2, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("mifware读数据块失败");
			}
	    }
	    if (receivedLength-1 != 16) {
			throw new Exception("mifware数据块长度错误");	    	
	    }
	    
	    byte[] data = new byte[receivedLength-1];
	    for (int i = 0; i < receivedLength-1; i++) {
	    	data[i] = receivedBytes[i+1];
	    }
	    return data;
	}
	
	/**
	 * Mifare 写块数据 
	 * @param   keyType 	密钥类型，MifCard.Mifare_KeyA 或 MifCard.Mifare_KeyB
	 * @param   blockData 	块数据
	 * @throws  Exception，写块失败时抛出异常
	 */
	public void mifareWrite(int blockNo, byte[] blockData) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (blockData.length < 16)
			throw new Exception("数据长度错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = (byte)0x88;
		sendBytes[1] = (byte)blockNo;
		for (int i=0; i<16; i++)
			sendBytes[i+2] = blockData[i];
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 18, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("mifware写数据块失败");
			}
	    }
	}
	
	/**
	 * Mifare 钱包减值 
	 * @param   blockNo 钱包块号
	 * @param   value 	减值
	 * @throws  Exception，钱包减值失败时抛出异常
	 */
	public void mifareDec(int blockNo, long value) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		long maxValue = (long)256*(long)256*(long)256*(long)256;
		if (value > maxValue || value < 0)
			throw new Exception("数值参数错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = (byte)0x89;
		sendBytes[1] = (byte)blockNo;
		sendBytes[2] = (byte)((value >> 24) & 0xFF) ;
		sendBytes[3] = (byte)((value >> 16) & 0xFF);
		sendBytes[4] = (byte)((value >> 8) & 0xFF);
		sendBytes[5] = (byte)(value & 0xFF);
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 6, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("mifware钱包减值失败");
			}
	    }
	}
	
	/**
	 * Mifare 钱包加值 
	 * @param   blockNo 钱包块号
	 * @param   value 	加值
	 * @throws  Exception，钱包加值失败时抛出异常
	 */
	public void mifareInc(int blockNo, long value) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		long maxValue = (long)256*(long)256*(long)256*(long)256;
		if (value > maxValue || value < 0)
			throw new Exception("数值参数错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[20];
		
		sendBytes[0] = (byte)0x8A;
		sendBytes[1] = (byte)blockNo;
		sendBytes[2] = (byte)((value >> 24) & 0xFF);
		sendBytes[3] = (byte)((value >> 16) & 0xFF);
		sendBytes[4] = (byte)((value >> 8) & 0xFF);
		sendBytes[5] = (byte)(value & 0xFF);
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 6, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("mifware钱包加值失败");
			}
	    }
	}
	
	/**
	 * Mifare 块拷贝 
	 * @param   srcBlockNo 源块号
	 * @param   desBlockNo 目的块号
	 * @throws  Exception，块拷贝失败时抛出异常 
	 */
	public void mifareCopy(int srcBlockNo, int desBlockNo) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		if (srcBlockNo < 0 || desBlockNo < 0)
			throw new Exception("参数错误");

		final byte[] receivedBytes = new byte[100];
		final byte[] sendBytes = new byte[6];
		
		sendBytes[0] = (byte)0x8B;
		sendBytes[1] = (byte)srcBlockNo;
		sendBytes[2] = (byte)desBlockNo;
		
	    int receivedLength = BluetoothThread.getInstance().sendReceive(sendBytes, 3, receivedBytes, 1);
	    if (receivedLength <= 0)
			throw new Exception("iMate通讯超时");	
	    if (receivedBytes[0] != 0) {
			try {
				throw new Exception(new String(receivedBytes,1,receivedLength-1,"GBK"));
			} catch (UnsupportedEncodingException e) {
				throw new Exception("mifware块拷贝失败");
			}
	    }
	}
	
	/**
	 * 读金融PBOC IC卡
	 * @param   timeout 超时时间
	 * @return	成功返回卡信息，否则抛出异常
	 * @throws  Exception
	 */	
	public String readPbocCard(int timeout) throws Exception
	{
		try {
			deviceTest();
		}catch (Exception e) {
			throw e;
		}
		
		waitCard(timeout);
		
		activeCard();
		
		final byte[] apduIn = new byte[300];
 
		apduIn[0] = 0x00;
		apduIn[1] = (byte)0xA4;
		apduIn[2] = 0x04;
		apduIn[3] = 0x00; //p2
		apduIn[4] = 0x05; //inLength
		apduIn[5] = (byte)0xA0;
		apduIn[6] = 0x00;
		apduIn[7] = 0x00;
		apduIn[8] = 0x03;
		apduIn[9] = 0x33;
		
		byte[] apduOut;
		try {
			apduOut = apdu(apduIn, 10);
		}catch (Exception e) {
			throw e;
		}
		
		int retLength = apduOut.length;
		if (retLength < 2)
			throw new Exception("卡片返回数据错误");
		
		if (apduOut[retLength - 2] != (byte)0x61 && apduOut[retLength - 2] != (byte)0x90) {
			throw new Exception("应用不支持");		
		}
		
		String cardNoString = null;
		String panSequenceNoString = null;
		String holderNameString = null;
		String holderIdString = null;
		String expireDateString = null;
		String track2String = null;
		
		// 读记录文件
		for (int i=1; i<10; i++) {
			for (int j=1; j<10; j++) {
				apduIn[0] = 0x00;
				apduIn[1] = (byte)0xB2;
				apduIn[2] = (byte)i;
				apduIn[3] = (byte)(((j << 3)|0x04)&0xff); //p2
				apduIn[4] = 0x00; //inLength
				
				try {
					apduOut = apdu(apduIn, 5);
				}catch (Exception e) {
					throw e;
				}
				retLength = apduOut.length;
				if (retLength < 2)
					throw new Exception("卡片返回数据错误");
			    		    
				if (apduOut[retLength - 2] != (byte)0x90 || retLength == 2)
					break;
				
				/*
				for (int p=0; p<apdu.getOutLength(); p++) {
					System.out.format("%02x", apdu.getAuduOutBytes()[p]);
				}
				System.out.println();
				*/
				
				byte[] tagValue;
				if (cardNoString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x5a&0xff);
					if (tagValue != null) {
						cardNoString = new String();
						for (int m=0; m<tagValue.length; m++) {
							cardNoString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						int point = cardNoString.indexOf('f');
						if (point > 0)
							cardNoString = cardNoString.substring(0, point);
						point = cardNoString.indexOf('F');
						if (point > 0)
							cardNoString = cardNoString.substring(0, point);
					}	
				}
				if (panSequenceNoString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x5f34&0xffff);
					if (tagValue != null) {
						String dateString = Integer.toHexString((tagValue[0]&0x000000ff)|0xffffff00).substring(6);
						panSequenceNoString = dateString.trim();
					}	
				}
				if (holderNameString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x9f0b&0xffff);
					String name;
					if (tagValue != null) {
						try {
						name = new String(tagValue, "GBK");
						} catch (UnsupportedEncodingException e) {
							name = new String(tagValue);
						}
						holderNameString = name.trim();	
					}	
				}
				if (holderNameString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x5f20&0xffff);
					String name;
					if (tagValue != null) {
						try {
						name = new String(tagValue, "GBK");
						} catch (UnsupportedEncodingException e) {
							name = new String(tagValue);
						}
						holderNameString = name.trim();	
					}	
				}
				tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x9f0b&0xffff);
				if (tagValue != null) {
					String name = new String(tagValue);
					holderNameString = name.trim();	
				}
				
				if (holderIdString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x9f61&0xffff);
					if (tagValue != null) {
						String holderId = new String(tagValue);
						holderIdString = holderId.trim();						
					}	
				}
				//if (holderIdString != null)
					//System.out.println(holderIdString);
				
				if (expireDateString == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x5f24&0xffff);
					if (tagValue != null) {
						String dateString = "";
						for (int m=0; m<3; m++) {
							dateString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						expireDateString = dateString.trim();
					}	
				}
				if (track2String == null) {
					tagValue = BluetoothThread.getTlvTagValue(apduOut, retLength - 2, 0x57&0xffff);
					if (tagValue != null) {
						String dateString = "";
						for (int m=0; m<tagValue.length; m++) {
							dateString += Integer.toHexString((tagValue[m]&0x000000ff)|0xffffff00).substring(6);
						}
						int point = dateString.indexOf('f');
						if (point > 0)
							dateString = dateString.substring(0, point);
						point = dateString.indexOf('F');
						if (point > 0)
							dateString = dateString.substring(0, point);
						track2String = dateString.trim();
					}	
				}
				
				if (panSequenceNoString != null && track2String != null && cardNoString != null && holderNameString != null && holderIdString != null && expireDateString != null) 
					return cardNoString + "," + panSequenceNoString + "," + holderNameString + "," + holderIdString + "," + expireDateString + "," + track2String;
			}
		}
		return cardNoString + "," + panSequenceNoString + "," + holderNameString + "," + holderIdString + "," + expireDateString + "," + track2String;
	}


	private void deviceTest() throws Exception
	{
		if (BluetoothThread.getInstance().deviceIsWorking() )
			throw new Exception("设备忙");
		if (!BluetoothThread.getInstance().deviceIsConnecting())
			throw new Exception("设备没有连接");
		
		if (BluetoothThread.getInstance() == null)
			throw new Exception("BluetoothThread未创建");			
	}
}
