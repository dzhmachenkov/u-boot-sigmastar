#ifndef _APK_UBOOT_H_
#define _APK_UBOOT_H_

#include "apical.h"


//#define APK_NOR_FLASH 	1     //!!!!!!! 根据实际修改
#define MAX_SUB_PARTS	(10)

#if APK_NOR_FLASH
#define FLASH_MIN_ERASE_SIZE	(0x4000)
#else
#define FLASH_MIN_ERASE_SIZE	(0x20000)
//注意mtdblock9的大小， 目前是640K， 只能放5个sub分区
#endif

#define APK_MTD_PART_UI_AUTH_NAME  				"MTD_PART_UI"
#define APK_MTD_PART_UI_AUTH_BASE_PART			"KEY_CUST"
#define APK_MTD_PART_UI_AUTH_OFFSET 			(99)

#define APK_MTD_PART_FACTORY_PARAM_NAME			"MTD_PART_FACTORY"
#define APK_MTD_PART_FACTORY_PARAM_BASE_PART	"KEY_CUST"
#define APK_MTD_PART_FACTORY_PARAM_OFFSET 		(0) //(APK_MTD_PART_UI_AUTH_OFFSET+FLASH_MIN_ERASE_SIZE) 	

#define APK_MTD_PART_SPEECH_AUTH_NAME			"MTD_PART_SPEECH"
#define APK_MTD_PART_SPEECH_AUTH_BASE_PART		"KEY_CUST"
#define APK_MTD_PART_SPEECH_AUTH_OFFSET			(99) //(APK_MTD_PART_FACTORY_PARAM_OFFSET+FLASH_MIN_ERASE_SIZE) 	

#define APK_MTD_PART_SETTINGS_NAME				"MTD_PART_SETTINGS"
#define APK_MTD_PART_SETTINGS_BASE_PART			"KEY_CUST"
#define APK_MTD_PART_SETTINGS_OFFSET			(APK_MTD_PART_FACTORY_PARAM_OFFSET+FLASH_MIN_ERASE_SIZE) //(APK_MTD_PART_SPEECH_AUTH_OFFSET+FLASH_MIN_ERASE_SIZE)

#define APK_MTD_PART_END_NAME				"MTD_PART_END"
#define APK_MTD_PART_END_BASE_PART			"END"
#define APK_MTD_PART_END_OFFSET				(0)


#endif //_APK_UBOOT_H_
