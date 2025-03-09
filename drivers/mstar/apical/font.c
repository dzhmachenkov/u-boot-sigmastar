#include <common.h>
#include <command.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"
#include <ubi_uboot.h>
#include <cmd_osd.h>

#include "draw2d.h"
#include "font.h"

//#include "F28B_ASCII.c"

#define _FONT_DEBUG  1

static GUI_FONT* _cur_font  = NULL;
static u32 s_txtColros=0;
static u32 s_bgColros=0;

//----------------------------------------------
GUI_CHARINFO* ApkFontFindCharInfo(GUI_FONT_PROP* pProp, char asc)
{
	int ret = 0;
	GUI_CHARINFO* pCharInfo;
	
	do
	{
		if(!pProp)
		{
			ret = -1;
			break;
		}
		
		//printf("ApkFontFindCharInfo:%c(0x%x) [0x%x, 0x%x]\r\n", asc, asc, pProp->First, pProp->Last);
		if((asc>=pProp->First) && (asc<=pProp->Last))
		{
			pCharInfo = pProp->paCharInfo + asc - pProp->First;
			//printf("ApkFontFindCharInfo got:%d, %d\r\n", pCharInfo->XDist, pCharInfo->BytesPerLine);
		}
		else
		{
			printf("ApkFontFindCharInfo find next:%d (%d,%d)\r\n", asc, pProp->First, pProp->Last);
			ApkFontFindCharInfo(pProp->pNext, asc);
		}
	}while(0);

	if(ret) return NULL;
	return pCharInfo;
}
GUI_CHARINFO* ApkFontFindCharInfoEx(char asc)
{
	int ret = 0;
	GUI_CHARINFO* pCharInfo = NULL;
	
	do
	{
		if(!_cur_font)
		{
			ret = -1;
			break;
		}

		//printf("ApkFontFindCharInfoEx: %c\r\n", asc);
		pCharInfo = ApkFontFindCharInfo(&_cur_font->prop, asc);
		if(pCharInfo == NULL)
		{
			ret = -2;
			break;
		}
			
	}while (0);
	
	if(ret)
	{
		printf("can not get char info of:%c(%x)\r\n", asc, asc);
		return NULL;
	}

	return pCharInfo;
}


//+++++++++++++++++++++++++++++++++++++++++
int loadfont(int FontSize)
{
#if 1
	printf("Load font error!\n");
	_cur_font = NULL;
	return TRUE;
#else
	if(FontSize == 28)
		_cur_font  = &GUI_Font28B_ASCII;
	else if(FontSize == 32)
		_cur_font  = &GUI_Font28B_ASCII;  //todo...
	else  //default
		_cur_font  = &GUI_Font28B_ASCII;
    return TRUE;
#endif	
}

void unloadfont()
{
    _cur_font = NULL;
}

void setTxtColor(u32 RGBcolor)
{
	int Y=GetY(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int U=GetU(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int V=GetV(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));	
	Y=YUV_RANG(Y);
	U=YUV_RANG(U);
	V=YUV_RANG(V);
	
	//printf("setTxtColor:%d,%d,%d\r\n", Y,U,V);

	s_txtColros = MAKE_YUV(Y, U, V);
}
u32 getTxtColor()
{
	return s_txtColros;
}

void setBgColor(u32 RGBcolor)
{
	int Y=GetY(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int U=GetU(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int V=GetV(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));	
	Y=YUV_RANG(Y);
	U=YUV_RANG(U);
	V=YUV_RANG(V);
	
	//printf("setBgColor:%d,%d,%d\r\n", Y,U,V);

	s_bgColros = MAKE_YUV(Y, U, V);
}
u32 getBgColor()
{
	return s_bgColros;
}

//返回水平方向画的点数
static int _draw_asc(int x, int y, char asc)
{
	GUI_CHARINFO* pInfo;
	u8* pLine; 
	int w=0, h=0;
	int pointX, pointY;
	u32 linePix;

	if(_cur_font == NULL)
		return 0;
	
	pInfo = ApkFontFindCharInfoEx(asc);
	if(pInfo == NULL)
		return 0;

	for(h=0; h<_cur_font->font_h; h++)
	{
		pLine = pInfo->pData + h*pInfo->BytesPerLine;
		for(w=0; w<pInfo->XDist;)
		{
			linePix = *(pLine+w/8);
			pointX = x + w;
			pointY = y + h;
			if(linePix & (1<<((7-w%8))))
				putpixel(pointX, pointY, s_txtColros);
			else
				putpixel(pointX, pointY, s_bgColros);
			 w++;
		}
	}

	return pInfo->XDist;
}

void ApkShowString(char* str, RECT rect, int TextAlign, int ySpace)
{
	int i=0;
	int iPosX, iPosY;
	GUI_CHARINFO* pInfo;

	if(_cur_font == NULL)
		return;
	
	iPosX = rect.x;
	iPosY = rect.y;
	for(i=0; i<strlen(str); i++)
	{
		if(str[i] == '\r')
			continue;
		if(str[i] == '\n')
		{
			iPosY += _cur_font->font_h + ySpace;
			iPosX = rect.x;
			continue;
		}
		
		pInfo = ApkFontFindCharInfoEx(str[i]);
		if(iPosX < (rect.x+rect.w-pInfo->XDist))
		{
			iPosX += _draw_asc(iPosX, iPosY, str[i]);
		}
		else
		{
			//宽度超过rect范围，自动换行
			iPosY += _cur_font->font_h + ySpace;
			if((iPosY+_cur_font->font_h) > (rect.y+rect.h))
			{
				//高度超出了rect的范围
				return;
			}
			iPosX = rect.x;
			pInfo = ApkFontFindCharInfoEx(str[i]);
			if(iPosX < (rect.x+rect.w-pInfo->XDist))
			{
				iPosX += _draw_asc(iPosX, iPosY, str[i]);
			}
		}
	}
}

void ShowDialog(int x, int y, int w, int h, char *txt, int mode, int percent)
{
}

extern u32 g_VirDispW;
extern u32 g_VirDispH;
void ShowProgressBar(char* strinfo, char dwBarPos)
{
	static int bInited = 0;
	RECT rectTxt={20, 360, 800, 38};
	char str[128]={0};
	int progx = 0;
	static int runMaxProg=1;
	extern void drawProgBar(int x, int y, int h, DWORD bgRGBcolor, DWORD RGBcolor, int prog);

	//printf("enter ShowProgressBar +++++++++++:%s\r\n", strinfo);
	rectTxt.y = g_VirDispH-g_VirDispH/6 - 40;
	rectTxt.w = g_VirDispW;
	
	if(bInited == 0)
	{
		ApkDrawInit();
		loadfont(28);
		if(g_VirDispH && g_VirDispW)
			bInited = 1;
		
		setbufmode(1);
		//clearscreen(0x0);
		copyScreen();
	}
	
	setBgColor(0);
	setTxtColor(0xFFFFFF);
	setcolor(0);
	if(runMaxProg)
		bar(rectTxt.x, rectTxt.y, rectTxt.w, rectTxt.h);

	if(dwBarPos > 200)
		sprintf(str, "%s", strinfo);
	else
		sprintf(str, "%s:%d\%", strinfo, dwBarPos);
	ApkShowString(str, rectTxt, 0, 10);
	if(dwBarPos == 100)
		runMaxProg = 1;
	else
		runMaxProg = 0;

	drawProgBar(0, g_VirDispH-g_VirDispH/6, g_VirDispH/6, 0, 0x2BD591, dwBarPos);
	
	postbackbuf();
}	

