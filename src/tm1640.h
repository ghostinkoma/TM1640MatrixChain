// tm1640.h - TM1640 bi-color 8x8 LED matrix driver
#ifndef TM1640_H
#define TM1640_H

#include <Arduino.h>
#include <stdint.h>

// Platform detection
#if defined(ESP32)
#define IS_ESP32 1
#elif defined(__AVR_ATmega328P__)
#define IS_AVR328 1
#elif defined(LGT8F328P)
#define IS_LGT8F328 1
#else
#define IS_UNKNOWN 1
#endif

// Default communication frequency
#if IS_ESP32
#define DEFAULT_COMM_HZ 1000000UL
#elif IS_AVR328 || IS_LGT8F328
#define DEFAULT_COMM_HZ 500000UL
#else
#define DEFAULT_COMM_HZ 500000UL
#endif

// Color definitions
#define OFF     0
#define RED     1
#define GREEN   2
#define ORANGE  3

// Enable high-speed mode (TM16xx-inspired direct port access)
// #define FAST_TM1640  // Uncomment for max speed (after stability testing)

class TM1640 {
public:
    // Constructor: DIO pin, CLK pin, comm frequency, rotation (0/90/270)
    TM1640(uint8_t dioPin, uint8_t clkPin, uint32_t commHz = DEFAULT_COMM_HZ, uint16_t rotation_deg = 0);
    ~TM1640();

    // Getters
    uint8_t get_seg_count() const { return _segCount; }
    uint8_t get_grid_count() const { return _gridCount; }
    uint8_t get_display_height() const { return 8; }
    uint16_t get_rotation() const { return _rotation; }

    // Initialization and control
    void initialize();                    // Initialize module
    void set_duty(uint8_t val);           // Set brightness: 0=off, 1=1/16, ..., 7=14/16

    // Local drawing
    void draw_dot_local(uint8_t x, uint8_t y, uint8_t color);  // Draw single pixel
    void drawbin_local(const uint8_t segBytes[], uint8_t segLen, uint8_t color); // Draw 8x8 bitmap
    void get_gram_local(uint8_t out[]) const;                  // Read current GRAM (16 bytes: R+G)
    void force_update();                                       // Force display update

private:
    static constexpr uint8_t _segCount = 8;
    static constexpr uint8_t _gridCount = 16;
    uint8_t _dioPin;
    uint8_t _clkPin;
    uint32_t _commHz;
    uint16_t _rotation;
    uint8_t _duty = 7;

    uint8_t* planeRed = nullptr;   // Shadow RAM: Red plane
    uint8_t* planeGreen = nullptr; // Shadow RAM: Green plane

#ifdef FAST_TM1640
    uint32_t _clk_mask = 0;        // Precomputed pin masks for speed
    uint32_t _dio_mask = 0;
#endif

    void hw_pin_init();            // Initialize GPIO pins
    void write_all_from_shadow();  // Write shadow RAM to TM1640
    uint8_t seggrid_to_addr_local(uint8_t segIndex, uint8_t gridIndex) const { return gridIndex; }
    void send_byte_hw(uint8_t b) const;  // Send one byte via bit-banging
};

#endif