#include <common.h>
#include <command.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"
#include <ubi_uboot.h>
#include <cmd_osd.h>

#include <stdlib.h>
#include "draw2d.h"

#define DISP_ROTATE		90

static BOOL _EN_SCR_BACKBUF = FALSE;
static DWORD _cur_YUVcolor      = 0;

extern u32 ApkDispGetFrameBufferAddr();
extern void ApkDispGetInfo(u32* pW, u32* pH);

/////////////////////////////////////////////////////////////////////////////////////
static u8* g_pdw_v_Temp_Disp_Buff;
static u8* g_pdw_v_Main_Disp_Buff;
static u8* g_pBuffer_for_rotate;

static u8* _CUR_DISP_BUF_BASE;
static u32 DISPLAY_WIDTH=0;
static u32 DISPLAY_HEIGHT=0;
u32 g_VirDispW=0;
u32 g_VirDispH=0;
/////////////////////////////////////////////////////////////////////////////////////

void ApkDrawInit()
{
	if(g_pdw_v_Temp_Disp_Buff)
		return;
	
	g_pdw_v_Main_Disp_Buff = ApkDispGetFrameBufferAddr();
	ApkDispGetInfo(&DISPLAY_WIDTH, &DISPLAY_HEIGHT);
	#if (DISP_ROTATE==90)
	g_VirDispW = DISPLAY_HEIGHT;
	g_VirDispH = DISPLAY_WIDTH;
	#endif
	#if (DISP_ROTATE==270)
	g_VirDispW = DISPLAY_HEIGHT;
	g_VirDispH = DISPLAY_WIDTH;	
	#endif
	#if (DISP_ROTATE==0)
	g_VirDispW = DISPLAY_WIDTH;
	g_VirDispH = DISPLAY_HEIGHT;
	#endif
	
	if(DISPLAY_WIDTH==0 && DISPLAY_HEIGHT==0)
	{
		printf("ApkDrawInit: screen:%d x %d, error!\r\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
		return;
	}
	printf("ApkDrawInit: screen:%d x %d\r\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	g_pdw_v_Temp_Disp_Buff = malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*3/2);
	if(!g_pdw_v_Temp_Disp_Buff)
		printf("ApkDrawInit: alloc temp disp buffer error!\r\n");
	g_pBuffer_for_rotate = malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*3/2);
	if(!g_pBuffer_for_rotate)
		printf("ApkDrawInit: alloc rotate disp buffer error!\r\n");
	printf("ApkDrawInit: g_pdw_v_Temp_Disp_Buff= 0x%x\r\n", g_pdw_v_Temp_Disp_Buff);
	return;
}

void Rotate270_line(u8* dst, u8* src, int w, int h, int line)
{
	int i=0;
	u8* psrc = src+ line*w;
	u8* pdst = dst + h - line -1;
	
	for(i=0; i<w; i++)
	{
		*pdst = *psrc++;
		pdst += h;
	}
}

void YUV420spRotate270(u8 *dst, const u8 *src, int w, int h)
{
	int i=0; 

	for(i=0; i<h*3/2; i++)
	{
		Rotate270_line(dst, src, w, h, i);
	}
}

void Rotate90_line(u8* dst, u8* src, int w, int h, int line)
{
	int i=0;
	u8* psrc = src+ line*w;
	u8* pdst = dst + (w-1)*h + line;
	
	for(i=0; i<w; i++)
	{
		*pdst = *psrc++;
		pdst -= h;
	}
}
void YUV420spRotate90(u8 *dst, const u8 *src, int w, int h)
{
	int i=0; 

	for(i=0; i<h*3/2; i++)
	{
		Rotate90_line(dst, src, w, h, i);
	}
}

void YUV420spRotatePic90(u8 *src, int w, int h)
{
	int i=0; 

	if(g_pBuffer_for_rotate)
	    memcpy((void*)g_pBuffer_for_rotate, (void*)src, w * h * 3 / 2);
	
	for(i=0; i<h*3/2; i++)
	{
		Rotate90_line(src, g_pBuffer_for_rotate, w, h, i);
	}
}

void setcolor(DWORD RGBcolor)
{
	int Y=GetY(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int U=GetU(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int V=GetV(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));	
	Y=YUV_RANG(Y);
	U=YUV_RANG(U);
	V=YUV_RANG(V);
	
	//printf("setcolor:%d,%d,%d\r\n", Y,U,V);
    _cur_YUVcolor = MAKE_YUV(Y, U, V);
}

DWORD getcolor()
{
    return _cur_YUVcolor;
}

void setbufmode(int flag)
{
    _EN_SCR_BACKBUF = flag;
    if (flag) 
		_CUR_DISP_BUF_BASE = (u8*)g_pdw_v_Temp_Disp_Buff;
    else 
		_CUR_DISP_BUF_BASE = (u8*)g_pdw_v_Main_Disp_Buff;
}

void postbackbuf()
{
    if (_EN_SCR_BACKBUF)
    {
    	u8* ptmp = 0;

		#if 0  //use pix map
		if(g_pBuffer_for_rotate)
			ptmp = g_pBuffer_for_rotate;
		else
			ptmp = g_pdw_v_Temp_Disp_Buff;

    	#if(DISP_ROTATE == 90) //旋转90度
		YUV420spRotate90(ptmp, g_pdw_v_Temp_Disp_Buff, DISPLAY_WIDTH, DISPLAY_HEIGHT);
		if(ptmp)
			memcpy((void*)g_pdw_v_Main_Disp_Buff, (void*)ptmp, DISPLAY_WIDTH * DISPLAY_HEIGHT * 3 / 2);
		return;
		#endif
		
		#if(DISP_ROTATE == 270)
		YUV420spRotate270(ptmp, g_pdw_v_Temp_Disp_Buff, DISPLAY_WIDTH, DISPLAY_HEIGHT);
		if(ptmp)
			memcpy((void*)g_pdw_v_Main_Disp_Buff, (void*)ptmp, DISPLAY_WIDTH * DISPLAY_HEIGHT * 3 / 2);
		return;
		#endif
		#endif
		
	    memcpy((void*)g_pdw_v_Main_Disp_Buff, (void*)g_pdw_v_Temp_Disp_Buff, DISPLAY_WIDTH * DISPLAY_HEIGHT * 3 / 2);
    }
}
void copyScreen()
{
	memcpy((void*)g_pdw_v_Temp_Disp_Buff, (void*)g_pdw_v_Main_Disp_Buff, DISPLAY_WIDTH * DISPLAY_HEIGHT * 3 / 2);
}
void clearscreen(DWORD RGBcolor)
{
	int i=0;
	int Y=GetY(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int U=GetU(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	int V=GetV(GetR(RGBcolor), GetG(RGBcolor), GetB(RGBcolor));
	u8* pDst420spY;
	u8* pDst420spUV;
	int YBytes = DISPLAY_WIDTH*DISPLAY_HEIGHT;
	int UVBytes = YBytes/2;
	
	Y=YUV_RANG(Y);
	U=YUV_RANG(U);
	V=YUV_RANG(V);
	printf("clearscreen111:%d,%d,%d\r\n", Y,U,V);
	//Y=128, U=68; V=188;
	pDst420spY = _CUR_DISP_BUF_BASE;
	pDst420spUV = _CUR_DISP_BUF_BASE+DISPLAY_WIDTH*DISPLAY_HEIGHT;

    for(i=0; i<YBytes; i++)
		*pDst420spY++ = Y;
	
    for(i=0; i<UVBytes; i++)
	{
		*pDst420spUV++ = U;
		*pDst420spUV++ = V;
    }
}

void putpixel(int x, int y, DWORD YUVcolor)
{	
	u8 Y=YUV_Y(YUVcolor);
	u8 U=YUV_U(YUVcolor);
	u8 V=YUV_V(YUVcolor);
	u8* pDst420spY;
	u8* pDst420spUV;

	//map pix
	#if (DISP_ROTATE==270)
	int xo=x, yo=y;
	x = DISPLAY_WIDTH-yo;
	y = xo;
	#endif
	#if (DISP_ROTATE==90)
	int xo=x, yo=y;
	x = yo;
	y = DISPLAY_HEIGHT-xo;
	#endif
	
    if (x<0 || x>=DISPLAY_WIDTH || y<0 || y>=DISPLAY_HEIGHT)
	{
		//printf("putpixel out of screen: map=(%d,%d)->(%d,%d)\r\n", xo, yo, x, y);
		return;   
    }
    else 
    {
		pDst420spY = _CUR_DISP_BUF_BASE + y*DISPLAY_WIDTH + x;
		pDst420spUV = _CUR_DISP_BUF_BASE+DISPLAY_WIDTH*DISPLAY_HEIGHT+y/2+x;

		*pDst420spY = Y;
		*pDst420spUV++ = U;
		*pDst420spUV++ = V;
    }
}

DWORD getpixel(int x, int y)
{
	u8* pDst420spY;
	u8* pDst420spU;
	u8* pDst420spV;

	//map pix
	#if (DISP_ROTATE==270)
	int xo=x, yo=y;
	x = DISPLAY_WIDTH-yo;
	y = xo;
	#endif
	#if (DISP_ROTATE==90)
	int xo=x, yo=y;
	x = yo;
	y = DISPLAY_HEIGHT-xo;
	#endif

    if (x<0 || x>=DISPLAY_WIDTH || y<0 || y>=DISPLAY_HEIGHT) 
		return 0;
	else
    {
		pDst420spY = _CUR_DISP_BUF_BASE + y*DISPLAY_WIDTH + x;
		pDst420spU = _CUR_DISP_BUF_BASE+DISPLAY_WIDTH*DISPLAY_HEIGHT+y/2+x;
		pDst420spV = pDst420spU++;
    }

	return RGB32(YUV_2_R(*pDst420spY, *pDst420spU, *pDst420spV), YUV_2_G(*pDst420spY, *pDst420spU, *pDst420spV), YUV_2_B(*pDst420spY, *pDst420spU, *pDst420spV));
}

int abs_sub(int a, int b)
{
    if(a>b) return a-b;
    else return b-a;
}

void line(int x0, int y0, int x1, int y1)
{
    int x;
    int y;
    int dx;
    int dy;
    int e;

    dx = abs_sub(x1, x0);
    dy = abs_sub(y1, y0);
    e  = -dx;

    if (dy<dx)
    {
        if (x0>x1)
        {
            x  = x0;
            x0 = x1;
            x1 = x;
            y  = y0;
            y0 = y1;
            y1 = y;
        }

        y = y0;
        x = x0;
        while (x<=x1)
        {
            putpixel(x, y, _cur_YUVcolor);

            e += dy*2;
            if (e>=0)
            {
                if (y0<y1) y++;
                else y--;
                e -= dx*2;
            }
            x++;
        }
    }
    else
    {
        if (y0>y1)
        {
            x  = x0;
            x0 = x1;
            x1 = x;
            y  = y0;
            y0 = y1;
            y1 = y;
        }

        y = y0;
        x = x0;
        while (y<=y1)
        {
            putpixel(x, y, _cur_YUVcolor);

            e += dx*2;
            if (e>0)
            {
                if (x0<x1) x++;
                else x--;
                e -= dy*2;
            }
            y++;
        }
    }
}

void bar(int x, int y, int w, int h)
{
    int i;
    int j;

    for (i=0; i<h; i++)
    {
        for (j=0; j<w; j++)
        {
            putpixel(x+j, y+i, _cur_YUVcolor);
        }
    }
}

void rectangle(int x1, int y1, int x2, int y2)
{
    line(x1, y1, x2, y1);
    line(x1, y2, x2, y2);
    line(x1, y1, x1, y2);
    line(x2, y1, x2, y2);
}

void drawmodel(int x, int y, int w, int h, u8* pdata)
{
    static const u8 mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    int i;
    int j;
    int k;
    int nc;
    int cols;

    w  = (w+7)/8*8;
    nc = 0;

    for (i=0; i<h; i++)
    {
        cols = 0;
        for (k=0; k<w/8; k++)
        {
            for (j=0; j<8; j++)
            {
                if (pdata[nc] & mask[j]) putpixel(x+cols, y+i, _cur_YUVcolor);
                cols++;
            }
            nc++;
        }
    }
}

void drawcursor(int x, int y)
{
	line(x, y, x+8, y+6);
	line(x, y, x, y+12);
	line(x, y+12, x+8, y+6);	
}

void drawProgBar(int x, int y, int h, DWORD bgRGBcolor, DWORD RGBcolor, int prog)
{
	int MaxW = DISPLAY_WIDTH;
	int progx=0;
	
	MaxW = g_VirDispW;

	if(prog == 100)
	{
		progx = MaxW;
		setcolor(RGBcolor); 
		bar(x, y, progx, h);
	}
	else
	{
		progx = MaxW*prog/100;
		setcolor(RGBcolor);
		bar(x, y, progx, h);
		setcolor(bgRGBcolor); 
		bar(x+progx, y, MaxW-progx, h);
	}
}
