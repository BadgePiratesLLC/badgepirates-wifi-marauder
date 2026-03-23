#pragma once
// BSidesKC Badge - FT6336U Touch Adapter
// Bridges FT6336U I2C capacitive touch to Marauder's XPT2046 SPI interface
// by providing a drop-in XPT2046_Touchscreen-compatible class.

#include <FT6336U.h>
#include "bsideskc_pins.h"

// Initialize the FT6336U hardware. Call once in setup().
void touchAdapterInit();

// Get the global FT6336U instance (used by the XPT2046 shim)
FT6336U& getTouchController();
