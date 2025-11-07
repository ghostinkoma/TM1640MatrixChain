#include "tm1640.h"

static void initclass(uint8_t sclk, uint8_t din) {
  pinMode(sclk, OUTPUT_PULLUP);
  pinMode(din,  OUTPUT_PULLUP);
  
}

TM1640::TM1640(uint8_t gpio_sclk, uint8_t gpio_din, int)
{
  initPinsPullup(gpio_sclk, gpio_din);
}
