#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libvideo_sunxi.o

COBJS-$(CONFIG_VIDEO_SUNXI_V1) += sunxi_v1/dev_disp.o \
							sunxi_v1/OSAL/OSAL_Clock.o sunxi_v1/OSAL/OSAL_Int.o sunxi_v1/OSAL/OSAL_IrqLock.o sunxi_v1/OSAL/OSAL_Cache.o\
							sunxi_v1/OSAL/OSAL_Lib_C.o sunxi_v1/OSAL/OSAL_Pin.o sunxi_v1/OSAL/OSAL_Parser.o\
							sunxi_v1/de_bsp/de/ebios/de_be.o sunxi_v1/de_bsp/de/ebios/de_fe.o sunxi_v1/de_bsp/de/ebios/de_hwc.o\
							sunxi_v1/de_bsp/de/ebios/de_layer.o sunxi_v1/de_bsp/de/ebios/de_lcd.o sunxi_v1/de_bsp/de/ebios/de_tve.o sunxi_v1/de_bsp/de/ebios/de_dsi.o\
							sunxi_v1/de_bsp/de/disp_clk.o sunxi_v1/de_bsp/de/disp_combined.o sunxi_v1/de_bsp/de/disp_de.o sunxi_v1/de_bsp/de/disp_display.o\
							sunxi_v1/de_bsp/de/disp_event.o sunxi_v1/de_bsp/de/disp_hdmi.o sunxi_v1/de_bsp/de/disp_hwc.o sunxi_v1/de_bsp/de/disp_layer.o\
							sunxi_v1/de_bsp/de/disp_lcd.o sunxi_v1/de_bsp/de/disp_scaler.o sunxi_v1/de_bsp/de/disp_tv.o\
							sunxi_v1/de_bsp/de/disp_vga.o sunxi_v1/de_bsp/de/disp_video.o sunxi_v1/de_bsp/de/disp_capture.o\
							sunxi_v1/de_bsp/iep/cmu/iep_cmu.o sunxi_v1/de_bsp/de/disp_iep.o sunxi_v1/de_bsp/de/disp_features.o\
							sunxi_v1/de_bsp/iep/drc/iep_drc.o sunxi_v1/de_bsp/iep/drc/iep_drc_ebios.o\
							sunxi_v1/de_bsp/iep/deu/iep_deu.o sunxi_v1/de_bsp/iep/deu/iep_deu_ebios.o\
							sunxi_v1/de_bsp/de/disp_lcd_iic.o\
							sunxi_v1/de_bsp/lcd/lcd_panel_cfg.o sunxi_v1/de_bsp/lcd/lcd_source_interface.o sunxi_v1/de_bsp/lcd/dev_lcd.o\
							sunxi_v1/de_bsp/lcd/panels/panels.o sunxi_v1/de_bsp/lcd/panels/default_panel.o\
							sunxi_v1/de_bsp/lcd/panels/lcd_edp_anx9804.o sunxi_v1/de_bsp/lcd/panels/lcd_edp_anx6345.o sunxi_v1/de_bsp/lcd/panels/lcd_b079xan01.o\
							sunxi_v1/de_bsp/lcd/panels/lp079x01.o\
							sunxi_v1/de_bsp/hdmi/aw/hdmi_core.o sunxi_v1/de_bsp/hdmi/aw/hdmi_edid.o sunxi_v1/de_bsp/hdmi/aw/hdmi_hal.o sunxi_v1/de_bsp/hdmi/aw/hdmi_interface.o\


COBJS-$(CONFIG_VIDEO_SUNXI_V2) += sunxi_v2/dev_disp.o \
							sunxi_v2/OSAL/OSAL_Clock.o sunxi_v2/OSAL/OSAL_Int.o sunxi_v2/OSAL/OSAL_IrqLock.o sunxi_v2/OSAL/OSAL_Cache.o\
							sunxi_v2/OSAL/OSAL_Lib_C.o sunxi_v2/OSAL/OSAL_Pin.o sunxi_v2/OSAL/OSAL_Parser.o sunxi_v2/OSAL/OSAL_Pwm.o\
							sunxi_v2/de_bsp/de/lowlevel/de_be.o sunxi_v2/de_bsp/de/lowlevel/de_fe.o sunxi_v2/de_bsp/de/lowlevel/de_hwc.o sunxi_v2/de_bsp/de/lowlevel/de_top_clk.o\
							sunxi_v2/de_bsp/de/lowlevel/de_layer.o sunxi_v2/de_bsp/de/lowlevel/de_lcd.o sunxi_v2/de_bsp/de/lowlevel/de_dsi.o sunxi_v2/de_bsp/de/lowlevel/vga_std.o\
							sunxi_v2/de_bsp/de/lowlevel/iep_cmu.o sunxi_v2/de_bsp/de/lowlevel/iep_deu.o sunxi_v2/de_bsp/de/lowlevel/iep_deu_ebios.o\
							sunxi_v2/de_bsp/de/lowlevel/iep_drc.o sunxi_v2/de_bsp/de/lowlevel/iep_drc_ebios.o\
							sunxi_v2/de_bsp/de/disp_al.o sunxi_v2/de_bsp/de/disp_features.o sunxi_v2/de_bsp/de/disp_private.o sunxi_v2/de_bsp/de/disp_hdmi.o\
							sunxi_v2/de_bsp/de/disp_lcd.o sunxi_v2/de_bsp/de/disp_layer.o sunxi_v2/de_bsp/de/disp_cursor.o sunxi_v2/de_bsp/de/disp_manager.o\
							sunxi_v2/de_bsp/de/disp_smart_backlight.o sunxi_v2/de_bsp/de/disp_smart_color.o sunxi_v2/de_bsp/de/disp_display.o\
							sunxi_v2/de_bsp/lcd/lcd_panel_cfg.o  sunxi_v2/de_bsp/lcd/dev_lcd.o sunxi_v2/de_bsp/lcd/lcd_source_interface.o\
							sunxi_v2/de_bsp/lcd/panels/panels.o sunxi_v2/de_bsp/lcd/panels/default_panel.o\

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################



