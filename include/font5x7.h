/**
 * font5x7.h
 *
 * Declares the U8g2 5x7 ASCII font used by TM1640MatrixChain.
 * The actual bitmap data lives in the U8g2 library (installed via Library Manager).
 *
 * Example usage:
 *   chain->setFont(u8g2_font_5x7_tf);
 *
 * No changes needed – just include this header and the U8g2 library
 * will supply the real font table at link-time.
 *
 * --------------------------------------------------------------------
 * Author: Ghostinkoma
 * Version: 1.1.0
 * License: MIT (same as the TM1640MatrixChain library)
 * --------------------------------------------------------------------
 */

#ifndef FONT5X7_H
#define FONT5X7_H

#include <U8g2lib.h>   // Brings in all U8g2 font definitions

/* 5x7 proportional ASCII font (compact, fast, English-only) */
extern const uint8_t u8g2_font_5x7_tf[] U8G2_FONT_SECTION("u8g2_font_5x7_tf");

#endif /* FONT5X7_H */