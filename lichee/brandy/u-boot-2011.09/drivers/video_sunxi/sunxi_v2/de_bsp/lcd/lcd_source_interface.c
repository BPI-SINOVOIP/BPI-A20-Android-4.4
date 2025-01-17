/* linux/drivers/video/sunxi/lcd/video_source_interface.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Video source interface for LCD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

//#include "dev_lcd.h"
#include "lcd_source_interface.h"

/**
 * sunxi_lcd_delay_ms.
 * @ms: Delay time, unit: millisecond.
 */
s32 sunxi_lcd_delay_ms(u32 ms)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_delay_ms) {
		return g_lcd_drv.src_ops.sunxi_lcd_delay_ms(ms);
	}

	return -1;
}

/**
 * sunxi_lcd_delay_us.
 * @us: Delay time, unit: microsecond.
 */
s32 sunxi_lcd_delay_us(u32 us)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_delay_us) {
		return g_lcd_drv.src_ops.sunxi_lcd_delay_us(us);
	}

	return -1;
}

/**
 * sunxi_lcd_tcon_enable - enable timing controller.
 * @screen_id: The index of screen.
 */
void sunxi_lcd_tcon_enable(u32 screen_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_tcon_enable) {
		g_lcd_drv.src_ops.sunxi_lcd_tcon_enable(screen_id);
	}

	return ;
}

/**
 * sunxi_lcd_tcon_disable - disable timing controller.
 * @screen_id: The index of screen.
 */
void sunxi_lcd_tcon_disable(u32 screen_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_tcon_disable) {
		g_lcd_drv.src_ops.sunxi_lcd_tcon_disable(screen_id);
	}

	return ;
}

/**
 * sunxi_lcd_backlight_enable - enable the backlight of panel.
 * @screen_id: The index of screen.
 */
void sunxi_lcd_backlight_enable(u32 screen_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_backlight_enable) {
		g_lcd_drv.src_ops.sunxi_lcd_backlight_enable(screen_id);
	}

	return ;
}

/**
 * sunxi_lcd_backlight_disable - disable the backlight of panel.
 * @screen_id: The index of screen.
 */
void sunxi_lcd_backlight_disable(u32 screen_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_backlight_disable) {
		g_lcd_drv.src_ops.sunxi_lcd_backlight_disable(screen_id);
	}

	return ;
}

/**
 * sunxi_lcd_power_enable - enable the power of panel.
 * @screen_id: The index of screen.
 * @pwr_id:     The index of power
 */
void sunxi_lcd_power_enable(u32 screen_id, u32 pwr_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_power_enable) {
		g_lcd_drv.src_ops.sunxi_lcd_power_enable(screen_id, pwr_id);
	}

	return ;
}

/**
 * sunxi_lcd_power_disable - disable the power of panel.
 * @screen_id: The index of screen.
 * @pwr_id:     The index of power
 */
void sunxi_lcd_power_disable(u32 screen_id, u32 pwr_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_power_disable) {
		g_lcd_drv.src_ops.sunxi_lcd_power_disable(screen_id, pwr_id);
	}

	return ;
}

/**
 * sunxi_lcd_pwm_enable - enable pwm modules, start ouput pwm wave.
 * @pwm_channel: The index of pwm channel.
 *
 * need to conifg gpio for pwm function
 */
s32 sunxi_lcd_pwm_enable(u32 pwm_channel)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_pwm_enable) {
		return g_lcd_drv.src_ops.sunxi_lcd_pwm_enable(pwm_channel);
	}

	return -1;
}

/**
 * sunxi_lcd_pwm_disable - disable pwm modules, stop ouput pwm wave.
 * @pwm_channel: The index of pwm channel.
 */
s32 sunxi_lcd_pwm_disable(u32 pwm_channel)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_pwm_disable) {
		return g_lcd_drv.src_ops.sunxi_lcd_pwm_disable(pwm_channel);
	}

	return -1;
}

/**
 * sunxi_lcd_cpu_write - write command and para to cpu panel.
 * @scree_id: The index of screen.
 * @command: Command to be transfer.
 * @para: The pointer to para
 * @para_num: The number of para
 */
s32 sunxi_lcd_cpu_write(u32 scree_id, u32 command, u32 *para, u32 para_num)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_cpu_write) {
		return g_lcd_drv.src_ops.sunxi_lcd_cpu_write(scree_id, command, para, para_num);
	}

	return -1;
}

/**
 * sunxi_lcd_cpu_write_index - write command to cpu panel.
 * @scree_id: The index of screen.
 * @index: Command or index to be transfer.
 */
s32 sunxi_lcd_cpu_write_index(u32 scree_id, u32 index)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_cpu_write_index) {
		return g_lcd_drv.src_ops.sunxi_lcd_cpu_write_index(scree_id, index);
	}

	return -1;
}

/**
 * sunxi_lcd_cpu_write_data - write data to cpu panel.
 * @scree_id: The index of screen.
 * @data: Data to be transfer.
 */
s32 sunxi_lcd_cpu_write_data(u32 scree_id, u32 data)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_cpu_write_data) {
		return g_lcd_drv.src_ops.sunxi_lcd_cpu_write_data(scree_id, data);
	}

	return -1;
}

/**
 * sunxi_lcd_dsi_write - write command and para to mipi panel.
 * @scree_id: The index of screen.
 * @command: Command to be transfer.
 * @para: The pointer to para.
 * @para_num: The number of para
 */
s32 sunxi_lcd_dsi_write(u32 scree_id, u8 command, u8 *para, u32 para_num)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_dsi_write) {
		return g_lcd_drv.src_ops.sunxi_lcd_dsi_write(scree_id, command, para, para_num);
	}

	return -1;
}

/**
 * sunxi_lcd_dsi_io_enable - enable dsi io.
 * @scree_id: The index of screen.
 */
s32 sunxi_lcd_dsi_io_enable(u32 scree_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_dsi_io_enable) {
		return g_lcd_drv.src_ops.sunxi_lcd_dsi_io_enable(scree_id);
	}

	return -1;
}

/**
 * sunxi_lcd_dsi_io_disable - disable dsi io.
 * @scree_id: The index of screen.
 */
s32 sunxi_lcd_dsi_io_disable(u32 scree_id)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_dsi_io_disable) {
		return g_lcd_drv.src_ops.sunxi_lcd_dsi_io_disable(scree_id);
	}

	return -1;
}

/**
 * sunxi_disp_get_num_screens - get number of screen supported.
 *
 */
s32 sunxi_disp_get_num_screens(void)
{
	if(g_lcd_drv.src_ops.sunxi_disp_get_num_screens) {
		return g_lcd_drv.src_ops.sunxi_disp_get_num_screens();
	}

	return 0;
}
#if 0
/**
 * sunxi_disp_panel_register - register panel.
 * @panel: The pointer to sunxi_panel.
 */
s32 sunxi_disp_panel_register(struct sunxi_panel *panel)
{
	if(g_lcd_drv.src_ops.sunxi_disp_panel_register) {
		return sunxi_disp_panel_register(panel);
	}

	return -1;
}
#endif

/**
 * sunxi_disp_panel_register .
 */
s32 sunxi_lcd_get_driver_name(u32 screen_id, char *name)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_get_driver_name) {
		return g_lcd_drv.src_ops.sunxi_lcd_get_driver_name(screen_id, name);
	}

	return -1;
}

/**
 * sunxi_lcd_set_panel_funs -  set panel functions.
 * @name: The panel driver name.
 * @lcd_cfg: The functions.
 */
s32 sunxi_lcd_set_panel_funs(char *name, disp_lcd_panel_fun * lcd_cfg)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_set_panel_funs) {
		return g_lcd_drv.src_ops.sunxi_lcd_set_panel_funs(name, lcd_cfg);
	}

	return -1;
}

/**
 * sunxi_lcd_pin_cfg - enable the power of panel.
 * @screen_id: The index of screen.
 * @pwr_id:     The index of power
 */
s32 sunxi_lcd_pin_cfg(u32 screen_id, u32 bon)
{
	if(g_lcd_drv.src_ops.sunxi_lcd_pin_cfg) {
		return g_lcd_drv.src_ops.sunxi_lcd_pin_cfg(screen_id, bon);
	}

	return -1;
}