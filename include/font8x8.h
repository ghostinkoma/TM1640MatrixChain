/**
 * font8x8.h
 *
 * This file declares the U8g2 Japanese 8x8 font used by TM1640MatrixChain.
 * The actual font data is provided by the U8g2 library (installed via Library Manager).
 *
 * Usage in your sketch:
 *   chain->setFont(u8g2_font_japanese1_tf);
 *
 * No need to modify this file – it simply extern-declares the font
 * so the compiler can find it when the U8g2 library is linked.
 *
 * --------------------------------------------------------------------
 * Author: Ghostinkoma
 * Version: 1.1.0
 * License: MIT (same as the TM1640MatrixChain library)
 * --------------------------------------------------------------------
 */

#ifndef FONT8X8_H
#define FONT8X8_H

#include <U8g2lib.h>   // Pulls in the full U8g2 font definitions

/* 8x8 Japanese font (covers Hiragana, Katakana, some Kanji & ASCII) */
extern const uint8_t u8g2_font_japanese1_tf[] U8G2_FONT_SECTION("u8g2_font_japanese1_tf");

#endif /* FONT8X8_H */