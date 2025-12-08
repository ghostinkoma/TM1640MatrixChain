/*
  TM1640MatrixChain – High-Speed TM1640 40×8 Bi-color Matrix Driver
  Copyright (c) 2025 ghostinkoma (ghostinkoma@gmail.com)

  This work is licensed under the Creative Commons 
  Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
  
  https://creativecommons.org/licenses/by-nc-sa/4.0/
  
  You are free to share and adapt this work for non-commercially,
  provided you give appropriate credit and distribute any derivative works
  under the same license.
  
  For commercial licensing, please contact: ghostinkoma@gmail.com
*/


#include "TM1640.h"

TM1640::TM1640(uint8_t gpio_sclk, uint8_t gpio_din)
  : _sclk_pin(gpio_sclk),
    _din_pin(gpio_din)
{
  pinMode(_sclk_pin, OUTPUT);
  pinMode(_din_pin, OUTPUT);

  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
  delayMicroseconds(100);

#if defined(__AVR_ATmega328P__) || defined(__LGT8F__)
  // ピン番号からポートのアドレスとビットマスクを作成
  _sclk_port = portOutputRegister(digitalPinToPort(_sclk_pin));
  _din_port  = portOutputRegister(digitalPinToPort(_din_pin));
  _sclk_bm   = digitalPinToBitMask(_sclk_pin);
  _din_bm    = digitalPinToBitMask(_din_pin);
#endif
}

int TM1640::SetDuty(uint8_t Duty){
  if (Duty > 7) return TM1640_NG;
  _sendStartCondi();
  _sendByte(TM1640_CMD_DATA_FIXED);
  _sendEndCondi();
  _sendStartCondi();
  _sendByte(TM1640_CMD_DISPLAY_BASE | Duty);
  _sendEndCondi();
  _lastDuty = Duty;
  return TM1640_OK;
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
#if defined(__AVR_ATmega328P__) || defined(__LGT8F__)
  *_din_port &= ~_din_bm;   // LOW
  *_sclk_port &= ~_sclk_bm; // LOW
#else
  digitalWrite(_din_pin, LOW);
  digitalWrite(_sclk_pin, LOW);
#endif
}

void TM1640::_sendEndCondi(){
#if defined(__AVR_ATmega328P__) || defined(__LGT8F__)
  *_sclk_port |= _sclk_bm;  // HIGH
  *_din_port  |= _din_bm;   // HIGH
#else
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
#endif
}

void TM1640::_sendByte(uint8_t byte){
  for (int j = 0; j < 8; j++) {
    bool bitVal = (byte >> j) & 0x01;
    _sendBit(bitVal);
  }
}

void TM1640::_sendBit(bool bitVal){
#if defined(__AVR_ATmega328P__) || defined(__LGT8F__)

  if (bitVal)
    *_din_port |= _din_bm;      // HIGH
  else
    *_din_port &= ~_din_bm;     // LOW

  *_sclk_port |=  _sclk_bm;     // HIGH
  *_sclk_port &= ~_sclk_bm;     // LOW

#else
  digitalWrite(_din_pin, bitVal ? HIGH : LOW);
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_sclk_pin, LOW);
#endif
}
