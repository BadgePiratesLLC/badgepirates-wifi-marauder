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
  #include "hardware/button_handler.h"
#endif

#include "hardware/encoder_handler.h"
#include "hardware/buzzer.h"
#include "hardware/battery_monitor.h"
#include "hardware/input_test.h"
#include "hardware/wifi_scan_test.h"
#include "hardware/led_feedback.h"
#ifdef HAS_SCREEN
  #include "hardware/badge_menu.h"
#endif
#include "hardware/power_manager.h"

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

// SD card uses dedicated SPI bus (MOSI:35, SCK:36, MISO:37, CS:47).
// HAS_CYD_TOUCH path in SDInterface::initSD() creates its own SPIClass
// using SD_SCK/SD_MISO/SD_MOSI/SD_CS pins — no external SPI object needed.
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

  // Initialize badge button handler (BOOT pin; ENTER/BACK handled by Switches)
  #ifdef HAS_BUTTONS
    buttonHandlerInit();
  #endif

  // Initialize rotary encoder (A:45, B:48, Button:20)
  encoder_init();
  Serial.println(F("[BSidesKC] Rotary encoder initialized"));

  // Initialize buzzer (GPIO 19)
  buzzerInit();

  // WiFi scan test: hold ENTER + BACK during boot (check combo FIRST)
  #if defined(HAS_SCREEN) && defined(HAS_BUTTONS)
  if (digitalRead(C_BTN) == LOW && digitalRead(D_BTN) == LOW) {
    runWifiScanTest();
    display_obj.clearScreen();
  }
  else
  #endif
  // Input validation test: hold BOOT during boot
  #ifdef HAS_SCREEN
  if (digitalRead(BTN_BOOT) == LOW) {
    runInputValidationTest();
    display_obj.clearScreen();
  }
  else
  #endif
  // Encoder test: hold BACK during boot
  #ifdef HAS_BUTTONS
  if (digitalRead(D_BTN) == LOW) {
    display_obj.tft.fillScreen(TFT_BLACK);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Encoder Test", TFT_WIDTH / 2, 4, 2);
    display_obj.tft.drawCentreString("Rotate / Press knob", TFT_WIDTH / 2, 30, 1);
    display_obj.tft.drawCentreString("Hold ENTER to exit", TFT_WIDTH / 2, TFT_HEIGHT - 20, 1);
    int pos = 0;
    while (true) {
      if (encoder_turned_up())   { pos--; Serial.printf("[Enc] UP   pos=%d\n", pos); }
      if (encoder_turned_down()) { pos++; Serial.printf("[Enc] DOWN pos=%d\n", pos); }
      if (encoder_button_pressed()) { Serial.println("[Enc] BUTTON"); }
      display_obj.tft.fillRect(0, 80, TFT_WIDTH, 40, TFT_BLACK);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawCentreString("Pos: " + String(pos), TFT_WIDTH / 2, 90, 2);
      if (digitalRead(C_BTN) == LOW) break;
      delay(50);
    }
    display_obj.clearScreen();
  }
  else
  #endif
  #ifdef HAS_CYD_TOUCH
  if (digitalRead(C_BTN) == LOW) {
    runTouchTest();
    display_obj.clearScreen();
  }
  else
  #endif
  { /* no test mode */ }

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
    batteryMonitorInit();
  #endif

  #ifdef HAS_NEOPIXEL_LED
    led_obj.RunSetup();
  #endif

  // Badge NeoPixel feedback (6× ring + status LED)
  led_feedback_init();

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    menu_function_obj.RunSetup();
    badgeMenuSetup();
  #endif

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  cli_obj.RunSetup();

  Serial.println(F("[BSidesKC] Marauder ready."));

  // Initialize power management (auto-sleep, backlight dimming)
  powerManagerInit();
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
    batteryMonitorUpdate(currentTime);
  #endif

  #ifdef HAS_SCREEN
    menu_function_obj.main(currentTime);
  #endif

  // ---- Rotary encoder → menu navigation ----
  #ifdef HAS_SCREEN
  {
    bool inMenu = (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
                  (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
                  (wifi_scan_obj.currentScanMode == OTA_UPDATE);

    bool enc_up = encoder_turned_up();
    bool enc_down = encoder_turned_down();

    if ((enc_up || enc_down) && inMenu) {
      powerManagerResetActivity();
      Menu* m = menu_function_obj.current_menu;
      int count = m->list->size();
      if (count > 0) {
        if (enc_up)
          m->selected = (m->selected == 0) ? count - 1 : m->selected - 1;
        else
          m->selected = (m->selected >= count - 1) ? 0 : m->selected + 1;

        // Compute page start for the new selection
        int page = 0;
        if (m->selected >= BUTTON_SCREEN_LIMIT)
          page = m->selected + 1 - BUTTON_SCREEN_LIMIT;
        menu_function_obj.buildButtons(m, page);
        menu_function_obj.displayCurrentMenu(page);
      }
    }

    if (encoder_button_pressed() && inMenu) {
      powerManagerResetActivity();
      buzzerPlay(TONE_BUTTON_PRESS);
      Menu* m = menu_function_obj.current_menu;
      if (m->list->size() > 0) {
        MenuNode node = m->list->get(m->selected);
        if (node.callable) node.callable();
      }
    }
  }
  #endif

  // BOOT button: short press cycles backlight
  #ifdef HAS_BUTTONS
  {
    ButtonEvent evt = buttonBootPoll();
    if (evt == BTN_EVT_RELEASE) {
      powerManagerResetActivity();
      buzzerPlay(TONE_BUTTON_PRESS);
      brightnessCycle();
    }
  }
  #endif

  // Advance async buzzer patterns
  buzzerUpdate();

  // Sync badge LED feedback with Marauder scan state
  {
    uint8_t mode = wifi_scan_obj.currentScanMode;
    if (mode == WIFI_SCAN_OFF || mode == WIFI_CONNECTED ||
        mode == OTA_UPDATE || mode == SHOW_INFO || mode == ESP_UPDATE)
      led_feedback_set(LED_IDLE);
    else if (mode <= BT_SCAN_SKIMMERS || mode == WIFI_SCAN_ESPRESSIF ||
             mode == WIFI_SCAN_TARGET_AP || mode == WIFI_SCAN_TARGET_AP_FULL ||
             mode == WIFI_SCAN_STATION || mode == WIFI_SCAN_SIG_STREN ||
             mode == WIFI_SCAN_GPS_DATA || mode == WIFI_SCAN_WAR_DRIVE ||
             mode == BT_SCAN_WAR_DRIVE || mode == BT_SCAN_AIRTAG ||
             mode == BT_SCAN_FLIPPER || mode == WIFI_SCAN_CHAN_ANALYZER ||
             mode == BT_SCAN_ANALYZER || mode == WIFI_SCAN_PINESCAN)
      led_feedback_set(LED_SCANNING);
    else
      led_feedback_set(LED_ATTACK);

    // Reset activity timer during active scans/attacks
    if (mode != WIFI_SCAN_OFF && mode != WIFI_CONNECTED)
      powerManagerResetActivity();
  }
  led_feedback_update();

  // Power management: auto-dim and auto-sleep
  powerManagerUpdate(currentTime);

  #ifdef HAS_NEOPIXEL_LED
    led_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
}
