#include "hardware/buzzer.h"

static const uint8_t BUZZER_CHANNEL = 2;  // LEDC channel (0 used by backlight)
static bool _muted = false;

// Async pattern state
static const uint16_t* _pattern = nullptr;
static uint8_t _patIdx = 0;
static uint8_t _patLen = 0;
static uint32_t _noteEnd = 0;

// Patterns: pairs of {freq_hz, duration_ms}, terminated
// freq=0 means silence gap
static const uint16_t PAT_BUTTON[]  = {2000, 30, 0, 0};
static const uint16_t PAT_SCAN_START[] = {1000, 80, 0, 30, 2000, 80, 0, 0};
static const uint16_t PAT_SCAN_STOP[]  = {2000, 80, 0, 30, 1000, 80, 0, 0};
static const uint16_t PAT_CAPTURE[]    = {1500, 60, 0, 40, 2500, 60, 0, 40, 3500, 80, 0, 0};
static const uint16_t PAT_ERROR[]      = {400, 150, 0, 80, 400, 150, 0, 0};
static const uint16_t PAT_LOW_BAT[]    = {800, 100, 0, 100, 800, 100, 0, 0};

struct PatternDef { const uint16_t* data; uint8_t len; };
static const PatternDef PATTERNS[] = {
  {PAT_BUTTON,     sizeof(PAT_BUTTON)/sizeof(uint16_t)},
  {PAT_SCAN_START, sizeof(PAT_SCAN_START)/sizeof(uint16_t)},
  {PAT_SCAN_STOP,  sizeof(PAT_SCAN_STOP)/sizeof(uint16_t)},
  {PAT_CAPTURE,    sizeof(PAT_CAPTURE)/sizeof(uint16_t)},
  {PAT_ERROR,      sizeof(PAT_ERROR)/sizeof(uint16_t)},
  {PAT_LOW_BAT,    sizeof(PAT_LOW_BAT)/sizeof(uint16_t)},
};

static void toneOn(uint16_t freq) {
  if (freq > 0)
    ledcWriteTone(BUZZER_CHANNEL, freq);
  else
    ledcWriteTone(BUZZER_CHANNEL, 0);
}

void buzzerInit() {
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWriteTone(BUZZER_CHANNEL, 0);
  Serial.println(F("[Buzzer] Initialized on GPIO 19"));
}

void buzzerPlay(BuzzerTone tone) {
  if (_muted) return;
  if (tone >= sizeof(PATTERNS)/sizeof(PATTERNS[0])) return;
  _pattern = PATTERNS[tone].data;
  _patLen  = PATTERNS[tone].len;
  _patIdx  = 0;
  // Start first note immediately
  toneOn(_pattern[0]);
  _noteEnd = millis() + _pattern[1];
  _patIdx = 2;
}

void buzzerUpdate() {
  if (!_pattern) return;
  if (millis() < _noteEnd) return;

  if (_patIdx >= _patLen || (_pattern[_patIdx] == 0 && _pattern[_patIdx+1] == 0)) {
    // Pattern done
    toneOn(0);
    _pattern = nullptr;
    return;
  }

  toneOn(_pattern[_patIdx]);
  _noteEnd = millis() + _pattern[_patIdx + 1];
  _patIdx += 2;
}

void buzzerMute(bool muted) {
  _muted = muted;
  if (muted) { toneOn(0); _pattern = nullptr; }
}

bool buzzerIsMuted() { return _muted; }
