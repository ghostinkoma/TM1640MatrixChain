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

/**
 * Sends a single byte over the TM1640's bit-banged I2C-like protocol.
 * Supports FAST mode for ESP32/AVR with direct register access for performance.
 * Timing: CLK low for data setup, CLK high for latching (1us hold per datasheet).
 * LSB first transmission.
 */
void TM1640::send_byte_hw(uint8_t b) const {
#ifdef FAST_TM1640
#if IS_ESP32
    // ESP32 optimized: Use GPIO output registers for minimal overhead.
    uint32_t clk_mask = _clk_mask;
    uint32_t dio_mask = _dio_mask;
    for (uint8_t i = 0; i < 8; ++i) {
        *_gpio_out_clr = clk_mask;  // CLK low: Setup data
        if (b & (1 << i)) *_gpio_out = dio_mask; else *_gpio_out_clr = dio_mask;  // DIO = bit i (LSB first)
        esp_rom_delay_us(1);  // Hold data low phase (min 1us per datasheet)
        *_gpio_out = clk_mask;  // CLK high: Latch data
        esp_rom_delay_us(1);  // Hold high phase (min 1us)
    }
    *_gpio_out = clk_mask;  // Idle CLK high
#elif IS_AVR328 || IS_LGT8F328
    // AVR optimized: Direct port manipulation for speed on 328p/LGT8F.
    uint8_t clk_mask = (uint8_t)_clk_mask;
    uint8_t dio_mask = (uint8_t)_dio_mask;
    DDR_REG |= (clk_mask | dio_mask);  // Set pins as outputs
    for (uint8_t i = 0; i < 8; ++i) {
        PORT_REG &= ~clk_mask;  // CLK low
        if (b & (1 << i)) PORT_REG |= dio_mask; else PORT_REG &= ~dio_mask;  // DIO bit
        _delay_us(1);  // Data hold low
        PORT_REG |= clk_mask;  // CLK high
        _delay_us(1);  // Latch hold
    }
    PORT_REG |= clk_mask;  // Idle high
#else
    // Fallback: Standard digitalWrite for portability (slower but works everywhere).
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(_clkPin, LOW);  // CLK low: Prepare data
        digitalWrite(_dioPin, (b & (1 << i)) ? HIGH : LOW);  // Set DIO to bit i
        delayMicroseconds(1);  // Setup/hold time
        digitalWrite(_clkPin, HIGH);  // CLK high: Transfer
        delayMicroseconds(1);  // Latch time
    }
    digitalWrite(_clkPin, HIGH);  // Idle state
#endif
#else
    // Standard stable bit-banging: No FAST mode, reliable for debugging.
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

/**
 * Writes the red and green planes from shadow buffers to TM1640 SRAM.
 * Sequence: Start cond -> Cmd/Data -> End cond (critical for command recognition).
 * Red plane: GRID1-8 (addr 00-07H), Green: GRID9-16 (addr 08-0FH).
 * Fixes orange-only bug by adding explicit start/end conditions.
 * Assumes planeRed/Green are 8-byte arrays (8x8 matrix rows, MSB first?).
 */
void TM1640::write_all_from_shadow() {
    if (!planeRed || !planeGreen) return;  // Skip if buffers invalid (null check for safety)
    
    pinMode(_dioPin, OUTPUT);
    pinMode(_clkPin, OUTPUT);  // Ensure CLK output for start/end
    
    // Start condition: CLK high, DIO high -> low (initiates frame per datasheet)
    digitalWrite(_clkPin, HIGH);
    digitalWrite(_dioPin, HIGH);
    delayMicroseconds(1);  // Stabilize
    digitalWrite(_dioPin, LOW);  // Edge trigger
    delayMicroseconds(1);
    
    // Command: Auto-increment address mode (write multiple bytes sequentially)
    send_byte_hw(0x44);  // 01000100: Data set command, auto addr inc (B3=1)
    
    // Address: Start at 00H for GRID1
    send_byte_hw(0xC0);  // 11000000: Display address 00H
    
    // Write red plane (8 bytes for rows 1-8)
    for (uint8_t i = 0; i < 8; ++i) {
        send_byte_hw(planeRed[i]);  // Red LED segments (upper grid)
    }
    
    // Write green plane (continues auto-inc to addr 08H for rows 9-16)
    for (uint8_t i = 0; i < 8; ++i) {
        send_byte_hw(planeGreen[i]);  // Green LED segments (lower grid)
    }
    
    // Control: Display on with duty cycle
    send_byte_hw(0x88 | _duty);  // 10001000 | duty: B3=1 on, B0-2=duty (0-7)
    
    // End condition: CLK high, DIO low -> high (ends frame, releases bus)
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(_dioPin, HIGH);  // Back to pull-up idle
    delayMicroseconds(1);
    
    // Restore pin modes (DIO input for pull-up, CLK optional)
    pinMode(_dioPin, INPUT);
    // pinMode(_clkPin, INPUT);  // Uncomment if CLK needs pull-up idle
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