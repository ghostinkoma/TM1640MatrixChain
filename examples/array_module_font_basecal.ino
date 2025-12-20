#include <Arduino.h>
#include "TM1640MatrixChain.h"

//LED modlue settings
#define MODULES_NUMBER 5
#define MODULES_GPIO_SCLK 2
#define MODULES_GPIO_DIN {A0, A1, A2, A3, A6}

// DIN 配列
const uint8_t dinPins[MODULES_NUMBER] = MODULES_GPIO_DIN;
TM1640MatrixChain TM1640_Matrix(MODULES_GPIO_SCLK, dinPins, MODULES_NUMBER);


const uint8_t ColoeNum[3] = { COLOR_RED, COLOR_GREEN, COLOR_ORANGE };
uint8_t ClorCount = 0;

void setup() {
    TM1640_Matrix.SetDuty(TM1640_DUTY_14_16);
    delay(10);
    TM1640_Matrix.clearExtFrame();
    delay(1);

}


void loop() {
    const uint16_t stepDelay_us = 1;
    const uint32_t scrollDelay = 40;
    for(uint8_t cycle =0 ;cycle < 3; cycle ++){
      TM1640_Matrix.clearExtFrame();
      for (uint8_t y = 0; y < HEIGHT; y++) {
          for (uint8_t x = 0; x < WIDTH; x++) {
              TM1640_Matrix.drawDotExt(x, y, ColoeNum[ClorCount]);
              TM1640_Matrix.flushFrameExtToDisplay();
          }
      }
      TM1640_Matrix.clearExtFrame();
      ClorCount++;
      if (ClorCount > 2) ClorCount = 0;
    }




    // 2) 列優先描画
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = 0; y < HEIGHT; y++) {
            TM1640_Matrix.drawDotExt(x, y, COLOR_RED);
            TM1640_Matrix.flushFrameExtToDisplay();
            delayMicroseconds(stepDelay_us);
        }
    }
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = 0; y < HEIGHT; y++) {
            TM1640_Matrix.drawDotExt(x, y, COLOR_GREEN);
            TM1640_Matrix.flushFrameExtToDisplay();
            delayMicroseconds(stepDelay_us);
        }
    }
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = 0; y < HEIGHT; y++) {
            TM1640_Matrix.drawDotExt(x, y, COLOR_ORANGE);
            TM1640_Matrix.flushFrameExtToDisplay();
            delayMicroseconds(stepDelay_us);
        }
    }
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = 0; y < HEIGHT; y++) {
            TM1640_Matrix.drawDotExt(x, y, COLOR_NONE);
            TM1640_Matrix.flushFrameExtToDisplay();
            delayMicroseconds(stepDelay_us);
        }
    }

    // 文字列スクロール
    TM1640_Matrix.ScrollString("0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ", ColoeNum[ClorCount], 12);
    TM1640_Matrix.ScrollString("abcdefghijilmnopqrstuvwxyz <>?*+()/:;=       ", ColoeNum[ClorCount], 12);
    ClorCount++;
    if (ClorCount > 2) ClorCount = 0;

    delay(200);

    TM1640_Matrix.ScrollString("Hello well come to", COLOR_GREEN, 9);
    TM1640_Matrix.ScrollString(" TM1640", COLOR_ORANGE, 9);
    TM1640_Matrix.ScrollString(" test. This librally is more than", COLOR_GREEN, 9);
    TM1640_Matrix.ScrollString("100", COLOR_RED, 9);
    TM1640_Matrix.ScrollString("FPS       ", COLOR_GREEN, 9);
}

