# TM1640MatrixChain

High-performance Arduino library for **bi-color 8x8 LED matrix modules** based on **TM1640**.  
Supports **chaining**, **rotation**, **U8g2 fonts (incl. Japanese)**, **smooth scrolling**, and **high-speed 
---

## Features

- **Bi-color control**: Red, Green, Orange, Off
- **Chaining**: Up to 255 modules (global coordinates)
- **Rotation**: 0°, 90°, 270° per module
- **U8g2 integration**: UTF-8 text, Japanese fonts
- **Smooth scroll animation** with easing (60 FPS)
- **High-speed mode** (optional, TM16xx-inspired)
- **Memory-safe** with null checks and padding

---

## Dependencies

- [U8g2](https://github.com/olikraus/u8g2) (install via Library Manager)

---

## Installation

1. Download as ZIP or clone
2. Place in `Arduino/libraries/TM1640MatrixChain`
3. Restart Arduino IDE

---

## Usage

```cpp
#include "matrixchain.h"

TM1640* mods[8];
MatrixChain chain(mods, 8, HORIZONTAL);

void setup() {
  for (int i = 0; i < 8; i++) mods[i] = new TM1640(3+i, 2);
  chain.initialize_all();
  chain.beginU8g2Virtual();
  chain.setFont(u8g2_font_japanese1_tf);
  chain.drawUTF8(0, 0, "Hello 世界", ORANGE);
  chain.start_scroll_animation(LEFT, 0, 0, 0, 0, 1500);
}

void loop() {
  chain.update_scroll_animation();
  delay(16);
}