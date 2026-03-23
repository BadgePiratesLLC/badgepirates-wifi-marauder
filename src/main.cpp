// BSidesKC Badge - ESP32Marauder Port
// Phase 1.5: Upstream source integration — prove we can compile & link

#include <Arduino.h>
#include "configs.h"  // our shim → marauder_config.h

// Pull in upstream Marauder headers to prove linkage
#include "WiFiScan.h"
#include "CommandLine.h"
#include "Buffer.h"
#include "settings.h"
#include "EvilPortal.h"
#include "LedInterface.h"
#include "lang_var.h"

#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef HAS_SD
  #include "SDInterface.h"
#endif

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif

// ---- Upstream global objects (must match esp32_marauder.ino externs) ----
WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#ifdef HAS_NEOPIXEL_LED
  LedInterface led_obj;
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
#endif

#ifdef HAS_C5_SD
  SPIClass sharedSPI(SPI);
  SDInterface sd_obj = SDInterface(&sharedSPI, SD_CS);
#elif defined(HAS_SD)
  SDInterface sd_obj;
#endif

const String PROGMEM version_number = MARAUDER_VERSION;

// Brightness stubs (upstream .ino defines these, we provide minimal versions)
#ifdef HAS_SCREEN
  #include <Preferences.h>
  #define BL_CHANNEL 0
  #define BL_FREQ 5000
  #define BL_RESOLUTION 8
  const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
  const uint8_t BL_NUM_LEVELS = 10;
  uint8_t bl_level_idx = 9;
  Preferences bl_prefs;

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    #define BL_SETUP()   ledcAttach(TFT_BL, BL_FREQ, BL_RESOLUTION)
    #define BL_SET(duty) ledcWrite(TFT_BL, (duty))
  #else
    #define BL_SETUP()   do { ledcSetup(BL_CHANNEL, BL_FREQ, BL_RESOLUTION); ledcAttachPin(TFT_BL, BL_CHANNEL); } while(0)
    #define BL_SET(duty) ledcWrite(BL_CHANNEL, (duty))
  #endif
#endif

void brightnessInit() {
  #ifdef HAS_SCREEN
    BL_SETUP();
    bl_prefs.begin("backlight", false);
    bl_level_idx = bl_prefs.getUChar("level", 9);
    if (bl_level_idx >= BL_NUM_LEVELS) bl_level_idx = 9;
    BL_SET(BL_LEVELS[bl_level_idx]);
  #endif
}

void brightnessCycle() {
  #ifdef HAS_SCREEN
    bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
    BL_SET(BL_LEVELS[bl_level_idx]);
    bl_prefs.putUChar("level", bl_level_idx);
  #endif
}

uint8_t getBrightnessLevel() {
  #ifdef HAS_SCREEN
    return bl_level_idx;
  #else
    return 0;
  #endif
}

void brightnessSave(uint8_t level) {
  #ifdef HAS_SCREEN
    if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
    bl_level_idx = level;
    BL_SET(BL_LEVELS[bl_level_idx]);
    bl_prefs.putUChar("level", bl_level_idx);
  #endif
}

void backlightOn() {
  #ifdef HAS_SCREEN
    BL_SET(BL_LEVELS[bl_level_idx]);
  #endif
}

void backlightOff() {
  #ifdef HAS_SCREEN
    BL_SET(0);
  #endif
}

uint32_t currentTime = 0;

void setup() {
  randomSeed(esp_random());
  esp_log_level_set("*", ESP_LOG_NONE);

  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("[BSidesKC] Booting ESP32 Marauder..."));
  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

  #ifdef HAS_C5_SD
    sharedSPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    delay(100);
  #endif

  #ifdef HAS_SCREEN
    pinMode(TFT_BL, OUTPUT);
  #endif
  backlightOff();

  #ifdef HAS_SCREEN
    digitalWrite(TFT_CS, HIGH);
  #endif

  #if defined(HAS_SD) && !defined(HAS_C5_SD)
    pinMode(SD_CS, OUTPUT);
    delay(10);
    digitalWrite(SD_CS, HIGH);
    delay(10);
  #endif

  #ifdef HAS_PSRAM
    if (!psramInit())
      Serial.println(F("PSRAM not available"));
  #endif

  // SD init for C5_SD path (before display)
  #ifdef HAS_SIMPLEX_DISPLAY
    #ifdef HAS_SD
      if (!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));
    #endif
  #endif

  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  brightnessInit();
  backlightOff();

  #ifdef HAS_SCREEN
    display_obj.tft.drawCentreString("BSidesKC Badge", TFT_WIDTH / 2, TFT_HEIGHT * 0.25, 1);
    display_obj.tft.drawCentreString("ESP32 Marauder", TFT_WIDTH / 2, TFT_HEIGHT * 0.40, 1);
    display_obj.tft.drawCentreString(display_obj.version_number, TFT_WIDTH / 2, TFT_HEIGHT * 0.55, 1);
  #endif

  backlightOn();

  settings_obj.begin();
  buffer_obj = Buffer();

  #ifndef HAS_SIMPLEX_DISPLAY
    #ifdef HAS_SD
      if (!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));
    #endif
  #endif

  wifi_scan_obj.RunSetup();

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Initializing...", TFT_WIDTH / 2, TFT_HEIGHT * 0.70, 1);
  #endif

  evil_portal_obj.setup();

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif

  #ifdef HAS_NEOPIXEL_LED
    led_obj.RunSetup();
  #endif

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    menu_function_obj.RunSetup();
  #endif

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  cli_obj.RunSetup();

  Serial.println(F("[BSidesKC] Marauder ready."));
}

void loop() {
  currentTime = millis();

  cli_obj.main(currentTime);
  wifi_scan_obj.main(currentTime);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif

  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    menu_function_obj.main(currentTime);
  #endif

  #ifdef HAS_NEOPIXEL_LED
    led_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
}
