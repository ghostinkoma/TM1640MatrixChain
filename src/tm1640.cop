// tm1640.cpp - TM1640 driver implementation
#include "tm1640.h"
#include <stdlib.h>
#include <string.h>

#if IS_ESP32
#include "driver/gpio.h"
#include "esp_rom_sys.h"
volatile uint32_t* _gpio_out = &GPIO.out_w1ts;
volatile uint32_t* _gpio_out_clr = &GPIO.out_w1tc;
#elif IS_AVR328 || IS_LGT8F328
#include <util/delay.h>
#define PORT_REG PORTB
#define DDR_REG DDRB
#endif

TM1640::TM1640(uint8_t dioPin, uint8_t clkPin, uint32_t commHz, uint16_t rotation_deg)
    : _dioPin(dioPin), _clkPin(clkPin), _commHz(commHz) {
    // Normalize rotation to 0, 90, or 270
    if (rotation_deg != 0 && rotation_deg != 90 && rotation_deg != 270) rotation_deg = 0;
    _rotation = rotation_deg;

    // Allocate shadow RAM
    planeRed = (uint8_t*)malloc(8);
    planeGreen = (uint8_t*)malloc(8);
    if (planeRed) memset(planeRed, 0, 8);
    if (planeGreen) memset(planeGreen, 0, 8);

#ifdef FAST_TM1640
    // Precompute pin masks for high-speed access
    if (IS_ESP32) {
        _clk_mask = (1UL << _clkPin);
        _dio_mask = (1UL << _dioPin);
    } else if (IS_AVR328 || IS_LGT8F328) {
        _clk_mask = (1 << _clkPin);
        _dio_mask = (1 << _dioPin);
    }
#endif
}

TM1640::~TM1640() {
    if (planeRed) { free(planeRed); planeRed = nullptr; }
    if (planeGreen) { free(planeGreen); planeGreen = nullptr; }
}

void TM1640::hw_pin_init() {
    pinMode(_clkPin, OUTPUT);
    pinMode(_dioPin, OUTPUT);
    digitalWrite(_clkPin, LOW);
    digitalWrite(_dioPin, LOW);

#ifdef FAST_TM1640
    if (IS_ESP32) {
        gpio_set_direction((gpio_num_t)_clkPin, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)_dioPin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)_clkPin, 0);
        gpio_set_level((gpio_num_t)_dioPin, 0);
    }
#endif
}

void TM1640::send_byte_hw(uint8_t b) const {
#ifdef FAST_TM1640
#if IS_ESP32
    uint32_t clk_mask = _clk_mask;
    uint32_t dio_mask = _dio_mask;
    for (uint8_t i = 0; i < 8; ++i) {
        *_gpio_out_clr = clk_mask;
        if (b & (1 << i)) *_gpio_out = dio_mask; else *_gpio_out_clr = dio_mask;
        esp_rom_delay_us(1);
        *_gpio_out = clk_mask;
        esp_rom_delay_us(1);
    }
    *_gpio_out = clk_mask;
#elif IS_AVR328 || IS_LGT8F328
    uint8_t clk_mask = (uint8_t)_clk_mask;
    uint8_t dio_mask = (uint8_t)_dio_mask;
    DDR_REG |= (clk_mask | dio_mask);
    for (uint8_t i = 0; i < 8; ++i) {
        PORT_REG &= ~clk_mask;
        if (b & (1 << i)) PORT_REG |= dio_mask; else PORT_REG &= ~dio_mask;
        _delay_us(1);
        PORT_REG |= clk_mask;
        _delay_us(1);
    }
    PORT_REG |= clk_mask;
#else
    // Fallback: standard bit-banging
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(_clkPin, LOW);
        digitalWrite(_dioPin, (b & (1 << i)) ? HIGH : LOW);
        delayMicroseconds(1);
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(1);
    }
    digitalWrite(_clkPin, HIGH);
#endif
#else
    // Standard stable implementation
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(_clkPin, LOW);
        digitalWrite(_dioPin, (b & (1 << i)) ? HIGH : LOW);
        delayMicroseconds(1);
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(1);
    }
    digitalWrite(_clkPin, HIGH);
#endif
}

void TM1640::write_all_from_shadow() {
    if (!planeRed || !planeGreen) return;
    pinMode(_dioPin, OUTPUT);

    send_byte_hw(0x44);  // Auto address increment
    send_byte_hw(0xC0);  // Start address 0
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeRed[i]);
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeGreen[i]);
    send_byte_hw(0x88 | _duty);  // Display on + duty cycle

    pinMode(_dioPin, INPUT);
}

void TM1640::set_duty(uint8_t val) {
    _duty = (val <= 7) ? val : 7;
    force_update();
}

void TM1640::initialize() {
    hw_pin_init();
    if (planeRed) memset(planeRed, 0, 8);
    if (planeGreen) memset(planeGreen, 0, 8);
    pinMode(_dioPin, OUTPUT);
    send_byte_hw(0x80);  // Display off
    pinMode(_dioPin, INPUT);
    write_all_from_shadow();
}

void TM1640::draw_dot_local(uint8_t x, uint8_t y, uint8_t color) {
    if (!planeRed || !planeGreen || x >= 8 || y >= 8) return;
    uint8_t bit = (1U << x);
    if (color & RED) planeRed[y] |= bit;
    if (color & GREEN) planeGreen[y] |= bit;
    if (color == OFF) {
        planeRed[y] &= ~bit;
        planeGreen[y] &= ~bit;
    }
}

void TM1640::drawbin_local(const uint8_t segBytes[], uint8_t segLen, uint8_t color) {
    if (!planeRed || !planeGreen) return;
    uint8_t len = (segLen < 8) ? segLen : 8;
    for (uint8_t i = 0; i < len; ++i) {
        if (color & RED) planeRed[i] = segBytes[i];
        if (color & GREEN) planeGreen[i] = segBytes[i];
        if (color == OFF) {
            planeRed[i] = 0;
            planeGreen[i] = 0;
        }
    }
}

void TM1640::get_gram_local(uint8_t out[]) const {
    if (!out || !planeRed || !planeGreen) return;
    memcpy(out, planeRed, 8);
    memcpy(out + 8, planeGreen, 8);
}

void TM1640::force_update() {
    write_all_from_shadow();
}