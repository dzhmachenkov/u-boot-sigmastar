#include <common.h>
#include <command.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"
#include <ubi_uboot.h>
#include <cmd_osd.h>
#include "apicaluboot.h"

extern int Apk_Uboot_MtdRead(char* strSubPartName, unsigned char* pBuffer, int len);
extern int Apk_Uboot_MtdWrite(char* strSubPartName, unsigned char* pBuffer, int len, int bEraseOnly);

#define SETTINGS_ENTRY_ARROW_SIZE		48  //sync kernel apkhal
typedef struct _apk_entry {  //修改此结构同时需要修改驱动中的结构
    char pkey[SETTINGS_ENTRY_ARROW_SIZE];
    char pval[SETTINGS_ENTRY_ARROW_SIZE];
} APKENTRY,*PAPKENTRY;
typedef struct _apk_entryEx {  //修改此结构同时需要修改驱动中的结构
    int len;
    char pval[SETTINGS_ENTRY_ARROW_SIZE];
} APKENTRY_EX,*PAPKENTRY_EX;
#define NVRAM_MAGIC_S       (('A')|('P'<<8)|('K'<<16)|('S'<<24))
#define NVRAM_MAGIC_E       (('S')|('K'<<8)|('P'<<16)|('A'<<24))
#pragma pack(1) 
typedef struct _apk_nvconfhdr {  //修改此结构同时需要修改驱动中的结构
    int magicnumS;
    int crcnum;
    int len;
    int magicnumE;
	char newLine; //always \n
} APKNVHDR, *PAPKNVHDR;
#pragma pack() 
static APKNVHDR s_NvHdr = {0};

static const unsigned char s_crc8_table[256] = {
	0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
	0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
	0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
	0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
	0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
	0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
	0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
	0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
	0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
	0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
	0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
	0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
	0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
	0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
	0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
	0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
	0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
	0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
	0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
	0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
	0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
	0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
	0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
	0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
	0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
	0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
	0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
	0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
	0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
	0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
	0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
	0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

unsigned char Apk_Uboot_CheckCrc8 (
	unsigned char * pdata,  /* pointer to array of data to process */
	unsigned int nbytes,  	/* number of input data bytes to process */
	unsigned char crc       /* either CRC8_INIT_VALUE or previous return value */
) {
	while (nbytes-- > 0)
	{
		crc = s_crc8_table[(crc ^ *pdata++) & 0xff];
	}
	return crc;
}

static char* UbootGetNextLine(char* pData)
{
	char* pLineEnd = NULL;
	char* pLineS = pData;
	
	if(!pData)
		return NULL;
	if(pData[0] == 0 || pData[0] == ';')
		return pData;

	pLineEnd = strchr(pData, '\n');
	if(pLineEnd)
	{
		*pLineEnd = 0;
		pData = pLineEnd+1;
		return pLineS;
	}
	return NULL;
}

static int ParseEntryByLine(char* pLine, char* key, char* pOutVal, int isize)
{
	char* pKey = NULL;
	char* pVal = NULL;
	char* pTmp = NULL;

	//1 skip ' '
	while(*pLine == ' ') 
	{
		pLine++;
		if(*pLine == 0)
			return -1;
	}

	//2 去掉注释
	pTmp = strchr(pLine, ';');
	if(pTmp) *pTmp = 0;
		
	//3. get key
	if(*pLine == ';')
		return 1;
	if(*pLine == '#')
		return 1;
	
	pKey = pLine;
	pTmp = strchr(pLine, '=');
	if(*pTmp)
	{
		*pTmp = 0;
		pVal = pTmp+1;
	}

	//4 skip ' '
	pTmp = pVal;
	while(*pTmp == ' ') 
	{
		pTmp++;
		if(*pTmp == 0)
			break;
	}
	if(strcmp(pKey, key) == 0)
	{
		strncpy(pOutVal, pVal, isize);
		pOutVal[isize] = 0;
		printf("uboot setings get key:%s = %s\n", key , pVal);
		return 0;
	}
	
	return 2;
}

static int UbootParserSettings(char* pData, int iLen, char* key, char* pVal, int isize)
{
	unsigned char crc;
	char* pstr = NULL;
	char* pLine = NULL;
	int len = 0;
	int iLenSettings = iLen-sizeof(APKNVHDR);
	int ret = 0;
	
	memcpy(&s_NvHdr, pData, sizeof(APKNVHDR));
	len += sizeof(APKNVHDR);
	if((s_NvHdr.magicnumS != (int)NVRAM_MAGIC_S) || (s_NvHdr.magicnumE != (int)NVRAM_MAGIC_E))
	{
		printf("UbootParserSettings: magicnum error!\n");
		return -1;
	}
	pstr = pData+sizeof(APKNVHDR);
	crc = Apk_Uboot_CheckCrc8((unsigned char*)pstr, iLenSettings, 0xFF);
	if(crc != s_NvHdr.crcnum)
	{
		printf("UbootParserSettings: crc error[%x != %x] len[%d,%d]!\n", crc, s_NvHdr.crcnum, s_NvHdr.len, iLen-sizeof(APKNVHDR));
        return -2;
	}

	while(1)
	{
		if(pstr[0]==';')
			break;
		pLine = UbootGetNextLine(pstr);
		if(pLine == NULL)
			break;
		pstr+=strlen(pLine)+1;
		if(pLine)
		{
			len += strlen(pLine)+1;
			ret = ParseEntryByLine(pLine, key, pVal, isize);
			if(ret == 0)
				break;
			if(len >= iLenSettings || pstr[0]==';')
				break;
		}
		else
			break;
	}
	return 0;
}

int Apk_Uboot_Setings_GetStrVal(char* key, char* pVal, int isize)
{
	unsigned char* pData = NULL;
	int len = 0;
	int ret = 0;
	APKNVHDR lNvHdr={0};
	
	do 
	{		
		len = Apk_Uboot_MtdRead(APK_MTD_PART_SETTINGS_NAME, (unsigned char*)&lNvHdr, sizeof(APKNVHDR));
		if(len!=sizeof(APKNVHDR))
		{
			ret = -2;
			break;
		}
		if((lNvHdr.magicnumS != (int)NVRAM_MAGIC_S) || (lNvHdr.magicnumE != (int)NVRAM_MAGIC_E) || (lNvHdr.len > (10<<10)))
		{
			ret = -21;
			break;
		}

		len = sizeof(APKNVHDR)+lNvHdr.len;
		pData = (char*)malloc(len+1);
		memset(pData, 0, len+1);
		if(Apk_Uboot_MtdRead(APK_MTD_PART_SETTINGS_NAME, pData, len)!=len)
		{ 
			ret = -3;
			break;
		}
		pData[len] = 0;
		ret = UbootParserSettings(pData, len, key, pVal, isize);
	}while(0);
	
	if(pData)
		free(pData);
	
	if(ret)
		printf("Apk_Uboot_Setings_GetStrVal ret = %d\n", ret);
		
	return ret;
}

int Apk_Uboot_EraseSettings()
{
	Apk_Uboot_MtdWrite(APK_MTD_PART_SETTINGS_NAME, (unsigned char*)NULL, sizeof(APKNVHDR), 1);
}
int Apk_Uboot_EraseUiAuth()
{
	Apk_Uboot_MtdWrite(APK_MTD_PART_UI_AUTH_NAME, (unsigned char*)NULL, sizeof(APKNVHDR), 1);
}

int Apk_Uboot_Settings_init()
{
#if APK_NOR_FLASH
	run_command("sf probe 0", 0);
#else
	
#endif
	return 0;
}
