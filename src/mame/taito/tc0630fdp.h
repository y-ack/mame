#ifndef MAME_TAITO_TC0630FDP_H
#define MAME_TAITO_TC0630FDP_H

#pragma once

#include "tilemap.h"
#include "emupal.h"
#include <bitset>

#define FDP tc0630fdp_device

class FDP : public device_t, public device_gfx_interface
{
public:
	static inline constexpr int H_TOTAL = 432;
	static inline constexpr int H_START = 46;
	static inline constexpr int H_VIS   = 320;
	static inline constexpr int H_END   = H_START+H_VIS;
	
	static inline constexpr int V_TOTAL = 262;
	static inline constexpr int V_START = 24;
	static inline constexpr int V_VIS   = 232;
	static inline constexpr int V_END   = V_START+V_VIS;
	
	static inline constexpr int NUM_PLAYFIELDS = 4;
	static inline constexpr int NUM_TILEMAPS = 5;
	static inline constexpr int NUM_SPRITEGROUPS = 4; // high 2 bits of color
	static inline constexpr int NUM_CLIPPLANES = 4;
	
	FDP(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	
	void device_add_mconfig(machine_config &config) override;
	
	virtual void device_start() override;
	
	DECLARE_GFXDECODE_MEMBER(gfxinfo);
	DECLARE_GFXDECODE_MEMBER(gfx_bubsympb);
	
	void map_ram(address_map &map);
	void map_control(address_map &map);
	
	void tile_decode();
	
	void create_tilemaps(bool extend);
	
	int m_sprite_lag = 0;
	bitmap_ind8 m_pri_alp_bitmap;
	bitmap_ind16 m_sprite_framebuffers[NUM_SPRITEGROUPS]{};
	bool m_flipscreen = false;
	bool m_extend = false;
	
	void read_sprite_info();
	void draw_sprites();
	void scanline_draw(bitmap_rgb32 &bitmap, const rectangle &cliprect);
	
	required_device<palette_device> m_palette;
	required_device<palette_device> m_palette_12bit;
	
protected:
	
	using fixed8 = s32;
	
	struct tempsprite {
		int code; // 17 bits
		u8 color;
		bool flip_x, flip_y;
		fixed8 x, y;
		fixed8 scale_x, scale_y;
		u8 pri;
	};
	
	struct clip_plane_inf {
		s16 l;
		s16 r;

		clip_plane_inf() { l = 0; r = 0; }
		clip_plane_inf(s16 left, s16 right)
		{
			l = left;
			r = right;
		}
		clip_plane_inf& set_upper(s8 left, s8 right)
		{
			l = (l & 0xff) | left<<8;
			r = (r & 0xff) | right<<8;
			return *this;
		}
		clip_plane_inf& set_lower(u8 left, u8 right)
		{
			l = (l & 0x100) | left;
			r = (r & 0x100) | right;
			return *this;
		}
	};

	struct draw_source {
		draw_source() { };
		draw_source(bitmap_ind16 *bitmap)
		{
			src = bitmap;
			flags = nullptr;
		}
		draw_source(tilemap_t *tilemap)
		{
			if (!tilemap)
				return;
			src = &tilemap->pixmap();
			flags = &tilemap->flagsmap();
		};
		bitmap_ind16 *src{nullptr};
		bitmap_ind8  *flags{nullptr};
	};

	struct mix_pix { // per-pixel information for the blending circuit
		u16 src_pal{0};
		u16 dst_pal{0};
		u8  src_blend{0x00};
		u8  dst_blend{0xff};
	};

	struct f3_line_inf;

	struct mixable {// layer compositing information
		draw_source bitmap;
		bool x_sample_enable{false};
		u16 mix_value{0};
		u8 prio{0};
		void set_mix(u16 v) { mix_value = v; prio = v & 0xf; };
		void set_prio(u8 p) { mix_value = (mix_value & 0xfff0) | p; prio = p; };
		auto clip_inv() const { return std::bitset<4>(mix_value >> 4); };
		auto clip_enable() const { return std::bitset<4>(mix_value >> 8); };
		bool clip_inv_mode() const { return mix_value & 0x1000; };
		inline bool layer_enable() const;
		u8 blend_mask() const { return BIT(mix_value, 14, 2); };
		bool blend_a() const { return mix_value & 0x4000; };
		bool blend_b() const { return mix_value & 0x8000; };

		inline bool operator<(const mixable &rhs) const noexcept { return this->prio < rhs.prio; };
		inline bool operator>(const mixable &rhs) const noexcept { return this->prio > rhs.prio; };

		u16 palette_adjust(u16 pal) const { return pal; };
		inline int y_index(int y) const;
		inline int x_index(int x) const;
		bool blend_select(const u8 *line_flags, int x) const { return false; };

		bool used(int y) const { return true; };
		u8 debug_index{0};
		const char *debug_name() { return "MX"; };
	};

	struct sprite_inf : mixable {
		// alpha mode in 6000
		// mosaic enable in 6400
		// line enable, clip settings in 7400
		// priority in 7600
		bool blend_select_v{false}; // 7400 0xf000
		bool blend_select(const u8 *line_flags, int x) const { return blend_select_v; };
		inline bool layer_enable() const;

		u8 (*sprite_pri_usage)[256]{nullptr};
		bool used(int y) const { return (*sprite_pri_usage)[y] & (1<<debug_index); }
		const char *debug_name() { return "SP"; };
	};

	struct pivot_inf : mixable {
		u8 pivot_control{0};     // 6000
		bool blend_select_v{false};
		bool blend_select(const u8 *line_flags, int x) const { return blend_select_v; };
		// mosaic enable in 6400
		u16 pivot_enable{0}; // 7000 - what is in this word ?
		// mix info from 7200
		bool use_pix() const { return pivot_control & 0xa0; };

		u16 reg_sx{0};
		u16 reg_sy{0};
		inline int y_index(int y) const;
		inline int x_index(int x) const;
		const char *debug_name() { return "PV"; };
	};

	struct playfield_inf : mixable {
		u16 colscroll{0};            // 4000
		bool alt_tilemap{false};     // 4000
		// mosaic enable in 6400
		fixed8 x_scale{0x80};        // 8000
		fixed8 y_scale{0};           // 8000
		u16 pal_add{0};              // 9000
		fixed8 rowscroll{0};         // a000

		fixed8 reg_sx{0};
		fixed8 reg_sy{0};
		fixed8 reg_fx_y{0};
		fixed8 reg_fx_x{0};

		u16 width_mask{0};

		inline u16 palette_adjust(u16 pal) const;
		inline int y_index(int y) const;
		inline int x_index(int x) const;
		bool blend_select(const u8 *line_flags, int x) const { return BIT(line_flags[x], 0); };
		const char *debug_name() { return "PF"; };
	};

	struct pri_mode {
		u8 src_prio{0};
		u8 dst_prio{0};
		u8 src_blendmode{0xff};
		u8 dst_blendmode{0xff};
	};

	struct f3_line_inf {
		int y{0};
		int screen_y{0};
		pri_mode pri_alp[432]{};
		// 5000/4000
		clip_plane_inf clip[NUM_CLIPPLANES];
		// 6000 - pivot_control, sprite alpha
		u16 maybe_sync_reg{0};
		bool no_opaque_dest{false};
		// 6200
		u8 blend[4]{}; // less 0 - 8 more
		// 6400
		u8 x_sample{16 - 0}; // mosaic effect
		u8 fx_6400{0}; // unemulated other effects (palette interpretation + unused bits)
		bool pf4_shadow{0}; // UNIMPLEMENTED
		bool blur{0}; // UNIMPLEMENTED
		bool palette_12bit{0};
		// 6600
		u16 bg_palette{0}; // always palette 0 in existing games
		// 7200
		pivot_inf pivot;
		sprite_inf sp[NUM_SPRITEGROUPS];
		playfield_inf pf[NUM_PLAYFIELDS];
	};
	
	//f3_line_inf m_line_inf;

	std::unique_ptr<u8[]> m_decoded_gfx4;
	std::unique_ptr<u8[]> m_decoded_gfx5;
	
	u16 *m_pf_data[8]{};
	tilemap_t *m_tilemap[8] = {nullptr};
	tilemap_t *m_pixel_layer = nullptr;
	tilemap_t *m_vram_layer = nullptr;
	
	template<unsigned Layer>
	TILE_GET_INFO_MEMBER(get_tile_info);
	TILE_GET_INFO_MEMBER(get_tile_info_text);
	TILE_GET_INFO_MEMBER(get_tile_info_pixel);
	
//protected:
	
	// sprites /////////////////////////////////////////////////////
	
	std::unique_ptr<tempsprite[]> m_spritelist;
	const tempsprite *m_sprite_end = nullptr;
	bool m_sprite_bank = 0;
	u8 m_sprite_extra_planes = 0;
	u8 m_sprite_pen_mask = 0;
	bool m_sprite_trails = false;
	u8 m_sprite_pri_row_usage[256]{};
	
	inline void f3_drawgfx(const tempsprite &sprite);
	
	// rendering //////////////////////////////////////////////////////
	
	u16 m_width_mask = 0;
	
   void get_pf_scroll(int pf_num, fixed8 &reg_sx, fixed8 &reg_sy);
	void read_line_ram(f3_line_inf &line, int y);
	template<typename Mix>
	std::vector<clip_plane_inf> calc_clip(const clip_plane_inf (&clip)[NUM_CLIPPLANES], const Mix line);
	
	template<typename Mix>
	bool mix_line(Mix *gfx, mix_pix *z, pri_mode *pri, const f3_line_inf &line, const clip_plane_inf &range);
	void render_line(pen_t *dst, const mix_pix (&z)[432], const f3_line_inf &line);
	
	// memory //////////////////////////////////////////////////////
	
	memory_share_creator<u16> m_spriteram;
	memory_share_creator<u16> m_pfram;
	memory_share_creator<u16> m_textram;
	memory_share_creator<u16> m_charram;
	memory_share_creator<u16> m_lineram;
	memory_share_creator<u16> m_pivotram;
	u16 m_control_0[8]{};
	u16 m_control_1[8]{};
	
	u16 spriteram_r(offs_t offset);
	void spriteram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u16 pfram_r(offs_t offset);
	void pfram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u16 textram_r(offs_t offset);
	void textram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u16 charram_r(offs_t offset);
	void charram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u16 lineram_r(offs_t offset);
	void lineram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u16 pivotram_r(offs_t offset);
	void pivotram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	void control_0_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	void control_1_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	void paletteram_w(offs_t offset, u32 data, u32 mem_mask = ~0);
	
};

DECLARE_DEVICE_TYPE(TC0630FDP, FDP)

#endif // MAME_TAITO_TC0630FDP_H
