#pragma once
// BSidesKC Badge simulator — fake MenuFunctions / Menu / MenuNode.
//
// Field-for-field the same shape as the real
// esp32marauder-upstream/esp32_marauder/MenuFunctions.h (struct MenuNode /
// struct Menu) so badge_menu.cpp compiles against this UNMODIFIED. The
// *behavior* of MenuFunctionsSim::main()/changeMenu() (in the matching
// .cpp) is a deliberate, line-cited port of upstream's real logic — this
// header only declares the shape; see MenuFunctions.cpp for why it acts
// the way it does.

#include "Arduino.h"
#include "Display.h"
#include <vector>

// Icon ids badge_menu.cpp references (subset of the real MenuFunctions.h list).
#define DEVICE 17

// Minimal stand-in for ivanseidel/LinkedList — same get/size/add/remove
// surface, backed by std::vector instead of a hand-rolled linked list.
template <typename T>
class LinkedList {
public:
  int size() const { return (int)_v.size(); }
  T get(int i) const { return _v[i]; }
  T* getPtr(int i) { return &_v[i]; }
  void add(T item) { _v.push_back(item); }
  void remove(int i) { if (i >= 0 && i < (int)_v.size()) _v.erase(_v.begin() + i); }
private:
  std::vector<T> _v;
};

struct MenuNode {
  String name;
  bool command;
  uint8_t color;
  uint8_t icon;
  TFT_eSPI_Button* button;
  bool selected;
  std::function<void()> callable;
};

struct Menu {
  String name;
  LinkedList<MenuNode>* list = nullptr;
  Menu* parentMenu = nullptr;
  uint16_t selected = 0;
};

extern Display display_obj;

class MenuFunctions {
public:
  Menu* current_menu = nullptr;

  void RunSetup();
  void changeMenu(Menu* menu, bool simple_change = false);
  void main(uint32_t currentTime);

  // Harness-visible proof counter, not part of the real upstream API: how
  // many times changeMenu() actually ran. One physical tap on a
  // submenu-opening card should produce exactly one navigation. If it reads
  // 2, the tap fired through both upstream's own touch path (main(), below)
  // and badgeMenuLoop()'s — the double-fire this ticket exists to catch.
  int changeMenuCallCount = 0;

  // Harness-visible: incremented if bluetoothMenu's 2nd item ("Discover")
  // ever runs with NO scripted touch on it — see sim/README.md,
  // "phantom activation" finding.
  int phantomDiscoverFireCount = 0;

private:
  Menu mainMenu, wifiMenu, bluetoothMenu, deviceMenu;
  LinkedList<MenuNode> mainList, wifiList, bluetoothList, deviceList;
  int menu_start_index = 0;
  bool disable_touch = false;

  void buildButtons(Menu* menu, int starting_index = 0);
  void displayCurrentMenu(int start_index = 0);
};

extern MenuFunctions menu_function_obj;
