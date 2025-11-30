// TM1640.cpp
#include "TM1640.h"

TM1640::TM1640(uint8_t gpio_sclk, 
               uint8_t gpio_din, 
               int frequency_khz)
  : _sclk_pin(gpio_sclk),
    _din_pin(gpio_din),
    _frequency_khz(frequency_khz){
  if (_frequency_khz <= 0) {
    _frequency_khz = (int)(TM1640_FCPU_HZ / 1000UL);  // Hz → kHz 換算
  }
  halfPeriod_us = 500U / static_cast<uint32_t>(_frequency_khz);
  if (halfPeriod_us < 1U) halfPeriod_us = 1U;

  pinMode(_sclk_pin, OUTPUT);
  pinMode(_din_pin, OUTPUT);

  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
  delayMicroseconds(100);
}


int TM1640::Display(bool on_off){
  static uint8_t currentDuty = 0x07;
  uint8_t cmd = 0x80u;  // ← 正しいベースは 0x80

  if (on_off) {
    cmd |= 0x08u;  // D bit
  }

  cmd |= (currentDuty & 0x07u);  // duty
  _sendStartCondi();
  _sendByte(cmd);
  _sendEndCondi();
  return TM1640_OK;
}

int TM1640::Test(bool start){
  uint8_t cmd = start ? 0x01 : 0x00;
  _sendStartCondi();
  _sendByte(cmd);
  _sendEndCondi();
  return TM1640_OK;
}


int TM1640::DrawAddrInc(const uint8_t *Bytes, uint8_t len){
  if (!Bytes || len == 0 || len > 16) return TM1640_NG;
  _sendStartCondi();
  _sendByte(TM1640_CMD_DATA_BASE);
  _sendEndCondi();
  _sendStartCondi();
  _sendByte(TM1640_CMD_ADDRESS_BASE);
  for(uint8_t i = 0; i < len; i++){
    _sendByte(Bytes[i]);
  }
  _sendEndCondi();
  
  return TM1640_OK;
}
int TM1640::DrawAddrInc(const uint8_t *Bytes, uint8_t len, uint8_t duty){
  DrawAddrInc(Bytes,len);
  _setDuty(duty);
}
int TM1640::DrawAddrFix(uint8_t addr, const uint8_t* Bytes, uint8_t len){
  if (!Bytes || len == 0) return TM1640_NG;
  if (addr > TM1640_ADDR_MAX) return TM1640_NG;
  if (_frequency_khz <= 0) return TM1640_NG;
  _sendStartCondi();
  _sendByte(TM1640_CMD_DATA_FIXED);
  _sendEndCondi();
  _sendStartCondi();
  _sendByte(TM1640_CMD_ADDRESS_BASE | (addr & 0x0F));
  for(uint8_t i = 0; i < len; i++){
    _sendByte(Bytes[i]);
  }
  _sendEndCondi();
  return TM1640_OK;
}

int TM1640::_setDuty(uint8_t duty){
  if (duty > 0x07) return TM1640_NG;
  _sendStartCondi();
  _sendByte(0x88 | (duty & 0x07));
  _sendEndCondi();
  return TM1640_OK;
}
void TM1640::_sendStartCondi(){
  digitalWrite(_din_pin, LOW);
  digitalWrite(_sclk_pin, LOW);
}

void TM1640::_sendEndCondi(){
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
}

//LSB first.
void TM1640::_sendByte(uint8_t byte){
  for (int j = 0; j < 8; j++) {  
    bool bitVal = (byte >> j) & 0x01;
    _sendBit(bitVal);
  }
}

void TM1640::_sendBit(bool bitVal){
  digitalWrite(_din_pin, bitVal ? HIGH : LOW);
  digitalWrite(_sclk_pin, HIGH);
  delayMicroseconds(halfPeriod_us);
  digitalWrite(_sclk_pin, LOW);
  delayMicroseconds(halfPeriod_us);
}

