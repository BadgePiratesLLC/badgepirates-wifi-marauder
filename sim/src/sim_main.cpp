// BSidesKC Badge simulator — Nexus 0f35e128.
//
// Headless screenshot-mode harness: runs the REAL production UI code
// (badge_menu.cpp, touch_input.cpp, badge_nav.cpp, badge_ui_theme.h,
// unmodified) against fakes of the hardware seam (TFT/Display/touch/
// encoder/MenuFunctions), scripts a tap sequence, and writes PNGs. See
// sim/README.md for what this does and does not prove.
//
// Order-of-work per Jared (Nexus 78e62be0, 01:46): screenshot mode first,
// then reproduce the half-2 bug with it. This file does both: `root` mode
// renders the three review screens the ticket wants Kevin to eventually
// approve; `repro` mode is the specific acceptance test — "render the root
// and one submenu and look at them... if the card layer overlays upstream's
// menu in the simulator, the bug is reproduced with no hardware."

#include <cstdio>
#include <cstring>
#include <string>
#include "configs.h"
#include "Arduino.h"
#include "Display.h"
#include "MenuFunctions.h"
#include "hardware/badge_menu.h"
#include "sim_counters.h"
#include "Framebuffer.h"
#include "png_writer.h"

// ---- globals the fakes declare extern ----
Framebuffer gFb;
SimState g_sim;
Print Serial;
static uint32_t s_millis = 0;
uint32_t millis() { return s_millis; }
void sim_advance_millis(uint32_t ms) { s_millis += ms; }

static void shot(const std::string& path) {
  if (!write_png(path.c_str(), gFb)) {
    std::fprintf(stderr, "FAILED to write %s\n", path.c_str());
  } else {
    std::printf("wrote %s\n", path.c_str());
  }
}

// sim_capture_frame() is badge_menu.cpp's SIM_FRAME() hook target (see
// SIM_BUILD guard there). It exists because ledBrightnessOptionsScreen() -
// the badge's one true "options" screen, req #3 from Nexus 78e62be0 half 1
// - draws itself and then blocks in its own while(true) input loop with no
// other seam back out to a headless harness. The hook fires once, right
// after that screen's first real render() call, with the exact same
// production drawing code that runs on hardware. It screenshots gFb, then
// arms a fake encoder press so the very next loop iteration exits cleanly
// - no touch scripted, no risk of spinning forever on stale touch state.
static std::string s_captureLabel;
static std::string s_capturePath;
void sim_capture_frame(const char* label) {
  if (s_captureLabel.empty() || s_captureLabel != label) return;
  shot(s_capturePath);
  g_sim.encPress = true;
}
extern void ledBrightnessOptionsScreen();  // SIM_BUILD gives this external linkage

// One real loop() tick, split so the harness can screenshot in between the
// two draw owners — main.cpp itself calls these back-to-back every
// iteration (main.cpp:370 then :379); splitting them here doesn't change
// what runs, only lets us see what's on screen after each one.
static void tickUpstreamOnly() {
  sim_advance_millis(5);
  menu_function_obj.main(millis());
}
static void tickAdapterOnly() {
  badgeMenuLoop();
}

static void tap(uint16_t x, uint16_t y) {
  sim_set_touch(true, x, y);
  tickUpstreamOnly();
  tickAdapterOnly();
  sim_set_touch(false, x, y);
  tickUpstreamOnly();
  tickAdapterOnly();
}

// Mirrors main.cpp's post-fix loop() exactly (Nexus 78e62be0): exactly one
// of {menu_function_obj.main(), badgeMenuLoop()} touches touch/screen state
// per tick, decided by badgeMenuOwnsScreen(). "repro"/"doublefire" modes
// below call tickUpstreamOnly()/tickAdapterOnly() unconditionally instead,
// on purpose - that's the pre-fix behavior being proven buggy.
static void fixedTick() {
  sim_advance_millis(5);
  if (!badgeMenuOwnsScreen()) menu_function_obj.main(millis());
  badgeMenuLoop();
}
static void fixedTap(uint16_t x, uint16_t y) {
  sim_set_touch(true, x, y);
  fixedTick();
  sim_set_touch(false, x, y);
  fixedTick();
}

int main(int argc, char** argv) {
  std::string mode = argc > 1 ? argv[1] : "root";
  std::string outdir = argc > 2 ? argv[2] : "sim/out";

  gFb.init(TFT_WIDTH, TFT_HEIGHT);

  menu_function_obj.RunSetup();
  badgeMenuSetup();  // real production code, unmodified

  if (mode == "root") {
    tickAdapterOnly();  // first frame after setup: badgeMenuLoop's initial render
    shot(outdir + "/01_root.png");

    // One submenu: root's first card is upstream's real "WiFi" entry
    // (mainMenu list order set in MenuFunctionsSim::RunSetup, matching
    // production's mainMenu). Tap it, then screenshot the result.
    int before = menu_function_obj.changeMenuCallCount;
    tap(120, 78);  // inside the WiFi card (row 0): see sim/README.md for the math
    shot(outdir + "/02_submenu.png");

    std::printf("changeMenu() calls caused by this one WiFi tap: %d (expect 1)\n",
                menu_function_obj.changeMenuCallCount - before);
    return 0;
  }

  if (mode == "repro") {
    // Acceptance test from Jared's 01:46 comment on Nexus 78e62be0: render
    // root, tap into a submenu, and LOOK — specifically at the frame drawn
    // right after changeMenu() runs (upstream's own list) versus the frame
    // after badgeMenuLoop() runs next (our cards). Two different renderers
    // touching the same screen for one tap is the bug, reproduced with no
    // badge attached.
    tickAdapterOnly();
    shot(outdir + "/repro_00_root_cards.png");

    uint16_t wifiX = 120, wifiY = 78;  // row 0 card center, see sim/README.md
    sim_set_touch(true, wifiX, wifiY);
    tickUpstreamOnly();
    tickAdapterOnly();
    sim_set_touch(false, wifiX, wifiY);

    tickUpstreamOnly();  // this is the tick where our own card's callable
                         // fires changeMenu(&wifiMenu) -> upstream's OWN
                         // buildButtons()+displayCurrentMenu() draws
    shot(outdir + "/repro_01_upstream_list_after_changeMenu.png");
    std::printf("changeMenuCallCount right after tickUpstreamOnly: %d\n",
                menu_function_obj.changeMenuCallCount);

    tickAdapterOnly();  // badgeMenuLoop() runs next, same physical loop()
                        // iteration as the line above on real hardware
    shot(outdir + "/repro_02_cards_after_badgeMenuLoop.png");
    std::printf("changeMenuCallCount after tickAdapterOnly: %d\n",
                menu_function_obj.changeMenuCallCount);
    std::printf("ourButtonPressBuzzCount: %d\n", g_sim.ourButtonPressBuzzCount);

    // Next loop() tick, no new touch: this is the frame where badgeMenuLoop()
    // finally notices current_menu changed and redraws cards. On real
    // hardware this is a few milliseconds later - fast, but not zero, and
    // not free: it's a real, visible flash of upstream's raw list on every
    // single navigation, not a one-time glitch.
    tickUpstreamOnly();
    tickAdapterOnly();
    shot(outdir + "/repro_03_cards_finally_repainted.png");
    return 0;
  }

  if (mode == "doublefire") {
    // Bluetooth (mainMenu row 1) sits at screen y~108-152 - inside
    // upstream's own SELECT third (Display.cpp's menuButton() splits the
    // full panel into UP/SELECT/DOWN bands at y<106 / 106-213 / y>213,
    // regardless of where OUR cards are). A tap there can fire upstream's
    // OWN "current_menu->list->get(selected).callable()" (MenuFunctions.cpp
    // :598) directly, independent of - and in addition to - our own
    // adaptedActivate() firing the same node because our TapDetector
    // correctly hit-tested the real Bluetooth card. Two independent code
    // paths, same physical tap: this is the double-fire half of Jared's
    // 01:44 diagnosis, not just the overlay.
    tickAdapterOnly();
    int before = menu_function_obj.changeMenuCallCount;
    int buzzBefore = g_sim.ourButtonPressBuzzCount;

    uint16_t btX = 120, btY = 130;
    sim_set_touch(true, btX, btY);
    tickUpstreamOnly();
    tickAdapterOnly();
    sim_set_touch(false, btX, btY);
    tickUpstreamOnly();
    int afterUpstream = menu_function_obj.changeMenuCallCount;
    tickAdapterOnly();
    int afterAdapter = menu_function_obj.changeMenuCallCount;

    std::printf("changeMenu() calls from upstream's OWN touch path alone: %d\n", afterUpstream - before);
    std::printf("changeMenu() calls from badgeMenuLoop()'s path (same tick, same tap): %d\n", afterAdapter - afterUpstream);
    std::printf("TOTAL changeMenu() calls for ONE physical tap: %d (expect 1; >1 is the double-fire)\n", afterAdapter - before);
    std::printf("menu actually open after tapping Bluetooth: %s (expect Bluetooth)\n",
                menu_function_obj.current_menu->name.c_str());
    std::printf("buzzerPlay(TONE_BUTTON_PRESS) calls for this one tap: %d\n", g_sim.ourButtonPressBuzzCount - buzzBefore);
    shot(outdir + "/doublefire_final.png");

    // No new touch input at all from here on - the finger is off the glass.
    // badgeMenuLoop()'s OWN TapDetector never got to process the touch-up
    // above (it bailed out early because current_menu had already changed
    // underneath it), so its _down/_downId state is stale: it still thinks
    // a touch begun on the OLD menu (mainMenu, row 1) is being held. Watch
    // whether that stale state "fires" a card on the NEW menu with zero
    // physical input.
    for (int i = 0; i < 3 && menu_function_obj.phantomDiscoverFireCount == 0; i++) {
      tickUpstreamOnly();
      tickAdapterOnly();
    }
    std::printf("phantomDiscoverFireCount with NO further touch input: %d (expect 0)\n",
                menu_function_obj.phantomDiscoverFireCount);
    shot(outdir + "/doublefire_after_idle_ticks.png");
    return 0;
  }

  if (mode == "fixed") {
    // Same two scenarios as "repro" and "doublefire", run through
    // fixedTap() (main.cpp's post-fix gating) instead of raw unconditional
    // ticks. This is the simulator-side proof the fix works, before it
    // goes anywhere near the physical badge.
    fixedTick();
    shot(outdir + "/fixed_00_root.png");

    int before = menu_function_obj.changeMenuCallCount;
    fixedTap(120, 78);  // WiFi
    shot(outdir + "/fixed_01_after_wifi_tap.png");
    std::printf("[fixed] changeMenu() calls for the WiFi tap: %d (expect 1)\n",
                menu_function_obj.changeMenuCallCount - before);
    std::printf("[fixed] menu open after WiFi tap: %s (expect WiFi)\n",
                menu_function_obj.current_menu->name.c_str());

    // Back to root, then the exact tap that used to open the WRONG menu.
    menu_function_obj.RunSetup();
    badgeMenuSetup();
    fixedTick();
    before = menu_function_obj.changeMenuCallCount;
    int buzzBefore = g_sim.ourButtonPressBuzzCount;
    fixedTap(120, 130);  // Bluetooth
    shot(outdir + "/fixed_02_after_bluetooth_tap.png");
    std::printf("[fixed] changeMenu() calls for the Bluetooth tap: %d (expect 1)\n",
                menu_function_obj.changeMenuCallCount - before);
    std::printf("[fixed] menu open after Bluetooth tap: %s (expect Bluetooth)\n",
                menu_function_obj.current_menu->name.c_str());
    std::printf("[fixed] buzzerPlay(TONE_BUTTON_PRESS) calls for this tap: %d (expect 1)\n",
                g_sim.ourButtonPressBuzzCount - buzzBefore);
    return 0;
  }

  if (mode == "options") {
    // The third review screen the ticket wants (root / submenu / options):
    // LED Brightness, the options-as-buttons grid from half 1. Reached on
    // hardware via root -> "Badge" card -> "LED Brightness" card, but both
    // of those are blocking loops (drawBadgeSubmenu(), then this screen) -
    // there's no tick boundary the harness can script taps across without
    // risking a spin on stale touch state (delay() is a no-op in the sim,
    // see sim/fakes/Arduino.h). So this mode calls the real, unmodified
    // ledBrightnessOptionsScreen() directly and uses the SIM_FRAME hook
    // (badge_menu.cpp) to capture the frame from inside it - same drawing
    // code, same theme tokens, just reached directly instead of replayed
    // through two levels of nested blocking taps.
    s_captureLabel = "led_brightness";
    s_capturePath = outdir + "/03_options.png";
    ledBrightnessOptionsScreen();
    return 0;
  }

  std::fprintf(stderr, "usage: %s [root|repro|doublefire|fixed|options] [outdir]\n", argv[0]);
  return 1;
}
