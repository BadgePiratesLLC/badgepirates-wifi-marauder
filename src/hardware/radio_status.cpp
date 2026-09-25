#include "hardware/radio_status.h"
#include <WiFi.h>
#include <NimBLEDevice.h>

bool wifiIsUp() {
  return WiFi.status() == WL_CONNECTED || WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
}

bool bluetoothIsUp() {
  return NimBLEDevice::getInitialized();
}
