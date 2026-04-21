#include <Arduino.h>
#include <TFT_eSPI.h>

#define BL_PIN 14

TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== TFT TEST START ===");

  // 直接 GPIO 拉高背光，排除 PWM/ledcSetup 问题
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);
  Serial.println("backlight: GPIO HIGH");

  tft.init();
  tft.setRotation(0);
  Serial.println("tft: init done");

  tft.fillScreen(TFT_RED);
  Serial.println("fill RED");
  delay(1000);

  tft.fillScreen(TFT_GREEN);
  Serial.println("fill GREEN");
  delay(1000);

  tft.fillScreen(TFT_BLUE);
  Serial.println("fill BLUE");
  delay(1000);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.print("TFT OK!");
  Serial.println("=== DONE ===");
}

void loop() {
  delay(1000);
}
