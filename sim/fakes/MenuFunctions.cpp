// BSidesKC Badge simulator — behavioral model of upstream MenuFunctions.
//
// This is NOT a port of the real 3828-line
// esp32marauder-upstream/esp32_marauder/MenuFunctions.cpp (it drags in
// WiFiScan/BLE/GPS/SD and dozens of scan-mode branches that have nothing to
// do with the UI bug this ticket exists to catch, and are not host-portable
// without a much larger stubbing effort — see sim/README.md, "what this
// does not cover"). Instead this reproduces, faithfully and with the real
// file/line cited at each spot, the ONE interaction Jared's 01:44 diagnosis
// named as the root cause: upstream's own touch-read + own-render code
// paths keep running, unsuppressed, alongside badgeMenuLoop()'s.
//
// Cited source (read directly, quoted line numbers as of commit 5c93981):
//   MenuFunctions.cpp:103        MenuFunctions::main() entry
//   MenuFunctions.cpp:184-186    pressed = display_obj.updateTouch(...)
//                                 — upstream's OWN touch read, independent
//                                 of touch_input.cpp's touchRead().
//   MenuFunctions.cpp:464        display_obj.menuButton(&t_x,&t_y,pressed)
//   MenuFunctions.cpp:598-599    if (menu_button == SELECT_BUTTON)
//                                   current_menu->list->get(current_menu
//                                     ->selected).callable();
//                                 — fires the selected node directly,
//                                 uncoordinated with badgeMenuLoop().
//   Display.cpp:14-40            Display::menuButton() — hit-tests THREE
//                                 invisible "Chicken" buttons buildButtons()
//                                 lays across the full screen width in
//                                 vertical thirds (UP/SELECT/DOWN); ANY tap
//                                 anywhere on the panel lands in one of them.
//   MenuFunctions.cpp:3587-3611  changeMenu(): always buildButtons() +
//                                 displayCurrentMenu(), regardless of what
//                                 else currently owns the screen.
//   MenuFunctions.cpp:3681-3735  displayCurrentMenu(): clearScreen() then
//                                 draws upstream's own button list.
//
// main.cpp calls menu_function_obj.main(currentTime) THEN badgeMenuLoop()
// every loop() iteration (main.cpp:370, :379) — both read touch, both can
// fire a callable, and changeMenu() draws upstream's own screen no matter
// which path reached it. Nothing here stops that; nothing in half 2 did
// either, which is the bug.

#include "configs.h"
#include "MenuFunctions.h"
#include "hardware/buzzer.h"

Display display_obj;
MenuFunctions menu_function_obj;
WiFiScan wifi_scan_obj;

#define UP_BUTTON     0
#define SELECT_BUTTON 1
#define DOWN_BUTTON   2

void MenuFunctions::RunSetup() {
  // WiFi -> Attacks -> Evil Portal: the one branch given a real 4th level
  // (matching esp32marauder-upstream's own wifiAttackMenu "Evil Portal"
  // entry), so root-menu depth is provable without inventing screens that
  // don't exist upstream. Leaves stay no-op deliberately - they're where a
  // real scan/attack mode would start (out of scope per sim/README.md's
  // "what this does NOT cover"), not another menu to navigate.
  wifiAttackEvilPortalList.add(MenuNode{"Start", false, 0, 0, nullptr, false, []() {}});
  wifiAttackEvilPortalList.add(MenuNode{"Stop",  false, 0, 0, nullptr, false, []() {}});
  wifiAttackEvilPortalMenu.name = "Evil Portal"; wifiAttackEvilPortalMenu.parentMenu = &wifiAttackMenu;
  wifiAttackEvilPortalMenu.list = &wifiAttackEvilPortalList;

  wifiSnifferList.add(MenuNode{"Probe Sniff",  false, 0, 0, nullptr, false, []() {}});
  wifiSnifferList.add(MenuNode{"Beacon Sniff", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferList.add(MenuNode{"Deauth Sniff", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferMenu.name = "Sniffers"; wifiSnifferMenu.parentMenu = &wifiMenu; wifiSnifferMenu.list = &wifiSnifferList;

  wifiScannerList.add(MenuNode{"AP Scan",      false, 0, 0, nullptr, false, []() {}});
  wifiScannerList.add(MenuNode{"Station Scan", false, 0, 0, nullptr, false, []() {}});
  wifiScannerMenu.name = "Scanners"; wifiScannerMenu.parentMenu = &wifiMenu; wifiScannerMenu.list = &wifiScannerList;

  wifiAttackList.add(MenuNode{"Deauth Attack", false, 0, 0, nullptr, false, []() {}});
  wifiAttackList.add(MenuNode{"Beacon Spam",   false, 0, 0, nullptr, false, []() {}});
  wifiAttackList.add(MenuNode{"Evil Portal",   false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiAttackEvilPortalMenu); }});
  wifiAttackMenu.name = "Attacks"; wifiAttackMenu.parentMenu = &wifiMenu; wifiAttackMenu.list = &wifiAttackList;

  wifiList.add(MenuNode{"Sniffers", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiSnifferMenu); }});
  wifiList.add(MenuNode{"Scanners", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiScannerMenu); }});
  wifiList.add(MenuNode{"Attacks",  false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiAttackMenu); }});
  wifiMenu.name = "WiFi"; wifiMenu.parentMenu = &mainMenu; wifiMenu.list = &wifiList;

  bluetoothScanList.add(MenuNode{"Classic", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanList.add(MenuNode{"BLE",     false, 0, 0, nullptr, false, []() {}});
  bluetoothScanMenu.name = "Scan"; bluetoothScanMenu.parentMenu = &bluetoothMenu; bluetoothScanMenu.list = &bluetoothScanList;

  bluetoothList.add(MenuNode{"Scan", false, 0, 0, nullptr, false, [this]() { changeMenu(&bluetoothScanMenu); }});
  bluetoothList.add(MenuNode{"Discover", false, 0, 0, nullptr, false, [this]() { phantomDiscoverFireCount++; }});
  bluetoothMenu.name = "Bluetooth"; bluetoothMenu.parentMenu = &mainMenu; bluetoothMenu.list = &bluetoothList;

  // "Settings " (trailing space, deliberate): src/hardware/badge_menu.cpp's
  // settingsScreen() -> navigateByNodeName() looks for this exact upstream
  // node name (its own comment cites lang_var.h's text1_9/text1_18,
  // "trailing space included") to reach Device -> Settings the same way a
  // real tap on those two cards would. The old fake's plain "Device" (no
  // space) meant that lookup always failed silently - the gear icon's
  // "Marauder" card was a second, independent dead end from the one this
  // ticket's diagnosis found in badgeMenuLoop() itself.
  deviceSettingsList.add(MenuNode{"Brightness", false, 0, 0, nullptr, false, []() {}});
  deviceSettingsList.add(MenuNode{"Sound",      false, 0, 0, nullptr, false, []() {}});
  deviceSettingsMenu.name = "Settings"; deviceSettingsMenu.parentMenu = &deviceMenu; deviceSettingsMenu.list = &deviceSettingsList;

  deviceList.add(MenuNode{"About", false, 0, 0, nullptr, false, []() {}});
  deviceList.add(MenuNode{"Settings ", false, 0, 0, nullptr, false, [this]() { changeMenu(&deviceSettingsMenu); }});
  deviceMenu.name = "Device"; deviceMenu.parentMenu = &mainMenu; deviceMenu.list = &deviceList;

  mainList.add(MenuNode{"WiFi",      false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiMenu); }});
  mainList.add(MenuNode{"Bluetooth", false, 0, 0, nullptr, false, [this]() { changeMenu(&bluetoothMenu); }});
  // "Device " (trailing space, deliberate): see the "Settings " comment
  // above - settingsScreen()'s navigateByNodeName(s_mainMenu, "Device ")
  // needs this exact name to find its way to deviceMenu at all.
  mainList.add(MenuNode{"Device ",   false, 0, 0, nullptr, false, [this]() { changeMenu(&deviceMenu); }});
  mainList.add(MenuNode{"Reboot",    false, 0, 0, nullptr, false, []() {}});
  mainMenu.name = "Main Menu"; mainMenu.parentMenu = nullptr; mainMenu.list = &mainList;

  current_menu = &mainMenu;
}

// Real upstream: MenuFunctions.cpp:3614-3663 (buildButtons). Lays out one
// visible-list button per item PLUS the three invisible full-width
// "Chicken" thirds (Display.cpp:14 reads these back as UP/SELECT/DOWN).
void MenuFunctions::buildButtons(Menu* menu, int starting_index) {
  menu_start_index = starting_index;
  if (menu->list) {
    int visible = min((int)BUTTON_SCREEN_LIMIT, menu->list->size() - starting_index);
    for (int i = 0; i < visible; i++) {
      display_obj.key[i].initButtonUL(&display_obj.tft, 4, 30 + i * 20, TFT_WIDTH - 8, 18,
                                       TFT_LIGHTGREY, TFT_BLACK, TFT_LIGHTGREY,
                                       menu->list->get(starting_index + i).name.c_str(), 1);
    }
  }
  for (int i = BUTTON_ARRAY_LEN; i < BUTTON_ARRAY_LEN + 3; i++) {
    int band = i - BUTTON_ARRAY_LEN;
    int bandH = TFT_HEIGHT / 3;
    display_obj.key[i].initButtonUL(&display_obj.tft, 0, band * bandH, TFT_WIDTH, bandH,
                                     TFT_LIGHTGREY, TFT_BLACK, TFT_BLACK, "Chicken", 1);
  }
}

// Real upstream: MenuFunctions.cpp:3681-3735 (displayCurrentMenu). Draws
// upstream's OWN full-screen button list every time changeMenu() runs.
void MenuFunctions::displayCurrentMenu(int start_index) {
  display_obj.clearScreen();
  if (!current_menu->list) return;
  int n = min((int)BUTTON_SCREEN_LIMIT, current_menu->list->size() - start_index);
  for (int i = 0; i < n; i++)
    display_obj.key[i].drawButton(current_menu->selected == (uint16_t)(start_index + i));
}

// Real upstream: MenuFunctions.cpp:3587-3611 (changeMenu).
void MenuFunctions::changeMenu(Menu* menu, bool simple_change) {
  changeMenuCallCount++;
  if (!simple_change) display_obj.init();
  current_menu = menu;
  current_menu->selected = 0;
  buildButtons(menu);
  displayCurrentMenu();
}

// Real upstream: MenuFunctions.cpp:103 (main), reduced to the touch-select
// path that matters for this bug — see file header for the cited lines.
void MenuFunctions::main(uint32_t) {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = !disable_touch && (display_obj.updateTouch(&t_x, &t_y) > 0);

  if (!current_menu ||
      (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF &&
       wifi_scan_obj.currentScanMode != WIFI_CONNECTED))
    return;

  int8_t menu_button = display_obj.menuButton(&t_x, &t_y, pressed);
  if (menu_button < 0) return;

  if (menu_button == UP_BUTTON) {
    if (current_menu->selected > 0) current_menu->selected--;
  } else if (menu_button == DOWN_BUTTON) {
    if (current_menu->list && current_menu->selected < (uint16_t)(current_menu->list->size() - 1))
      current_menu->selected++;
  } else if (menu_button == SELECT_BUTTON) {
    if (current_menu->list) current_menu->list->get(current_menu->selected).callable();
  }
}
