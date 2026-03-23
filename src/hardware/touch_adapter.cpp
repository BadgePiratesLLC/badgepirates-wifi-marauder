// BSidesKC Badge - FT6336U Touch Adapter
#include "touch_adapter.h"
#include <Wire.h>

static FT6336U ft6336u(TOUCH_SDA_PIN, TOUCH_SCL_PIN, TOUCH_RST_PIN, TOUCH_INT_PIN);

void touchAdapterInit() {
    ft6336u.begin();
    Serial.println(F("[Touch] FT6336U initialized (I2C)"));
    Serial.printf("[Touch] Chip ID: 0x%02X  Firmware: 0x%02X\n",
                  ft6336u.read_chip_id(), ft6336u.read_firmware_id());
}

FT6336U& getTouchController() {
    return ft6336u;
}
