#ifndef TM1640_H
#define TM1640_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TM1640_OK   0
#define TM1640_NG  -1

#define TM1640_CMD_DATA_FIXED    0x40
#define TM1640_CMD_ADDRESS_BASE  0xC0
#define TM1640_CMD_DISPLAY_BASE  0x88

#define TM1640_DUTY_1_16   0x00
#define TM1640_DUTY_2_16   0x01
#define TM1640_DUTY_4_16   0x02
#define TM1640_DUTY_10_16  0x03
#define TM1640_DUTY_11_16  0x04
#define TM1640_DUTY_12_16  0x05
#define TM1640_DUTY_13_16  0x06
#define TM1640_DUTY_14_16  0x07

class TM1640 {
public:
  TM1640(uint8_t gpio_sclk, uint8_t gpio_din);
  int SetDuty(uint8_t duty);
  int DrawAddrInc(const uint8_t *Bytes, uint8_t len);

private:
  uint8_t _sclk_pin;
  uint8_t _din_pin;
  uint32_t halfPeriod_us;
  uint8_t _lastDuty = 0x07;

  // --- 追加（高速 I/O 用） ---
#if defined(__AVR_ATmega328P__) || defined(__LGT8F__)
  volatile uint8_t* _sclk_port;
  volatile uint8_t* _din_port;
  uint8_t _sclk_bm;
  uint8_t _din_bm;
#endif

  void _sendByte(uint8_t byte);
  void _sendBit(bool bitVal);
  void _sendStartCondi();
  void _sendEndCondi();
};

#endif /* TM1640_H */
