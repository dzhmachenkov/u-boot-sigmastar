#include <common.h>
#include <command.h>
#include <config.h>
#include <malloc.h>

#include "MsTypes.h"
#include "../flash_isp/drvSERFLASH.h"
#include "../flash_isp/drvDeviceInfo.h"
#include "../flash_isp/infinity6b0/regSERFLASH.h"
#include "../flash_isp/infinity6b0/halSERFLASH.h"

#include "apical.h"

extern hal_SERFLASH_t* Apk_Get_SpiFlash_info();
extern int APK_SPI_Flash_Cmd(unsigned char cmd, int bRead, char* pData, int iSize);
extern int Apk_Uboot_GetStorageSizeBytes(char* strBasePartName);
extern int Apk_Uboot_GetStorageOffsetBytes(char* strBasePartName);

int Apk_GetFlash_SizeBytes()
{
	hal_SERFLASH_t *pFlashInfo = NULL;	
	
	//1. get nand info
	pFlashInfo = Apk_Get_SpiFlash_info();
	return pFlashInfo->u32FlashSize;
}

#if APK_NOR_FLASH
#define BYTES_1M	(1<<20)
#define BYTES_4M	(4<<20)
#define BYTES_8M	(8<<20)
#define BYTES_16M	(16<<20)

#if 1 //WPS
//WPS: status3 bit3
int Apk_WinBand_Check_WPSBit()
{
	unsigned char ubStatus = 0;
	
	APK_SPI_Flash_Cmd(0x15, 1, &ubStatus, 1);
	printf("Apk_WinBand_Check_WPSBit:0x%x\r\n", ubStatus);
	return (ubStatus&0x04);
}
int Apk_WinBand_Set_WPSBit(int bLock)
{
	unsigned char ubStatus = 0;

	APK_SPI_Flash_Cmd(0x15, 1, &ubStatus, 1);
	if(bLock) {
		ubStatus |= 0x4;
	} else {
		ubStatus &= ~0x4;
	}
	printf("Apk_WinBand_Check_WPSBit:0x%x\r\n", ubStatus);
	
	return APK_SPI_Flash_Cmd(0x11, 0, &ubStatus, 1);
}

int Apk_WinBand_Set_GlobalLock(int bLock)
{
	int ret = 0;
	
	if(bLock) {
		ret = APK_SPI_Flash_Cmd(0x7E, 0, NULL, 0);
	} else {
		ret = APK_SPI_Flash_Cmd(0x98, 0, NULL, 0);
	}
	
	return ret;
}
int Apk_WinBand_Set_BlockLock(unsigned int Addr, int bLock)
{
	int ret = 0;
	char cData[4] = {0};

	cData[0] = (Addr>>16) &0xff;
	cData[1] = (Addr>>8) &0xff;
	cData[2] = (Addr>>0) &0xff;
	
	if(bLock) {
		ret = APK_SPI_Flash_Cmd(0x36, 0, cData, 3);
	} else {
		ret = APK_SPI_Flash_Cmd(0x39, 0, cData, 3);
	}
	
	return ret;
}

int Apk_Flash_Wps_ProtectWhole(int bLock)
{
	hal_SERFLASH_t *pFlashInfo = NULL;	
	int ret = 0;
	
	//1. get nand info
	pFlashInfo = Apk_Get_SpiFlash_info();
	if(pFlashInfo->u32FlashSize < BYTES_8M) {
		printf("spi flash too small!\n");
		return -1;
	}
	
	if(pFlashInfo->u8MID != MID_WB) {
		printf("now only WINBAND can support protect!\n");
		return -2;
	}
	printf("WPS: FLAHS:pagesize:%d, cap:%d!\n", pFlashInfo->u32SecSize, pFlashInfo->u32FlashSize);

	//2. check WPS bit
	if(bLock) {
		if(!Apk_WinBand_Check_WPSBit()) {
			ret = Apk_WinBand_Set_WPSBit(1);
		}
	} else {
		ret = Apk_WinBand_Set_WPSBit(0);
	}
	
	//3. global lock , must global lock befor part unlock
	ret |= Apk_WinBand_Set_GlobalLock(bLock);

	printf("WPS: Apk_Flash_Wps_ProtectWhole lock:%d ok!\n", bLock);
	return ret;
}

//[UBOOT_ENV]    miservice  KEY_CUST
int Apk_Flash_Wps_ProtectSomePart(E_APK_PART eParts, int bLock)
{
	unsigned int uPartStartAddr = 0;
	unsigned int uPartEndAddr = 0;
	unsigned int uPartSize = 0;
	E_APK_PART ePartsRem = eParts;
	E_APK_PART eCurPart = 0;
	int iRet = 0;
	int i=0;

	hal_SERFLASH_t *pFlashInfo = NULL;
	do
	{
		//1. get nand info
		pFlashInfo = Apk_Get_SpiFlash_info();
		if(pFlashInfo->u32FlashSize < BYTES_8M) {
			printf("spi flash too small!\n");
			return -1;
		}
		
		if(pFlashInfo->u8MID != MID_WB) {
			printf("now only WINBAND can support protect!\n");
			return -2;
		}
		printf("WPS: FLAHS:pagesize:%d, cap:%d!\n", pFlashInfo->u32SecSize, pFlashInfo->u32FlashSize);

		//protect whole flash
		//2. check WPS bit
		if(!Apk_WinBand_Check_WPSBit()) {
			iRet = Apk_WinBand_Set_WPSBit(1);
		}
		//3. global lock , must global lock befor part unlock
		iRet |= Apk_WinBand_Set_GlobalLock(1);

		for(i=0; i<16; i++) {
			if(ePartsRem & E_PART_BOOT) {
				//ÔÝ²»Ö§³Ö¶¯Ì¬½âËø£¬¿ÉÒÔÊ¹ÓÃApk_Flash_Wps_ProtectWhole
				printf("unsupport E_PART_BOOT :%x (%x)\n", i, ePartsRem);
				eCurPart = E_PART_BOOT;
				ePartsRem &= ~eCurPart;
				continue;
			} else if(ePartsRem & E_PART_ENV) {
				uPartStartAddr = Apk_Uboot_GetStorageOffsetBytes("UBOOT_ENV");
				uPartSize = Apk_Uboot_GetStorageSizeBytes("UBOOT_ENV");
				uPartEndAddr = uPartStartAddr + uPartSize;
				eCurPart = E_PART_ENV;
				ePartsRem &= ~eCurPart;
			} else if(ePartsRem & E_PART_KERNEL) {
				//ÔÝ²»Ö§³Ö¶¯Ì¬½âËø£¬¿ÉÒÔÊ¹ÓÃApk_Flash_Wps_ProtectWhole
				printf("unsupport E_PART_KERNEL :%x\n", i);
				eCurPart = E_PART_KERNEL;
				ePartsRem &= ~eCurPart;
				continue;
			} else if(ePartsRem & E_PART_ROOTFS) {
				//ÔÝ²»Ö§³Ö¶¯Ì¬½âËø£¬¿ÉÒÔÊ¹ÓÃApk_Flash_Wps_ProtectWhole
				printf("unsupport E_PART_ROOTFS :%x\n", i);
				eCurPart = E_PART_ROOTFS;
				ePartsRem &= ~eCurPart;
				continue;
			} else if(ePartsRem & E_PART_CUSTOMER) {
				//ÔÝ²»Ö§³Ö¶¯Ì¬½âËø£¬¿ÉÒÔÊ¹ÓÃApk_Flash_Wps_ProtectWhole
				printf("unsupport E_PART_CUSTOMER :%x\n", i);
				eCurPart = E_PART_CUSTOMER;
				ePartsRem &= ~eCurPart;
				continue;
			} else if(ePartsRem & E_PART_CONFIG) {
				uPartStartAddr = Apk_Uboot_GetStorageOffsetBytes("miservice");
				uPartSize = Apk_Uboot_GetStorageSizeBytes("miservice");
				uPartEndAddr = uPartStartAddr + uPartSize;
				eCurPart = E_PART_CONFIG;
				ePartsRem &= ~eCurPart;
			} else if(ePartsRem & E_PART_MISC) {
				uPartStartAddr = Apk_Uboot_GetStorageOffsetBytes("MISC");
				uPartSize = Apk_Uboot_GetStorageSizeBytes("MISC");
				uPartEndAddr = uPartStartAddr + uPartSize;
				eCurPart = E_PART_MISC;
				ePartsRem &= ~eCurPart;
			} else if(ePartsRem & E_PART_KEYCUST_UI) {
				uPartStartAddr = Apk_Uboot_GetStorageOffsetBytes("KEY_CUST");
				uPartSize = Apk_Uboot_GetStorageSizeBytes("KEY_CUST");
				uPartEndAddr = uPartStartAddr + KEYCUST_NOR_SUBPART_SIZE;
				eCurPart = E_PART_KEYCUST_UI;
				ePartsRem &= ~eCurPart;
			} else if(ePartsRem & E_PART_KEYCUST_OTH) {
				uPartStartAddr = Apk_Uboot_GetStorageOffsetBytes("KEY_CUST");
				uPartSize = Apk_Uboot_GetStorageSizeBytes("KEY_CUST");
				uPartEndAddr = uPartStartAddr + uPartSize;
				uPartStartAddr = uPartStartAddr + KEYCUST_NOR_SUBPART_SIZE;
				eCurPart = E_PART_KEYCUST_OTH;
				ePartsRem &= ~eCurPart;
			} else {
				printf("unsupport part:%x (%x)\n", i, ePartsRem);
				continue;
			}
			
			printf("WPS lock:%d part:%x, addr :[%x~%x]!\n", bLock, eCurPart, uPartStartAddr, uPartEndAddr);
			while (uPartStartAddr < uPartEndAddr)
			{
				printf("WPS lock:%d part:%x, addr :%x!\n", bLock, eCurPart, uPartStartAddr);
				if (Apk_WinBand_Set_BlockLock(uPartStartAddr, bLock))
				{
					iRet = 4;
					printf("Apk_Flash_Wps_ProtectSomePart lock[%d] error:%d!\n", bLock, iRet);
					break;
				}
				uPartStartAddr += pFlashInfo->u32SecSize;
			}
		}		
	}while(0);

	printf("Apk_Flash_Wps_ProtectSomePart lock:%d part:0x%x ok!\n", bLock, eParts);
	
	return 0;
}
#endif

#if 1 //·ÇWPSÐ´±£»¤Ä£Ê½
//reg status1 bit6~bit2
//Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 Bit0
//SRP0 BP4	BP3  BP2  BP1  BP0	WEL  WIP   //GD
//SRP  SEC	TB	 BP2  BP1  BP0	WEL  BUSY  //WINBOND
unsigned char Apk_SpiNor_ReadStatus1()
{
	unsigned char ubStatus = 0;
	
	APK_SPI_Flash_Cmd(0x5, 1, &ubStatus, 1);
	printf("Apk_SpiNor_ReadStatus1:0x%x\r\n", ubStatus);
	return ubStatus;
}
unsigned char Apk_SpiNor_WriteStatus1(unsigned char ubStatus)
{
	int ret = 0;
	
	ret = APK_SPI_Flash_Cmd(0x1, 0, &ubStatus, 1);
	printf("Apk_SpiNor_WriteStatus1:0x%x, ret=%d\r\n", ubStatus, ret);
	return ret;
}
unsigned char Apk_SpiNor_ReadStatus2()
{
	unsigned char ubStatus = 0;
	
	APK_SPI_Flash_Cmd(0x35, 1, &ubStatus, 1);
	printf("Apk_SpiNor_ReadStatus2:0x%x\r\n", ubStatus);
	return ubStatus;
}
int Apk_SpiNor_WriteStatus2(unsigned char ubStatus)
{
	int ret = 0;

	//æ­¦æ±‰æ–°èŠ¯flash ä¸å…è®¸å†™ LB3 LB2 LB1 ä¸‰ä¸ªä½ï¼Œè¿™ä¸‰ä¸ªä½æ˜¯ä¸€æ¬¡ç¼–ç¨‹ï¼Œä¸€æ—¦å†™äº†flashå°†ä¸€ç›´å¤„äºŽå†™ä¿æŠ¤æ— æ³•è§£é”
	unsigned char checkNum = ubStatus >> 3;
	if(checkNum <= 0x7 && checkNum > 0)
	{
		printf("set error!!!!cant set LB3-LB1!!!!\n");
		return -1;
	}
		
	ret = APK_SPI_Flash_Cmd(0x31, 0, &ubStatus, 1);
	printf("Apk_SpiNor_WriteStatus2:0x%x, ret=%d\r\n", ubStatus, ret);
	return ret;
}

int Apk_SpiNor_Lock_Except_High512K(int bLock, int iCapMB)
{
	int ret = 0;
	unsigned char ubStatus = 0;

	ubStatus = Apk_SpiNor_ReadStatus2();
	if(!(ubStatus & (1<<6))) {
		ubStatus |= (1<<6);  //set CMP=1
		ret = Apk_SpiNor_WriteStatus2(ubStatus);
	}

	//æ­¦æ±‰æ–°èŠ¯flash
	//01h  SRP0 SEC TB BP2 BP1 BP0 WEL BUSY
	//	   0     0    0   1   1    0  0   0		8å…†
	//	   0     0    0   1   1    1  0   0   å…³
	//    0      0    0   0   0    0  0   0   16å…†
	//31h SUS CMP LB3 LB2 LB1 R QE SRP1
	//     0    1    0   0   0  0  0    0
	//11h HOLD/RST DRV1 DRV0
	Apk_SpiNor_ReadStatus2();
	ubStatus = Apk_SpiNor_ReadStatus1();

	if(bLock) {
		ubStatus &= ~(0x3F<<2);  //SRP  SEC  TB   BP2  BP1  BP0 all=0
		if(iCapMB == BYTES_8M) {
			//SEC TB BP2 BP1 BP0
			// 0  0   1   1   0
			ubStatus |= 0x3<<3;
		} else if(iCapMB == BYTES_16M) {
			//SEC TB BP2 BP1 BP0
			// 0  0   0   0   0
			//ubStatus |= 0x2<<2;
		}
		printf(">>unlock up 512k\n");
	} else {  
		//unlock all
		//SEC TB BP2 BP1 BP0
		// x  x   1   1   1
		ubStatus &= ~(0x3F<<2);
		ubStatus |= 0x7<<3;  	//lock none
		printf(">>unlock all\n");
	}

	ret = Apk_SpiNor_WriteStatus1(ubStatus);
	return ret;
}

int Apk_Flash_NoWps_ProtectWhole(int bLockAll)
{
	hal_SERFLASH_t *pFlashInfo = NULL;	
	unsigned char ubStatus = 0;
	int ret = 0;

	pFlashInfo = Apk_Get_SpiFlash_info();
	if(pFlashInfo->u32FlashSize < BYTES_8M) {
		printf("spi flash too small!\n");
		return -1;
	}
	if(Apk_WinBand_Check_WPSBit()) {
		//Èç¹ûwpsËø¶¨£¬ÔòÏÈ½âËøwps
		Apk_WinBand_Set_WPSBit(0);
		Apk_WinBand_Set_GlobalLock(0);
	}
	
	ubStatus = Apk_SpiNor_ReadStatus2();
	if(!(ubStatus & (1<<6))) {
		ubStatus |= (1<<6);  //set CMP=1
		ret = Apk_SpiNor_WriteStatus2(ubStatus);
	}

	/*  Ëø¶¨×´Ì¬¼Ä´æÆ÷£¬ÐèÒªÖØÆôºó²ÅÄÜ½âËø
	if(!(ubStatus & (1<<0))) {
		ubStatus |= (1<<0);  //set SRP1=1
		ret = Apk_SpiNor_WriteStatus2(ubStatus);
	}
	*/
	
	ubStatus = Apk_SpiNor_ReadStatus2();
	printf("Apk_Flash_NoWps_ProtectWhole FLAHS:pagesize:%d, cap:%d, CMP=%d, bLockAll=%d!\n", pFlashInfo->u32SecSize, pFlashInfo->u32FlashSize, !!(ubStatus&(1<<6)), bLockAll);

	ubStatus = Apk_SpiNor_ReadStatus1();
	ubStatus &= ~(0x3F<<2);  //SRP	SEC  TB   BP2  BP1	BP0 all=0
	if(!bLockAll) {
		//SEC TB BP2 BP1 BP0
		// x  x   1   1   1
		ubStatus |= 0x7<<2;  //lock none
		printf(">>lock none\n");
	} else {
		//SEC TB BP2 BP1 BP0
		// x  x   0   0   0
		//lock all
		printf(">>lock all\n");
	}
	ret = Apk_SpiNor_WriteStatus1(ubStatus);
	
	printf("Apk_Flash_NoWps_ProtectWhole lock:%d ok!\n", bLockAll);
	return ret;
}

//²ÎÊýÎÞÐ§£¬¹Ì¶¨Ð´±£»¤²¢½âËø×î¸ß512kµÄÐ´±£»¤
int Apk_Flash_NoWps_ProtectSomePart(E_APK_PART eParts, int bLock)
{
	//³ý×î¸ß512kÍâ£¬È«²¿Ð´±£»¤
	hal_SERFLASH_t *pFlashInfo = NULL;	
	unsigned char ubStatus = 0;

	/*if(bLock) { //for test
		ubStatus = Apk_SpiNor_ReadStatus2();
		//  Ëø¶¨×´Ì¬¼Ä´æÆ÷£¬ÐèÒªÖØÆôºó²ÅÄÜ½âËø
		if(!(ubStatus & (1<<0))) {
			ubStatus |= (1<<0);  //set SRP1=1
			Apk_SpiNor_WriteStatus2(ubStatus);
		}
		return 0;
	}*/

	pFlashInfo = Apk_Get_SpiFlash_info();
	if(pFlashInfo->u32FlashSize < BYTES_8M) {
		printf("spi flash too small!\n");
		return -1;
	}
	
	if(Apk_WinBand_Check_WPSBit()) {
		//Èç¹ûwpsËø¶¨£¬ÔòÏÈ½âËøwps
		Apk_WinBand_Set_WPSBit(0);
		Apk_WinBand_Set_GlobalLock(0);
	}

	ubStatus = Apk_SpiNor_ReadStatus2();
	printf("Apk_Flash_NoWps_ProtectSomePart FLAHS:pagesize:%d, cap:%d, CMP=%d!\n", pFlashInfo->u32SecSize, pFlashInfo->u32FlashSize, !!(ubStatus&(1<<6)));
	
	return Apk_SpiNor_Lock_Except_High512K(bLock, pFlashInfo->u32FlashSize);
}
#endif

int Apk_Flash_ProtectWhole(int bLock)
{
	if(Apk_Flash_Wps_ProtectWhole(bLock) < 0)
		return Apk_Flash_NoWps_ProtectWhole(bLock);
	if(bLock == 0) {
		Apk_Flash_NoWps_ProtectWhole(bLock);
	}
	return 0;
}

int Apk_Flash_ProtectSomePart(E_APK_PART eParts, int bLock)
{
	if(Apk_Flash_Wps_ProtectSomePart(eParts, bLock) < 0)
		return Apk_Flash_NoWps_ProtectSomePart(eParts, bLock);
	return 0;
}

#else  //SPINAND
int Apk_Flash_ProtectWhole(int bLock)
{
	return 0;
}

int Apk_Flash_ProtectSomePart(E_APK_PART eParts, int bLock)
{
	return 0;
}
#endif
