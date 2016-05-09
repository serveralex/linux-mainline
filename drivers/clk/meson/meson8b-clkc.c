/*
 * Copyright (c) 2015 Endless Mobile, Inc.
 * Author: Carlo Caione <carlo@endlessm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <dt-bindings/clock/meson8b-clkc.h>

#include "clkc.h"

#define MESON8B_REG_CTL0_ADDR		0x0000
#define MESON8B_REG_SYS_CPU_CNTL1	0x015c
#define MESON8B_REG_HHI_GCLK_MPEG0	0x0140
#define MESON8B_REG_HHI_GCLK_MPEG1	0x0144
#define MESON8B_REG_HHI_GCLK_MPEG2	0x0148
#define MESON8B_REG_HHI_MPEG		0x0174
#define MESON8B_REG_MALI		0x01b0
#define MESON8B_REG_PLL_FIXED		0x0280
#define MESON8B_REG_PLL_SYS		0x0300
#define MESON8B_REG_PLL_VID		0x0320

static const struct pll_rate_table sys_pll_rate_table[] = {
	PLL_RATE(312000000, 52, 1, 2),
	PLL_RATE(336000000, 56, 1, 2),
	PLL_RATE(360000000, 60, 1, 2),
	PLL_RATE(384000000, 64, 1, 2),
	PLL_RATE(408000000, 68, 1, 2),
	PLL_RATE(432000000, 72, 1, 2),
	PLL_RATE(456000000, 76, 1, 2),
	PLL_RATE(480000000, 80, 1, 2),
	PLL_RATE(504000000, 84, 1, 2),
	PLL_RATE(528000000, 88, 1, 2),
	PLL_RATE(552000000, 92, 1, 2),
	PLL_RATE(576000000, 96, 1, 2),
	PLL_RATE(600000000, 50, 1, 1),
	PLL_RATE(624000000, 52, 1, 1),
	PLL_RATE(648000000, 54, 1, 1),
	PLL_RATE(672000000, 56, 1, 1),
	PLL_RATE(696000000, 58, 1, 1),
	PLL_RATE(720000000, 60, 1, 1),
	PLL_RATE(744000000, 62, 1, 1),
	PLL_RATE(768000000, 64, 1, 1),
	PLL_RATE(792000000, 66, 1, 1),
	PLL_RATE(816000000, 68, 1, 1),
	PLL_RATE(840000000, 70, 1, 1),
	PLL_RATE(864000000, 72, 1, 1),
	PLL_RATE(888000000, 74, 1, 1),
	PLL_RATE(912000000, 76, 1, 1),
	PLL_RATE(936000000, 78, 1, 1),
	PLL_RATE(960000000, 80, 1, 1),
	PLL_RATE(984000000, 82, 1, 1),
	PLL_RATE(1008000000, 84, 1, 1),
	PLL_RATE(1032000000, 86, 1, 1),
	PLL_RATE(1056000000, 88, 1, 1),
	PLL_RATE(1080000000, 90, 1, 1),
	PLL_RATE(1104000000, 92, 1, 1),
	PLL_RATE(1128000000, 94, 1, 1),
	PLL_RATE(1152000000, 96, 1, 1),
	PLL_RATE(1176000000, 98, 1, 1),
	PLL_RATE(1200000000, 50, 1, 0),
	PLL_RATE(1224000000, 51, 1, 0),
	PLL_RATE(1248000000, 52, 1, 0),
	PLL_RATE(1272000000, 53, 1, 0),
	PLL_RATE(1296000000, 54, 1, 0),
	PLL_RATE(1320000000, 55, 1, 0),
	PLL_RATE(1344000000, 56, 1, 0),
	PLL_RATE(1368000000, 57, 1, 0),
	PLL_RATE(1392000000, 58, 1, 0),
	PLL_RATE(1416000000, 59, 1, 0),
	PLL_RATE(1440000000, 60, 1, 0),
	PLL_RATE(1464000000, 61, 1, 0),
	PLL_RATE(1488000000, 62, 1, 0),
	PLL_RATE(1512000000, 63, 1, 0),
	PLL_RATE(1536000000, 64, 1, 0),
	{ /* sentinel */ },
};

static const struct clk_div_table cpu_div_table[] = {
	{ .val = 1, .div = 1 },
	{ .val = 2, .div = 2 },
	{ .val = 3, .div = 3 },
	{ .val = 2, .div = 4 },
	{ .val = 3, .div = 6 },
	{ .val = 4, .div = 8 },
	{ .val = 5, .div = 10 },
	{ .val = 6, .div = 12 },
	{ .val = 7, .div = 14 },
	{ .val = 8, .div = 16 },
	{ /* sentinel */ },
};

PNAME(p_xtal)		= { "xtal" };
PNAME(p_fclk_div)	= { "fixed_pll" };
PNAME(p_cpu_clk)	= { "sys_pll" };
PNAME(p_clk81)		= { "fclk_div3", "fclk_div4", "fclk_div5" };
PNAME(p_mali)		= { "fclk_div3", "fclk_div4", "fclk_div5",
			    "fclk_div7", "zero" };
PNAME(p_clk81_gate)	= { "clk81" };

static u32 mux_table_clk81[]	= { 6, 5, 7 };
static u32 mux_table_mali[]	= { 6, 5, 7, 4, 0 };

static struct pll_conf pll_confs = {
	.m		= PARM(0x00, 0,  9),
	.n		= PARM(0x00, 9,  5),
	.od		= PARM(0x00, 16, 2),
};

static struct pll_conf sys_pll_conf = {
	.m		= PARM(0x00, 0,  9),
	.n		= PARM(0x00, 9,  5),
	.od		= PARM(0x00, 16, 2),
	.rate_table	= sys_pll_rate_table,
};

static const struct composite_conf clk81_conf __initconst = {
	.mux_table		= mux_table_clk81,
	.mux_flags		= CLK_MUX_READ_ONLY,
	.mux_parm		= PARM(0x00, 12, 3),
	.div_parm		= PARM(0x00, 0, 7),
	.gate_parm		= PARM(0x00, 7, 1),
};

static const struct composite_conf mali_conf __initconst = {
	.mux_table		= mux_table_mali,
	.mux_parm		= PARM(0x00, 9, 3),
	.div_parm		= PARM(0x00, 0, 7),
	.gate_parm		= PARM(0x00, 8, 1),
};

static const struct clk_conf meson8b_xtal_conf __initconst =
	FIXED_RATE_P(MESON8B_REG_CTL0_ADDR, CLKID_XTAL, "xtal",
		     CLK_IS_ROOT, PARM(0x00, 4, 7));

static const struct clk_conf meson8b_clk_confs[] __initconst = {
	FIXED_RATE(CLKID_ZERO, "zero", CLK_IS_ROOT, 0),
	PLL(MESON8B_REG_PLL_FIXED, CLKID_PLL_FIXED, "fixed_pll",
	    p_xtal, 0, &pll_confs),
	PLL(MESON8B_REG_PLL_VID, CLKID_PLL_VID, "vid_pll",
	    p_xtal, 0, &pll_confs),
	PLL(MESON8B_REG_PLL_SYS, CLKID_PLL_SYS, "sys_pll",
	    p_xtal, 0, &sys_pll_conf),
	FIXED_FACTOR_DIV(CLKID_FCLK_DIV2, "fclk_div2", p_fclk_div, 0, 2),
	FIXED_FACTOR_DIV(CLKID_FCLK_DIV3, "fclk_div3", p_fclk_div, 0, 3),
	FIXED_FACTOR_DIV(CLKID_FCLK_DIV4, "fclk_div4", p_fclk_div, 0, 4),
	FIXED_FACTOR_DIV(CLKID_FCLK_DIV5, "fclk_div5", p_fclk_div, 0, 5),
	FIXED_FACTOR_DIV(CLKID_FCLK_DIV7, "fclk_div7", p_fclk_div, 0, 7),
	CPU(MESON8B_REG_SYS_CPU_CNTL1, CLKID_CPUCLK, "a5_clk", p_cpu_clk,
	    cpu_div_table),
	COMPOSITE(MESON8B_REG_HHI_MPEG, CLKID_CLK81, "clk81", p_clk81,
		  CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED, &clk81_conf),
	COMPOSITE(MESON8B_REG_MALI, CLKID_MALI, "mali", p_mali,
		  CLK_IGNORE_UNUSED, &mali_conf),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_DDR, "ddr", p_clk81_gate,
		CLK_IGNORE_UNUSED, 0),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_DOS, "dos", p_clk81_gate,
		CLK_IGNORE_UNUSED, 1),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_0, "reserved0_0",
		p_clk81_gate, CLK_IGNORE_UNUSED, 2),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_1, "reserved0_1",
		p_clk81_gate, CLK_IGNORE_UNUSED, 3),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_AHB_BRIDGE, "ahb_bridge",
		p_clk81_gate, CLK_IGNORE_UNUSED, 4),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_ISA, "isa", p_clk81_gate,
		CLK_IGNORE_UNUSED, 5),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_PL310_CBUS, "pl310_cbus",
		p_clk81_gate, CLK_IGNORE_UNUSED, 6),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_PPERIPHS_TOP, "pperiphs_top",
		p_clk81_gate, CLK_IGNORE_UNUSED, 7),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SPICC, "spicc", p_clk81_gate,
		CLK_IGNORE_UNUSED, 8),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_I2C, "i2c", p_clk81_gate,
		CLK_IGNORE_UNUSED, 9),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SAR_ADC, "sar_adc", p_clk81_gate,
		CLK_IGNORE_UNUSED, 10),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SMART_CARD_MPEG_DOMAIN,
		"smart_card_mpeg_domain", p_clk81_gate, CLK_IGNORE_UNUSED, 11),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RANDOM_NUM_GEN, "random_num_gen",
		p_clk81_gate, CLK_IGNORE_UNUSED, 12),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_UART0, "uart0", p_clk81_gate,
		CLK_IGNORE_UNUSED, 13),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SDHC, "sdhc", p_clk81_gate,
		CLK_IGNORE_UNUSED, 14),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_STREAM, "stream", p_clk81_gate,
		CLK_IGNORE_UNUSED, 15),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_ASYNC_FIFO, "async_fifo",
		p_clk81_gate, CLK_IGNORE_UNUSED, 16),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SDIO, "sdio", p_clk81_gate,
		CLK_IGNORE_UNUSED, 17),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_AUD_BUF, "auf_buf", p_clk81_gate,
		CLK_IGNORE_UNUSED, 18),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_HIU_PARSER, "hiu_parser",
		p_clk81_gate, CLK_IGNORE_UNUSED, 19),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_2, "reserved0_2",
		p_clk81_gate, CLK_IGNORE_UNUSED, 20),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_3, "reserved0_3",
		p_clk81_gate, CLK_IGNORE_UNUSED, 21),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_4, "reserved0_4",
		p_clk81_gate, CLK_IGNORE_UNUSED, 22),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_ASSIST_MISC, "assist_misc",
		p_clk81_gate, CLK_IGNORE_UNUSED, 23),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_5, "reserved0_5",
		p_clk81_gate, CLK_IGNORE_UNUSED, 24),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_6, "reserved0_6",
		p_clk81_gate, CLK_IGNORE_UNUSED, 25),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_7, "reserved0_7",
		p_clk81_gate, CLK_IGNORE_UNUSED, 26),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_8, "reserved0_8",
		p_clk81_gate, CLK_IGNORE_UNUSED, 27),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_9, "reserved0_9",
		p_clk81_gate, CLK_IGNORE_UNUSED, 28),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_A, "reserved0_a",
		p_clk81_gate, CLK_IGNORE_UNUSED, 29),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_SPI, "api", p_clk81_gate,
		CLK_IGNORE_UNUSED, 30),
	GATE(MESON8B_REG_HHI_GCLK_MPEG0, CLKID_RESERVED0_B, "reserved0_b",
		p_clk81_gate, CLK_IGNORE_UNUSED, 31),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_1, "reserved1_1",
		p_clk81_gate, CLK_IGNORE_UNUSED, 0),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_2, "reserved1_2",
		p_clk81_gate, CLK_IGNORE_UNUSED, 1),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AUD_IN, "aud_in", p_clk81_gate,
		CLK_IGNORE_UNUSED, 2),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_ETHERNET, "ethernet",
		p_clk81_gate, CLK_IGNORE_UNUSED, 3),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_DEMUX, "demux", p_clk81_gate,
		CLK_IGNORE_UNUSED, 4),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_3, "reserved1_3",
		p_clk81_gate, CLK_IGNORE_UNUSED, 5),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_AI_TOP_GLUE,
		"aiu_ai_top_glue", p_clk81_gate, CLK_IGNORE_UNUSED, 6),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_IEC958, "aiu_iec958",
		p_clk81_gate, CLK_IGNORE_UNUSED, 7),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_I2S_OUT, "aiu_i2s_out",
		p_clk81_gate, CLK_IGNORE_UNUSED, 8),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_AMCLK_MEASURE,
		"aiu_amclk_measure", p_clk81_gate, CLK_IGNORE_UNUSED, 9),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_AIFIFO2, "aiu_aififo2",
		p_clk81_gate, CLK_IGNORE_UNUSED, 10),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_AUD_MIXER, "aiu_aud_mixer",
		p_clk81_gate, CLK_IGNORE_UNUSED, 11),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_MIXER_REG, "aiu_mixer_reg",
		p_clk81_gate, CLK_IGNORE_UNUSED, 12),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_ADC, "aiu_adc",
		p_clk81_gate, CLK_IGNORE_UNUSED, 13),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_BLK_MOV, "blk_mov",
		p_clk81_gate, CLK_IGNORE_UNUSED, 14),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AIU_TOP_LEVEL, "aiu_top_level",
		p_clk81_gate, CLK_IGNORE_UNUSED, 15),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_UART1, "uart1", p_clk81_gate,
		CLK_IGNORE_UNUSED, 16),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_4, "reserved1_4",
		p_clk81_gate, CLK_IGNORE_UNUSED, 17),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_5, "reserved1_5",
		p_clk81_gate, CLK_IGNORE_UNUSED, 18),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_6, "reserved1_6",
		p_clk81_gate, CLK_IGNORE_UNUSED, 19),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_GE2D, "ge2d", p_clk81_gate,
		CLK_IGNORE_UNUSED, 20),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_USB0, "usb0", p_clk81_gate,
		CLK_IGNORE_UNUSED, 21),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_USB1, "usb1", p_clk81_gate,
		CLK_IGNORE_UNUSED, 22),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESET, "reset", p_clk81_gate,
		CLK_IGNORE_UNUSED, 23),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_NAND, "nand", p_clk81_gate,
		CLK_IGNORE_UNUSED, 24),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_HIU_PARSER_TOP,
		"hiu_parser_top", p_clk81_gate, CLK_IGNORE_UNUSED, 25),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_USB_GENERAL, "usb_general",
		p_clk81_gate, CLK_IGNORE_UNUSED, 26),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_RESERVED1_7, "reserved1_7",
		p_clk81_gate, CLK_IGNORE_UNUSED, 27),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_VDIN1, "vdin1", p_clk81_gate,
		CLK_IGNORE_UNUSED, 28),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_AHB_ARB0, "ahb_arb0",
		p_clk81_gate, CLK_IGNORE_UNUSED, 29),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_EFUSE, "efuse", p_clk81_gate,
		CLK_IGNORE_UNUSED, 30),
	GATE(MESON8B_REG_HHI_GCLK_MPEG1, CLKID_ROM_CLK, "rom_clk",
		p_clk81_gate, CLK_IGNORE_UNUSED, 31),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_0, "reserved2_0",
		p_clk81_gate, CLK_IGNORE_UNUSED, 0),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_AHB_DATA_BUS, "ahb_data_bus",
		p_clk81_gate, CLK_IGNORE_UNUSED, 1),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_AHB_CONTROL_BUS,
		"ahb_control_bus", p_clk81_gate, CLK_IGNORE_UNUSED, 2),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_HDMI_INTR_SYNC,
		"hdmi_intr_sync", p_clk81_gate, CLK_IGNORE_UNUSED, 3),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_HDMI_PCLK, "hdmi_pclk",
		p_clk81_gate, CLK_IGNORE_UNUSED, 4),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_1, "reserved2_1",
		p_clk81_gate, CLK_IGNORE_UNUSED, 5),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_2, "reserved2_2",
		p_clk81_gate, CLK_IGNORE_UNUSED, 6),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_3, "reserved2_3",
		p_clk81_gate, CLK_IGNORE_UNUSED, 7),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_MISC_USB1_TO_DDR,
		"misc_usb1_to_ddr", p_clk81_gate, CLK_IGNORE_UNUSED, 8),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_MISC_USB0_TO_DDR,
		"misc_usb0_to_ddr", p_clk81_gate, CLK_IGNORE_UNUSED, 9),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_4, "reserved2_4",
		p_clk81_gate, CLK_IGNORE_UNUSED, 10),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_MMC_PCLK, "mmc_pclk",
		p_clk81_gate, CLK_IGNORE_UNUSED, 11),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_MISC_DVIN, "misc_dvin",
		p_clk81_gate, CLK_IGNORE_UNUSED, 12),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_5, "reserved2_5",
		p_clk81_gate, CLK_IGNORE_UNUSED, 13),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_6, "reserved2_6",
		p_clk81_gate, CLK_IGNORE_UNUSED, 14),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_UART2, "uart2", p_clk81_gate,
		CLK_IGNORE_UNUSED, 15),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_7, "reserved2_7",
		p_clk81_gate, CLK_IGNORE_UNUSED, 16),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_8, "reserved2_8",
		p_clk81_gate, CLK_IGNORE_UNUSED, 17),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_9, "reserved2_9",
		p_clk81_gate, CLK_IGNORE_UNUSED, 18),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_A, "reserved2_a",
		p_clk81_gate, CLK_IGNORE_UNUSED, 19),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_B, "reserved2_b",
		p_clk81_gate, CLK_IGNORE_UNUSED, 20),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_C, "reserved2_c",
		p_clk81_gate, CLK_IGNORE_UNUSED, 21),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_SANA, "sana", p_clk81_gate,
		CLK_IGNORE_UNUSED, 22),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_D, "reserved2_d",
		p_clk81_gate, CLK_IGNORE_UNUSED, 23),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_E, "reserved2_e",
		p_clk81_gate, CLK_IGNORE_UNUSED, 24),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_VPU_INTR, "vpu_intr",
		p_clk81_gate, CLK_IGNORE_UNUSED, 25),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_SECURE_AHP_APB3,
		"secure_ahp_apb3", p_clk81_gate, CLK_IGNORE_UNUSED, 26),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_F, "reserved2_f",
		p_clk81_gate, CLK_IGNORE_UNUSED, 27),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_10, "reserved2_10",
		p_clk81_gate, CLK_IGNORE_UNUSED, 28),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_TO_A9, "to_a9", p_clk81_gate,
		CLK_IGNORE_UNUSED, 29),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_11, "reserved2_11",
		p_clk81_gate, CLK_IGNORE_UNUSED, 30),
	GATE(MESON8B_REG_HHI_GCLK_MPEG2, CLKID_RESERVED2_12, "reserved2_12",
		p_clk81_gate, CLK_IGNORE_UNUSED, 31),
};

static void __init meson8b_clkc_init(struct device_node *np)
{
	void __iomem *clk_base;

	if (!meson_clk_init(np, CLK_NR_CLKS))
		return;

	/* XTAL */
	clk_base = of_iomap(np, 0);
	if (!clk_base) {
		pr_err("%s: Unable to map xtal base\n", __func__);
		return;
	}

	meson_clk_register_clks(&meson8b_xtal_conf, 1, clk_base);
	iounmap(clk_base);

	/*  Generic clocks and PLLs */
	clk_base = of_iomap(np, 1);
	if (!clk_base) {
		pr_err("%s: Unable to map clk base\n", __func__);
		return;
	}

	meson_clk_register_clks(meson8b_clk_confs,
				ARRAY_SIZE(meson8b_clk_confs),
				clk_base);
}
CLK_OF_DECLARE(meson8b_clock, "amlogic,meson8b-clkc", meson8b_clkc_init);
