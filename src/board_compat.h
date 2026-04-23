#pragma once

#ifdef BUDDY_BOARD_BBCLAW

#include <Arduino.h>
#include <TFT_eSPI.h>

#ifndef GREEN
#define GREEN TFT_GREEN
#endif

#ifndef RED
#define RED TFT_RED
#endif

#ifndef BUDDY_BTN_A_GPIO
#define BUDDY_BTN_A_GPIO 7
#endif

#ifndef BUDDY_BTN_B_GPIO
#define BUDDY_BTN_B_GPIO 41
#endif

#ifndef BUDDY_BTN_ACTIVE_LEVEL
#define BUDDY_BTN_ACTIVE_LEVEL HIGH
#endif

#ifndef BUDDY_BTN_A_ACTIVE
#define BUDDY_BTN_A_ACTIVE HIGH
#endif

#ifndef BUDDY_BTN_A_PULLUP
#define BUDDY_BTN_A_PULLUP 0
#endif

#ifndef BUDDY_BTN_B_ACTIVE
#define BUDDY_BTN_B_ACTIVE LOW
#endif

#ifndef BUDDY_BTN_B_PULLUP
#define BUDDY_BTN_B_PULLUP 1
#endif

#ifndef BUDDY_BACKLIGHT_GPIO
#define BUDDY_BACKLIGHT_GPIO 14
#endif

#ifndef BUDDY_BACKLIGHT_ACTIVE_LOW
#define BUDDY_BACKLIGHT_ACTIVE_LOW 0
#endif

#ifndef BUDDY_BATTERY_ADC_GPIO
#define BUDDY_BATTERY_ADC_GPIO 3
#endif

typedef struct {
  uint8_t Hours;
  uint8_t Minutes;
  uint8_t Seconds;
} RTC_TimeTypeDef;

typedef struct {
  uint8_t WeekDay;
  uint8_t Month;
  uint8_t Date;
  uint16_t Year;
} RTC_DateTypeDef;

class BoardButton {
 public:
  BoardButton();
  void begin(int pin, int activeLevel, bool pullup);
  void update();
  bool isPressed() const;
  bool wasPressed() const;
  bool wasReleased() const;
  bool pressedFor(uint32_t ms) const;

 private:
  int _pin;
  int _activeLevel;
  bool _pullup;
  bool _stablePressed;
  bool _lastRawPressed;
  bool _wasPressed;
  bool _wasReleased;
  uint32_t _lastBounceMs;
  uint32_t _pressedAtMs;
};

class BoardImu {
 public:
  void Init();
  void getAccelData(float* ax, float* ay, float* az);
};

class BoardBeep {
 public:
  void begin();
  void update();
  void tone(uint16_t freq, uint16_t dur);
};

class BoardRtc {
 public:
  BoardRtc();
  void SetTime(const RTC_TimeTypeDef* tm);
  void SetDate(const RTC_DateTypeDef* dt);
  void GetTime(RTC_TimeTypeDef* tm) const;
  void GetDate(RTC_DateTypeDef* dt) const;

 private:
  void syncBase(const struct tm& parts);
  void current(struct tm* parts) const;

  time_t _baseEpoch;
  uint32_t _baseMillis;
  bool _hasTime;
};

class BoardAxp {
 public:
  void begin();
  void ScreenBreath(uint8_t levelPct);
  void SetLDO2(bool on);
  float GetBatVoltage() const;
  float GetBatCurrent() const;
  float GetVBusVoltage() const;
  float GetTempInAXP192() const;
  uint8_t GetBtnPress() const;
  void PowerOff() const;

 private:
  void applyBacklight() const;

  bool _backlightOn;
  uint8_t _brightnessPct;
};

class BoardCompat {
 public:
  TFT_eSPI Lcd;
  BoardButton BtnA;
  BoardButton BtnB;
  BoardImu Imu;
  BoardBeep Beep;
  BoardRtc Rtc;
  BoardAxp Axp;

  BoardCompat();
  void begin();
  void update();
};

extern BoardCompat M5;

#else

#include <M5StickCPlus.h>

#endif
