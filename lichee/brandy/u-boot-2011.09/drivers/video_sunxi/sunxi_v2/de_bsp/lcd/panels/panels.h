#ifndef __PANEL_H__
#define __PANEL_H__
//#include "../lcd_source_interface.h"
//#include "../lcd_panel_cfg.h"
#include "../../de/bsp_display.h"
#include <asm/arch/drv_display.h>

typedef struct
{
	char name[32];
	disp_lcd_panel_fun func;
}__lcd_panel_t;

extern __lcd_panel_t * panel_array[];

#endif