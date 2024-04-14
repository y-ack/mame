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
	
	DECLARE_GFXDECODE_MEMBER(gfx_taito_f3);
	DECLARE_GFXDECODE_MEMBER(gfx_bubsympb);
};

DECLARE_DEVICE_TYPE(TC0630FDP, FDP)

#endif // MAME_TAITO_TC0630FDP_H
