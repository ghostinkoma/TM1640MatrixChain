#include "tm1640.h"
#include <Arduino.h>
int frequency ;

TM1640::TM1640(uint8_t gpio_sclk, uint8_t gpio_din, int frequency_khz)
  : _sclk_pin(gpio_sclk), _din_pin(gpio_din), _frequency_khz(frequency_khz){
  pinMode(_sclk_pin, INPUT_PULLUP);
  pinMode(_din_pin,  INPUT_PULLUP);
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
  frequency = frequency_khz;
}

/* private helpers */
void TM1640::_pinSet(uint8_t /*pin*/, bool /*level*/) { 
}
void TM1640::_delayForBitTiming(void) { 
}
int TM1640::_sendRawByte(uint8_t /*b*/) {

}

/* public API */
int TM1640::Display(bool /*on*/) {
}
int TM1640::Test(bool /*start*/) {
}
int TM1640::SetDuty(uint8_t duty){
}
int TM1640::DrawAddrInc(const uint8_t * /*chars*/, uint16_t /*len*/) {
}
int TM1640::DrawAddrFix(uint8_t /*addr*/, const uint8_t * /*chars*/, uint16_t /*len*/) {}