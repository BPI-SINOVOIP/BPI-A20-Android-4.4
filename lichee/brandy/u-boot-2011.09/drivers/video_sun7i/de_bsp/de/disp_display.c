#include "disp_display.h"
#include "disp_de.h"
#include "disp_lcd.h"
#include "disp_tv.h"
#include "disp_event.h"
#include "disp_combined.h"
#include "disp_scaler.h"
#include "disp_video.h"
#include "disp_clk.h"
#include "disp_hdmi.h"

__disp_dev_t gdisp;
extern __panel_para_t              gpanel_info[2];


__s32 bsp_disp_init(__disp_bsp_init_para * para)
{
	__u32 i = 0, screen_id = 0;
	__u32 num_screens;

	printf("%s\n", __func__);

	memset(&gdisp,0x00,sizeof(__disp_dev_t));

	bsp_disp_feat_init();

	num_screens = bsp_disp_feat_get_num_screens();

	for(screen_id = 0; screen_id < num_screens; screen_id++) {
		gdisp.screen[screen_id].max_layers = bsp_disp_feat_get_num_layers(screen_id);;
		for(i = 0;i < gdisp.screen[screen_id].max_layers;i++)	{
			gdisp.screen[screen_id].layer_manage[i].para.prio = IDLE_PRIO;
		}
		gdisp.screen[screen_id].image_output_type = IMAGE_OUTPUT_LCDC;

		gdisp.screen[screen_id].bright = 50;
		gdisp.screen[screen_id].contrast = 50;
		gdisp.screen[screen_id].saturation = 50;
		gdisp.screen[screen_id].hue = 50;

		gdisp.scaler[screen_id].bright = 50;
		gdisp.scaler[screen_id].contrast = 50;
		gdisp.scaler[screen_id].saturation = 50;
		gdisp.scaler[screen_id].hue = 50;

		gdisp.screen[screen_id].lcd_cfg.backlight_bright = 197;
		gdisp.screen[screen_id].lcd_cfg.backlight_dimming = 256;
#ifdef __LINUX_OSAL__
		spin_lock_init(&gdisp.screen[screen_id].flag_lock);
#endif
	}
	memcpy(&gdisp.init_para,para,sizeof(__disp_bsp_init_para));
	gdisp.print_level = DEFAULT_PRINT_LEVLE;

	for(screen_id = 0; screen_id < num_screens; screen_id++) {
		if(screen_id == 0) {
			DE_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_BE0]);
			DE_SCAL_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_FE0]);
			tcon_set_reg_base(screen_id,para->reg_base[DISP_MOD_LCD0]);
			TVE_set_reg_base(screen_id, para->reg_base[DISP_MOD_TVE0]);

			
			#if (!defined CONFIG_ARCH_SUN7I)
			if(bsp_disp_feat_get_image_detail_enhance_support(screen_id)) {
			IEP_Deu_Set_Reg_base(screen_id, para->reg_base[DISP_MOD_DEU0]);
			}

			if(bsp_disp_feat_get_smart_backlight_support(screen_id)) {
			IEP_Drc_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_DRC0]);
			}

			IEP_CMU_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_CMU0]);
			dsi_set_reg_base(screen_id, para->reg_base[DISP_MOD_DSI0]);
			#endif
			
			bsp_disp_close_lcd_backlight(screen_id);
		} else if(screen_id == 1)	{
			DE_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_BE1]);
			DE_SCAL_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_FE1]);
			tcon_set_reg_base(screen_id,para->reg_base[DISP_MOD_LCD1]);
			TVE_set_reg_base(screen_id, para->reg_base[DISP_MOD_TVE1]);

			#if (!defined CONFIG_ARCH_SUN7I)
			if(bsp_disp_feat_get_image_detail_enhance_support(screen_id)) {
				IEP_Deu_Set_Reg_base(screen_id, para->reg_base[DISP_MOD_DEU1]);
			}

			if(bsp_disp_feat_get_smart_backlight_support(screen_id)) {
				IEP_Drc_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_DRC1]);
			}

			IEP_CMU_Set_Reg_Base(screen_id, para->reg_base[DISP_MOD_CMU1]);
			#endif
			
			bsp_disp_close_lcd_backlight(screen_id);
		}
	}

	disp_pll_init();
	for(screen_id = 0; screen_id < num_screens; screen_id++) {
		Scaler_Init(screen_id);
		Image_init(screen_id);
		disp_lcdc_init(screen_id);
		Disp_TVEC_Init(screen_id);

		#if (!defined CONFIG_ARCH_SUN7I)
		iep_init(screen_id);
		#endif
		if(bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_HDMI) {
			disp_hdmi_init();
		}
	}

	//disp_video_init();


	return DIS_SUCCESS;
}

__s32 bsp_disp_exit(__u32 mode)
{
	__u32 num_screens;
	__u32 screen_id;

	num_screens = bsp_disp_feat_get_num_screens();

	if(mode == DISP_EXIT_MODE_CLEAN_ALL) {
		bsp_disp_close();

		for(screen_id = 0; screen_id<num_screens; screen_id++) {
			Scaler_Exit(screen_id);
			Image_exit(screen_id);
			disp_lcdc_exit(screen_id);
			if(bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_TV) {
				Disp_TVEC_Exit(screen_id);
			}

			if(bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_HDMI) {
				disp_hdmi_exit();
			}
			iep_exit(screen_id);
		}

		disp_video_exit();
	}	else if(mode == DISP_EXIT_MODE_CLEAN_PARTLY) {
#ifndef __LINUX_OSAL__
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_LCD0]);
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_FE0]);
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_BE0]);
#endif
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_LCD0],disp_lcdc_event_proc,(void*)0);
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_FE0],scaler_event_proc,(void*)0);
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_BE0],scaler_event_proc,(void*)0);
		if(bsp_disp_feat_get_num_screens() > 1) {
#ifndef __LINUX_OSAL__
			OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_LCD1]);
			OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_FE1]);
			OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_BE1]);
#endif
			OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_LCD1],disp_lcdc_event_proc,(void*)0);
			OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_FE1],scaler_event_proc,(void*)0);
		  OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_BE1],scaler_event_proc,(void*)0);
		}
#ifndef __LINUX_OSAL__
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_DSI0]);
#endif
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_DSI0],disp_lcdc_event_proc,(void*)0);
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_open(void)
{
	return DIS_SUCCESS;
}

__s32 bsp_disp_close(void)
{
	__u32 screen_id = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for(screen_id = 0; screen_id<num_screens; screen_id++) {
		Image_close(screen_id);
		if(gdisp.scaler[screen_id].status & SCALER_USED) {
			Scaler_close(screen_id);
		}
		if(gdisp.screen[screen_id].lcdc_status & LCDC_TCON0_USED)	{
			tcon0_close(screen_id);
			tcon_exit(screen_id);
		}	else if(gdisp.screen[screen_id].lcdc_status & LCDC_TCON1_USED) {
			tcon1_close(screen_id);
			tcon_exit(screen_id);
		}	else if(gdisp.screen[screen_id].status & (TV_ON | VGA_ON)) {
			if(bsp_disp_feat_get_supported_output_types(screen_id) & 	DISP_OUTPUT_TYPE_TV) {
				TVE_close(screen_id);
			}
		}

		gdisp.screen[screen_id].status &= (IMAGE_USED_MASK & LCD_OFF & TV_OFF & VGA_OFF & HDMI_OFF);
		gdisp.screen[screen_id].lcdc_status &= (LCDC_TCON0_USED_MASK & LCDC_TCON1_USED_MASK);
	}

	return DIS_SUCCESS;
}


__s32 bsp_disp_print_reg(__bool b_force_on, __disp_mod_id_t id)
{
	__u32 base = 0, size = 0;
	__u32 i = 0;
	unsigned char str[20];

	base = gdisp.init_para.reg_base[id];
	size = gdisp.init_para.reg_size[id];

	if(b_force_on) {
		OSAL_PRINTF("%s", str);
	}	else {
		DE_INF("%s", str);
	}
	for(i=0; i<size; i+=16)	{
		__u32 reg[4];

		reg[0] = sys_get_wvalue(base + i);
		reg[1] = sys_get_wvalue(base + i + 4);
		reg[2] = sys_get_wvalue(base + i + 8);
		reg[3] = sys_get_wvalue(base + i + 12);
#ifdef __LINUX_OSAL__
		if(b_force_on) {
			OSAL_PRINTF("0x%08x:%08x,%08x:%08x,%08x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
		}	else {
			DE_INF("0x%08x:%08x,%08x:%08x,%08x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
		}
#else
		if(b_force_on) {
			OSAL_PRINTF("0x%x:%x,%x,%x,%x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
		}	else {
			DE_INF("0x%x:%x,%x:%x,%x\n", base + i, reg[0], reg[1], reg[2], reg[3]);
		}
#endif
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_set_print_level(__u32 print_level)
{
	gdisp.print_level = print_level;

	return 0;
}

__s32 bsp_disp_get_print_level(void)
{
	return gdisp.print_level;
}

