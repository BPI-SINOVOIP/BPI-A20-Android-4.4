#include "disp_lcd.h"

struct disp_lcd_private_data
{
	disp_lcd_flow             open_flow;
	disp_lcd_flow             close_flow;
	disp_panel_para           panel_info;
	__disp_lcd_cfg_t          lcd_cfg;
	disp_lcd_panel_fun        lcd_panel_fun;
	bool                      enabling;
	bool                      disabling;
	u32                       irq_no;
	u32                       reg_base;
	u32                       irq_no_dsi;
	u32                       reg_base_dsi;
	u32                       enabled;
	struct {
		__hdle                  dev;
		u32                     channel;
		u32                     polarity;
		u32                     period_ns;
		u32                     duty_ns;
		u32                     enabled;
	}pwm_info;
	disp_clk_info_t           lcd_clk;
	disp_clk_info_t           dsi_clk;
	disp_clk_info_t           lvds_clk;
	disp_clk_info_t           edp_clk;
	disp_clk_info_t           extra_clk;
};
#if defined(__LINUX_PLAT__)
static spinlock_t lcd_data_lock;
#endif

static struct disp_lcd *lcds = NULL;
static struct disp_lcd_private_data *lcd_private;

s32 disp_lcd_set_bright(struct disp_lcd *lcd, u32 bright);
s32 disp_lcd_get_bright(struct disp_lcd *lcd, u32 *bright);

struct disp_lcd* disp_get_lcd(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	return &lcds[screen_id];
}
struct disp_lcd_private_data *disp_lcd_get_priv(struct disp_lcd *lcd)
{
	if(NULL == lcd) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &lcd_private[lcd->channel_id];
}

s32 disp_lcd_is_used(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	} else {
		return lcdp->lcd_cfg.lcd_used;
	}
}

s32 lcd_parse_panel_para(u32 screen_id, disp_panel_para * info)
{
    s32 ret = 0;
    char primary_key[25];
    s32 value = 0;

    sprintf(primary_key, "lcd%d_para", screen_id);

//lcd_used
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);

    if(value == 0) //no need to get panel para if !lcd_used
        return -1;

    sprintf(primary_key, "lcd%d_para", screen_id);

    memset(info, 0, sizeof(disp_panel_para));

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_x", &value, 1);
    if(ret == 0)
    {
        info->lcd_x = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_y", &value, 1);
    if(ret == 0)
    {
        info->lcd_y = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_width", &value, 1);
    if(ret == 0)
    {
        info->lcd_width = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_height", &value, 1);
    if(ret == 0)
    {
        info->lcd_height = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dclk_freq", &value, 1);
    if(ret == 0)
    {
        info->lcd_dclk_freq = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_used", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_used = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_ch", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_ch = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_freq", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_freq = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_pol", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_pol = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_if", &value, 1);
    if(ret == 0)
    {
        info->lcd_if = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hbp", &value, 1);
    if(ret == 0)
    {
        info->lcd_hbp = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_ht", &value, 1);
    if(ret == 0)
    {
        info->lcd_ht = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vbp", &value, 1);
    if(ret == 0)
    {
        info->lcd_vbp = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vt", &value, 1);
    if(ret == 0)
    {
        info->lcd_vt = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_if", &value, 1);
    if(ret == 0)
    {
        info->lcd_hv_if = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vspw", &value, 1);
    if(ret == 0)
    {
        info->lcd_vspw = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hspw", &value, 1);
    if(ret == 0)
    {
        info->lcd_hspw = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_if", &value, 1);
    if(ret == 0)
    {
        info->lcd_lvds_if = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_mode", &value, 1);
    if(ret == 0)
    {
        info->lcd_lvds_mode = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_colordepth", &value, 1);
    if(ret == 0)
    {
        info->lcd_lvds_colordepth= value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_io_polarity", &value, 1);
    if(ret == 0)
    {
        info->lcd_lvds_io_polarity = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cpu_if", &value, 1);
    if(ret == 0)
    {
        info->lcd_cpu_if = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cpu_te", &value, 1);
    if(ret == 0)
    {
        info->lcd_cpu_te = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_frm", &value, 1);
    if(ret == 0)
    {
        info->lcd_frm = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_if", &value, 1);
    if(ret == 0)
    {
        info->lcd_dsi_if = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_lane", &value, 1);
    if(ret == 0)
    {
        info->lcd_dsi_lane = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_format", &value, 1);
    if(ret == 0)
    {
        info->lcd_dsi_format = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_eotp", &value, 1);
    if(ret == 0)
    {
        info->lcd_dsi_eotp = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_te", &value, 1);
    if(ret == 0)
    {
        info->lcd_dsi_te = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_ic", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_tx_ic= value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_rate", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_tx_rate = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_lane", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_tx_lane= value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_colordepth", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_colordepth = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_clk_phase", &value, 1);
    if(ret == 0)
    {
        info->lcd_hv_clk_phase = value;
    }

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_sync_polarity", &value, 1);
    if(ret == 0)
    {
        info->lcd_hv_sync_polarity = value;
    }
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_gamma_en", &value, 1);
    if(ret == 0)
    {
        info->lcd_gamma_en = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cmap_en", &value, 1);
    if(ret == 0)
    {
        info->lcd_cmap_en = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_xtal_freq", &value, 1);
    if(ret == 0)
    {
        info->lcd_xtal_freq = value;
    }


    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_size", (int*)info->lcd_size, 2);
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_model_name", (int*)info->lcd_model_name, 2);

    return 0;
}

void lcd_get_sys_config(u32 screen_id, __disp_lcd_cfg_t *lcd_cfg)
{
    static char io_name[28][20] = {"lcdd0", "lcdd1", "lcdd2", "lcdd3", "lcdd4", "lcdd5", "lcdd6", "lcdd7", "lcdd8", "lcdd9", "lcdd10", "lcdd11",
                         "lcdd12", "lcdd13", "lcdd14", "lcdd15", "lcdd16", "lcdd17", "lcdd18", "lcdd19", "lcdd20", "lcdd21", "lcdd22",
                         "lcdd23", "lcdclk", "lcdde", "lcdhsync", "lcdvsync"};
    disp_gpio_set_t  *gpio_info;
    int  value = 1;
    char primary_key[20], sub_name[25];
    int i = 0;
    int  ret;

    sprintf(primary_key, "lcd%d_para", screen_id);

//lcd_used
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);
    if(ret == 0)
    {
        lcd_cfg->lcd_used = value;
    }

    if(lcd_cfg->lcd_used == 0) //no need to get lcd config if lcd_used eq 0
        return ;

//lcd_bl_en
    lcd_cfg->lcd_bl_en_used = 0;
    gpio_info = &(lcd_cfg->lcd_bl_en);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_bl_en", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_bl_en_used = 1;
    }

//lcd_power0
	for(i=0; i<3; i++)
	{
		if(i==0)
			sprintf(sub_name, "lcd_power");
		else
			sprintf(sub_name, "lcd_power%d", i);
		lcd_cfg->lcd_power_used[i] = 0;
		gpio_info = &(lcd_cfg->lcd_power[i]);
		ret = OSAL_Script_FetchParser_Data(primary_key,sub_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
		if(ret == 0) {
		  lcd_cfg->lcd_power_used[i] = 1;
		}
	}

//lcd_gpio
    for(i=0; i<4; i++)
    {
        sprintf(sub_name, "lcd_gpio_%d", i);

        gpio_info = &(lcd_cfg->lcd_gpio[i]);
        ret = OSAL_Script_FetchParser_Data(primary_key,sub_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 0)
        {
            lcd_cfg->lcd_gpio_used[i]= 1;
        }
    }

//lcd_gpio_scl,lcd_gpio_sda
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_scl", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL]= 1;
    }
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_sda", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA]= 1;
    }

//lcd io
    for(i=0; i<28; i++)
    {
        gpio_info = &(lcd_cfg->lcd_io[i]);
        ret = OSAL_Script_FetchParser_Data(primary_key,io_name[i], (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 0)
        {
            lcd_cfg->lcd_io_used[i]= 1;
        }
    }

//backlight adjust
	for(i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if(i == 100)
		lcd_cfg->backlight_curve_adjust[i] = 255;

		ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
		if(ret == 0) {
			value = (value > 100)? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}


//init_bright
    sprintf(primary_key, "disp_init");
    sprintf(sub_name, "lcd%d_backlight", screen_id);

    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->backlight_bright = 197;
    }
    else
    {
        if(value > 256)
        {
            value = 256;
        }
        lcd_cfg->backlight_bright = value;
    }

//bright,constraction,saturation,hue
    sprintf(primary_key, "disp_init");
    sprintf(sub_name, "lcd%d_bright", screen_id);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_bright = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_bright = value;
    }

    sprintf(sub_name, "lcd%d_contrast", screen_id);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_contrast = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_contrast = value;
    }

    sprintf(sub_name, "lcd%d_saturation", screen_id);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_saturation = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_saturation = value;
    }

    sprintf(sub_name, "lcd%d_hue", screen_id);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_hue = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_hue = value;
    }
}

s32 disp_lcd_pin_cfg(struct disp_lcd *lcd, u32 bon)
{
	__hdle lcd_pin_hdl;
	int  i;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d pin config, state %s, %d\n", lcd->channel_id, (bon)? "on":"off", bon);

#if defined (__FPGA_DEBUG__)
	if(!bon) {
		//pd28--pwm, ph0--pwr
		writel(0x77777777, 0xf6000800+0x6c);
		writel(0x77777777, 0xf6000800+0x70);
		writel(0x77777777, 0xf6000800+0x74);
		writel(0x77727777, 0xf6000800+0x78);
		writel((readl(0xf6000800+0xfc) & (~0x0000000f)) | 0x00000007, 0xf6000800+0xfc);
	} else {
		//pd28--pwm, ph0--pwr
		writel(0x22222222, 0xf6000800+0x6c);
		writel(0x22222222, 0xf6000800+0x70);
		writel(0x22222222, 0xf6000800+0x74);
		writel(0x77722222, 0xf6000800+0x78);
	}
#endif
	for(i=0; i<28; i++)	{
		if(lcdp->lcd_cfg.lcd_io_used[i]) {
			disp_gpio_set_t  gpio_info[1];

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_io[i]), sizeof(disp_gpio_set_t));
			if(!bon) {
				gpio_info->mul_sel = 7;
			}	else {
				if((lcdp->panel_info.lcd_if == 3) && (gpio_info->mul_sel==2))	{
					gpio_info->mul_sel = 3;
				}
			}
			lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(lcd_pin_hdl, 2);
		}
	}

	disp_al_lcd_io_cfg(lcd->channel_id, bon, &lcdp->panel_info);

	return DIS_SUCCESS;
}

s32 disp_lcd_get_driver_name(struct disp_lcd *lcd, char* name)
{
	char primary_key[20];
	s32 ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(primary_key, "lcd%d_para", lcd->channel_id);

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_driver_name",  (int*)name, 32);
	DE_INF("disp_lcd_get_driver_name, %s\n", name);
	return ret;
}
s32 lcd_clk_init(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	mod_clk_set_src(lcdp->lcd_clk.clk, lcdp->lcd_clk.clk_src);

	if(LCD_IF_LVDS == lcdp->panel_info.lcd_if) {
	} else if(LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		mod_clk_set_src(lcdp->dsi_clk.clk, lcdp->dsi_clk.clk_src);
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if) {
	}

	return DIS_SUCCESS;
}

s32 lcd_clk_exit(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	lcdp->lcd_clk.enabled = 0;
	return DIS_SUCCESS;
}

static s32 lcd_clk_config(struct disp_lcd* lcd)
{
	u32 lcd_dclk_freq;	//Hz
	u32 pll_freq = 0;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	lcd_dclk_freq = lcdp->panel_info.lcd_dclk_freq * 1000000;
	if ((lcdp->panel_info.lcd_if == LCD_IF_HV) || (lcdp->panel_info.lcd_if == LCD_IF_CPU)
	    || (lcdp->panel_info.lcd_if == LCD_IF_EDP))	{
		lcdp->lcd_clk.clk_div = 6;//todo?
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_LVDS) {
		lcdp->lcd_clk.clk_div = 7;
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		u32 lane = lcdp->panel_info.lcd_dsi_lane;
		u32 bitwidth = 0;

		switch(lcdp->panel_info.lcd_dsi_format) {
			case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
			case LCD_DSI_FORMAT_RGB666P://todo?
			bitwidth = 18;
			break;
		}

		lcdp->lcd_clk.clk_div = bitwidth/lane;
	}

	pll_freq = lcd_dclk_freq * (lcdp->lcd_clk.clk_div);

	set_src_freq(lcdp->lcd_clk.clk_src, pll_freq);

	return 0;
}

s32 lcd_clk_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	lcd_clk_config(lcd);

	mod_clk_enable(lcdp->lcd_clk.clk, CLK_ON);
	if(LCD_IF_LVDS == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->lvds_clk.clk, CLK_ON);
	} else if(LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->dsi_clk.clk, CLK_ON);
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->edp_clk.clk, CLK_ON);
	}

	de_top_clk_enable(lcdp->extra_clk.clk, CLK_ON);
	lcdp->lcd_clk.enabled = 1;
	return	DIS_SUCCESS;
}

s32 lcd_clk_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	mod_clk_enable(lcdp->lcd_clk.clk, CLK_OFF);
	if(LCD_IF_LVDS == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->lvds_clk.clk, CLK_OFF);
	} else if(LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->dsi_clk.clk, CLK_OFF);
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if) {
		mod_clk_enable(lcdp->edp_clk.clk, CLK_OFF);
	}

	de_top_clk_enable(lcdp->extra_clk.clk, CLK_OFF);
	lcdp->lcd_clk.enabled = 0;
	return	DIS_SUCCESS;
}

s32 disp_lcd_tcon_enable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_enable(lcd->channel_id, 1, &lcdp->panel_info);
}

s32 disp_lcd_tcon_disable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_enable(lcd->channel_id, 0, &lcdp->panel_info);
}

s32 disp_lcd_set_open_func(struct disp_lcd* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(func) {
		lcdp->open_flow.func[lcdp->open_flow.func_num].func = func;
		lcdp->open_flow.func[lcdp->open_flow.func_num].delay = delay;
		lcdp->open_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

s32 disp_lcd_set_close_func(struct disp_lcd* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(func) {
		lcdp->close_flow.func[lcdp->close_flow.func_num].func = func;
		lcdp->close_flow.func[lcdp->close_flow.func_num].delay = delay;
		lcdp->close_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

s32 disp_lcd_set_panel_funs(struct disp_lcd* lcd, disp_lcd_panel_fun * lcd_cfg)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memset(&lcdp->lcd_panel_fun, 0, sizeof(disp_lcd_panel_fun));
	lcdp->lcd_panel_fun.cfg_panel_info= lcd_cfg->cfg_panel_info;
	lcdp->lcd_panel_fun.cfg_open_flow = lcd_cfg->cfg_open_flow;
	lcdp->lcd_panel_fun.cfg_close_flow = lcd_cfg->cfg_close_flow;
	lcdp->lcd_panel_fun.lcd_user_defined_func = lcd_cfg->lcd_user_defined_func;

	return 0;
}

disp_lcd_flow * disp_lcd_get_open_flow(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &(lcdp->open_flow);
}

disp_lcd_flow * disp_lcd_get_close_flow(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &(lcdp->close_flow);
}

s32 disp_lcd_pre_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 data[2];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
    unsigned long flags;
    spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->enabling = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	/* notifier */
	data[0] = 1;/* enable */
	data[1] = (u32)DISP_OUTPUT_TYPE_LCD;
	disp_notifier_call_chain(DISP_EVENT_OUTPUT_ENABLE, lcd->channel_id, (void*)data);

	/* clk enable */
	lcd_clk_enable(lcd);

	disp_al_lcd_init(lcd->channel_id);
	disp_al_lcd_cfg(lcd->channel_id, &lcdp->panel_info);
	disp_al_lcd_set_clk_div(lcd->channel_id, lcdp->lcd_clk.clk_div);
	//disp_al_check_display_size(lcd->channel_id);
	//disp_al_check_csc(lcd->channel_id);

	lcdp->open_flow.func_num = 0;
	if(lcdp->lcd_panel_fun.cfg_open_flow)	{
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->channel_id);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->channel_id);
	}
	return 0;
}

s32 disp_lcd_post_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	//for_test
//	int value = sys_get_wvalue(0x01c0c040) & (~(0x1));
//	sys_put_wvalue(0x01c202c4, value | 0x5);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
  	lcdp->enabling = 0;
		lcdp->enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_lcd_pre_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->disabling = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	lcdp->close_flow.func_num = 0;
	if(lcdp->lcd_panel_fun.cfg_close_flow)	{
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->channel_id);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->channel_id);
	}

	return 0;
}

s32 disp_lcd_post_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 data[2];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcd_clk_disable(lcd);
#if defined(__LINUX_PLAT__)
  {
    unsigned long flags;
    spin_lock_irqsave(&lcd_data_lock, flags);
#endif
    lcdp->disabling = 0;
		lcdp->enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	/* notifier */
	data[0] = 0;/* enable */
	data[1] = (u32)DISP_OUTPUT_TYPE_LCD;
	disp_notifier_call_chain(DISP_EVENT_OUTPUT_ENABLE, lcd->channel_id, (void*)data);

	return 0;
}

s32 disp_lcd_is_enabled(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return (1 == lcdp->enabled);
}

s32 disp_lcd_backlight_enable(struct disp_lcd *lcd)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd) && (0 < lcdp->lcd_cfg.backlight_bright)) {
		if(lcdp->lcd_cfg.lcd_bl_en_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));

			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	} else {
	}

	return 0;
}

s32 disp_lcd_backlight_disable(struct disp_lcd *lcd)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_bl_en_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));
			gpio_info->data = (gpio_info->data==0)?1:0;
			gpio_info->mul_sel = 7;
			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	}

	return 0;
}

s32 disp_lcd_pwm_enable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		return sunxi_pwm_enable(lcdp->pwm_info.channel);
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

s32 disp_lcd_pwm_disable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	//for_test
	if(disp_lcd_is_used(lcd)/* && lcdp->pwm_info.dev*/) {
		sunxi_pwm_disable(lcdp->pwm_info.channel);
		return 0;
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}


s32 disp_lcd_power_enable(struct disp_lcd *lcd, u32 power_id)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_power_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_power[power_id]), sizeof(disp_gpio_set_t));

			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	}

	return 0;
}

s32 disp_lcd_power_disable(struct disp_lcd *lcd, u32 power_id)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_power_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_power[power_id]), sizeof(disp_gpio_set_t));
			gpio_info->data = (gpio_info->data==0)?1:0;
			gpio_info->mul_sel = 7;
			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	}

	return 0;
}

s32 disp_lcd_set_bright(struct disp_lcd *lcd, u32 bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	bool need_enable_bl = false, need_disable_bl = false;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if((0 == lcdp->lcd_cfg.backlight_bright) && (0 != bright))
		need_enable_bl = true;
	if((0 != lcdp->lcd_cfg.backlight_bright) && (0 == bright))
		need_disable_bl = true;

	backlight_bright = (backlight_bright > 255)? 255:backlight_bright;
	if(lcdp->lcd_cfg.backlight_bright != backlight_bright) {
		lcdp->lcd_cfg.backlight_bright = backlight_bright;
		disp_notifier_call_chain(DISP_EVENT_BACKLIGHT_UPDATE, lcd->channel_id, (void*)bright);
	}

	if(backlight_bright != 0)	{
		backlight_bright += 1;
	}

	lcdp->lcd_cfg.backlight_dimming = (0 == lcdp->lcd_cfg.backlight_dimming)? 256:lcdp->lcd_cfg.backlight_dimming;
	backlight_dimming = lcdp->lcd_cfg.backlight_dimming;
	period_ns = lcdp->pwm_info.period_ns;
	duty_ns = (backlight_bright * backlight_dimming *  period_ns/256 + 128) / 256;
	lcdp->pwm_info.duty_ns = duty_ns;

//		DE_DBG("[PWM]bright=%d, bright_modify=%d, backlight_dimming=%d, period_ns=%d, duty_ns=%d\n",
//		bright,(u32)backlight_bright, lcdp->lcd_cfg.backlight_dimming,  (u32)period_ns, (u32)duty_ns);
	sunxi_pwm_config(lcdp->pwm_info.channel, duty_ns, period_ns);

	if(need_enable_bl)
		disp_lcd_backlight_enable(lcd);
	if(need_disable_bl)
		disp_lcd_backlight_enable(lcd);

	return DIS_SUCCESS;
}

s32 disp_lcd_get_bright(struct disp_lcd *lcd, u32 *bright)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*bright = lcdp->lcd_cfg.backlight_bright;
	return DIS_SUCCESS;
}

s32 disp_lcd_update_bright_dimming(struct disp_lcd *lcd, u32 bright_dimming)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcdp->lcd_cfg.backlight_dimming = bright_dimming;
	return DIS_SUCCESS;
}

s32 disp_lcd_get_resolution(struct disp_lcd *lcd, u32 *xres, u32 *yres)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*xres = lcdp->panel_info.lcd_x;
	*yres = lcdp->panel_info.lcd_y;

	return 0;
}

s32 disp_lcd_get_physical_size(struct disp_lcd *lcd, u32 *width, u32 *height)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*width = lcdp->panel_info.lcd_width;
	*height = lcdp->panel_info.lcd_height;

	return 0;
}

s32 disp_lcd_get_input_csc(struct disp_lcd *lcd, disp_out_csc_type *csc_type)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*csc_type = DISP_OUT_CSC_TYPE_LCD;

	return 0;
}

s32 disp_lcd_get_timing(struct disp_lcd *lcd, disp_video_timing * tt)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memset(tt, 0, sizeof(disp_video_timing));
	tt->pixel_clk = lcdp->panel_info.lcd_dclk_freq * 1000;
	tt->hor_pixels = lcdp->panel_info.lcd_x;
	tt->ver_pixels = lcdp->panel_info.lcd_y;
	tt->hor_total_time= lcdp->panel_info.lcd_ht;
	tt->hor_sync_time= lcdp->panel_info.lcd_hspw;
	tt->hor_back_porch= lcdp->panel_info.lcd_hbp-lcdp->panel_info.lcd_hspw;
	tt->hor_front_porch= lcdp->panel_info.lcd_ht-lcdp->panel_info.lcd_hbp - lcdp->panel_info.lcd_x;
	tt->ver_total_time= lcdp->panel_info.lcd_vt;
	tt->ver_sync_time= lcdp->panel_info.lcd_vspw;
	tt->ver_back_porch= lcdp->panel_info.lcd_vbp-lcdp->panel_info.lcd_vspw;
	tt->ver_front_porch= lcdp->panel_info.lcd_vt-lcdp->panel_info.lcd_vbp -lcdp->panel_info.lcd_y;

	return 0;
}

extern void sync_event_proc(u32 screen_id);
#if defined(__LINUX_PLAT__)
s32 disp_lcd_event_proc(int irq, void *parg)
#else
s32 disp_lcd_event_proc(void *parg)
#endif
{
	u32 screen_id = (u32)parg;
	static u32 cntr=0;
	if(tcon_irq_query(screen_id,LCD_IRQ_TCON0_VBLK) || tcon_irq_query(screen_id,LCD_IRQ_TCON1_VBLK)
	    || dsi_irq_query(screen_id,DSI_IRQ_VIDEO_VBLK)) {
		sync_event_proc(screen_id);
	}

	if(tcon_irq_query(screen_id,LCD_IRQ_TCON0_CNTR)) {
		sync_event_proc(screen_id);

		if(disp_al_lcd_tri_busy(screen_id))	{
			if(cntr>=1)	{
				cntr = 0;
			}	else {
				cntr++;
			}
		}	else {
			cntr = 0;
		}

		if(cntr==0)	{
			/* todo? cpu_isr */
			disp_al_lcd_tri_start(screen_id);
		}
	}

	return OSAL_IRQ_RETURN;
}

s32 disp_lcd_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	struct disp_lcd *lcd;
	u32 *ptr = (u32 *)data;
	u32 backlight, backlight_dimming;

	lcd = disp_get_lcd(sel);
	if(!lcd)
		return -1;

	DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_BACKLIGHT_DIMMING_UPDATE:
		backlight_dimming = (u32)ptr;
		disp_lcd_update_bright_dimming(lcd, backlight_dimming);
		disp_lcd_get_bright(lcd, &backlight);
		disp_lcd_set_bright(lcd, backlight);
		break;

	default:
		break;
	}
	return 0;
}

s32 disp_lcd_init(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_notifier_block *nb;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	/* register one notifier for all lcd */
	if(0 == lcd->channel_id) {
		nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
		if(nb) {
			nb->notifier_call = &disp_lcd_notifier_callback;
			disp_notifier_register(nb);
		} else
			DE_WRN("malloc memory fail!\n");
	}

	lcd_get_sys_config(lcd->channel_id, &lcdp->lcd_cfg);
	lcd_parse_panel_para(lcd->channel_id, &lcdp->panel_info);
	if(disp_lcd_is_used(lcd)) {
		if(lcdp->panel_info.lcd_pwm_used) {
			lcdp->pwm_info.channel = lcdp->panel_info.lcd_pwm_ch;
			lcdp->pwm_info.polarity = lcdp->panel_info.lcd_pwm_pol;
//			lcdp->pwm_info.dev = OSAL_Pwm_request(lcdp->panel_info.lcd_pwm_ch);
		}
		disp_lcd_backlight_disable(lcd);
	}

	lcd_clk_init(lcd);
	lcd_clk_enable(lcd);
	disp_al_lcd_init(lcd->channel_id);
	lcd_clk_disable(lcd);

	if(disp_al_query_lcd_mod(lcd->channel_id)) {
		OSAL_RegISR(lcdp->irq_no,0,disp_lcd_event_proc,(void*)lcd->channel_id,0,0);
#if !defined(__LINUX_PLAT__)
		OSAL_InterruptEnable(lcdp->irq_no);
#endif
	}

	if(LCD_IF_DSI == lcdp->panel_info.lcd_if)	{
		OSAL_RegISR(lcdp->irq_no_dsi,0,disp_lcd_event_proc,(void*)lcd->channel_id,0,0);
#if !defined(__LINUX_PLAT__)
		OSAL_InterruptEnable(lcdp->irq_no_dsi);
#endif
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if)	{
		/* todo? register edp vint proc */
	}

	__u64 backlight_bright;
	__u64 period_ns, duty_ns;

	if(lcdp->panel_info.lcd_pwm_freq != 0) {
		period_ns = 1000*1000*1000 / lcdp->panel_info.lcd_pwm_freq;
	} else {
		DE_WRN("lcd%d.lcd_pwm_freq is ZERO\n", lcd->channel_id);
		period_ns = 1000*1000*1000 / 1000;  //default 1khz
	}

	backlight_bright = lcdp->lcd_cfg.backlight_bright;

	duty_ns = (backlight_bright * period_ns) / 256;
//		DE_DBG("[PWM]backlight_bright=%d,period_ns=%d,duty_ns=%d\n",(u32)backlight_bright,(u32)period_ns, (u32)duty_ns);
	sunxi_pwm_set_polarity(lcdp->pwm_info.channel, lcdp->pwm_info.polarity);
	sunxi_pwm_config(lcdp->pwm_info.channel, duty_ns, period_ns);
	lcdp->pwm_info.duty_ns = duty_ns;
	lcdp->pwm_info.period_ns = period_ns;

	return 0;
}

s32 disp_lcd_exit(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_al_query_lcd_mod(lcd->channel_id)) {
		OSAL_InterruptDisable(lcdp->irq_no);
		OSAL_UnRegISR(lcdp->irq_no, disp_lcd_event_proc,(void*)lcd->channel_id);
	}

	if(LCD_IF_DSI == lcdp->panel_info.lcd_if)	{
		OSAL_InterruptDisable(lcdp->irq_no_dsi);
		OSAL_UnRegISR(lcdp->irq_no_dsi, disp_lcd_event_proc,(void*)lcd->channel_id);
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if)	{
		/* todo? register edp vint proc */
	}

	disp_al_lcd_exit(lcd->channel_id);
	lcd_clk_exit(lcd);

	return 0;
}

s32 disp_init_lcd(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 screen_id;

	DE_INF("disp_init_lcd\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&lcd_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	lcds = (struct disp_lcd *)OSAL_malloc(sizeof(struct disp_lcd) * num_screens);
	if(NULL == lcds) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	lcd_private = (struct disp_lcd_private_data *)OSAL_malloc(sizeof(struct disp_lcd_private_data) * num_screens);
	if(NULL == lcd_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		struct disp_lcd *lcd = &lcds[screen_id];
		struct disp_lcd_private_data *lcdp = &lcd_private[screen_id];

		switch(screen_id) {
		case 0:
			lcd->name = "lcd0";
			lcd->channel_id = 0;
			lcd->type = DISP_OUTPUT_TYPE_LCD;
			lcdp->irq_no = para->irq_no[DISP_MOD_LCD0];
			lcdp->irq_no_dsi = para->irq_no[DISP_MOD_DSI0];
			lcdp->reg_base = para->reg_base[DISP_MOD_LCD0];
			lcdp->reg_base_dsi = para->reg_base[DISP_MOD_DSI0];
			lcdp->lcd_clk.clk = DISP_MOD_LCD0;
			lcdp->lcd_clk.clk_src = CLK_LCD_SRC;
			lcdp->lvds_clk.clk = DISP_MOD_LVDS;
			lcdp->dsi_clk.clk = DISP_MOD_DSI0;
			lcdp->dsi_clk.clk_src = CLK_DSI_SRC;
			lcdp->dsi_clk.clk_div = 1;
			lcdp->extra_clk.clk = DISP_MOD_DRC0;
			lcdp->extra_clk.clk_src = CLK_BE_SRC;
			lcdp->extra_clk.clk_div = 3;
			break;

		case 1:
			lcd->name = "lcd1";
			lcd->channel_id = 1;
			lcd->type = DISP_OUTPUT_TYPE_LCD;
			lcdp->irq_no = para->irq_no[DISP_MOD_LCD1];
			lcdp->irq_no_dsi = para->irq_no[DISP_MOD_DSI1];
			lcdp->reg_base = para->reg_base[DISP_MOD_LCD1];
			lcdp->reg_base_dsi = para->reg_base[DISP_MOD_DSI1];
			lcdp->lcd_clk.clk = DISP_MOD_LCD1;
			lcdp->lcd_clk.clk_src = CLK_LCD_SRC;
			lcdp->lvds_clk.clk = DISP_MOD_LVDS;
			lcdp->dsi_clk.clk = DISP_MOD_DSI1;
			lcdp->dsi_clk.clk_src = CLK_DSI_SRC;
			lcdp->dsi_clk.clk_div = 1;
			lcdp->extra_clk.clk = DISP_MOD_DRC1;
			lcdp->extra_clk.clk_src = CLK_BE_SRC;
			lcdp->extra_clk.clk_div = 3;
			break;

		case 2:
			break;
		}
		DE_INF("lcd %d, reg_base=0x%x, irq_no=%d, reg_base_dsi=0x%x, irq_no_dsi=%d\n",\
				screen_id, lcdp->reg_base, lcdp->irq_no, lcdp->reg_base_dsi, lcdp->irq_no_dsi);

		lcd->is_enabled = disp_lcd_is_enabled;
		lcd->is_used = disp_lcd_is_used;
		lcd->get_resolution = disp_lcd_get_resolution;
		lcd->get_physical_size = disp_lcd_get_physical_size;
		lcd->get_input_csc = disp_lcd_get_input_csc;

		lcd->init = disp_lcd_init;
		lcd->exit = disp_lcd_exit;

		//lcd->apply

//		lcd->early_suspend
//		lcd->late_resume
//		lcd->suspend
//		lcd->resume

		lcd->backlight_enable = disp_lcd_backlight_enable;
		lcd->backlight_disable = disp_lcd_backlight_disable;
		lcd->pwm_enable = disp_lcd_pwm_enable;
		lcd->pwm_disable = disp_lcd_pwm_disable;
		lcd->power_enable = disp_lcd_power_enable;
		lcd->power_disable = disp_lcd_power_disable;
		lcd->pin_cfg = disp_lcd_pin_cfg;
		lcd->set_bright = disp_lcd_set_bright;
		lcd->get_bright = disp_lcd_get_bright;
		lcd->get_timing = disp_lcd_get_timing;
		lcd->get_open_flow = disp_lcd_get_open_flow;
		lcd->get_close_flow  = disp_lcd_get_close_flow;
		lcd->pre_enable = disp_lcd_pre_enable;
		lcd->post_enable = disp_lcd_post_enable;;
		lcd->pre_disable = disp_lcd_pre_disable;
		lcd->post_disable  = disp_lcd_post_disable;
		lcd->tcon_enable = disp_lcd_tcon_enable;
		lcd->tcon_disable = disp_lcd_tcon_disable;
		lcd->set_panel_func = disp_lcd_set_panel_funs;
		lcd->set_open_func = disp_lcd_set_open_func;
		lcd->set_close_func = disp_lcd_set_close_func;
		lcd->get_panel_driver_name = disp_lcd_get_driver_name;

		lcd->init(lcd);
	}

	return 0;
}

