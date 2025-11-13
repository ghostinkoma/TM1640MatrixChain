// TM1640.cpp
#include "TM1640.h"

TM1640::TM1640(uint8_t gpio_sclk, 
               uint8_t gpio_din, 
               int frequency_khz)
  : _sclk_pin(gpio_sclk),
    _din_pin(gpio_din),
    _frequency_khz(frequency_khz){
  // frequency_khz が 0 または負値ならデフォルト値（CPUクロック由来）を使用
  if (_frequency_khz <= 0) {
    _frequency_khz = (int)(TM1640_FCPU_HZ / 1000UL);  // Hz → kHz 換算
  }

  pinMode(_sclk_pin, OUTPUT);
  pinMode(_din_pin, OUTPUT);

  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
}


int TM1640::Display(bool on) {
  (void)on;
  return TM1640_OK;
}

int TM1640::Test(bool start) {
  (void)start;
  return TM1640_OK;
}

int TM1640::SetDuty(uint8_t duty) {
  (void)duty;
  return TM1640_OK;
}

int TM1640::DrawAddrInc(const uint8_t *chars, uint16_t len) {
  (void)chars;
  (void)len;
  return TM1640_OK;
}

int TM1640::DrawAddrFix(uint8_t addr, const uint8_t *chars, uint16_t len) {
  (void)addr;
  (void)chars;
  (void)len;
  return TM1640_OK;
}

int TM1640::_sendChars(
  char * sendChars,
  bool addStopBitAfter1stChar,
  bool addStopBitBeforLastChar,
  int charLength){
  if (!sendChars || charLength <= 0)
    return TM1640_NG;

  // 周波数指定があれば遅延時間を算出（1周期 ≈ 1 / freq[kHz] [µs]）
  uint32_t period_us = (_frequency_khz > 0) ? (1000U / _frequency_khz) : 10U;
  if (period_us < 1U) period_us = 1U;
  uint32_t half = period_us / 2;

  // --- Start Condition (データシート準拠) ---
  digitalWrite(_din_pin, HIGH);
  digitalWrite(_sclk_pin, HIGH);
  delayMicroseconds(half);

  digitalWrite(_din_pin, LOW);  // CLK=HIGH中にDINをLOWへ
  delayMicroseconds(half);
  digitalWrite(_sclk_pin, LOW);
  delayMicroseconds(half);

  // --- Data Transmission ---
  for (int i = 0; i < charLength; ++i) {
    uint8_t data = (uint8_t)sendChars[i];

    for (int bit = 0; bit < 8; ++bit) {
      bool bitVal = (data >> bit) & 0x01;  // LSB first

      digitalWrite(_sclk_pin, LOW);                 // CLK low
      digitalWrite(_din_pin, bitVal ? HIGH : LOW);  // set data
      delayMicroseconds(half);

      digitalWrite(_sclk_pin, HIGH);                // CLK rising → latch
      delayMicroseconds(half);
    }

    // --- Stop Bit Handling (制御引数に基づく) ---
    if (addStopBitAfter1stChar && i == 0) {
      digitalWrite(_sclk_pin, LOW);
      delayMicroseconds(half);
      digitalWrite(_din_pin, HIGH);
      delayMicroseconds(half);
      digitalWrite(_sclk_pin, HIGH);
      delayMicroseconds(half);
    }

    if (addStopBitBeforLastChar && i == (charLength - 1)) {
      digitalWrite(_sclk_pin, LOW);
      delayMicroseconds(half);
      digitalWrite(_din_pin, HIGH);
      delayMicroseconds(half);
      digitalWrite(_sclk_pin, HIGH);
      delayMicroseconds(half);
    }
  }

  // --- Stop Condition (通信終了) ---
  digitalWrite(_sclk_pin, LOW);
  delayMicroseconds(half);
  digitalWrite(_din_pin, HIGH);
  delayMicroseconds(half);
  digitalWrite(_sclk_pin, HIGH);
  delayMicroseconds(half);

  return TM1640_OK;
}

