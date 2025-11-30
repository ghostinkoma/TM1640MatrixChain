// TM1640.cpp
#include "TM1640.h"

TM1640::TM1640(uint8_t gpio_sclk, 
               uint8_t gpio_din)
  : _sclk_pin(gpio_sclk),
    _din_pin(gpio_din){
 
  pinMode(_sclk_pin, OUTPUT);
  pinMode(_din_pin, OUTPUT);

  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
  delayMicroseconds(100);
}

int TM1640::SetDuty(uint8_t Duty){
  if ( Duty > 7) return TM1640_NG;
  _sendStartCondi();
  _sendByte(TM1640_CMD_DATA_FIXED);
  _sendEndCondi();
  _sendStartCondi();
  _sendByte(TM1640_CMD_DISPLAY_BASE | Duty);
  _sendEndCondi();
  _lastDuty = Duty;
}

int TM1640::DrawAddrInc(const uint8_t *Bytes, uint8_t len){
  if (!Bytes || len == 0 || len > 16) return TM1640_NG;
  _sendStartCondi();
  _sendByte(TM1640_CMD_DATA_FIXED);
  _sendEndCondi();
  _sendStartCondi();
  _sendByte(TM1640_CMD_ADDRESS_BASE);
  for(uint8_t i=0; i<len;i++){
    _sendByte(Bytes[i]);
  }
  _sendEndCondi();
  return TM1640_OK;
}

void TM1640::_sendStartCondi(){
  digitalWrite(_din_pin, LOW);
  //delayMicroseconds(halfPeriod_us);
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
  digitalWrite(_sclk_pin, LOW);
}

