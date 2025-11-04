/*c:\Users\sinhex\Desktop\TM1640test\TM1640MatrixChain-main\src\matrixchain.cpp c:\Users\sinhex\Desktop\TM1640test\TM1640MatrixChain-main\src\matrixchain.h c:\Users\sinhex\Desktop\TM1640test\TM1640MatrixChain-main\src\tm1640.cpp c:\Users\sinhex\Desktop\TM1640test\TM1640MatrixChain-main\src\tm1640.h
 * TM1640MatrixChain - Basic Demo
 * 
 * Features demonstrated:
 *  - 8-module horizontal chain (shared CLK, individual DIO)
 *  - Bi-color text (Japanese + English) via U8g2
 *  - Icon printing
 *  - Smooth left-scroll animation (1.5s)
 *  - High-speed mode (FAST_TM1640) toggle
 *  - Update time measurement via Serial
 * 
 * Wiring:
 *  - CLK: Pin 2 (shared)
 *  - DIO: Pins 3 to 10 (8 modules)
 * 
 * Author: Ghostinkoma
 * Version: 1.1.0
 */

// === HIGH-SPEED MODE ===
// Uncomment to enable TM16xx-inspired direct port access (~2x faster)
// #define FAST_TM1640

#include <Arduino.h>
#include "tm1640.h"
#include "matrixchain.h"

// Pin configuration
#define CLK_SHARED 2
#define DIO_BASE   3
#define NUM_MODS   8

// Module array and chain instance
TM1640* mods[NUM_MODS];
MatrixChain* chain;

void setup() {
    // Initialize serial for performance logging
    Serial.begin(115200);
    while (!Serial) { delay(10); }  // Wait for serial (optional)
    delay(100);

    Serial.println(F("TM1640MatrixChain Basic Demo Starting..."));

    // Create TM1640 instances (no rotation for simplicity)
    for (uint8_t i = 0; i < NUM_MODS; ++i) {
        mods[i] = new TM1640(DIO_BASE + i, CLK_SHARED, DEFAULT_COMM_HZ, 0);
    }

    // Create chain: 8 modules, horizontal layout, 8 columns
    chain = new MatrixChain(mods, NUM_MODS, HORIZONTAL, NUM_MODS);

    // Initialize hardware
    chain->initialize_all();
    chain->set_duty(4);  // Medium brightness (0-7)

    // Initialize U8g2 virtual buffer for text rendering
    chain->beginU8g2Virtual();

    Serial.print(F("Total display size: "));
    Serial.print(chain->total_width());
    Serial.print(F("x"));
    Serial.println(chain->total_height());

    // === Draw static content ===

    // Japanese text (8x8 font)
    chain->setFont(u8g2_font_maniac_tf);
    chain->drawUTF8(0, 0, "こんにちは", ORANGE);

    // English text (5x7 font)
    chain->setFont(u8g2_font_5x7_tf);
    chain->drawUTF8(24, 0, "Hello World!", RED);

    // Icon: Heart (8x8 bitmap)
    uint8_t heart[8] = {
        0b00000000,
        0b01100110,
        0b11111111,
        0b11111111,
        0b01111110,
        0b00111100,
        0b00011000,
        0b00000000
    };
    chain->icon_print(heart, 56, GREEN);

    // Force update to display content
    unsigned long updateStart = micros();
    chain->force_update_all();
    unsigned long updateTime = micros() - updateStart;

    Serial.print(F("Static content drawn. Update time: "));
    Serial.print(updateTime);
    Serial.println(F(" µs"));

    // === Start smooth scroll animation ===
    // Scroll left: full screen, 1500ms duration
    chain->start_scroll_animation(LEFT, 0, 0, 0, 0, 1500);
    Serial.println(F("Scroll animation started (1500ms)."));
}

void loop() {
    // Update animation at ~60 FPS
    chain->update_scroll_animation();
    delay(16);
}
