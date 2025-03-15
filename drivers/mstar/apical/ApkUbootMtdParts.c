#include <common.h>
#include <command.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"
#include <ubi_uboot.h>
#include <cmd_osd.h>
#include "apicaluboot.h"

typedef struct _APK_MTD_SUB_PARTS
{
	char* strPartName;
	char* strBaseMtdPart;
	unsigned int iOffset;
}APK_MTD_SUB_PARTS;

#define APK_MTD_PART(x)        \
    {                          \
        APK_##x##_NAME,        \
		APK_##x##_BASE_PART,   \
		APK_##x##_OFFSET,	   \
    }                          \

#define APK_MTD_PART_INFO \
    APK_MTD_PART(MTD_PART_UI_AUTH),  \
    APK_MTD_PART(MTD_PART_FACTORY_PARAM),  \
    APK_MTD_PART(MTD_PART_SPEECH_AUTH),  \
    APK_MTD_PART(MTD_PART_SETTINGS),  \
    APK_MTD_PART(MTD_PART_END)  \

//----------------------------------------------------------------
static APK_MTD_SUB_PARTS s_ApkSubMtdParts[] = {APK_MTD_PART_INFO};

#if APK_NOR_FLASH
#include "../partition/part_mxp.h"
int Apk_Nor_GetPart(char* strPartName, mxp_record* pRec)
{
	extern int mxp_init_nor_flash(void);
	int ret=0;
	int idx=0;
	
	if((ret=mxp_init_nor_flash())<0)
	{
		return -1;
	}

    idx=mxp_get_record_index(strPartName);
    if(idx>=0)
    {
        mxp_record rec;
        if(0==mxp_get_record_by_index(idx,&rec))
        {
            //print_mxp_record(0,&rec);
            if(pRec) memcpy(pRec, &rec, sizeof(rec));
			ret = 0;
        }
        else
        {
            printf("apk failed to get MXP record with name: %s\n",strPartName);
            ret = -2;
        }
    }
    else
    {
        printf("apk can not found mxp record: %s\n",strPartName);
        ret = -3;
    }

	return ret;
}
#endif

static char* Apk_UbootMtdMgr_GetBasePart(char* strSubPartName)
{
	int i = 0;
	
	for(i=0; i<MAX_SUB_PARTS; i++)
	{
		if(strcmp(strSubPartName, s_ApkSubMtdParts[i].strPartName) == 0)
			return s_ApkSubMtdParts[i].strBaseMtdPart;
		if(strcmp(APK_MTD_PART_END_NAME, s_ApkSubMtdParts[i].strPartName) == 0)
			return NULL;
	}

	return NULL;
}

static unsigned int Apk_UbootMtdMgr_GetOffSet(char* strSubPartName)
{
	int i = 0;
	
	for(i=0; i<MAX_SUB_PARTS; i++)
	{
		if(strcmp(strSubPartName, s_ApkSubMtdParts[i].strPartName) == 0)
			return s_ApkSubMtdParts[i].iOffset;
		if(strcmp(APK_MTD_PART_END_NAME, s_ApkSubMtdParts[i].strPartName) == 0)
			return 0;
	}

	return 0;
}

int Apk_Uboot_GetStorageSizeBytes(char* strBasePartName)
{
#if APK_NOR_FLASH
	mxp_record rec;
	if(Apk_Nor_GetPart(strBasePartName, &rec) >= 0)
		return rec.size;
	else 
		return 0;
#else
	extern int get_part(const char *partname, int *idx, loff_t *off, loff_t *size, loff_t *maxsize);

	int idx=0;
	loff_t offset, size, maxsize;
	get_part(strBasePartName, &idx, &offset, &size, &maxsize);
	printf("%s: %d\t%d\t%d\t\r\n", strBasePartName, idx, offset, size);
	return size;
#endif
}

int Apk_Uboot_GetStorageOffsetBytes(char* strBasePartName)
{
#if APK_NOR_FLASH
	mxp_record rec;
	if(Apk_Nor_GetPart(strBasePartName, &rec) >= 0)
		return rec.start;
	else 
		return 0;
#else
	extern int get_part(const char *partname, int *idx, loff_t *off, loff_t *size, loff_t *maxsize);

	int idx=0;
	loff_t offset, size, maxsize;
	get_part(strBasePartName, &idx, &offset, &size, &maxsize);
	printf("%s: %d\t%d\t%d\t\r\n", strBasePartName, idx, offset, size);
	return offset;
#endif
}

int Apk_Uboot_MtdRead(char* strSubPartName, unsigned char* pBuffer, int len)
{
	int ret = 0;
	char* strBasePartName = NULL;	//"mtdblock9"
	char* strBasePart = NULL;		//"/dev/mtdblock9"
	unsigned int iOffset = 0;
	unsigned int iBaseOffset = 0;
	int iRetLen = 0;
	int iPartSize =0; 
	char* pData= NULL;
	char cmd[128] = {0};
	
	do
	{
		if(strSubPartName == NULL) {
			ret = -1;
			break;
		}

		strBasePartName = Apk_UbootMtdMgr_GetBasePart(strSubPartName);
		if(strBasePartName == NULL)	{
			ret = -2;
			break;
		}

		iOffset = Apk_UbootMtdMgr_GetOffSet(strSubPartName);
		if(iOffset == 0) {
			ret = -3;
			break;
		}
			
		iBaseOffset = Apk_Uboot_GetStorageOffsetBytes(strBasePartName);
		if(iBaseOffset == 0) {
			ret = -31;
			break;
		}
		
		iPartSize = Apk_Uboot_GetStorageSizeBytes(strBasePartName);
		if(iPartSize == 0) {
			ret = -32;
			break;
		}

		if((iOffset+len)>iPartSize) {
			ret = -33;
			break;
		}
		printf("%s rang:%d~%d, %s read:%d~%d\n", strBasePartName, iBaseOffset, iBaseOffset+iPartSize, strSubPartName, iBaseOffset+iOffset, iBaseOffset+iOffset+len);
		#if APK_NOR_FLASH
		sprintf(cmd, "sf read 0x%x 0x%x 0x%x", pBuffer, iOffset+iBaseOffset, len);
		#else
		sprintf(cmd, "nand read 0x%x 0x%x 0x%x", pBuffer, iOffset+iBaseOffset, len);
		#endif
		printf("run cmd:%s\n", cmd);
		run_command(cmd, 0);
		ret = len;
	}
	while (0);
	
	if(ret < 0)
	{
		printf("read mtd part [%s] ret = %d\n", strSubPartName?strSubPartName:"UNK", ret);
	}

	return ret;
}


int Apk_Uboot_MtdWrite(char* strSubPartName, unsigned char* pBuffer, int len, int bEraseOnly)
{
	int ret = 0;
	char* strBasePartName = NULL;	//"mtdblock9"
	char* strBasePart = NULL;		//"/dev/mtdblock9"
	unsigned int iOffset = 0;
	unsigned int iBaseOffset = 0;
	int iRetLen = 0;
	int iPartSize =0; 
	char cmd[128] = {0};
	
	do
	{
		if(strSubPartName == NULL) {
			ret = -1;
			break;
		}

		strBasePartName = Apk_UbootMtdMgr_GetBasePart(strSubPartName);
		if(strBasePartName == NULL)	{
			ret = -2;
			break;
		}

		iOffset = Apk_UbootMtdMgr_GetOffSet(strSubPartName);
		if(iOffset == 0) {
			ret = -3;
			break;
		}
			
		iBaseOffset = Apk_Uboot_GetStorageOffsetBytes(strBasePartName);
		if(iBaseOffset == 0) {
			ret = -31;
			break;
		}
		
		iPartSize = Apk_Uboot_GetStorageSizeBytes(strBasePartName);
		if(iPartSize == 0) {
			ret = -32;
			break;
		}

		if((iOffset+len)>iPartSize) {
			ret = -33;
			break;
		}
		printf("%s rang:%d~%d, %s write:%d~%d\n", strBasePartName, iBaseOffset, iBaseOffset+iPartSize, strSubPartName, iBaseOffset+iOffset, iBaseOffset+iOffset+len);

		#if APK_NOR_FLASH
		snprintf(cmd, sizeof(tmp) - 1,"sf erase 0x%x 0x%x", iBaseOffset+iOffset, len);
		#else
		snprintf(cmd, sizeof(tmp) - 1,"nand erase 0x%x 0x%x", iBaseOffset+iOffset, len);
		#endif
		printf("run cmd:%s\n", cmd);
		run_command(cmd, 0);

		if(!bEraseOnly) {
			#if APK_NOR_FLASH
			sprintf(cmd, "sf write 0x%x 0x%x 0x%x", pBuffer, iBaseOffset+iOffset, len);
			#else
			sprintf(cmd, "nand write 0x%x 0x%x 0x%x", pBuffer, iBaseOffset+iOffset, len);
			#endif
			printf("run cmd:%s\n", cmd);
			run_command(cmd, 0);
		}
		ret = len;
	}
	while (0);
	
	if(ret < 0)
	{
		printf("[%s] ret = %d\n", strSubPartName?strSubPartName:"UNK", ret);
	}
	
	return ret;
}
