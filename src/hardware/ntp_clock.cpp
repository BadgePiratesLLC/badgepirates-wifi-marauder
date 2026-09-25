#include "hardware/ntp_clock.h"
#include <time.h>

// America/Chicago - where these badges actually ship (BSidesKC) and
// Kevin's own timezone. POSIX TZ rule, DST-aware. Named here, not
// inline, so a future badge for a different con is a one-line change.
static const char* NTP_TZ = "CST6CDT,M3.2.0,M11.1.0/2";
static const char* NTP_SERVER = "pool.ntp.org";

// A synced ESP32 clock reads well past this (2024-01-01 00:00 UTC); the
// unsynced default is epoch 0 (1970-01-01). Anything before this line is
// "never synced," not "an unlikely-but-real 1970 timestamp."
static const time_t NTP_SANITY_EPOCH = 1704067200;

static bool s_requested = false;
static bool s_wasAssociated = false;

void ntpClockUpdate(bool wifiAssociated, uint32_t /*currentTime*/) {
  if (wifiAssociated && !s_wasAssociated) {
    // Just associated - fire ONE non-blocking sync attempt. configTzTime()
    // starts the SNTP client and returns immediately; it does not wait
    // for a reply, so this is safe to call from the same loop() that
    // also has to keep servicing touch/encoder input.
    configTzTime(NTP_TZ, NTP_SERVER);
    s_requested = true;
  }
  s_wasAssociated = wifiAssociated;
}

bool ntpIsSynced() {
  if (!s_requested) return false;
  return time(nullptr) >= NTP_SANITY_EPOCH;
}

void ntpGetTimeString(char* buf, size_t bufLen) {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  snprintf(buf, bufLen, "%02d:%02d", t.tm_hour, t.tm_min);
}
