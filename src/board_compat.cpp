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
constexpr bool kBacklightActiveLow = false;

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
}

void BoardButton::update() {
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
  Serial.println("[board] raw backlight high");
  rawBacklight(true);
  delay(800);
  Serial.println("[board] fill red");
  Lcd.fillScreen(TFT_RED);
  delay(800);
  Serial.println("[board] fill green");
  Lcd.fillScreen(TFT_GREEN);
  delay(800);
  Serial.println("[board] raw backlight low");
  rawBacklight(false);
  delay(800);
  Serial.println("[board] raw backlight high");
  rawBacklight(true);
  delay(800);
  Serial.println("[board] fill blue");
  Lcd.fillScreen(TFT_BLUE);
  delay(800);
  Serial.println("[board] fill black");
  Lcd.fillScreen(TFT_BLACK);
  Serial.println("[board] buttons");
  BtnA.begin(BUDDY_BTN_A_GPIO, BUDDY_BTN_ACTIVE_LEVEL, true);
  BtnB.begin(BUDDY_BTN_B_GPIO, BUDDY_BTN_ACTIVE_LEVEL, true);
  Serial.println("[board] ready");
}

void BoardCompat::update() {
  BtnA.update();
  BtnB.update();
}

BoardCompat M5;

#endif
