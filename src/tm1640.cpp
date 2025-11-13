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
　//ここに周波数チェックいれる0ならエラーで処理終了
  if (!sendChars || charLength <= 0)
    return TM1640_NG;

  // --- Start Condition (データシート準拠) ---
  //このクラスでスタートコンディションはいらない。むしろ処理終了でハイになる担保を毎度1msはレスポンス低下

  // --- Data Transmission ---
  for (int i = 0; i < charLength; ++i) {
    uint8_t data = (uint8_t)sendChars[i];
    bool addStopAfter = (addStopBitAfter1stChar && i == 0);
    bool addStopBefore = (addStopBitBeforLastChar && i == (charLength - 1));

    _sendChar(data, addStopAfter, addStopBefore, half);
  }

  // --- Stop Condition (通信終了) ---
  //ここはsclk とdinをハイのみつまり定評のありarduino のライブラリでしっかりhiになればいい

  return TM1640_OK;
}



void TM1640::_sendBit(bool bitVal)
{
  // 1ビット送信 — データシート準拠
  digitalWrite(_din_pin, bitVal ? HIGH : LOW);

  digitalWrite(_sclk_pin, LOW);

//このウエイト周波数がこうりょされていない。
  delayMicroseconds(1);
  digitalWrite(_sclk_pin, HIGH);

  // 周波数設定を反映（上位で保証済み）
  uint32_t delay_us = 1000 / static_cast<uint32_t>(_frequency_khz);
  delayMicroseconds(delay_us);
}






