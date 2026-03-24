#pragma once
#include <esp_wifi.h>

// Patch WiFi config for ESP32-S3 without PSRAM
inline void patch_wifi_config_no_psram(wifi_init_config_t* cfg) {
    cfg->cache_tx_buf_num = 0;  // Disable PSRAM TX cache buffers
}
