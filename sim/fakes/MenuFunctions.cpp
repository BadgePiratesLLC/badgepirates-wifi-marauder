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

  // Nexus 155c1226: canned Probe Sniff results — "<mac> -> <ssid probed for>"
  // lines, matching real WiFiScan.cpp:6141-6153's frame-string format.
  wifiSnifferProbeResultsList.add(MenuNode{"A4:C3:F0:12:34:56 -> HomeWifi-2G",    false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"88:C6:26:AA:11:02 -> NETGEAR87",      false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"D8:BB:2C:5E:9A:41 -> xfinitywifi",    false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"F0:18:98:33:7C:2A -> ATT-9F2K",       false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"3C:5A:B4:A1:DD:09 -> eduroam",        false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"9C:B6:D0:44:1F:E7 -> Marriott_GUEST", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"AC:37:43:22:8B:56 -> iPhone (Kevin)", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"5C:F9:38:70:12:CC -> BSidesKC-Guest", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"E8:9F:80:65:2D:11 -> Galaxy S22",     false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"70:4F:57:19:BE:4A -> HP-Print-42",    false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"BC:14:85:0C:6F:33 -> (hidden)",       false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"14:7D:DA:98:C2:5F -> linksys",        false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"D0:37:45:2B:81:E0 -> CoffeeShopWiFi", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsList.add(MenuNode{"98:F1:70:AE:3D:76 -> pixel-7-pro",    false, 0, 0, nullptr, false, []() {}});
  wifiSnifferProbeResultsMenu.name = "Probe Sniff"; wifiSnifferProbeResultsMenu.parentMenu = &wifiSnifferMenu;
  wifiSnifferProbeResultsMenu.list = &wifiSnifferProbeResultsList;

  // Beacon Sniff — AP beacon frames, same "<mac> -> <essid>" shape.
  wifiSnifferBeaconResultsList.add(MenuNode{"02:1A:11:9D:44:70 -> BSidesKC-Guest",     false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"C0:56:27:8E:03:1B -> NETGEAR87",          false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"48:D3:43:7A:B0:9C -> xfinitywifi",        false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"E4:5F:01:2C:96:D8 -> ATT-9F2K",           false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"7C:B7:73:11:4A:E2 -> eduroam",            false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"F4:28:53:60:D7:19 -> Marriott_GUEST",     false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"1C:69:7A:DE:22:88 -> HP-Print-42-OfficeJ",false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"90:9A:4A:33:1E:5C -> linksys",            false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"6C:B0:CE:87:F0:2D -> CoffeeShopWiFi",     false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"B8:27:EB:15:9C:44 -> (hidden)",           false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"D4:6E:0E:2A:77:F1 -> AndroidAP-8834",     false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"58:BF:25:0D:5B:63 -> Xfinitywifi5G",      false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"24:F5:A2:99:E1:07 -> Verizon_5G_Home",    false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsList.add(MenuNode{"A0:36:9F:44:C8:2E -> WorkNet-Corp",       false, 0, 0, nullptr, false, []() {}});
  wifiSnifferBeaconResultsMenu.name = "Beacon Sniff"; wifiSnifferBeaconResultsMenu.parentMenu = &wifiSnifferMenu;
  wifiSnifferBeaconResultsMenu.list = &wifiSnifferBeaconResultsList;

  // Deauth Sniff — src/dst mac pairs + 802.11 reason code.
  wifiSnifferDeauthResultsList.add(MenuNode{"AA:BB:CC:11:22:33 -> FF:FF:FF:FF:FF:FF reason:7", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"02:1A:11:9D:44:70 -> A4:C3:F0:12:34:56 reason:1", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"C0:56:27:8E:03:1B -> 88:C6:26:AA:11:02 reason:3", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"48:D3:43:7A:B0:9C -> D8:BB:2C:5E:9A:41 reason:6", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"E4:5F:01:2C:96:D8 -> F0:18:98:33:7C:2A reason:2", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"7C:B7:73:11:4A:E2 -> 3C:5A:B4:A1:DD:09 reason:7", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"F4:28:53:60:D7:19 -> 9C:B6:D0:44:1F:E7 reason:1", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"1C:69:7A:DE:22:88 -> AC:37:43:22:8B:56 reason:8", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"90:9A:4A:33:1E:5C -> 5C:F9:38:70:12:CC reason:3", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsList.add(MenuNode{"6C:B0:CE:87:F0:2D -> E8:9F:80:65:2D:11 reason:6", false, 0, 0, nullptr, false, []() {}});
  wifiSnifferDeauthResultsMenu.name = "Deauth Sniff"; wifiSnifferDeauthResultsMenu.parentMenu = &wifiSnifferMenu;
  wifiSnifferDeauthResultsMenu.list = &wifiSnifferDeauthResultsList;

  wifiSnifferList.add(MenuNode{"Probe Sniff",  false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiSnifferProbeResultsMenu); }});
  wifiSnifferList.add(MenuNode{"Beacon Sniff", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiSnifferBeaconResultsMenu); }});
  wifiSnifferList.add(MenuNode{"Deauth Sniff", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiSnifferDeauthResultsMenu); }});
  wifiSnifferMenu.name = "Sniffers"; wifiSnifferMenu.parentMenu = &wifiMenu; wifiSnifferMenu.list = &wifiSnifferList;

  // AP Scan — SSID + RSSI + channel + security, 14 rows to force paging.
  wifiScannerApResultsList.add(MenuNode{"BSidesKC-Guest    -47dBm CH6  WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"NETGEAR87         -61dBm CH1  WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"xfinitywifi        -55dBm CH11 OPEN", false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"ATT-9F2K          -72dBm CH6  WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"HP-Print-42-OfficeJet -66dBm CH3 WEP",false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"iPhone (Kevin)    -38dBm CH149 WPA3", false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"eduroam           -80dBm CH36 WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"Marriott_GUEST    -89dBm CH1  OPEN",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"(hidden)          -58dBm CH11 WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"CorpNet-5F-Secure -44dBm CH40 WPA3",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"linksys           -69dBm CH6  WEP",   false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"AndroidAP-8834    -50dBm CH1  WPA2",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"This-SSID-Is-Exactly-32-Chars-L -30dBm CH9 WPA3", false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsList.add(MenuNode{"CoffeeShopWiFi    -63dBm CH11 OPEN",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerApResultsMenu.name = "AP Scan"; wifiScannerApResultsMenu.parentMenu = &wifiScannerMenu;
  wifiScannerApResultsMenu.list = &wifiScannerApResultsList;

  // Station Scan — client mac + associated AP index + packet count.
  wifiScannerStationResultsList.add(MenuNode{"A4:C3:F0:12:34:56  AP:2   148pkts", false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"88:C6:26:AA:11:02  AP:0   62pkts",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"D8:BB:2C:5E:9A:41  AP:5   903pkts", false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"F0:18:98:33:7C:2A  AP:1   11pkts",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"3C:5A:B4:A1:DD:09  AP:6   274pkts", false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"9C:B6:D0:44:1F:E7  AP:7   5pkts",   false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"AC:37:43:22:8B:56  AP:5   1041pkts",false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"5C:F9:38:70:12:CC  AP:0   389pkts", false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"E8:9F:80:65:2D:11  AP:2   17pkts",  false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsList.add(MenuNode{"70:4F:57:19:BE:4A  AP:9   462pkts", false, 0, 0, nullptr, false, []() {}});
  wifiScannerStationResultsMenu.name = "Station Scan"; wifiScannerStationResultsMenu.parentMenu = &wifiScannerMenu;
  wifiScannerStationResultsMenu.list = &wifiScannerStationResultsList;

  wifiScannerList.add(MenuNode{"AP Scan",      false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiScannerApResultsMenu); }});
  wifiScannerList.add(MenuNode{"Station Scan", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiScannerStationResultsMenu); }});
  wifiScannerMenu.name = "Scanners"; wifiScannerMenu.parentMenu = &wifiMenu; wifiScannerMenu.list = &wifiScannerList;

  wifiAttackList.add(MenuNode{"Deauth Attack", false, 0, 0, nullptr, false, []() {}});
  wifiAttackList.add(MenuNode{"Beacon Spam",   false, 0, 0, nullptr, false, []() {}});
  wifiAttackList.add(MenuNode{"Evil Portal",   false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiAttackEvilPortalMenu); }});
  wifiAttackMenu.name = "Attacks"; wifiAttackMenu.parentMenu = &wifiMenu; wifiAttackMenu.list = &wifiAttackList;

  wifiList.add(MenuNode{"Sniffers", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiSnifferMenu); }});
  wifiList.add(MenuNode{"Scanners", false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiScannerMenu); }});
  wifiList.add(MenuNode{"Attacks",  false, 0, 0, nullptr, false, [this]() { changeMenu(&wifiAttackMenu); }});
  wifiMenu.name = "WiFi"; wifiMenu.parentMenu = &mainMenu; wifiMenu.list = &wifiList;

  // Bluetooth Classic — device name + MAC + RSSI.
  bluetoothScanClassicResultsList.add(MenuNode{"JBL Flip 5     88:C6:26:AA:BB:CC -58dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"Kevin's AirPods A4:C3:F0:11:22:33 -49dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"Car Stereo     D8:BB:2C:44:55:66 -71dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"Bose SoundLink F0:18:98:77:88:99 -63dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"(unknown)      3C:5A:B4:AA:CC:EE -84dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"HP OfficeJet   9C:B6:D0:12:34:56 -68dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"Garmin Watch   AC:37:43:99:88:77 -55dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsList.add(MenuNode{"(unknown)      5C:F9:38:00:11:22 -90dBm",  false, 0, 0, nullptr, false, []() {}});
  bluetoothScanClassicResultsMenu.name = "Classic"; bluetoothScanClassicResultsMenu.parentMenu = &bluetoothScanMenu;
  bluetoothScanClassicResultsMenu.list = &bluetoothScanClassicResultsList;

  // BLE — noisier, more anonymous devices.
  bluetoothScanBleResultsList.add(MenuNode{"Galaxy Buds2    E8:9F:80:11:22:33 -52dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"Apple Watch     70:4F:57:44:55:66 -47dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       BC:14:85:77:88:99 -81dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       14:7D:DA:AA:BB:CC -88dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"Fitbit Charge 6 D0:37:45:00:11:22 -66dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       98:F1:70:33:44:55 -74dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"Tile Tracker    02:1A:11:66:77:88 -59dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       C0:56:27:99:AA:BB -85dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"Amazfit GTS 4   48:D3:43:CC:DD:EE -69dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       E4:5F:01:22:33:44 -92dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"Pixel Buds Pro  7C:B7:73:55:66:77 -54dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsList.add(MenuNode{"(unknown)       F4:28:53:88:99:00 -79dBm", false, 0, 0, nullptr, false, []() {}});
  bluetoothScanBleResultsMenu.name = "BLE"; bluetoothScanBleResultsMenu.parentMenu = &bluetoothScanMenu;
  bluetoothScanBleResultsMenu.list = &bluetoothScanBleResultsList;

  bluetoothScanList.add(MenuNode{"Classic", false, 0, 0, nullptr, false, [this]() { changeMenu(&bluetoothScanClassicResultsMenu); }});
  bluetoothScanList.add(MenuNode{"BLE",     false, 0, 0, nullptr, false, [this]() { changeMenu(&bluetoothScanBleResultsMenu); }});
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
