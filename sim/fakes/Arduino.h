#pragma once
// BSidesKC Badge simulator (Nexus 0f35e128) — fake Arduino.h.
//
// Stands in for the real Arduino core so our own UI files (badge_menu.cpp,
// touch_input.cpp, badge_nav.cpp, badge_ui_theme.h) compile natively on the
// host, unmodified. Only the subset those files actually use is provided —
// this is not a general Arduino-core replacement.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <functional>

// ---- millis()/delay(): sim clock is advanced explicitly by the harness,
// not by wall-clock time — a scripted tap sequence must be reproducible. ----
uint32_t millis();
void sim_advance_millis(uint32_t ms);
inline void delay(uint32_t) { /* no-op: sim steps time explicitly */ }

// ---- GPIO stand-ins (badge_menu.cpp's HAS_BUTTONS branch reads these; the
// sim doesn't define HAS_BUTTONS so these are never actually called, but the
// declarations keep any stray reference compiling). ----
#define OUTPUT 1
#define INPUT 0
#define LOW 0
#define HIGH 1
inline int digitalRead(int) { return HIGH; }
inline void digitalWrite(int, int) {}
inline void pinMode(int, int) {}

#define F(x) x
#define PROGMEM

// Templates, not macros: a macro named min/max poisons every later standard
// header that declares its own min()/max() (e.g. libstdc++'s <limits>,
// pulled in transitively by <cmath> from TFT_eSPI.h) by text-substituting
// inside their declarations. Worked by luck on macOS/libc++, hard-failed on
// Linux/libstdc++ in CI - see Nexus 0f35e128. Arduino's own newer cores
// make the same fix for the same reason.
template <typename A, typename B>
inline auto min(A a, B b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <typename A, typename B>
inline auto max(A a, B b) -> decltype(a > b ? a : b) { return a > b ? a : b; }

// ---- Arduino String, minimal subset (construct from const char*/int,
// c_str(), equals(), length(), toCharArray(), operator+, operator==). ----
class String {
public:
  String() {}
  String(const char* s) : _s(s ? s : "") {}
  String(int v) : _s(std::to_string(v)) {}
  String(int8_t v) : _s(std::to_string((int)v)) {}
  String(unsigned int v) : _s(std::to_string(v)) {}

  const char* c_str() const { return _s.c_str(); }
  unsigned int length() const { return (unsigned int)_s.size(); }
  bool equals(const char* other) const { return _s == other; }
  bool operator==(const char* other) const { return _s == other; }
  bool operator!=(const char* other) const { return _s != other; }
  void toCharArray(char* buf, unsigned int size) const {
    std::snprintf(buf, size, "%s", _s.c_str());
  }
  String operator+(const char* rhs) const { return String((_s + rhs).c_str()); }
  String operator+(const String& rhs) const { return String((_s + rhs._s).c_str()); }

private:
  std::string _s;
};

class Print {
public:
  void println(const char* s) { std::printf("%s\n", s); }
  void println() { std::printf("\n"); }
  void print(const char* s) { std::printf("%s", s); }
  void printf(const char* fmt, ...) {}
  void flush() {}
  void begin(unsigned long) {}
  void setTxTimeoutMs(unsigned long) {}
};
extern Print Serial;
