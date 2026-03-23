// BSidesKC Badge - Badge-specific menu integration
// Injects badge hardware items into Marauder's menu system

#include "configs.h"
#ifdef HAS_SCREEN

#include "hardware/badge_menu.h"
#include "MenuFunctions.h"
#include "Display.h"
#include "hardware/buzzer.h"
#include "hardware/battery_monitor.h"
#include "hardware/led_feedback.h"
#include "hardware/display_adapter.h"
#include "hardware/encoder_handler.h"
#include "hardware/input_test.h"

extern MenuFunctions menu_function_obj;
extern Display display_obj;

// Badge submenu (static so it persists)
static Menu badgeMenu;

static void showBatteryStatus() {
  display_obj.clearScreen();
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawCentreString("Battery Status", TFT_WIDTH / 2, 30, 2);

  int8_t pct = batteryGetPercent();
  uint16_t color = (pct > 50) ? TFT_GREEN : (pct > 20) ? TFT_YELLOW : TFT_RED;
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.drawCentreString(String(pct) + "%", TFT_WIDTH / 2, 80, 4);

  // Draw bar
  uint16_t barX = 40, barY = 140, barW = TFT_WIDTH - 80, barH = 24;
  display_obj.tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
  uint16_t fillW = max(0, (int)((barW - 4) * pct / 100));
  display_obj.tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, color);

  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.drawCentreString("Press knob to return", TFT_WIDTH / 2, TFT_HEIGHT - 30, 1);

  // Wait for encoder button or BACK
  while (true) {
    if (encoder_button_pressed()) break;
    #ifdef HAS_BUTTONS
      if (digitalRead(D_BTN) == LOW) break;
    #endif
    delay(50);
  }
  menu_function_obj.changeMenu(&badgeMenu, true);
}

static void toggleBuzzerMute() {
  buzzerMute(!buzzerIsMuted());
  display_obj.clearScreen();
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawCentreString(
    buzzerIsMuted() ? "Buzzer: MUTED" : "Buzzer: ON",
    TFT_WIDTH / 2, TFT_HEIGHT / 2 - 10, 2);
  delay(800);
  menu_function_obj.changeMenu(&badgeMenu, true);
}

static void ledBrightnessMode() {
  const uint8_t levels[] = {5, 15, 33, 66, 100, 150, 200, 255};
  const uint8_t numLevels = 8;
  static uint8_t idx = 2;  // default ~33

  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawCentreString("LED Brightness", TFT_WIDTH / 2, 20, 2);
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.drawCentreString("Rotate to adjust, press to save", TFT_WIDTH / 2, TFT_HEIGHT - 25, 1);

  auto drawBar = [&]() {
    uint16_t barX = 30, barY = TFT_HEIGHT / 2 - 15, barW = TFT_WIDTH - 60, barH = 30;
    display_obj.tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
    display_obj.tft.fillRect(barX + 2, barY + 2, barW - 4, barH - 4, TFT_BLACK);
    uint16_t fillW = (barW - 4) * (idx + 1) / numLevels;
    display_obj.tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, TFT_MAGENTA);
    display_obj.tft.fillRect(0, barY + barH + 5, TFT_WIDTH, 20, TFT_BLACK);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawCentreString(String(levels[idx] * 100 / 255) + "%", TFT_WIDTH / 2, barY + barH + 8, 2);
  };
  drawBar();

  led_feedback_set_brightness(levels[idx]);

  while (true) {
    if (encoder_turned_up() && idx > 0) {
      idx--;
      led_feedback_set_brightness(levels[idx]);
      drawBar();
    }
    if (encoder_turned_down() && idx < numLevels - 1) {
      idx++;
      led_feedback_set_brightness(levels[idx]);
      drawBar();
    }
    if (encoder_button_pressed()) break;
    #ifdef HAS_BUTTONS
      if (digitalRead(D_BTN) == LOW) break;
    #endif
    delay(30);
  }

  menu_function_obj.changeMenu(&badgeMenu, true);
}

static void runHwTest() {
  runInputValidationTest();
  display_obj.clearScreen();
  menu_function_obj.changeMenu(&badgeMenu, true);
}

void badgeMenuSetup() {
  // Build badge submenu
  badgeMenu.list = new LinkedList<MenuNode>();
  badgeMenu.name = "Badge";

  // current_menu points to mainMenu after RunSetup
  Menu* mainMenu = menu_function_obj.current_menu;
  badgeMenu.parentMenu = mainMenu;

  // Back item
  TFT_eSPI_Button* btn0 = new TFT_eSPI_Button();
  badgeMenu.list->add(MenuNode{"Back", false, TFTLIGHTGREY, 0, btn0, false, []() {
    menu_function_obj.changeMenu(badgeMenu.parentMenu, true);
  }});

  // LED Brightness
  TFT_eSPI_Button* btn1 = new TFT_eSPI_Button();
  badgeMenu.list->add(MenuNode{"LED Brightness", false, TFTMAGENTA, BRIGHTNESS, btn1, false, []() {
    ledBrightnessMode();
  }});

  // Buzzer Mute/Unmute
  TFT_eSPI_Button* btn2 = new TFT_eSPI_Button();
  badgeMenu.list->add(MenuNode{"Buzzer Mute", false, TFTYELLOW, DEVICE, btn2, false, []() {
    toggleBuzzerMute();
  }});

  // Battery Status
  TFT_eSPI_Button* btn3 = new TFT_eSPI_Button();
  badgeMenu.list->add(MenuNode{"Battery Status", false, TFTGREEN, STATUS_BAT, btn3, false, []() {
    showBatteryStatus();
  }});

  // Hardware Test
  TFT_eSPI_Button* btn4 = new TFT_eSPI_Button();
  badgeMenu.list->add(MenuNode{"Hardware Test", false, TFTCYAN, DEVICE_INFO, btn4, false, []() {
    runHwTest();
  }});

  // Insert "Badge" item into main menu (before Reboot which is last)
  int insertPos = mainMenu->list->size() - 1;  // before Reboot
  if (insertPos < 0) insertPos = 0;

  TFT_eSPI_Button* btnMain = new TFT_eSPI_Button();
  // LinkedList doesn't have insert, so we remove last, add Badge, re-add last
  MenuNode rebootNode = mainMenu->list->get(mainMenu->list->size() - 1);
  mainMenu->list->remove(mainMenu->list->size() - 1);
  mainMenu->list->add(MenuNode{"Badge", false, TFTMAGENTA, DEVICE, btnMain, false, []() {
    menu_function_obj.changeMenu(&badgeMenu, true);
  }});
  TFT_eSPI_Button* btnReboot = new TFT_eSPI_Button();
  rebootNode.button = btnReboot;
  mainMenu->list->add(rebootNode);

  // Rebuild display with new items
  menu_function_obj.changeMenu(mainMenu, true);

  Serial.println(F("[Badge] Menu items added"));
}

#endif
