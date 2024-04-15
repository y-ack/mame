// license:BSD-3-Clause
// copyright-holders:Bryan McPhail, ywy, 12Me21
#ifndef MAME_TAITO_TAITO_F3_H
#define MAME_TAITO_TAITO_F3_H

#pragma once

#include "taito_en.h"
#include "tc0630fdp.h"
#include "machine/eepromser.h"
#include "machine/watchdog.h"
#include "sound/okim6295.h"
#include "screen.h"

class taito_f3_state : public driver_device
{
public:
	taito_f3_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_watchdog(*this, "watchdog"),
		m_screen(*this, "screen"),
		//m_palette(*this, "palette"),
		//m_palette_12bit(*this, "palette_12bit"),
		m_eeprom(*this, "eeprom"),
		m_input(*this, "IN.%u", 0),
		m_dial(*this, "DIAL.%u", 0),
		m_eepromin(*this, "EEPROMIN"),
		m_eepromout(*this, "EEPROMOUT"),
		m_fdp(*this, "fdp"),
		m_taito_en(*this, "taito_en"),
		m_oki(*this, "oki"),
		m_paletteram32(*this, "paletteram"),
		m_okibank(*this, "okibank")
	{ }

	void f3(machine_config &config);
	void f3_224a(machine_config &config);
	void bubsympb(machine_config &config);
	void f3_224b(machine_config &config);
	void f3_224c(machine_config &config);

	void init_commandw();
	void init_pbobble2();
	void init_puchicar();
	void init_intcup94();
	void init_landmakr();
	void init_twinqix();
	void init_elvactr();
	void init_arabianm();
	void init_bubsympb();
	void init_ktiger2();
	void init_lightbr();
	void init_gekirido();
	void init_arkretrn();
	void init_kirameki();
	void init_qtheater();
	void init_popnpop();
	void init_spcinvdj();
	void init_pbobbl2p();
	void init_landmkrp();
	void init_bubblem();
	void init_ridingf();
	void init_gseeker();
	void init_bubsymph();
	void init_hthero95();
	void init_gunlock();
	void init_pbobble4();
	void init_dariusg();
	void init_recalh();
	void init_kaiserkn();
	void init_spcinv95();
	void init_trstaroj();
	void init_ringrage();
	void init_cupfinal();
	void init_quizhuhu();
	void init_pbobble3();
	void init_cleopatr();
	void init_scfinals();
	void init_pbobbl2x();

	template <int Num> DECLARE_CUSTOM_INPUT_MEMBER(f3_analog_r);
	template <int Num> DECLARE_CUSTOM_INPUT_MEMBER(f3_coin_r);
	DECLARE_CUSTOM_INPUT_MEMBER(eeprom_read);

protected:
	using fixed8 = s32;

	// should be 30.47618_MHz_XTAL / 2
	static inline constexpr XTAL F3_MAIN_CLK = 16_MHz_XTAL;

	struct F3config;
	/* This it the best way to allow game specific kludges until the system is fully understood */
	enum {
		/* Early F3 class games, these are not cartridge games and system features may be different */
		RINGRAGE=0, /* D21 */
		ARABIANM,   /* D29 */
		RIDINGF,    /* D34 */
		GSEEKER,    /* D40 */
		TRSTAR,     /* D53 */
		GUNLOCK,    /* D66 */
		TWINQIX,
		UNDRFIRE,   /* D67 - Heavily modified F3 hardware (different memory map) */
		SCFINALS,
		LIGHTBR,    /* D69 */

		/* D77 - F3 motherboard proms, all following games are 'F3 package system' */
		/* D78 I CUP */
		KAISERKN,   /* D84 */
		DARIUSG,    /* D87 */
		BUBSYMPH,   /* D90 */
		SPCINVDX,   /* D93 */
		HTHERO95,   /* D94 */
		QTHEATER,   /* D95 */
		EACTION2,   /* E02 */
		SPCINV95,   /* E06 */
		QUIZHUHU,   /* E08 */
		PBOBBLE2,   /* E10 */
		GEKIRIDO,   /* E11 */
		KTIGER2,    /* E15 */
		BUBBLEM,    /* E21 */
		CLEOPATR,   /* E28 */
		PBOBBLE3,   /* E29 */
		ARKRETRN,   /* E36 */
		KIRAMEKI,   /* E44 */
		PUCHICAR,   /* E46 */
		PBOBBLE4,   /* E49 */
		POPNPOP,    /* E51 */
		LANDMAKR,   /* E61 */
		RECALH,     /* prototype */
		COMMANDW,   /* prototype */
		TMDRILL
	};

	static const F3config f3_config_table[];

	virtual void machine_start() override;
	virtual void machine_reset() override;
	virtual void video_start() override;
	virtual void device_post_load(void) override;

	TIMER_CALLBACK_MEMBER(trigger_int3);

	required_device<cpu_device> m_maincpu;
	optional_device<watchdog_timer_device> m_watchdog;
	required_device<screen_device> m_screen;
	optional_device<eeprom_serial_base_device> m_eeprom;

	optional_ioport_array<6> m_input;
	optional_ioport_array<2> m_dial;
	optional_ioport m_eepromin;
	optional_ioport m_eepromout;

	emu_timer *m_interrupt3_timer;
	u32 m_coin_word[2];
	
	int m_game = 0;

	const F3config *m_game_config = nullptr;

	void bubsympb_map(address_map &map);
	void f3_map(address_map &map);
	
private:
	required_device<FDP> m_fdp;
	optional_device<taito_en_device> m_taito_en;
	optional_device<okim6295_device> m_oki;
	
	optional_shared_ptr<u32> m_paletteram32;
	optional_memory_bank m_okibank;

	void bubsympb_oki_w(u8 data);
	u32 f3_control_r(offs_t offset);
	void f3_control_w(offs_t offset, u32 data, u32 mem_mask = ~0);
	void f3_timer_control_w(offs_t offset, u16 data);
	void sound_reset_0_w(u32 data);
	void sound_reset_1_w(u32 data);
	void sound_bankswitch_w(offs_t offset, u32 data, u32 mem_mask = ~0);
	void palette_24bit_w(offs_t offset, u32 data, u32 mem_mask = ~0);

	INTERRUPT_GEN_MEMBER(interrupt2);

	void bubsympb_oki_map(address_map &map);

	u32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void screen_vblank(int state);
};

#endif // MAME_TAITO_TAITO_F3_H
