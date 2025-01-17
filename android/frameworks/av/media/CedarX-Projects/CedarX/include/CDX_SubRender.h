#ifndef _CDX_SUBRENDER_H__
#define _CDX_SUBRENDER_H__

#include "sub_total_inf.h"
#include "CDX_Charset.h"

#if 0
enum
{
	SUB_RENDER_ALIGN_NONE  		= 0,
	SUB_RENDER_HALIGN_LEFT 		= 1,
	SUB_RENDER_HALIGN_CENTER 	= 2,
	SUB_RENDER_HALIGN_RIGHT 	= 3,
	SUN_RENDER_HALIGN_MASK		= 0x0000000f,
	SUB_RENDER_VALIGN_TOP		= (1 << 4),
	SUB_RENDER_VALIGN_CENTER	= (2 << 4),
	SUB_RENDER_VALIGN_BOTTOM	= (3 << 4),
	SUN_RENDER_VALIGN_MASK		= 0x000000f0
};
#endif

 enum
 {
 	SUB_RENDER_STYLE_NONE		    = 0,
 	SUB_RENDER_STYLE_BOLD		    = 1 << 0,
    SUB_RENDER_STYLE_ITALIC         = 1 << 1,
    SUB_RENDER_STYLE_UNDERLINE      = 1 << 2,
    SUB_RENDER_STYLE_STRIKETHROUGH  = 1 << 3
 };

//int 	SubRenderCreate(int layerStack);
int 	SubRenderCreate(void *callback_info);
int 	SubRenderDestory();
int 	SubRenderDraw(sub_item_inf *sub_info);
int 	SubRenderShow();
int 	SubRenderHide(int  systemTime, int* hasSubShowFlag);
int 	SubRenderSetTextColor(int color);
int 	SubRenderGetTextColor();
int 	SubRenderSetBackColor(int color);
int 	SubRenderGetBackColor();
int 	SubRenderSetFontSize(int fontsize);
int 	SubRenderGetFontSize();
int 	SubRenderSetPosition(int index,int posx,int posy);
int 	SubRenderSetYPositionPercent(int index,int percent);
int 	SubRenderGetPositionX(int index);
int 	SubRenderGetPositionY(int index);
int 	SubRenderGetWidth(int index);
int 	SubRenderGetHeight();
int 	SubRenderSetCharset(int charset);
int 	SubRenderGetCharset();
int     SubRenderSetAlign(int align);
int		SubRenderGetAlign();
int		SubRenderSetZorderTop();
int     SubRenderSetZorderBottom();
int     SubRenderSetFontStyle(int style);
int     SubRenderGetFontStyle();

int 	SubRenderGetScreenWidth();
int 	SubRenderGetScreenHeight();
int 	SubRenderGetScreenDirection();

#endif
