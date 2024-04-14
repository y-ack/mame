#include "emu.h"
#include "tc0630fdp.h"

DEFINE_DEVICE_TYPE(TC0630FDP, FDP, "tc0630fdp", "Taito TC0630FDP")

FDP::FDP(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, TC0630FDP, tag, owner, clock)
	, device_gfx_interface(mconfig, *this)
{
}

void FDP::device_start() {
	
}
