#ifndef _DRAW2D_H_
#define _DRAW2D_H_
#include "font.h"
#include "types.h"

#define RGB32(R,G,B)	((R<<16)|(G<<8)|(B))
#define RGB32_R(rgb)	((rgb>>16)&0xFF)
#define RGB32_G(rgb)	((rgb>>8)&0xFF)
#define RGB32_B(rgb)	((rgb>>0)&0xFF)

#define GetY(R,G,B) (0.257*(R)+0.504*(G)+0.98*(B)+16)
#define GetU(R,G,B) (128-0.148*(R)-0.291*(G)+0.439*(B))
#define GetV(R,G,B) (0.439*(R)-0.368*(G)-0.071*(B)+128)
#define RGB888(R,G,B) ((R<<16)|(G<<8)|(B))
#define GetR(rgb888) ((rgb888>>16)&0xFF)
#define GetG(rgb888) ((rgb888>>8)&0xFF)
#define GetB(rgb888) ((rgb888>>0)&0xFF)

#define RGB_2_Y(R, G, B) (((66 * R + 129 * G + 25 * B + 128) >> 8) + 16)
#define RGB_2_U(R, G, B) (((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128)
#define RGB_2_V(R, G, B) (((112 * R - 94 * G - 18 * B + 128) >> 8) + 128)

#define YUV_2_R(Y,U,V) (Y + ((360 * (V - 128))>>8))
#define YUV_2_G(Y,U,V) (Y - (( ( 88 * (U - 128)  + 184 * (V - 128)) )>>8))
#define YUV_2_B(Y,U,V) (Y +((455 * (U - 128))>>8))

#define MAKE_YUV(Y,U,V) ((Y<<16)|(U<<8)|(V))
#define YUV_Y(rgb)	((rgb>>16)&0xFF)
#define YUV_U(rgb)	((rgb>>8)&0xFF)
#define YUV_V(rgb)	((rgb>>0)&0xFF)

#define YUV_RANG(temp) (temp<0)?0:((temp>255)?255:temp)

void ApkDrawInit();
void setcolor(DWORD RGBcolor);
DWORD getcolor();
void setbufmode(int flag);
void postbackbuf();

void clearscreen(DWORD RGBcolor);
void putpixel(int x, int y, DWORD YUVcolor);
DWORD getpixel(int x, int y);
void line(int x0, int y0, int x1, int y1);
void bar(int x, int y, int w, int h);
void rectangle(int x1, int y1, int x2, int y2);

#endif
