/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#define _HAL_DISP_IRQ_C_

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/string.h>

#include "drv_disp_os.h"
#include "hal_disp_common.h"
#include "disp_debug.h"
#include "hal_disp_util.h"
#include "hal_disp_reg.h"
#include "hal_disp_irq.h"
#include "hal_disp_chip.h"
#include "hal_disp_st.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HAL_DISP_IRQ_VSYNC_BIT (0x0010)      // SCL_IRQ (19)
#define HAL_DISP_ORQ_VSYNC_MSK (0x0010)

#define HAL_DISP_IRQ_MASK_REG  (REG_SCL0_49_L)
#define HAL_DISP_IRQ_FLAG_REG  (REG_SCL0_52_L)
#define HAL_DISP_IRQ_CLEAR_REG (REG_SCL0_11_L)
//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

__inline void _HalDispIrqEnableIsr(HalDispIrqType_e enType, bool *pbEn)
{
    u16 u16Val, u16Msk;
    bool bEnable = *pbEn;

    u16Val = 0;
    u16Msk = 0;

    if(enType & E_HAL_DISP_IRQ_TYPE_VSYNC)
    {
        u16Val |= bEnable ?  0 : HAL_DISP_IRQ_VSYNC_BIT;
        u16Msk |= HAL_DISP_ORQ_VSYNC_MSK;
    }

    W2BYTEMSK(HAL_DISP_IRQ_MASK_REG, u16Val, u16Msk);
}


__inline void _HalDispIrqGetFlag(HalDispIrqType_e enType, u32 *pu32Flag)
{
    u16 u16Val;

    *pu32Flag = 0;

    u16Val = R2BYTE(HAL_DISP_IRQ_FLAG_REG);

    if(enType & E_HAL_DISP_IRQ_TYPE_VSYNC)
    {
        *pu32Flag |= (u16Val & HAL_DISP_ORQ_VSYNC_MSK) ? E_HAL_DISP_IRQ_TYPE_VSYNC : 0;
    }
}


__inline void _HalDispIrqClear(HalDispIrqType_e enType)
{
    u16 u16Val, u16Msk;

    u16Val = 0;
    u16Msk = 0;
    if(enType & E_HAL_DISP_IRQ_TYPE_VSYNC)
    {
        u16Val |= HAL_DISP_IRQ_VSYNC_BIT;
        u16Msk |= HAL_DISP_ORQ_VSYNC_MSK;
    }

    W2BYTEMSK(HAL_DISP_IRQ_CLEAR_REG, u16Val, u16Msk);
}

__inline void _HalDispIrqGetId(u8 *pu8Id)
{
    *pu8Id = 0;
}

//------------------------------------------------------------------------------
// Internal Isr
//------------------------------------------------------------------------------
__inline void _HalDispIrqSupportedTimeZoneIsr(bool *pbEnable)
{
    *pbEnable = HAL_DISP_TIMEZONE_ISR_SUPPORT;
}

__inline void _HalDispIrqGetTimeZoneIsrDeviceIdx(u8 *pu8Id)
{
    *pu8Id = HAL_DISP_DEVICE_IRQ_TIMEZONE_ISR_IDX;
}


__inline void _HalDispIrqGetTimeZOneIsrStatus(u32 *pu32Status)
{
    *pu32Status = 0;
}


//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

bool HalDispIrqIoCtl(HalDispIrqIoctlConfig_t *pCfg)
{
    bool bRet = 1;

    DISP_DBG(DISP_DBG_LEVEL_IRQ, "%s %d, Ioctl:%s, IrqType:%s\n", __FUNCTION__, __LINE__, PARSING_HAL_IRQ_IOCTL(pCfg->enIoctlType), PARSING_HAL_IRQ_TYPE(pCfg->enIrqType));
    switch(pCfg->enIoctlType)
    {
        case E_HAL_DISP_IRQ_IOCTL_ENABLE:
            _HalDispIrqEnableIsr(pCfg->enIrqType, (bool *)pCfg->pParam);
            break;

        case E_HAL_DISP_IRQ_IOCTL_GET_FLAG:
            _HalDispIrqGetFlag(pCfg->enIrqType,(u32 * )pCfg->pParam);
            break;

        case E_HAL_DISP_IRQ_IOCTL_CLEAR:
            _HalDispIrqClear(pCfg->enIrqType);
            break;

        case E_HAL_DISP_IRQ_IOCTL_GET_ID:
            _HalDispIrqGetId((u8 *)pCfg->pParam);
            break;

        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_SUPPORTED:
            _HalDispIrqSupportedTimeZoneIsr((bool *)pCfg->pParam);
            break;

        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_GET_ID:
            _HalDispIrqGetTimeZoneIsrDeviceIdx((u8 *)pCfg->pParam);
            break;

        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_ENABLE:
        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_GET_FLAG:
        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_CLEAR:
            bRet = FALSE;
            DISP_ERR("%s %d, Not Support Irq Iocl:%s\n", __FUNCTION__, __LINE__,  PARSING_HAL_IRQ_IOCTL(pCfg->enIoctlType));
            break;

        case E_HAL_DISP_IRQ_IOCTL_TIMEZONE_GET_STATUS:
            _HalDispIrqGetTimeZOneIsrStatus((u32 *)pCfg->pParam);
            break;

        default:
            bRet = FALSE;
            DISP_ERR("%s %d, UnKnown Irq Iocl:%d\n", __FUNCTION__, __LINE__, pCfg->enIoctlType);
            break;
    }
    return bRet;
}

