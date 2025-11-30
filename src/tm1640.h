#ifndef TM1640_H
#define TM1640_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define TM1640_OK   0
#define TM1640_NG  -1

/* --- Command bases and masks (datasheet) --- */
/* NOTE: datasheet uses: Data command base 0x40 (auto), Addr base 0xC0, Display base 0x80/0x88 variants, Test base 0x80 in some docs.
   Many references and libraries use:
     - Data command (write mode) base: 0x40 (auto-increment) / 0x44 (fixed)
     - Address command base: 0xC0 + address
     - Display control command base: 0x80 | 0x08 (display on) | duty(3bits)
   See datasheet for exact phrasing. */
#define TM1640_CMD_DATA_BASE     0x40
#define TM1640_CMD_DATA_FIXED    0x44
#define TM1640_CMD_ADDRESS_BASE  0xC0u
#define TM1640_CMD_DISPLAY_BASE  0x88u   /* display control base (0x88 = on + duty bits) */
#define TM1640_CMD_TEST_BASE     0x80u   /* test mode uses command bits per datasheet */

#define TM1640_ADDR_MASK         0x0Fu
#define TM1640_ADDR_MAX          0x0Fu

/* Display control bits */
#define TM1640_DISPLAY_OFF   false
#define TM1640_DISPLAY_ON    true  /* D bit (bit3) = 1 -> display on */

/* Brightness (duty) — datasheet defines 8 levels (C2..C0). Use values 0..7. */
#define TM1640_DUTY_1_16   0x00u
#define TM1640_DUTY_2_16   0x01u
#define TM1640_DUTY_4_16   0x02u
#define TM1640_DUTY_10_16  0x03u
#define TM1640_DUTY_11_16  0x04u
#define TM1640_DUTY_12_16  0x05u
#define TM1640_DUTY_13_16  0x06u
#define TM1640_DUTY_14_16  0x07u

/* --- Command builders (datasheet-aligned) --- */
/* Data command: low 6 bits = mode bits (auto-increment or fixed mode bits). */
#define TM1640_MAKE_DATA_CMD(mode_bits) \
  ((uint8_t)(TM1640_CMD_DATA_BASE | ((uint8_t)(mode_bits) & 0x3Fu)))

/* Fixed-address data command (explicit) */
#define TM1640_MAKE_DATA_FIXED_CMD(mode_bits) \
  ((uint8_t)(TM1640_CMD_DATA_FIXED | ((uint8_t)(mode_bits) & 0x3Fu)))

/* Address command: 0xC0 | addr (addr = 0..0x0F) */
#define TM1640_MAKE_ADDRESS_CMD(addr) \
  ((uint8_t)(TM1640_CMD_ADDRESS_BASE | ((uint8_t)(addr) & TM1640_ADDR_MASK)))

/* Display control: 0x80 | D(bit3) | duty(3bits) — many libs use 0x88|duty to force ON */
#define TM1640_MAKE_DISPLAY_CMD(on, duty) \
  ((uint8_t)(TM1640_CMD_DISPLAY_BASE | ((on) ? TM1640_DISPLAY_ON : TM1640_DISPLAY_OFF) | ((uint8_t)(duty) & 0x07u)))

/* Test command: datasheet varies; treat as 0x80 | code (implementation may use 0x01 in LSB) */
#define TM1640_MAKE_TEST_CMD(code) \
  ((uint8_t)(TM1640_CMD_TEST_BASE | ((uint8_t)(code) & 0x3Fu)))

/* --- Timing / CPU detection (defaults; can be overridden) --- */
#if !defined(TM1640_FCPU_HZ)
  #if defined(XTENSA) || defined(riscv) || defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
    #define TM1640_FCPU_HZ 240000000UL
  #elif defined(AVR)
    #if defined(F_CPU)
      #define TM1640_FCPU_HZ ((uint32_t)F_CPU)
    #elif defined(LGT8F) || defined(LGT8F328)
      #define TM1640_FCPU_HZ 32000000UL
    #else
      #define TM1640_FCPU_HZ 16000000UL
    #endif
  #else
    #error "Define TM1640_FCPU_HZ or build for AVR/ESP32"
  #endif
#endif

#ifndef TM1640_CYCLES_SETDIN
  #if defined(XTENSA) || defined(riscv) || defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
    #define TM1640_CYCLES_SETDIN   3u
    #define TM1640_CYCLES_CLRCLK   3u
    #define TM1640_CYCLES_SETCLK   3u
    #define TM1640_CYCLES_DELAYBIT 1u
  #else
    #define TM1640_CYCLES_SETDIN   2u
    #define TM1640_CYCLES_CLRCLK   2u
    #define TM1640_CYCLES_SETCLK   2u
    #define TM1640_CYCLES_DELAYBIT 1u
  #endif
#endif

#define TM1640_BITS_PER_CHAR   8u
#define TM1640_CYCLES_PER_BIT  ((uint32_t)TM1640_CYCLES_SETDIN + (uint32_t)TM1640_CYCLES_CLRCLK + (uint32_t)TM1640_CYCLES_DELAYBIT + (uint32_t)TM1640_CYCLES_SETCLK)
#define TM1640_CYCLES_PER_CHAR ((uint32_t)TM1640_BITS_PER_CHAR * (uint32_t)TM1640_CYCLES_PER_BIT)

#ifndef TM1640_STOP_MULT
  #define TM1640_STOP_MULT 1u
#endif
#define TM1640_CYCLES_PER_STOP ((uint32_t)TM1640_STOP_MULT * (uint32_t)TM1640_CYCLES_PER_BIT)

#define TM1640_NS_PER_CYCLE_I64 ((uint64_t)1000000000ULL / (uint64_t)(TM1640_FCPU_HZ))
#define TM1640_NS_PER_CHAR_I64  ((uint64_t)TM1640_CYCLES_PER_CHAR * (uint64_t)TM1640_NS_PER_CYCLE_I64)
#define TM1640_NS_PER_STOP_I64  ((uint64_t)TM1640_CYCLES_PER_STOP * (uint64_t)TM1640_NS_PER_CYCLE_I64)

/* ===========================
 * Class Definition (members/signatures unchanged)
 * =========================== */
class TM1640 {
public:
  TM1640(uint8_t gpio_sclk, uint8_t gpio_din, int frequency_khz);

  /* Basic control (return TM1640_OK or negative error) */
  int Display(bool on);
  int Test(bool start);


  /* Drawing */
  int DrawAddrInc(const uint8_t *Bytes, uint8_t len);
  int DrawAddrInc(const uint8_t *Bytes, uint8_t len, uint8_t duty);
  
  int DrawAddrFix(uint8_t addr, const uint8_t* Bytes, uint8_t len);
  
private:
  uint8_t _sclk_pin;
  uint8_t _din_pin;
  int _frequency_khz; /* 0 = use default derived from TM1640_FCPU_HZ */
  uint32_t halfPeriod_us;
  uint8_t _lastDuty = 0x07;
  int _setDuty(uint8_t duty);
  void _sendByte(uint8_t byte);
  void _sendBit(bool bitVal);
  void _sendStartCondi();
  void _sendEndCondi();
};

#endif /* TM1640_H */
