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

  if (_frequency_khz <= 0)
    return TM1640_NG;

  for (int i = 0; i < charLength; ++i) {
    uint8_t data = (uint8_t)sendChars[i];
    bool addStopAfter = (addStopBitAfter1stChar && i == 0);
    bool addStopBefore = (addStopBitBeforLastChar && i == (charLength - 1));

    _sendChar(data, addStopAfter, addStopBefore);
  }

  // 通信終了時、ラインを安定化
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);

  return TM1640_OK;
}

void TM1640::_sendBit(bool bitVal){
  digitalWrite(_din_pin, bitVal ? HIGH : LOW);

  // 周波数に基づいた1周期の半分のディレイを設定
  if (_frequency_khz <= 0) return;  // 周波数が未設定なら即終了（上位が責任を持つ）

  uint32_t halfPeriod_us = 500U / static_cast<uint32_t>(_frequency_khz);
  if (halfPeriod_us < 1U) halfPeriod_us = 1U;

  // SCLK Low → High トグルで1bitラッチ
  digitalWrite(_sclk_pin, LOW);
  delayMicroseconds(halfPeriod_us);
  digitalWrite(_sclk_pin, HIGH);
  delayMicroseconds(halfPeriod_us);
}







