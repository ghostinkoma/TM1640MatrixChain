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

  return TM1640_OK;
}