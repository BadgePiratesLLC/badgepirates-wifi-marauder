// Phase 3: WiFi scan test mode
// Hold ENTER + BACK during boot to enter WiFi scan test
// Uses WiFi.scanNetworks() for a quick standalone scan, then displays results.

#include <Arduino.h>
#include "configs.h"

#ifdef HAS_SCREEN

#include <WiFi.h>
#include "esp_wifi.h"
#include "Display.h"
#include "wifi_scan_test.h"

extern Display display_obj;

static const char* encTypeStr(wifi_auth_mode_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2E";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    default:                        return "?";
  }
}

void runWifiScanTest() {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.drawCentreString("WiFi Scan Test", TFT_WIDTH / 2, 4, 2);
  display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  display_obj.tft.drawCentreString("Scanning...", TFT_WIDTH / 2, 30, 1);
  Serial.println(F("[WiFiTest] Starting scan..."));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  uint32_t t0 = millis();
  int n = WiFi.scanNetworks(false, true); // sync, show hidden
  uint32_t elapsed = millis() - t0;

  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.drawCentreString("WiFi Scan Test", TFT_WIDTH / 2, 4, 2);

  char buf[64];
  snprintf(buf, sizeof(buf), "Found %d APs in %lums", n, elapsed);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawCentreString(buf, TFT_WIDTH / 2, 26, 1);
  Serial.printf("[WiFiTest] %s\n", buf);

  // Channel hop test: scan channels 1, 6, 11
  uint8_t testChannels[] = {1, 6, 11};
  bool hopOk = true;
  for (int i = 0; i < 3; i++) {
    esp_err_t err = esp_wifi_set_channel(testChannels[i], WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) { hopOk = false; break; }
    delay(10);
  }
  snprintf(buf, sizeof(buf), "Chan hop: %s", hopOk ? "OK" : "FAIL");
  display_obj.tft.setTextColor(hopOk ? TFT_GREEN : TFT_RED, TFT_BLACK);
  display_obj.tft.drawCentreString(buf, TFT_WIDTH / 2, 42, 1);
  Serial.printf("[WiFiTest] %s\n", buf);

  // Display results — fit as many as screen allows
  int y = 60;
  int maxRows = (TFT_HEIGHT - y - 20) / 12;
  int show = min(n, maxRows);

  display_obj.tft.setTextSize(1);
  for (int i = 0; i < show; i++) {
    int rssi = WiFi.RSSI(i);
    int ch   = WiFi.channel(i);
    const char* enc = encTypeStr(WiFi.encryptionType(i));
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) ssid = "(hidden)";
    if (ssid.length() > 16) ssid = ssid.substring(0, 16) + "~";

    snprintf(buf, sizeof(buf), "%ddBm ch%02d %-5s %s", rssi, ch, enc, ssid.c_str());

    // Color by signal strength
    uint16_t color = TFT_RED;
    if (rssi > -50) color = TFT_GREEN;
    else if (rssi > -70) color = TFT_YELLOW;
    else if (rssi > -85) color = TFT_ORANGE;

    display_obj.tft.setTextColor(color, TFT_BLACK);
    display_obj.tft.setCursor(2, y);
    display_obj.tft.print(buf);
    y += 12;

    Serial.printf("[WiFiTest] %s\n", buf);
  }
  if (n > show) {
    display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    display_obj.tft.setCursor(2, y);
    snprintf(buf, sizeof(buf), "... +%d more", n - show);
    display_obj.tft.print(buf);
  }

  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.drawCentreString("Hold BACK to exit", TFT_WIDTH / 2, TFT_HEIGHT - 14, 1);

  Serial.printf("[WiFiTest] Done. %d APs, %lums, hop=%s\n", n, elapsed, hopOk ? "OK" : "FAIL");

  // Wait for BACK button to exit
  while (digitalRead(D_BTN) != LOW) delay(50);
  delay(200); // debounce

  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
}

#endif // HAS_SCREEN
