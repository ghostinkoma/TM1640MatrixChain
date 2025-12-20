#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

typedef struct {
    char c[4];          // 文字（UTF-8対応、終端\0含む）
    uint8_t bitmap[5];  // 5x7フォント（下位7ビット使用）
} FontChar;

// フォントテーブル（外部定義）
extern const FontChar font5x7[];

// 文字 → インデックス変換関数
uint8_t getFontIndex(const char* c);

#endif // FONT5X7_H
