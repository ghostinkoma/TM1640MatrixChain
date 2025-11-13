#include <Arduino.h>
#include "TM1640.h"

// === 定数定義（TM1640 データシート準拠） ===
#define SEG_MAX   8
#define GRID_MAX  16

// === ピン設定（必要に応じて変更） ===
#define PIN_SCLK  4
#define PIN_DIN   5
#define TM_FREQ_KHZ 100

// === グローバルオブジェクト ===
TM1640 LED_Matrix_TM1640(PIN_SCLK, PIN_DIN, TM_FREQ_KHZ);

// === デフォルト輝度 ===
#define TM1640_DEFAULT_DUTY TM1640_DUTY_10_16

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("TM1640 Segment Drive & Brightness Test Start"));

  // ディスプレイ初期化
  LED_Matrix_TM1640.Display(true);
  LED_Matrix_TM1640.SetDuty(TM1640_DEFAULT_DUTY);

  // テストモードワンショット
  LED_Matrix_TM1640.Test(true);
  delay(400);
  LED_Matrix_TM1640.Test(false);
  delay(400);
}

void loop() {
  uint8_t buf[GRID_MAX];

  // === 各セグメントを順に点灯 ===
  for (uint8_t seg = 1; seg <= SEG_MAX; ++seg) {
    Serial.print(F("Driving SEG "));
    Serial.println(seg);

    for (uint8_t grid = 1; grid <= GRID_MAX; ++grid) {
      memset(buf, 0x00, sizeof(buf));
      buf[grid - 1] = (1 << (seg - 1)); // grid位置にsegビットを立てる
      LED_Matrix_TM1640.DrawAddrFix(0x00, buf, GRID_MAX);
      delay(100);
    }
  }

  // === 全点灯 ===
  Serial.println(F("All ON"));
  for (uint8_t i = 0; i < GRID_MAX; ++i) buf[i] = 0xFF;
  LED_Matrix_TM1640.DrawAddrFix(0x00, buf, GRID_MAX);
  delay(500);

  // === Duty Up: 初期値 → 最大 ===
  Serial.println(F("Increasing brightness..."));
  for (uint8_t duty = TM1640_DEFAULT_DUTY; duty <= TM1640_DUTY_14_16; ++duty) {
    LED_Matrix_TM1640.SetDuty(duty);
    delay(200);
  }

  // === Duty Down: 最大 → 最小（1/16） ===
  Serial.println(F("Decreasing brightness..."));
  for (int duty = TM1640_DUTY_14_16; duty >= TM1640_DUTY_1_16; --duty) {
    LED_Matrix_TM1640.SetDuty((uint8_t)duty);
    delay(200);
  }

  // === Display OFF ===
  Serial.println(F("Display OFF"));
  LED_Matrix_TM1640.Display(false);
  delay(800);

  // === Display ON & Duty初期値へ復帰 ===
  Serial.println(F("Restoring display..."));
  LED_Matrix_TM1640.Display(true);
  LED_Matrix_TM1640.SetDuty(TM1640_DEFAULT_DUTY);

  Serial.println(F("Cycle complete.\n"));
  delay(1000);
}
