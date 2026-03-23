// BSidesKC Badge - ESP32Marauder Port
// Phase 2: Display & Input — uses DisplayAdapter for badge-specific backlight

#include <Arduino.h>
#include "configs.h"  // our shim → marauder_config.h

// Pull in upstream Marauder headers
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
  #include "hardware/display_adapter.h"
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

#ifdef HAS_BUTTONS
  #include "Switches.h"
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

#ifdef HAS_BUTTONS
  #if (C_BTN >= 0)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
  #endif
  #if (D_BTN >= 0)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
  #endif
#endif

#ifdef HAS_NEOPIXEL_LED
  LedInterface led_obj;
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
#endif

#ifdef HAS_SEPARATE_SD
  SPIClass sdSPI(SPI);
#endif

#ifdef HAS_SD
  SDInterface sd_obj;
#endif

const String PROGMEM version_number = MARAUDER_VERSION;

// ---- Brightness functions (delegate to DisplayAdapter) ----
void brightnessInit() {
  #ifdef HAS_SCREEN
    badge_display.begin();
  #endif
}

void brightnessCycle() {
  #ifdef HAS_SCREEN
    badge_display.backlightCycle();
  #endif
}

uint8_t getBrightnessLevel() {
  #ifdef HAS_SCREEN
    return badge_display.backlightLevel();
  #else
    return 0;
  #endif
}

void brightnessSave(uint8_t level) {
  #ifdef HAS_SCREEN
    badge_display.backlightSet(level);
    badge_display.backlightSave();
  #endif
}

void backlightOn() {
  #ifdef HAS_SCREEN
    badge_display.backlightOn();
  #endif
}

void backlightOff() {
  #ifdef HAS_SCREEN
    badge_display.backlightOff();
  #endif
}

// ---- Touch Test Mode ----
// Hold BTN_ENTER during boot to enter touch test (draws dots + coords)
#ifdef HAS_CYD_TOUCH
void runTouchTest() {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.drawCentreString("Touch Test - tap screen", TFT_WIDTH / 2, 4, 2);
  display_obj.tft.drawCentreString("Hold BACK to exit", TFT_WIDTH / 2, TFT_HEIGHT - 20, 1);

  while (true) {
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty)) {
      display_obj.tft.fillCircle(tx, ty, 3, TFT_CYAN);
      // Show coords in top-right
      display_obj.tft.fillRect(TFT_WIDTH - 90, 0, 90, 16, TFT_BLACK);
      display_obj.tft.setCursor(TFT_WIDTH - 88, 4);
      display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      display_obj.tft.printf("%d,%d", tx, ty);
      Serial.printf("[Touch] x=%d y=%d\n", tx, ty);
    }
    if (digitalRead(D_BTN) == LOW) break; // BACK button exits
    delay(10);
  }
}
#endif

uint32_t currentTime = 0;

void setup() {
  randomSeed(esp_random());
  esp_log_level_set("*", ESP_LOG_NONE);

  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("[BSidesKC] Booting ESP32 Marauder..."));

  #ifdef HAS_SCREEN
    pinMode(TFT_BL, OUTPUT);
  #endif
  backlightOff();

  #ifdef HAS_SCREEN
    digitalWrite(TFT_CS, HIGH);
  #endif

  #if defined(HAS_SD)
    pinMode(SD_CS, OUTPUT);
    delay(10);
    digitalWrite(SD_CS, HIGH);
    delay(10);
  #endif

  #ifdef HAS_PSRAM
    if (!psramInit())
      Serial.println(F("PSRAM not available"));
  #endif

  #ifdef HAS_SIMPLEX_DISPLAY
    #ifdef HAS_SD
      if (!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));
    #endif
  #endif

  // Display init — upstream RunSetup handles tft.init() + rotation + clear
  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  // Badge backlight PWM init (after display so ledcAttach overrides TFT_eSPI's pinMode)
  brightnessInit();
  backlightOff();

  #ifdef HAS_SCREEN
    display_obj.tft.drawCentreString("BSidesKC Badge", TFT_WIDTH / 2, TFT_HEIGHT * 0.25, 1);
    display_obj.tft.drawCentreString("ESP32 Marauder", TFT_WIDTH / 2, TFT_HEIGHT * 0.40, 1);
    display_obj.tft.drawCentreString(display_obj.version_number, TFT_WIDTH / 2, TFT_HEIGHT * 0.55, 1);
  #endif

  backlightOn();

  // Touch test: hold ENTER during boot
  #ifdef HAS_CYD_TOUCH
    pinMode(C_BTN, INPUT_PULLUP);
    pinMode(D_BTN, INPUT_PULLUP);
    if (digitalRead(C_BTN) == LOW) {
      runTouchTest();
      display_obj.clearScreen();
    }
  #endif

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
