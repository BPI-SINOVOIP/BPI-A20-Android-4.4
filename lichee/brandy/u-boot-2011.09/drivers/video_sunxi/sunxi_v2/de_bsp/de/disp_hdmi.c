#include "disp_hdmi.h"
struct disp_hdmi_private_data {
	u32 enabled;

	disp_tv_mode mode;

	disp_hdmi_func hdmi_func;
	disp_video_timing *video_info;

	disp_clk_info_t hdmi_clk;
	disp_clk_info_t hdmi_ddc_clk;
	disp_clk_info_t lcd_clk;
	disp_clk_info_t drc_clk;
};

u32 hdmi_used = 0;
u32 hdmi_init_flags = 0;

#if defined(__LINUX_PLAT__)
static spinlock_t hdmi_data_lock;
#endif

static struct disp_hdmi *hdmis = NULL;
static struct disp_hdmi_private_data *hdmi_private = NULL;
s32 disp_hdmi_set_mode(struct disp_hdmi* hdmi, disp_tv_mode mode);
s32 disp_hdmi_enable(struct disp_hdmi* hdmi);

struct disp_hdmi* disp_get_hdmi(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	    }

	if(!(bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_HDMI)) {
	    DE_WRN("screen_id %d do not support HDMI TYPE!\n", screen_id);
	    return NULL;
	    }

	return &hdmis[screen_id];
}

struct disp_hdmi_private_data *disp_hdmi_get_priv(struct disp_hdmi *hdmi)
{
	if(NULL == hdmi) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &hdmi_private[hdmi->channel_id];
}

//----------------------------
//----hdmi local functions----
//----------------------------
s32 hdmi_clk_init(struct disp_hdmi *hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	    }

	return 0;
}

s32 hdmi_clk_exit(struct disp_hdmi *hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	    }

	return 0;
}

s32 hdmi_clk_config(struct disp_hdmi *hdmi)
{
//	u32 pll_freq;
//	u32 clk_div;

	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	    }

	return 0;
}

s32 hdmi_clk_enable(struct disp_hdmi *hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	    }

	return 0;
}

s32 hdmi_clk_disable(struct disp_hdmi *hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	    }

	disp_al_hdmi_disable(hdmi->channel_id);

	return 0;
}

//--------------------------------
//----hdmi interface functions----
//--------------------------------

s32 disp_hdmi_set_func(struct disp_hdmi*  hdmi, disp_hdmi_func * func)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	hdmip->hdmi_func.hdmi_open = func->hdmi_open;
	hdmip->hdmi_func.hdmi_close= func->hdmi_close;
	hdmip->hdmi_func.hdmi_set_mode= func->hdmi_set_mode;
	hdmip->hdmi_func.hdmi_mode_support= func->hdmi_mode_support;
	hdmip->hdmi_func.hdmi_get_input_csc= func->hdmi_get_input_csc;
	hdmip->hdmi_func.hdmi_set_pll = func->hdmi_set_pll;

	return 0;
}

s32 disp_hdmi_set_video_info(struct disp_hdmi*  hdmi, disp_video_timing *video_info)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	hdmip->video_info = &video_info[hdmip->mode];

	if(hdmip->video_info == NULL) {
	    DE_WRN("hdmi set video info failed!\n");
	    return DIS_FAIL;
	    }

	return 0;
}

s32 disp_hdmi_init(struct disp_hdmi*  hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi init null hdl!\n");
	    return DIS_FAIL;
	    }

	hdmi_clk_init(hdmi);
//	disp_al_hdmi_init(hdmi->channel_id);
	return 0;
}

s32 disp_hdmi_exit(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if(!hdmi || !hdmip) {
	    DE_WRN("hdmi init null hdl!\n");
	    return DIS_FAIL;
	    }
	disp_al_hdmi_exit(hdmi->channel_id);
	hdmi_clk_exit(hdmi);

    return 0;
}

s32 disp_hdmi_enable(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	hdmi_clk_enable(hdmi);
	disp_al_hdmi_init(hdmi->channel_id);
	disp_al_hdmi_cfg(hdmi->channel_id, hdmip->video_info);
	disp_al_hdmi_enable(hdmi->channel_id);

	if(hdmip->hdmi_func.hdmi_open == NULL)
	    return -1;

	hdmip->hdmi_func.hdmi_open();

#if defined(__LINUX_PLAT__)
	{
	unsigned long flags;
	spin_lock_irqsave(&hdmi_data_lock, flags);
#endif
	hdmip->enabled = 1;
#if defined(__LINUX_PLAT__)
	    spin_unlock_irqrestore(&hdmi_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_hdmi_disable(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
	    DE_WRN("hdmi set func null  hdl!\n");
	    return DIS_FAIL;
	}

	disp_al_hdmi_disable(hdmi->channel_id);
	hdmi_clk_disable(hdmi);

	if(hdmip->hdmi_func.hdmi_close == NULL)
	    return -1;

	hdmip->hdmi_func.hdmi_close();

#if defined(__LINUX_PLAT__)
	{
	unsigned long flags;
	spin_lock_irqsave(&hdmi_data_lock, flags);
#endif
	hdmip->enabled = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&hdmi_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_hdmi_is_enabled(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
	    DE_WRN("hdmi set func null  hdl!\n");
	    return DIS_FAIL;
	}

	if(hdmi->is_enabled)
		return hdmip->enabled;

	return DIS_FAIL;
}


s32 disp_hdmi_set_mode(struct disp_hdmi* hdmi, disp_tv_mode mode)
{
	s32 ret = 0;
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if(hdmip->hdmi_func.hdmi_set_mode == NULL)
	    return -1;

	ret = hdmip->hdmi_func.hdmi_set_mode(mode);
	if(ret == 0)
		hdmip->mode = mode;

	return ret;
}

s32 disp_hdmi_get_mode(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	return hdmip->mode;
}

s32 disp_hdmi_check_support_mode(struct disp_hdmi* hdmi, u8 mode)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if(hdmip->hdmi_func.hdmi_mode_support == NULL)
	    return -1;

	return hdmip->hdmi_func.hdmi_mode_support(mode);
}

s32 disp_hdmi_get_input_csc(struct disp_hdmi* hdmi)
{
	struct disp_hdmi_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if(hdmip->hdmi_func.hdmi_get_input_csc == NULL)
	    return -1;

	return hdmip->hdmi_func.hdmi_get_input_csc();
}

s32 disp_init_hdmi(__disp_bsp_init_para * para)
{
	s32 ret;
	s32 value;
	//get sysconfig hdmi_used
	ret = OSAL_Script_FetchParser_Data("hdmi_para", "hdmi_used", &value, 1);
	if(ret == 0)
		hdmi_used = value;

	//hdmi_used == 1
	if(hdmi_used) {
		u32 num_screens;
		u32 screen_id;
		struct disp_hdmi* hdmi;
		struct disp_hdmi_private_data* hdmip;

		DE_INF("disp_init_hdmi\n");
#if defined(__LINUX_PLAT__)
		spin_lock_init(&hdmi_data_lock);
	    #endif

		num_screens = bsp_disp_feat_get_num_screens();
		hdmis = (struct disp_hdmi *)OSAL_malloc(sizeof(struct disp_hdmi) * num_screens);
		if(NULL == hdmis) {
			DE_WRN("malloc memory fail!\n");
			return DIS_FAIL;
			}

		hdmi_private = (struct disp_hdmi_private_data *)OSAL_malloc(sizeof(struct disp_hdmi_private_data) * num_screens);
		if(NULL == hdmi_private) {
			DE_WRN("malloc memory fail!\n");
			return DIS_FAIL;
			}

		for(screen_id=0; screen_id<num_screens; screen_id++) {
			hdmi = &hdmis[screen_id];
			hdmip = &hdmi_private[screen_id];

		switch(screen_id) {
		case 0:
			hdmi->channel_id = 0;
			hdmi->name = "hdmi0";
			hdmi->type = DISP_OUTPUT_TYPE_HDMI;
			hdmip->hdmi_clk.clk = DISP_MOD_HDMI;

			hdmip->hdmi_ddc_clk.clk = DISP_MOD_HDMI_DDC;
			hdmip->hdmi_clk.clk_src = CLK_HDMI_SRC;
			hdmip->lcd_clk.clk = DISP_MOD_LCD0;
			hdmip->lcd_clk.clk_src = CLK_LCD_SRC;
			hdmip->drc_clk.clk = DISP_MOD_DRC0;
			hdmip->drc_clk.clk_src = CLK_BE_SRC;
			hdmip->drc_clk.clk_div = 3;
			break;

		case 1:
			hdmi->channel_id = 1;
			hdmi->name = "hdmi1";
			hdmi->type = DISP_OUTPUT_TYPE_HDMI;
			hdmip->hdmi_clk.clk = DISP_MOD_HDMI;

			hdmip->hdmi_ddc_clk.clk = DISP_MOD_HDMI_DDC;
			hdmip->hdmi_clk.clk_src = CLK_HDMI_SRC;
			hdmip->lcd_clk.clk = DISP_MOD_LCD1;
			hdmip->lcd_clk.clk_src = CLK_LCD_SRC;
			hdmip->drc_clk.clk = DISP_MOD_DRC1;
			hdmip->drc_clk.clk_src = CLK_BE_SRC;
			hdmip->drc_clk.clk_div = 3;
			break;

		case 2:
			break;
			}
		/*
		hdmip->hdmi_func = NULL;
		hdmip->hdmi_func->hdmi_open = NULL;
		hdmip->hdmi_func->hdmi_close = NULL;
		hdmip->hdmi_func->hdmi_set_mode= NULL;
		hdmip->hdmi_func->hdmi_mode_support= NULL;
		hdmip->hdmi_func->hdmi_get_input_csc= NULL;*/
		hdmip->mode = DISP_TV_MOD_720P_50HZ;

		hdmi->init = disp_hdmi_init;
		hdmi->exit = disp_hdmi_exit;

		hdmi->set_func = disp_hdmi_set_func;
		hdmi->set_video_info = disp_hdmi_set_video_info;
		hdmi->enable = disp_hdmi_enable;
		hdmi->disable = disp_hdmi_disable;
		hdmi->is_enabled = disp_hdmi_is_enabled;
		hdmi->set_mode = disp_hdmi_set_mode;
		hdmi->get_mode = disp_hdmi_get_mode;
		hdmi->check_support_mode = disp_hdmi_check_support_mode;
		hdmi->get_input_csc = disp_hdmi_get_input_csc;

		hdmi->init(hdmi);
	        }
	    }
	return 0;
}
