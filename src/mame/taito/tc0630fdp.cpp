#include "emu.h"
#include "tc0630fdp.h"

DEFINE_DEVICE_TYPE(TC0630FDP, FDP, "tc0630fdp", "Taito TC0630FDP")

FDP::FDP(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, TC0630FDP, tag, owner, clock)
	, device_gfx_interface(mconfig, *this, gfxinfo, "palette")
	, m_spriteram(*this, "spriteram", 0x10000, ENDIANNESS_BIG)
	, m_pfram(*this, "pfram", 0xc000, ENDIANNESS_BIG)
	, m_textram(*this, "textram", 0x2000, ENDIANNESS_BIG)
	, m_charram(*this, "charram", 0x2000, ENDIANNESS_BIG)
	, m_lineram(*this, "lineram", 0x10000, ENDIANNESS_BIG)
	, m_pivotram(*this, "pivotram", 0x10000, ENDIANNESS_BIG)
{
}

void FDP::device_start() {
	//decode_gfx(gfxinfo);
}

/******************************************************************************/

static const gfx_layout charlayout = {
	8,8,
	256,
	4,
	{ 0,1,2,3 },
	{ 20, 16, 28, 24, 4, 0, 12, 8 },
	{ STEP8(0,4*8) },
	32*8
};

static const gfx_layout pivotlayout = {
	8,8,
	2048,
	4,
	{ 0,1,2,3 },
	{ 20, 16, 28, 24, 4, 0, 12, 8 },
	{ STEP8(0,4*8) },
	32*8
};

static const gfx_layout layout_6bpp_sprite_hi = {
	16,16,
	RGN_FRAC(1,1),
	6,
	{ STEP2(0,1)/**/,0,0,0,0/**/ },
	{ STEP4(3*2,-2), STEP4(7*2,-2), STEP4(11*2,-2), STEP4(15*2,-2) },
	{ STEP16(0,16*2) },
	16*16*2
};

static const gfx_layout layout_6bpp_tile_hi = {
	16,16,
	RGN_FRAC(1,1),
	6,
	{ 8,0/**/,0,0,0,0/**/ },
	{ STEP8(7,-1), STEP8(8*2+7,-1) },
	{ STEP16(0,8*2*2) },
	16*16*2
};

GFXDECODE_MEMBER( FDP::gfxinfo )
	GFXDECODE_DEVICE( nullptr,      0, charlayout,             0x0000, 0x0400>>4 ) /* Dynamically modified */
	GFXDECODE_DEVICE( nullptr,      0, pivotlayout,            0x0000,  0x400>>4 ) /* Dynamically modified */
	GFXDECODE_DEVICE( "sprites",    0, gfx_16x16x4_packed_lsb, 0x1000, 0x1000>>4 ) // low 4bpp of 6bpp sprite data
	GFXDECODE_DEVICE( "tilemap",    0, gfx_16x16x4_packed_lsb, 0x0000, 0x2000>>4 ) // low 4bpp of 6bpp tilemap data
	GFXDECODE_DEVICE( "tilemap_hi", 0, layout_6bpp_tile_hi,    0x0000, 0x2000>>4 ) // hi 2bpp of 6bpp tilemap data
	GFXDECODE_DEVICE( "sprites_hi", 0, layout_6bpp_sprite_hi,  0x1000, 0x1000>>4 ) // hi 2bpp of 6bpp sprite data
GFXDECODE_END

static const gfx_layout bubsympb_sprite_layout = {
	16,16,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(0,6), RGN_FRAC(1,6), RGN_FRAC(2,6), RGN_FRAC(3,6), RGN_FRAC(4,6), RGN_FRAC(5,6) },
	{ STEP16(15,-1) },
	{ STEP16(0,16) },
	16*16
};

static const gfx_layout bubsympb_layout_5bpp_tile_hi = {
	16,16,
	RGN_FRAC(1,1),
	5,
	{ 0/**/,0,0,0,0/**/ },
	{ STEP8(7,-1), STEP8(15,-1) },
	{ STEP16(0,16) },
	16*16
};


GFXDECODE_MEMBER( FDP::gfx_bubsympb )
	GFXDECODE_DEVICE( nullptr,      0, charlayout,                   0,  64 ) /* Dynamically modified */
	GFXDECODE_DEVICE( nullptr,      0, pivotlayout,                  0,  64 ) /* Dynamically modified */
	GFXDECODE_DEVICE( "sprites",    0, bubsympb_sprite_layout,    4096, 256 ) /* Sprites area (6bpp planar) */
	GFXDECODE_DEVICE( "tilemap",    0, gfx_16x16x4_packed_lsb,       0, 512 ) // low 4bpp of 5bpp tilemap data
	GFXDECODE_DEVICE( "tilemap_hi", 0, bubsympb_layout_5bpp_tile_hi, 0, 512 ) // hi 1bpp of 5bpp tilemap data
	GFXDECODE_DEVICE( "sprites",    0, bubsympb_sprite_layout,    4096, 256 ) // dummy gfx duplicate for avoid crash
GFXDECODE_END

void FDP::tile_decode()
{
	/* Setup ROM formats:

	    Some games will only use 4 or 5 bpp sprites, and some only use 4 bpp tiles,
	    I don't believe this is software or prom controlled but simply the unused data lines
	    are tied low on the game board if unused.  This is backed up by the fact the palette
	    indices are always related to 4 bpp data, even in 6 bpp games.

	    Most (all?) games with 5bpp tiles have the sixth bit set. Also, in Arabian Magic
	    sprites 1200-120f contain 6bpp data which is probably bogus.
	    video_start clears the fifth and sixth bit of the decoded graphics according
	    to the bit depth specified in f3_config_table.

	*/

	u8 *dest;
	// all but bubsymphb (bootleg board with different sprite gfx layout), 2mindril (no sprite gfx roms)
	if (gfx(5) != nullptr) {
		gfx_element *spr_gfx = gfx(2);
		gfx_element *spr_gfx_hi = gfx(5);

		// allocate memory for the assembled data
		m_decoded_gfx5 = std::make_unique<u8[]>(spr_gfx->elements() * spr_gfx->width() * spr_gfx->height());

		// loop over elements
		dest = m_decoded_gfx5.get();
		for (int c = 0; c < spr_gfx->elements(); c++) {
			const u8 *c1base = spr_gfx->get_data(c);
			const u8 *c3base = spr_gfx_hi->get_data(c);

			// loop over height
			for (int y = 0; y < spr_gfx->height(); y++) {
				const u8 *c1 = c1base;
				const u8 *c3 = c3base;

				/* Expand 2bits into 4bits format */
				for (int x = 0; x < spr_gfx->width(); x++)
					*dest++ = (*c1++ & 0xf) | (*c3++ & 0x30);

				c1base += spr_gfx->rowbytes();
				c3base += spr_gfx_hi->rowbytes();
			}
		}

		spr_gfx->set_raw_layout(m_decoded_gfx5.get(), spr_gfx->width(), spr_gfx->height(), spr_gfx->elements(), 8 * spr_gfx->width(), 8 * spr_gfx->width() * spr_gfx->height());
		set_gfx(5, nullptr);
	}

	if (gfx(4) != nullptr) {
		gfx_element *pf_gfx = gfx(3);
		gfx_element *pf_gfx_hi = gfx(4);

		// allocate memory for the assembled data
		m_decoded_gfx4 = std::make_unique<u8[]>(pf_gfx->elements() * pf_gfx->width() * pf_gfx->height());

		// loop over elements
		dest = m_decoded_gfx4.get();
		for (int c = 0; c < pf_gfx->elements(); c++) {
			const u8 *c0base = pf_gfx->get_data(c);
			const u8 *c2base = pf_gfx_hi->get_data(c);

			// loop over height
			for (int y = 0; y < pf_gfx->height(); y++) {
				const u8 *c0 = c0base;
				const u8 *c2 = c2base;

				for (int x = 0; x < pf_gfx->width(); x++)
					*dest++ = (*c0++ & 0xf) | (*c2++ & 0x30);

				c0base += pf_gfx->rowbytes();
				c2base += pf_gfx_hi->rowbytes();
			}
		}

		pf_gfx->set_raw_layout(m_decoded_gfx4.get(), pf_gfx->width(), pf_gfx->height(), pf_gfx->elements(), 8 * pf_gfx->width(), 8 * pf_gfx->width() * pf_gfx->height());
		set_gfx(4, nullptr);
	}
}

void FDP::map_ram(address_map &map) {
	map(0x00000, 0x0ffff).rw(FUNC(FDP::spriteram_r), FUNC(FDP::spriteram_w));
	map(0x10000, 0x1bfff).rw(FUNC(FDP::pfram_r), FUNC(FDP::pfram_w));
	map(0x1c000, 0x1dfff).rw(FUNC(FDP::textram_r), FUNC(FDP::textram_w));
	map(0x1e000, 0x1ffff).rw(FUNC(FDP::charram_r), FUNC(FDP::charram_w));
	map(0x20000, 0x2ffff).rw(FUNC(FDP::lineram_r), FUNC(FDP::lineram_w));
	map(0x30000, 0x3ffff).rw(FUNC(FDP::pivotram_r), FUNC(FDP::pivotram_w));
}

u16 FDP::spriteram_r(offs_t offset)
{
	return m_spriteram[offset];
}

void FDP::spriteram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_spriteram[offset]);
}

u16 FDP::pfram_r(offs_t offset)
{
	return m_pfram[offset];
}

void FDP::pfram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_pfram[offset]);

	if (m_game_config->extend) {
		if (offset < 0x4000) {
			m_tilemap[offset >> 12]->mark_tile_dirty((offset & 0xfff) >> 1);
		}
	} else {
		if (offset < 0x4000)
			m_tilemap[offset >> 11]->mark_tile_dirty((offset & 0x7ff) >> 1);
	}
}

u16 FDP::textram_r(offs_t offset)
{
	return m_textram[offset];
}

void FDP::textram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_textram[offset]);

	m_vram_layer->mark_tile_dirty(offset);

	// dirty the pixel layer too, since it uses palette etc. from text layer
	// convert the position (x and y are swapped, and the upper bit of y is ignored)
	//  text: [Yyyyyyxxxxxx]
	// pixel: [0xxxxxxyyyyy]
	const int y = BIT(offset, 6, 5);
	const int x = BIT(offset, 0, 6);
	const int col_off = x << 5 | y;

	m_pixel_layer->mark_tile_dirty(col_off);
}

u16 FDP::charram_r(offs_t offset)
{
	return m_charram[offset];
}

void FDP::charram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_charram[offset]);
	gfx(0)->mark_dirty(offset >> 4);
}

u16 FDP::lineram_r(offs_t offset)
{
	return m_lineram[offset];
}

void FDP::lineram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_lineram[offset]);
}

u16 FDP::pivotram_r(offs_t offset)
{
	return m_pivotram[offset];
}

void FDP::pivotram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_pivotram[offset]);
	gfx(1)->mark_dirty(offset >> 4);
}

void FDP::create_tilemaps(bool extend)
{
	m_extend = extend;
	// TODO: we need to free these if this is called multiple times
	if (m_extend) {
		m_tilemap[0] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<0>)), TILEMAP_SCAN_ROWS, 16, 16, 64, 32);
		m_tilemap[1] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<1>)), TILEMAP_SCAN_ROWS, 16, 16, 64, 32);
		m_tilemap[2] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<2>)), TILEMAP_SCAN_ROWS, 16, 16, 64, 32);
		m_tilemap[3] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<3>)), TILEMAP_SCAN_ROWS, 16, 16, 64, 32);
		m_tilemap[4] = m_tilemap[5] = m_tilemap[6] = m_tilemap[7] = nullptr;
	} else {
		m_tilemap[0] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<0>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[1] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<1>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[2] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<2>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[3] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<3>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[4] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<4>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[5] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<5>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[6] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<6>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
		m_tilemap[7] = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info<7>)), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
	}
	for (int i = 0; i < 8; i++) {
		if (m_tilemap[i])
			m_tilemap[i]->set_transparent_pen(0);
	}

	if (m_extend) {
		m_width_mask = 0x3ff; // 10 bits
		for (int i = 0; i < 4; i++)
			m_pf_data[i] = &m_pf_ram[(0x2000 * i) / 2];
	} else {
		m_width_mask = 0x1ff; // 9 bits
		for (int i = 0; i < 8; i++)
			m_pf_data[i] = &m_pf_ram[(0x1000 * i) / 2];
	}
	
	m_vram_layer = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info_text)), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);
	m_pixel_layer = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(FDP::get_tile_info_pixel)), TILEMAP_SCAN_COLS, 8, 8, 64, 32);
	m_vram_layer->set_transparent_pen(0);
	m_pixel_layer->set_transparent_pen(0);
	
	gfx(0)->set_source(reinterpret_cast<u8 *>(m_charram.target()));
	gfx(1)->set_source(reinterpret_cast<u8 *>(m_pivotram.target()));
}

template<unsigned Layer>
TILE_GET_INFO_MEMBER(FDP::get_tile_info)
{
	u16 *tilep = &m_pf_data[Layer][tile_index * 2];
	// tile info:
	// [yx?? ddac cccc cccc]
	// yx: x/y flip
	// ?: upper bits of tile number?
	// d: bpp
	// a: blend select
	// c: color

	const u16 palette_code = BIT(tilep[0],  0, 9);
	const u8 blend_sel     = BIT(tilep[0],  9, 1);
	const u8 extra_planes  = BIT(tilep[0], 10, 2); // 0 = 4bpp, 1 = 5bpp, 2 = unused?, 3 = 6bpp

	tileinfo.set(3,
			tilep[1],
			palette_code,
			TILE_FLIPYX(BIT(tilep[0], 14, 2)));

	tileinfo.category = blend_sel; // blend value select
	// gfx extra planes and palette code set the same bits of color address
	// we need to account for tilemap.h combining using "+" instead of "|"
	tileinfo.pen_mask = ((extra_planes & ~palette_code) << 4) | 0x0f;
}


TILE_GET_INFO_MEMBER(FDP::get_tile_info_text)
{
	const u16 vram_tile = m_textram[tile_index];
	// text tile info:
	// [yccc cccx tttt tttt]
	// y: y flip
	// c: palette
	// x: x flip
	// t: tile number

	u8 flags = 0;
	if (BIT(vram_tile,  8)) flags |= TILE_FLIPX;
	if (BIT(vram_tile, 15)) flags |= TILE_FLIPY;

	tileinfo.set(0,
			vram_tile & 0xff,
			BIT(vram_tile, 9, 6),
			flags);
}

TILE_GET_INFO_MEMBER(FDP::get_tile_info_pixel)
{
	/* attributes are shared with VRAM layer */
	// convert the index:
	// pixel: [0xxxxxxyyyyy]
	//  text: [?yyyyyxxxxxx]
	const int x = BIT(tile_index, 5, 6);
	int y = BIT(tile_index, 0, 5);
	// HACK: [legacy implementation of scroll offset check for pixel palette mirroring]
	// the pixel layer is 256px high, but uses the palette from the text layer which is twice as long
	// so normally it only uses the first half of textram, BUT if you scroll down, you get
	//   an alternate version of the pixel layer which gets its palette data from the second half of textram.
	// we simulate this using a hack, checking scroll offset to determine which version of the pixel layer is visible.
	// this means we SHOULD dirty parts of the pixel layer, if the scroll or flipscreen changes.. but we don't.
	// (really we should just apply the palette during rendering instead of this ?)
	int y_offs = y * 8 + m_control_1[5];
	if (m_flipscreen)
		y_offs += 0x100; // this could just as easily be ^= 0x100 or -= 0x100
	if ((y_offs & 0x1ff) >= 256)
		y += 32;

	const u16 vram_tile = m_textram[y << 6 | x];

	const int tile = tile_index;
	const u8 palette = BIT(vram_tile, 9, 6);
	u8 flags = 0;
	if (BIT(vram_tile, 8))  flags |= TILE_FLIPX;
	if (BIT(vram_tile, 15)) flags |= TILE_FLIPY;

	tileinfo.set(1, tile, palette, flags);
}
