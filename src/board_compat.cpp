#include "board_compat.h"

#ifdef BUDDY_BOARD_BBCLAW

#include <esp_sleep.h>
#include <math.h>
#include <time.h>

namespace {
constexpr uint32_t kButtonDebounceMs = 25;
constexpr int kBacklightChannel = 0;
constexpr int kBacklightFreq = 5000;
constexpr int kBacklightResolutionBits = 8;
constexpr bool kBacklightActiveLow = (bool)BUDDY_BACKLIGHT_ACTIVE_LOW;

static void rawBacklight(bool on) {
  pinMode(BUDDY_BACKLIGHT_GPIO, OUTPUT);
  digitalWrite(BUDDY_BACKLIGHT_GPIO, on ? HIGH : LOW);
}
}

BoardButton::BoardButton()
    : _pin(-1),
      _activeLevel(BUDDY_BTN_ACTIVE_LEVEL),
      _pullup(true),
      _stablePressed(false),
      _lastRawPressed(false),
      _wasPressed(false),
      _wasReleased(false),
      _lastBounceMs(0),
      _pressedAtMs(0) {}

void BoardButton::begin(int pin, int activeLevel, bool pullup) {
  _pin = pin;
  _activeLevel = activeLevel;
  _pullup = pullup;
  pinMode(_pin, _pullup ? INPUT_PULLUP : INPUT);
  bool rawPressed = digitalRead(_pin) == _activeLevel;
  _stablePressed = rawPressed;
  _lastRawPressed = rawPressed;
  _wasPressed = false;
  _wasReleased = false;
  _lastBounceMs = millis();
  _pressedAtMs = rawPressed ? millis() : 0;
  if (_pin >= 0) {
    Serial.printf("[btn] pin=%d active=%s pullup=%d init=%d\n", _pin, _activeLevel == LOW ? "LOW" : "HIGH", _pullup, rawPressed);
  }
}

void BoardButton::update() {
  if (_pin < 0) return;
  _wasPressed = false;
  _wasReleased = false;
  bool rawPressed = digitalRead(_pin) == _activeLevel;
  uint32_t now = millis();
  if (rawPressed != _lastRawPressed) {
    _lastRawPressed = rawPressed;
    _lastBounceMs = now;
  }
  if ((now - _lastBounceMs) < kButtonDebounceMs || rawPressed == _stablePressed) return;

  _stablePressed = rawPressed;
  if (_stablePressed) {
    _pressedAtMs = now;
    _wasPressed = true;
  } else {
    _wasReleased = true;
  }
  if (_wasPressed || _wasReleased) {
    Serial.printf("[btn] pin=%d pressed=%d released=%d raw=%d\n", _pin, _wasPressed, _wasReleased, rawPressed);
  }
}

bool BoardButton::isPressed() const { return _stablePressed; }
bool BoardButton::wasPressed() const { return _wasPressed; }
bool BoardButton::wasReleased() const { return _wasReleased; }

bool BoardButton::pressedFor(uint32_t ms) const {
  return _stablePressed && (millis() - _pressedAtMs) >= ms;
}

void BoardImu::Init() {}

void BoardImu::getAccelData(float* ax, float* ay, float* az) {
  if (ax) *ax = 0.0f;
  if (ay) *ay = 0.0f;
  if (az) *az = 1.0f;
}

void BoardBeep::begin() {}
void BoardBeep::update() {}
void BoardBeep::tone(uint16_t, uint16_t) {}

BoardRtc::BoardRtc() : _baseEpoch(946684800), _baseMillis(0), _hasTime(false) {}

void BoardRtc::syncBase(const struct tm& parts) {
  struct tm copy = parts;
  time_t epoch = mktime(&copy);
  if (epoch < 0) epoch = 946684800;
  _baseEpoch = epoch;
  _baseMillis = millis();
  _hasTime = true;
}

void BoardRtc::current(struct tm* parts) const {
  time_t epoch = _baseEpoch;
  if (_hasTime) epoch += (time_t)((millis() - _baseMillis) / 1000);
  gmtime_r(&epoch, parts);
}

void BoardRtc::SetTime(const RTC_TimeTypeDef* tm) {
  struct tm parts = {};
  current(&parts);
  parts.tm_hour = tm->Hours;
  parts.tm_min = tm->Minutes;
  parts.tm_sec = tm->Seconds;
  syncBase(parts);
}

void BoardRtc::SetDate(const RTC_DateTypeDef* dt) {
  struct tm parts = {};
  current(&parts);
  parts.tm_year = (int)dt->Year - 1900;
  parts.tm_mon = (int)dt->Month - 1;
  parts.tm_mday = dt->Date;
  syncBase(parts);
}

void BoardRtc::GetTime(RTC_TimeTypeDef* tm) const {
  struct tm parts = {};
  current(&parts);
  tm->Hours = (uint8_t)parts.tm_hour;
  tm->Minutes = (uint8_t)parts.tm_min;
  tm->Seconds = (uint8_t)parts.tm_sec;
}

void BoardRtc::GetDate(RTC_DateTypeDef* dt) const {
  struct tm parts = {};
  current(&parts);
  dt->WeekDay = (uint8_t)parts.tm_wday;
  dt->Month = (uint8_t)(parts.tm_mon + 1);
  dt->Date = (uint8_t)parts.tm_mday;
  dt->Year = (uint16_t)(parts.tm_year + 1900);
}

void BoardAxp::begin() {
  _backlightOn = true;
  _brightnessPct = 100;
  ledcSetup(kBacklightChannel, kBacklightFreq, kBacklightResolutionBits);
  ledcAttachPin(BUDDY_BACKLIGHT_GPIO, kBacklightChannel);
  applyBacklight();
}

void BoardAxp::applyBacklight() const {
  uint32_t duty = 0;
  if (_backlightOn && _brightnessPct > 0) {
    duty = ((1U << kBacklightResolutionBits) - 1U) * _brightnessPct / 100U;
  }
  if (kBacklightActiveLow) {
    duty = ((1U << kBacklightResolutionBits) - 1U) - duty;
  }
  ledcWrite(kBacklightChannel, duty);
}

void BoardAxp::ScreenBreath(uint8_t levelPct) {
  _brightnessPct = constrain(levelPct, 0, 100);
  applyBacklight();
}

void BoardAxp::SetLDO2(bool on) {
  _backlightOn = on;
  applyBacklight();
}

float BoardAxp::GetBatVoltage() const {
#if BUDDY_BATTERY_ADC_GPIO >= 0
  uint32_t mv = analogReadMilliVolts(BUDDY_BATTERY_ADC_GPIO);
  return (float)(mv * 2) / 1000.0f;
#else
  return 0.0f;
#endif
}

float BoardAxp::GetBatCurrent() const { return 0.0f; }
float BoardAxp::GetVBusVoltage() const { return 0.0f; }
float BoardAxp::GetTempInAXP192() const { return 0.0f; }
uint8_t BoardAxp::GetBtnPress() const { return 0; }

void BoardAxp::PowerOff() const {
  esp_deep_sleep_start();
}

#if BUDDY_ENCODER_ENABLE
static constexpr int8_t _encTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void BoardEncoder::begin(int pinA, int pinB) {
  _pinA = pinA; _pinB = pinB;
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  _prevState = (digitalRead(_pinA) << 1) | digitalRead(_pinB);
  _accumSteps = 0;
  Serial.printf("[enc] init A=%d B=%d state=%d\n", _pinA, _pinB, _prevState);
}

void BoardEncoder::update() {
  uint8_t s = (digitalRead(_pinA) << 1) | digitalRead(_pinB);
  int8_t delta = _encTable[(_prevState << 2) | s];
  if (s != _prevState) {
    // Only count transitions AWAY from center (state 3 = both pins HIGH/idle).
    // Transitions returning TO center (prev=1/2, cur=3) are filtered out.
    // This gives exactly one tick per encoder detent.
    if (_prevState == 3 && s != 3) {
      _accumSteps += delta;
      if (s != _prevState) {
        Serial.printf("[enc] tick prev=%d cur=%d delta=%d accum=%d\n", _prevState, s, delta, _accumSteps);
      }
    }
  }
  _prevState = s;
}

int8_t BoardEncoder::consumeSteps() {
  int8_t s = _accumSteps;
  if (s != 0) Serial.printf("[enc] consume=%d\n", s);
  _accumSteps = 0;
  return s;
}
#endif

BoardCompat::BoardCompat() : Lcd() {}

void BoardCompat::begin() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[board] begin");
#if BUDDY_BATTERY_ADC_GPIO >= 0
  analogReadResolution(12);
  analogSetPinAttenuation(BUDDY_BATTERY_ADC_GPIO, ADC_11db);
#endif
  Serial.println("[board] axp");
  Axp.begin();
  Serial.println("[board] tft init");
  Lcd.init();
  Serial.println("[board] tft rotation");
  Lcd.setRotation(0);
  Serial.println("[board] backlight on");
  Axp.SetLDO2(true);
  delay(800);
  Serial.println("[board] fill red");
  Lcd.fillScreen(TFT_RED);
  delay(800);
  Serial.println("[board] fill green");
  Lcd.fillScreen(TFT_GREEN);
  delay(800);
  Serial.println("[board] backlight off");
  Axp.SetLDO2(false);
  delay(800);
  Serial.println("[board] backlight on");
  Axp.SetLDO2(true);
  delay(800);
  Serial.println("[board] fill blue");
  Lcd.fillScreen(TFT_BLUE);
  delay(800);
  Serial.println("[board] fill black");
  Lcd.fillScreen(TFT_BLACK);
  Serial.println("[board] buttons");
  BtnA.begin(BUDDY_BTN_A_GPIO, BUDDY_BTN_A_ACTIVE, (bool)BUDDY_BTN_A_PULLUP);
  BtnB.begin(BUDDY_BTN_B_GPIO, BUDDY_BTN_B_ACTIVE, (bool)BUDDY_BTN_B_PULLUP);
#if BUDDY_ENCODER_ENABLE
  Encoder.begin(BUDDY_ENC_A_GPIO, BUDDY_ENC_B_GPIO);
  EncBtn.begin(BUDDY_ENC_KEY_GPIO, BUDDY_ENC_KEY_ACTIVE, (bool)BUDDY_ENC_KEY_PULLUP);
  Serial.println("[board] encoder");
#endif
  Serial.println("[board] ready");
}

void BoardCompat::update() {
  BtnA.update();
  BtnB.update();
#if BUDDY_ENCODER_ENABLE
  Encoder.update();
  EncBtn.update();
#endif
}

BoardCompat M5;

#endif
