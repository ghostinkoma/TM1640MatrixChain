#ifndef TM1640MATRIXCHAIN_H
#define TM1640MATRIXCHAIN_H

#include <Arduino.h>
#include "TM1640.h"
#include "font5x7.h"

#define COLOR_NONE   0
#define COLOR_RED    1
#define COLOR_GREEN  2
#define COLOR_ORANGE 3

#define MODULE_RESO 8       // 1モジュールあたり8ドット
#define LEFT_PAD    8       // スクロール用余白
#define WIDTH       40      // 表示幅（5モジュール × 8）
#define HEIGHT      8

#define RightToLeft 0
#define LeftToRight 1

class TM1640MatrixChain {
public:
    TM1640MatrixChain(uint8_t sclk, const uint8_t dinPins[], uint8_t moduleCount,
                      uint8_t displayDuty = TM1640_DUTY_14_16);

    ~TM1640MatrixChain();

    void SetDuty(uint8_t duty);
    void clearExtFrame();
    void drawDotExt(int x, int y, uint8_t color);
    void flushFrameExtToDisplay();
    void appendChar(char ch, int x, int y, uint8_t color);
    void Scroll(uint8_t dir);
    void ScrollString(const char *dispStr, uint8_t color, uint16_t scrollDelayMs = 50);

private:
    TM1640** _modules;
    uint8_t  _moduleCount;
    uint16_t _extWidth;
    uint8_t* _frameExtR;
    uint8_t* _frameExtG;
    uint8_t  DisplayDuty;
};

#endif // TM1640MATRIXCHAIN_H
