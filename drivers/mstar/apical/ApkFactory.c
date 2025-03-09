#include <common.h>
#include <command.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"
#include <ubi_uboot.h>
#include <cmd_osd.h>

#define USE_LFS 0

extern int ApkIsFileExist(char* filename);
extern int ApkLfsWriteBuffer(char* partName, char* fileName, int mode, char* buffer, int len, int offset);
char g_FactoryCmdLine[32] = {0};

static int atoi(char *str)
{
    int value = 0;

    while(*str >= '0' && *str <= '9')
    {
        value *= 10;
        value += *str - '0';
        str++;
    }

    return value;
}

void ApkAddToCmdLine(char* pCmdline, char *str)
{
	if(pCmdline && str)
	{
		if(str[0]) {
			strcat(pCmdline, " ");
			strcat(pCmdline, str);
		}
	}
}

char* ApkGetFactoryCmdLine()
{
	return g_FactoryCmdLine;
}

static int SetFactoryBootArgs(char* args)
{
	strncpy(g_FactoryCmdLine, args, sizeof(g_FactoryCmdLine)-1);
	g_FactoryCmdLine[sizeof(g_FactoryCmdLine)-1] = 0;
    return 0;
}
static int ClrFactoryBootArgs()
{
	g_FactoryCmdLine[0] = 0;
    return 0;
}

int ApkReadFile(const char* filename, char* pStr, unsigned int size)
{
	char tmp[64];

    memset(tmp,0,sizeof(tmp));
    snprintf(tmp, sizeof(tmp) - 1,"fatload mmc 0 0x%x %s 0x%x 0x0", (U32)pStr, filename, size);
    if(run_command(tmp, 0)){
		printf("ApkReadFile:%s failed!\r\n", filename);
        return 0;
    }

	printf("ApkReadFile:%s ok!\r\n", filename);
	return 1;
}

static int ApkGetFactoryParam(const char* szOrgParam, const char* szKey, unsigned int* puParam)
{
	char szParam[1024] = {0};
	char* pParam = NULL, *pParamEnd = NULL;
	int bRet = FALSE;
	unsigned char uRet = 0;
	int nParamGet = 0;

	strncpy(szParam, szOrgParam, sizeof(szParam));

	do
	{
		pParam = strstr(szParam, szKey);
		if (pParam == NULL)
		{
			uRet = 1;
			break;
		}
		
		pParamEnd = strchr(pParam, '\\');
		if (pParamEnd == NULL)
		{
			uRet = 2;
			break;
		}
		*pParamEnd = '\0';

		pParam = strchr(pParam, '=');
		if (pParam == NULL)
		{
			uRet = 3;
			break;
		}

		pParam++;
		if (pParam >= pParamEnd)
		{
			uRet = 4;
			break;
		}

		nParamGet = atoi(pParam);

		*puParam = (unsigned int)nParamGet;
	}while(0);

	if (uRet)
	{
		bRet = FALSE;
		printf("[APK]ApkGetFactoryParam Get %s Failed!uRet=%d\r\n", szKey, uRet);
	}
	else
	{
		//printf("[APK]ApkGetFactoryParam Get %s ok! *puParam=%d\r\n", szKey, *puParam);
		bRet = TRUE;
	}

	return bRet;
}

int ApkChkFactoryMode(void)
{
	unsigned char 	uRet = 0;
	unsigned char	uReadBuf[1024] = {0};
	unsigned char	uWriteBuf[128] = {0};
	unsigned char	i = 0;
	unsigned int g_uWSMode = 0;
	unsigned int g_uMicVol = 0;
	unsigned int g_uSpkVol = 0;
	unsigned int g_uTestOverAct = 0;
	
	const char* szParamKey[] = 
	{
		"SelWS", 					//—°‘Ò≥µº‰ 1:SMT, 2:◊È◊∞, 3:∞¸◊∞
		"TargetMicVol", 			//mic“Ù¡ø
		"TargetSpkVol", 			//spk“Ù¡ø
		"TestOverAct", 				//≤‚ ‘ÕÍ≥…∫Û∂Ø◊˜:1.¡¨Ω”u≈Ã£¨2.ªÿµΩ≤‚ ‘ΩÁ√Ê£¨3.πÿª˙£¨4.≤ª¥¶¿Ì
		NULL
	};

	unsigned int* puParam[] = 
	{
		&g_uWSMode,					\
		&g_uMicVol,					\
		&g_uSpkVol, 				\
		&g_uTestOverAct, 			\
		NULL
	};

	if(ApkIsFileExist("defatw.txt") || ApkIsFileExist("APK_FACTORY_PCCAM.txt"))
	{
		sprintf(uWriteBuf, "fa=focus");
		#if USE_LFS
		ApkLfsReadToBuffer("MISC", "factory.ini", uReadBuf, strlen(uWriteBuf), 0);
		printf("ApkChkFactoryMode: R:%s, W:%s\r\n", uReadBuf, uWriteBuf);
		if(memcmp(uWriteBuf, uReadBuf, strlen(uWriteBuf)))
			ApkLfsWriteBuffer("MISC", "factory.ini", 65, uWriteBuf, strlen(uWriteBuf), 0);
		#else
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		#endif
		return 1;
	}
	if(ApkIsFileExist("isp_api.xml"))
	{
		sprintf(uWriteBuf, "fa=usb0");
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		return 1;
	}
	if(ApkIsFileExist("apical_iqapi.bin"))
	{
		sprintf(uWriteBuf, "fa=IQ");
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		return 1;
	}

	if(ApkIsFileExist("Apk_USB_Serial.txt"))
	{
		sprintf(uWriteBuf, "fa=usbse");
		#if USE_LFS
		ApkLfsReadToBuffer("MISC", "factory.ini", uReadBuf, strlen(uWriteBuf), 0);
		printf("ApkChkFactoryMode: R:%s, W:%s\r\n", uReadBuf, uWriteBuf);
		if(memcmp(uWriteBuf, uReadBuf, strlen(uWriteBuf)))
			ApkLfsWriteBuffer("MISC", "factory.ini", 65, uWriteBuf, strlen(uWriteBuf), 0);
		#else
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		#endif
		return 1;
	}
	if(ApkIsFileExist("Apk_USB_Storage.txt"))
	{
		sprintf(uWriteBuf, "fa=msdc");
		#if USE_LFS
		ApkLfsReadToBuffer("MISC", "factory.ini", uReadBuf, strlen(uWriteBuf), 0);
		printf("ApkChkFactoryMode: R:%s, W:%s\r\n", uReadBuf, uWriteBuf);
		if(memcmp(uWriteBuf, uReadBuf, strlen(uWriteBuf)))
			ApkLfsWriteBuffer("MISC", "factory.ini", 65, uWriteBuf, strlen(uWriteBuf), 0);
		#else
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		#endif
		return 1;
	}
	if(ApkIsFileExist("Apk_Mic_Passed.txt"))
		return ClrFactoryBootArgs();
	
	//1 init factory test param
	{
		g_uWSMode = 0;
		g_uMicVol = 0;
		g_uSpkVol = 0;
		g_uTestOverAct = 0;
	}
	
	do
	{
		ApkReadFile("APK_FACTORY_MODE.txt", uReadBuf, sizeof(uReadBuf));
		
		for (i = 0; ((puParam[i]!=NULL)&&(szParamKey[i]!=NULL)); i++)
		{
			if((puParam[i]==NULL)||(szParamKey[i]==NULL))
				break;
			
			if (ApkGetFactoryParam((const char*)uReadBuf, szParamKey[i], puParam[i]))
			{
				printf("[APK]ApkGetFactoryParam-%s=%d\r\n", szParamKey[i], *puParam[i]);
			}
		}	
	}while(0);

	if(g_uWSMode == 1)  //SMT
	{
		sprintf(uWriteBuf, "fa=f_smt");
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		return 1;
	}
	else if(g_uWSMode)  //ÁªÑË£ÖÁ≠â
	{
		if(ApkIsFileExist("Apk_Mic_Test_Mode.txt"))
		{
			sprintf(uWriteBuf, "fa=audio");
			#if USE_LFS
			ApkLfsReadToBuffer("MISC", "factory.ini", uReadBuf, strlen(uWriteBuf), 0);
			printf("ApkChkFactoryMode: R:%s, W:%s\r\n", uReadBuf, uWriteBuf);
			if(memcmp(uWriteBuf, uReadBuf, strlen(uWriteBuf)))
				ApkLfsWriteBuffer("MISC", "factory.ini", 65, uWriteBuf, strlen(uWriteBuf), 0);
			#else
			printf("%s\r\n", uWriteBuf);
			SetFactoryBootArgs(uWriteBuf);
			#endif
			return 1;
		}
		
		sprintf(uWriteBuf, "fa=assem");
		printf("%s\r\n", uWriteBuf);
		SetFactoryBootArgs(uWriteBuf);
		return 1;
	}
	
	return ClrFactoryBootArgs();
}
