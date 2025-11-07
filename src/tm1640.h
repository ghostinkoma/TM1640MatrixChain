#ifndef TM1640_H
#define TM1640_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* return codes */
#define TM1640_OK           0
#define TM1640_ERR_PARAM   -1
#define TM1640_ERR_BUF     -2
#define TM1640_ERR_GPIO    -3

/* event / callback literals */
#define TM1640_EVT_OK             0x00u
#define TM1640_EVT_PARAM_ERR      0x01u
#define TM1640_EVT_BUF_ERR        0x02u
#define TM1640_EVT_GPIO_ERR       0x03u
#define TM1640_EVT_SEND_START     0x10u
#define TM1640_EVT_SEND_COMPLETE  0x11u

/* command bases and masks */
#define TM1640_CMD_DATA_BASE     0x00u
#define TM1640_CMD_DISPLAY_BASE  0x40u
#define TM1640_CMD_ADDRESS_BASE  0xC0u
#define TM1640_CMD_TEST_BASE     0x80u
#define TM1640_ADDR_MASK         0x0Fu
#define TM1640_ADDR_MAX          0x0Fu

/* display control bit and duty codes (Table 10) */
#define TM1640_DISPLAY_ON_BIT 0x08u

#define TM1640_DUTY_OFF 0x00u
#define TM1640_DUTY_1   0x01u
#define TM1640_DUTY_2   0x02u
#define TM1640_DUTY_3   0x03u
#define TM1640_DUTY_4   0x04u
#define TM1640_DUTY_5   0x05u
#define TM1640_DUTY_6   0x06u
#define TM1640_DUTY_7   0x07u
#define TM1640_DUTY_8   0x08u
#define TM1640_DUTY_9   0x09u
#define TM1640_DUTY_10  0x0Au
#define TM1640_DUTY_11  0x0Bu
#define TM1640_DUTY_12  0x0Cu
#define TM1640_DUTY_13  0x0Du
#define TM1640_DUTY_14  0x0Eu

/* command constructors */
#define TM1640_MAKE_DATA_CMD(mode_bits)   ((uint8_t)((TM1640_CMD_DATA_BASE) | ((uint8_t)(mode_bits) & 0x3Fu)))
#undef TM1640_MAKE_DISPLAY_CMD
#define TM1640_MAKE_DISPLAY_CMD(on,duty) \
  ((uint8_t)(TM1640_CMD_DISPLAY_BASE | ((on) ? TM1640_DISPLAY_ON_BIT : 0x00u) | ((uint8_t)(duty) & 0x0Fu)))
#define TM1640_MAKE_ADDRESS_CMD(addr)     ((uint8_t)((TM1640_CMD_ADDRESS_BASE) | ((uint8_t)(addr) & TM1640_ADDR_MASK)))
#define TM1640_MAKE_TEST_CMD(code)        ((uint8_t)((TM1640_CMD_TEST_BASE) | ((uint8_t)(code) & 0x3Fu)))

/* CPU clock detection (defaults; override with -DTM1640_FCPU_HZ or F_CPU) */
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

/* event callback type */
typedef void (*tm1640_event_cb_t)(uint8_t event, void *ctx);

/* Arduino C++ wrapper class
   Constructor prototype required by caller:
     TM1640 LEDmatrix(uint8_t gpio_sclk, uint8_t gpio_din, int frequency_khz);
   If frequency_khz == 0 the implementation should derive a sensible default
   from TM1640_FCPU_HZ (frequency_khz = TM1640_FCPU_HZ / 1000).
*/
class TM1640 {
public:
  TM1640(uint8_t gpio_sclk, uint8_t gpio_din, int frequency_khz);

  /* Basic control (return TM1640_OK or negative error) */
  int Display(bool on);
  int Test(bool start);
  int SetDuty(uint8_t duty); /* implementation should validate 0x00..0x0E and return TM1640_ERR_PARAM on invalid */

  /* Drawing */
  int DrawAddrInc(const uint8_t *chars, uint16_t len);
  int DrawAddrFix(uint8_t addr, const uint8_t *chars, uint16_t len);

  /* Optional: event callback registration */
  void SetEventCallback(tm1640_event_cb_t cb, void *ctx);

private:
  uint8_t _sclk_pin;
  uint8_t _din_pin;
  int _frequency_khz; /* 0 = use default derived from TM1640_FCPU_HZ */
  uint8_t _duty;
  tm1640_event_cb_t _evt_cb;
  void *_evt_ctx;

  /* low-level helpers (implement in .cpp) */
  void _pinSet(uint8_t pin, bool level);
  void _delayForBitTiming(void);
  int _sendRawByte(uint8_t b);
};

#endif /* TM1640_H */
