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
 * Timing: 2us hold per phase (datasheet min 1us; extended for stability on Uno/AVR).
 * LSB first: Shifts bits from i=0 (LSB) to i=7 (MSB).
 * No start/end conditions here; caller (e.g., write_all_from_shadow) handles frame.
 */
void TM1640::send_byte_hw(uint8_t b) const {
#ifdef FAST_TM1640
#if IS_ESP32
    // ESP32 fast path: GPIO registers for minimal loop overhead.
    uint32_t clk_mask = _clk_mask;
    uint32_t dio_mask = _dio_mask;
    for (uint8_t i = 0; i < 8; ++i) {
        *_gpio_out_clr = clk_mask;  // CLK low: Data setup (per datasheet)
        if (b & (1 << i)) *_gpio_out = dio_mask; else *_gpio_out_clr = dio_mask;  // DIO = LSB bit i
        esp_rom_delay_us(2);  // Setup/hold time (extended for bicolor sync)
        *_gpio_out = clk_mask;  // CLK high: Latch into TM1640 shift register
        esp_rom_delay_us(2);  // Latch hold (ensures stable transfer)
    }
    *_gpio_out = clk_mask;  // Idle: CLK high (default state)
#elif IS_AVR328 || IS_LGT8F328
    // AVR fast path: Direct PORT/DDR for 328p/LGT8F efficiency.
    uint8_t clk_mask = (uint8_t)_clk_mask;
    uint8_t dio_mask = (uint8_t)_dio_mask;
    DDR_REG |= (clk_mask | dio_mask);  // Set CLK/DIO as outputs
    for (uint8_t i = 0; i < 8; ++i) {
        PORT_REG &= ~clk_mask;  // CLK low
        if (b & (1 << i)) PORT_REG |= dio_mask; else PORT_REG &= ~dio_mask;  // DIO bit i
        _delay_us(2);  // Data hold low phase
        PORT_REG |= clk_mask;  // CLK high
        _delay_us(2);  // Latch hold
    }
    PORT_REG |= clk_mask;  // Idle high
#else
    // Portable fallback: digitalWrite for any Arduino (slower, debug-friendly).
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(_clkPin, LOW);  // CLK low: Prepare data change
        digitalWrite(_dioPin, (b & (1 << i)) ? HIGH : LOW);  // Set DIO to bit i (LSB first)
        delayMicroseconds(2);  // Setup time (datasheet compliant, extended)
        digitalWrite(_clkPin, HIGH);  // CLK high: Shift/latch byte
        delayMicroseconds(2);  // Hold time
    }
    digitalWrite(_clkPin, HIGH);  // Idle CLK high
#endif
#else
    // Non-FAST stable mode: Pure digitalWrite for reliability testing.
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(_clkPin, LOW);
        digitalWrite(_dioPin, (b & (1 << i)) ? HIGH : LOW);
        delayMicroseconds(2);  // Adjusted for orange bug stability
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(2);
    }
    digitalWrite(_clkPin, HIGH);
#endif
}

/**
 * Writes 16 bytes from red/green shadow planes to TM1640 SRAM (8x8 bicolor matrix).
 * Full frame: Start cond (DIN H→L on CLK H) → Data cmd → Addr → Planes (auto inc) → Display ctrl → End cond (DIN L→H on CLK H).
 * Data cmd: 0x40 (normal write, auto addr inc; per datasheet Command1 B7=0 B6=1 B5-B0=0).
 * Addr: 0xC0 (11000000B = 00H start; Table address command).
 * Planes: Red (GRID1-8, addr 00-07H) then Green (GRID9-16, addr 08-0FH; Table 9 SEG/GRID mapping).
 * Display: 0x88 | _duty (10001000B | duty; MSB=1 on, LSB=duty 0-7 for 1/16-14/16 pulse width; Table 10).
 * Fixes orange-only: Uses 0x40 (not 0x44 test mode), explicit start/end, 2us timing.
 * Optional: #define REVERSE_PLANES for green-then-red test (if grid swap suspected).
 * Assumes planeRed/Green: uint8_t[8] arrays (rows, SEG0-7 bits; MSB=SEG7 per Table 9).
 * Call after U8g2 draw (shadow fill); skips if null (memory safe).
 */
void TM1640::write_all_from_shadow() {
    if (!planeRed || !planeGreen) return;  // Safety: Skip invalid buffers
    
    pinMode(_dioPin, OUTPUT);
    pinMode(_clkPin, OUTPUT);  // CLK output for precise start/end control
    
    // Start condition: CLK high, DIN high → low (frame init; datasheet VII, DIN edge on CLK H)
    digitalWrite(_clkPin, HIGH);
    digitalWrite(_dioPin, HIGH);
    delayMicroseconds(2);  // Stabilize bus (extended for reliability)
    digitalWrite(_dioPin, LOW);  // Falling edge triggers reception
    delayMicroseconds(2);  // Hold edge
    
    // Data command: Normal write mode with auto address increment (Command1=0x40; B7=0 B6=1 B5-B0=0)
    send_byte_hw(0x40);  // 01000000B: Sets LED driver for data input (not 0x44 test mode)
    
    // Address command: Start at 00H (Table address: 11000000B for GRID1/SEG0-7)
    send_byte_hw(0xC0);  // Auto inc will advance to 08H after first 8 bytes
    
    // Shadow planes to SRAM: 16 bytes total for 8x8 bicolor (red upper grid, green lower)
#ifdef REVERSE_PLANES
    // Test variant: Green first (if orange due to red/green grid swap in Table 9 mapping)
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeGreen[i]);  // Green to addr 00-07H (GRID1-8?)
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeRed[i]);    // Red to 08-0FH (GRID9-16)
#else
    // Standard: Red first (upper grid GRID1-8 red LEDs; datasheet convention)
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeRed[i]);    // Red plane: Rows 0-7 to addr 00-07H
    for (uint8_t i = 0; i < 8; ++i) send_byte_hw(planeGreen[i]);  // Green plane: Rows 0-7 to 08-0FH (auto inc)
#endif
    
    // Display control: Turn on with duty cycle (Command3; MSB=1 for active, LSB=duty per Table 10)
    send_byte_hw(0x88 | _duty);  // 10001000B | duty (0-7): On, pulse width 1/16 to 14/16 (LSB=000=duty min)
    
    // End condition: CLK high, DIN low → high (frame termination; releases for next command)
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(2);  // Hold CLK high
    digitalWrite(_dioPin, HIGH);  // Rising edge ends frame (pull-up idle)
    delayMicroseconds(2);  // Bus settle
    
    // Restore: DIO input (pull-up for multi-device bus); CLK optional input if pulled high
    pinMode(_dioPin, INPUT);
    // pinMode(_clkPin, INPUT);  // Uncomment if external pull-up on CLK
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