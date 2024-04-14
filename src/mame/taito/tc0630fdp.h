#ifndef MAME_TAITO_TC0630FDP_H
#define MAME_TAITO_TC0630FDP_H

#pragma once

#include "tilemap.h"
#define FDP tc0630fdp_device

class FDP : public device_t, public device_gfx_interface
{
public:
	FDP(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	
	virtual void device_start() override;
	
	void tile_decode();
	
	DECLARE_GFXDECODE_MEMBER(gfxinfo);
	DECLARE_GFXDECODE_MEMBER(gfx_bubsympb);
	
	std::unique_ptr<u8[]> m_decoded_gfx4;
	std::unique_ptr<u8[]> m_decoded_gfx5;
	
	void map_ram(address_map &map);
	
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

	memory_share_creator<u16> m_spriteram;
	memory_share_creator<u16> m_pfram;
	memory_share_creator<u16> m_textram;
	memory_share_creator<u16> m_charram;
	memory_share_creator<u16> m_lineram;
	memory_share_creator<u16> m_pivotram;

	void create_tilemaps(bool extend);
	tilemap_t *m_tilemap[8] = {nullptr};
	tilemap_t *m_pixel_layer = nullptr;
	tilemap_t *m_vram_layer = nullptr;
	template<unsigned Layer> TILE_GET_INFO_MEMBER(get_tile_info);
	TILE_GET_INFO_MEMBER(get_tile_info_text);
	TILE_GET_INFO_MEMBER(get_tile_info_pixel);
	
	bool m_flipscreen = false;
	bool m_extend = false;
};

DECLARE_DEVICE_TYPE(TC0630FDP, FDP)

#endif // MAME_TAITO_TC0630FDP_H
