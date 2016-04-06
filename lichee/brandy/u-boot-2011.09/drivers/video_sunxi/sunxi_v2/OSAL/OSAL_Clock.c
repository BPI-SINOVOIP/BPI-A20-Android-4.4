/*
**********************************************************************************************************************
*											        eGon
*						           the Embedded GO-ON Bootloader System
*									       eGON arm boot sub-system
*
*						  Copyright(C), 2006-2010, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      : javen
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include "OSAL_Clock.h"

struct mod_clk mod_clk_array[] =
{	//		mod_id			gate_adr	shift	reset_adr	shift	mod_adr		enable	src	div
	MOD_CLK(DISP_MOD_LCD0,		0x06000588,	0,	0x060005a8,	0,	0x0600049c,	31,	24,	0)
	MOD_CLK(DISP_MOD_LCD1,		0x06000588,	1,	0x060005a8,	1,	0x060004a0,	31,	24,	0)
	MOD_CLK(DISP_MOD_EDP,		0x06000588,	2,	0x060005a8,	2,	0x06000494,	32,	32,	32)
	MOD_CLK(DISP_MOD_LVDS,		0x06000588,	32,	0x060005a8,	3,	0x0,		32,	32,	32)
	MOD_CLK(DISP_MOD_HDMI, 		0x06000588,	5,	0x060005a8,	5,	0x060004b0,	31,	24,	0)
	MOD_CLK(DISP_MOD_TOP,		0x06000588,	7, 	0x060005a8,	7,	0x06000490,	31,	33,	0)
	MOD_CLK(DISP_MOD_DSI0, 		0x06000588,	11,	0x060005a8,	11,	0x060004a8,	31,	24,	0)
	MOD_CLK(DISP_MOD_DSI1, 		0x06000588,	11,	0x060005a8,	11,	0x060004ac,	31,	24,	0)
	MOD_CLK(DISP_MOD_HDMI_DDC, 	0x0,		32,	0x0,		32,	0x060004b4,	31,	32,	32)
};

struct pll_set pll_set_array[] =
{
	PLL_SET(pll_src7,	0x06000018,	31,	16,	32,	8)
	PLL_SET(pll_src10,	0x06000024,	31,	16,	18,	8)
};

u32 dif(u32 value0, u32 value1)
{
	if(value0 > value1)
		return value0 - value1;
	else
		return value1 - value0;
}

void calc_src_coef3(u32 *div, u32 *divP, u32 *facN, u32 freq)
{
	u32 i, j, k;
	u32 temp_div = 0;
	u32 temp_divP = 0;
	u32 temp_facN = 1;
	u32 val;
	u32 temp;

	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++) {
			val = 24 * temp_facN / ((temp_div + 1) * (temp_divP + 1));
			k = freq * (i + 1) * (j + 1) / 24;
			temp = 24 * k / ((i + 1) * (j + 1));
			if(dif(val, freq) > dif(temp, freq)) {
				temp_div = i;
				temp_divP = j;
				temp_facN = k;
			}
		}
	}

	*div = temp_div;
	*divP = temp_divP;
	*facN = temp_facN;
}

void calc_src_coef2(u32 *div, u32 *facN, u32 freq)
{
	u32 temp_div0, temp_div1;
	u32 temp_facN0, temp_facN1;
	u32 val0, val1;
	temp_div0 = 0;
	temp_facN0 = freq * (temp_div0 + 1)/ 24;
	val0 = 24 * temp_facN0 / (temp_div0 + 1);

	temp_div1 = 1;
	temp_facN1 = freq * (temp_div1 + 1)/ 24;
	val1 = 24 * temp_facN1 / (temp_div1 + 1);

	if(dif(val0, freq) > dif(val1, freq)) {
		*div = temp_div1;
		*facN = temp_facN1;
	} else {
		*div = temp_div0;
		*facN = temp_facN0;
	}
}

u32 mod_clk_enable(u32 clk_id, u32 enable)
{
	struct mod_clk *clk;
	u32 i;

	for(i = 0; i < (sizeof(mod_clk_array) / sizeof(struct mod_clk)); i++) {
		if(clk_id == mod_clk_array[i].mod_id){
			u32 reg_val;
			clk = &mod_clk_array[i];

			if(mod_clk_array[i].ahb.gate_shift < 32) {
				reg_val = readl(clk->ahb.gate_adr);
				reg_val = SET_BITS(clk->ahb.gate_shift, 1, reg_val, enable);
				writel(reg_val, clk->ahb.gate_adr);
			}

			if(mod_clk_array[i].mod.enable_shift < 32) {
				reg_val = readl(clk->mod.mod_adr);
				reg_val = SET_BITS(clk->mod.enable_shift, 1, reg_val, enable);
				writel(reg_val, clk->mod.mod_adr);
			}

			if(mod_clk_array[i].ahb.reset_shift < 32) {
				reg_val = readl(clk->ahb.reset_adr);
				reg_val = SET_BITS(clk->ahb.reset_shift, 1, reg_val, enable);
				writel(reg_val, clk->ahb.reset_adr);
			}

			return 1;
		}
	}

	return 0;
}

u32 mod_clk_set_src(u32 clk_id, pll_src_sel src_sel)
{
	struct mod_clk *clk;
	u32 i;

	for(i = 0; i < (sizeof(mod_clk_array) / sizeof(struct mod_clk)); i++) {
		if((clk_id == mod_clk_array[i].mod_id) && (mod_clk_array[i].mod.src_shift < 32)) {
			u32 reg_val;
			clk = &mod_clk_array[i];

			reg_val = readl(clk->mod.mod_adr);
			reg_val = SET_BITS(clk->mod.src_shift, 4, reg_val, src_sel);
			writel(reg_val, clk->mod.mod_adr);

			return 1;
		}
	}

	return 0;
}

u32 mod_clk_set_div(u32 clk_id, u32 div)
{
	struct mod_clk *clk;
	u32 i;

	for(i = 0; i < (sizeof(mod_clk_array) / sizeof(struct mod_clk)); i++) {
		if((clk_id == mod_clk_array[i].mod_id) && (mod_clk_array[i].mod.div_shift < 32)) {
			u32 reg_val;
			clk = &mod_clk_array[i];

			reg_val = readl(clk->mod.mod_adr);
			reg_val = SET_BITS(clk->mod.div_shift, 4, reg_val, div);
			writel(reg_val, clk->mod.mod_adr);

			return 1;
		}
	}

	return 0;
}

u32 set_src_freq(u32 pll_id, u32 freq)
{
	struct pll_set *pll;
	u32 i;

	for(i = 0; i < (sizeof(pll_set_array) / sizeof(struct pll_set)); i++) {
		if(pll_id == pll_set_array[i].pll_name) {
			u32 reg_val;
			u32 div;
			u32 facN;
			pll = &pll_set_array[i];

			freq = freq / 1000000;

			if(pll->ext_div_P_shift < 32) {
				u32 divP;
				calc_src_coef3(&div, &divP, &facN, freq);

				reg_val = readl(pll->pll_adr);
				reg_val = SET_BITS(pll->div_shift, 1, reg_val, div);
				writel(reg_val, pll->pll_adr);

				reg_val = readl(pll->pll_adr);
				reg_val = SET_BITS(pll->ext_div_P_shift, 2, reg_val, divP);
				writel(reg_val, pll->pll_adr);

				reg_val = readl(pll->pll_adr);
				reg_val = SET_BITS(pll->fac_N_shift, 8, reg_val, facN);
				writel(reg_val, pll->pll_adr);
			} else {
				calc_src_coef2(&div, &facN, freq);

				reg_val = readl(pll->pll_adr);
				reg_val = SET_BITS(pll->div_shift, 1, reg_val, div);
				writel(reg_val, pll->pll_adr);

				reg_val = readl(pll->pll_adr);
				reg_val = SET_BITS(pll->fac_N_shift, 8, reg_val, facN);
				writel(reg_val, pll->pll_adr);
			}

			reg_val = readl(pll->pll_adr);
			reg_val = SET_BITS(pll->enable_shift, 1, reg_val, 1);
			writel(reg_val, pll->pll_adr);

			return 1;
		}
	}

	return 0;
}

u32 get_src_freq(u32 pll_id)
{
	struct pll_set *pll;
	u32 i;

	for(i = 0; i < (sizeof(pll_set_array) / sizeof(struct pll_set)); i++) {
		if(pll_id == pll_set_array[i].pll_name) {
			u32 reg_val;
			u32 div;
			u32 facN;
			pll = &pll_set_array[i];

			if(pll->ext_div_P_shift < 32) {
				u32 divP;
				reg_val = readl(pll->pll_adr);
				div = GET_BITS(pll->div_shift, 1, reg_val);
				divP = GET_BITS(pll->ext_div_P_shift, 1, reg_val);
				facN = GET_BITS(pll->fac_N_shift, 8, reg_val);

				return 24 * facN / ((div + 1) * (divP + 1)) * 1000000;
			} else {
				reg_val = readl(pll->pll_adr);
				div = GET_BITS(pll->div_shift, 1, reg_val);
				facN = GET_BITS(pll->fac_N_shift, 8, reg_val);

				return 24 * facN / (div + 1) * 1000000;
			}
		}
	}

	return 0;
}