
#ifndef __DISP_LCD_H__
#define __DISP_LCD_H__

#include "disp_private.h"

#define LCD_GPIO_SCL 4
#define LCD_GPIO_SDA 5
typedef struct
{
	bool                  lcd_used;

	bool                  lcd_bl_en_used;
	disp_gpio_set_t       lcd_bl_en;

	bool                  lcd_power_used[3];
	disp_gpio_set_t       lcd_power[3];

	bool                  lcd_gpio_used[6];  //index4: scl;  index5: sda
	disp_gpio_set_t       lcd_gpio[6];       //index4: scl; index5: sda

	bool                  lcd_io_used[28];
	disp_gpio_set_t       lcd_io[28];

	u32                   backlight_bright;
	u32                   backlight_dimming;//IEP-drc backlight dimming rate: 0 -256 (256: no dimming; 0: the most dimming)
	u32                   backlight_curve_adjust[101];

	u32                   lcd_bright;
	u32                   lcd_contrast;
	u32                   lcd_saturation;
	u32                   lcd_hue;
}__disp_lcd_cfg_t;

s32 disp_init_lcd(__disp_bsp_init_para * para);

#endif
