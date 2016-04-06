/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Clock.h
*
* Author 		: javen
*
* Description 	: 操作系统适配层
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   	2010-09-07          1.0         create this word
*		holi			2010-12-03			1.1			实现了具体的接口
*************************************************************************************
*/

#ifndef  __OSAL_CLOCK_H__
#define  __OSAL_CLOCK_H__

#include "OSAL.h"
#include "../de_bsp/de/lowlevel/ebios_de.h"

#define CLK_ON 1
#define CLK_OFF 0

/* define module clock id       */
typedef enum
{
	CLK_NONE = 0,

	SYS_CLK_PLL3 = 1,
	SYS_CLK_PLL7 = 2,
	SYS_CLK_PLL8 = 3,
	SYS_CLK_PLL9 = 4,
	SYS_CLK_PLL10 = 5,
	SYS_CLK_PLL3X2 = 6,
	SYS_CLK_PLL6 = 7,
	SYS_CLK_PLL6x2 = 8,
	SYS_CLK_PLL7X2 = 9,
	SYS_CLK_MIPIPLL = 10,

	MOD_CLK_DEBE0 = 16,
	MOD_CLK_DEBE1 = 17,
	MOD_CLK_DEFE0 = 18,
	MOD_CLK_DEFE1 = 19,
	MOD_CLK_LCD0CH0 = 20,
	MOD_CLK_LCD0CH1 = 21,
	MOD_CLK_LCD1CH0 = 22,
	MOD_CLK_LCD1CH1 = 23,
	MOD_CLK_HDMI = 24,
	MOD_CLK_HDMI_DDC = 25,
	MOD_CLK_MIPIDSIS = 26,
	MOD_CLK_MIPIDSIP = 27,
	MOD_CLK_IEPDRC0 = 28,
	MOD_CLK_IEPDRC1 = 29,
	MOD_CLK_IEPDEU0 = 30,
	MOD_CLK_IEPDEU1 = 31,
	MOD_CLK_LVDS = 32,
	MOD_CLK_DEBE2 = 34,
	MOD_CLK_DEFE2 = 35,

	AHB_CLK_MIPIDSI = 48,
	AHB_CLK_LCD0 = 49,
	AHB_CLK_LCD1 = 50,
	AHB_CLK_HDMI = 51,
	AHB_CLK_DEBE0 =52,
	AHB_CLK_DEBE1 =53,
	AHB_CLK_DEFE0 =54,
	AHB_CLK_DEFE1 = 55,
	AHB_CLK_DEU0 = 56,
	AHB_CLK_DEU1 = 57,
	AHB_CLK_DRC0 = 58,
	AHB_CLK_DRC1 = 59,
	AHB_CLK_TVE0 = 0,
	AHB_CLK_TVE1 = 0,

	DRAM_CLK_DRC0 = 80,
	DRAM_CLK_DRC1 = 81,
	DRAM_CLK_DEU0 = 82,
	DRAM_CLK_DEU1 = 83,
	DRAM_CLK_DEFE0 = 84,
	DRAM_CLK_DEFE1 = 85,
	DRAM_CLK_DEBE0 = 86,
	DRAM_CLK_DEBE1 = 87,
}__disp_clk_id_t;

#define CLK_BE_SRC pll_src10
#define CLK_FE_SRC pll_src10
#define CLK_HDMI_SRC pll_src7
#define CLK_LCD_SRC pll_src7
#define CLK_DSI_SRC pll_src7

#ifndef __OSAL_CLOCK_MASK__
#define RESET_OSAL
#define RST_INVAILD 0
#define RST_VAILD   1
#endif

typedef struct
{
	__disp_clk_id_t       id;     /* clock id         */
	char        *name;  /* clock name       */
	struct clk  *hdl;
}__disp_clk_t;

typedef enum {
	osc_24M = 0,
	pll_src7 = 8,
	pll_src8 = 9,
	pll_src10 = 11,
}pll_src_sel;

#define MOD_CLK(_mod_id, _gate_adr, _gate_shift, _reset_adr, _reset_shift,\
				_mod_adr, _enable_shift, _src_shift, _div_shift)\
{\
	.mod_id = _mod_id,\
	.ahb = {\
		.gate_adr = _gate_adr,\
		.gate_shift = _gate_shift,\
		.reset_adr = _reset_adr,\
		.reset_shift = _reset_shift,\
	},\
	.mod = {\
		.mod_adr = _mod_adr,\
		.enable_shift = _enable_shift,\
		.src_shift = _src_shift,\
		.div_shift = _div_shift,\
	},\
},

struct mod_clk_ahb {
	u32	gate_adr;
	u32	gate_shift;
	u32	reset_adr;
	u32	reset_shift;
};

struct mod_clk_mod {
	u32	mod_adr;
	u32	enable_shift;
	u32	src_shift;
	u32	div_shift;
};

struct mod_clk {
	u32 mod_id;
	struct mod_clk_ahb	ahb;
	struct mod_clk_mod	mod;
};

#define PLL_SET(_pll_name, _pll_adr, _enable_shift, _div_shift, _ext_div_P_shift, _fac_N_shift)\
{\
	.pll_name = _pll_name,\
	.pll_adr = _pll_adr,\
	.enable_shift = _enable_shift,\
	.div_shift = _div_shift,\
	.ext_div_P_shift = _ext_div_P_shift,\
	.fac_N_shift = _fac_N_shift,\
},

struct	pll_set {
	u32	pll_name;
	u32	pll_adr;
	u32	enable_shift;
	u32	div_shift;
	u32	ext_div_P_shift;
	u32	fac_N_shift;
};

u32 mod_clk_enable(u32 clk_id, u32 enable);
u32 mod_clk_set_src(u32 clk_id, pll_src_sel src_sel);
u32 mod_clk_set_div(u32 clk_id, u32 div);
u32 set_src_freq(u32 pll_id, u32 freq);
u32 get_src_freq(u32 pll_id);

#endif   //__OSAL_CLOCK_H__

