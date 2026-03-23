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
  badge_display.drawCenteredTitle("Battery Status");

  int8_t pct = batteryGetPercent();
  uint16_t color = (pct > 50) ? UI_COLOR_OK : (pct > 20) ? UI_COLOR_WARN : UI_COLOR_ERR;
  display_obj.tft.setTextColor(color, UI_COLOR_BG);
  display_obj.tft.drawCentreString(String(pct) + "%", TFT_WIDTH / 2, 70, 4);

  badge_display.drawProgressBar(UI_BAR_MARGIN, 130, TFT_WIDTH - UI_BAR_MARGIN * 2, UI_BAR_H, max(0, (int)pct), color);
  badge_display.drawStatusHint("Press knob to return");

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
  badge_display.drawCenteredTitle(buzzerIsMuted() ? "Buzzer: MUTED" : "Buzzer: ON");
  delay(800);
  menu_function_obj.changeMenu(&badgeMenu, true);
}

static void ledBrightnessMode() {
  const uint8_t levels[] = {5, 15, 33, 66, 100, 150, 200, 255};
  const uint8_t numLevels = 8;
  static uint8_t idx = 2;  // default ~33

  display_obj.tft.fillScreen(UI_COLOR_BG);
  badge_display.drawCenteredTitle("LED Brightness");
  badge_display.drawStatusHint("Rotate to adjust, press to save");

  auto drawBar = [&]() {
    uint16_t barX = UI_BAR_MARGIN, barY = TFT_HEIGHT / 2 - 15;
    uint16_t barW = TFT_WIDTH - UI_BAR_MARGIN * 2;
    uint8_t pct = (idx + 1) * 100 / numLevels;
    badge_display.drawProgressBar(barX, barY, barW, UI_BAR_H, pct, TFT_MAGENTA);
    display_obj.tft.fillRect(0, barY + UI_BAR_H + 5, TFT_WIDTH, 20, UI_COLOR_BG);
    display_obj.tft.setTextColor(UI_COLOR_BODY, UI_COLOR_BG);
    display_obj.tft.drawCentreString(String(levels[idx] * 100 / 255) + "%", TFT_WIDTH / 2, barY + UI_BAR_H + 8, UI_TITLE_FONT);
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
