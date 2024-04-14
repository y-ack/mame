#include "emu.h"
#include "tc0630fdp.h"

DEFINE_DEVICE_TYPE(TC0630FDP, FDP, "tc0630fdp", "Taito TC0630FDP")

FDP::FDP(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, TC0630FDP, tag, owner, clock)
	, device_gfx_interface(mconfig, *this, gfxinfo, "palette")
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
