#include <Arduino.h>
#include "TM1640.h"
#include "font5x7.h"

// =========================
// 定義
// =========================
#define MODULE_COUNT 5
#define WIDTH 40
#define HEIGHT 8

#define COLOR_NONE   0
#define COLOR_RED    1
#define COLOR_GREEN  2
#define COLOR_ORANGE 3

// 拡張バッファ：表示領域の左右にそれぞれ8列ずつ余白を持たせる。
// 必要範囲：x = -8 .. 48 までサポートする => 長さ = 48 - (-8) + 1 = 57
#define LEFT_PAD 8
#define EXT_WIDTH 57  // indices 0 .. 56 cover x = -8 .. 48

// =========================
// TM1640 接続（変更不可）
// =========================
TM1640 module1(2, A0);
TM1640 module2(3, A1);
TM1640 module3(4, A2);
TM1640 module4(5, A3);
TM1640 module5(6, A4);

TM1640 LED[MODULE_COUNT] = {
  module1, module2, module3, module4, module5
};

// =========================
// フレームバッファ（赤・緑）
// 表示用は WIDTH（40）だが、描画用に拡張バッファ EXT_WIDTH を保持
// frameExtR/G[x_ext] の x_ext = 0..EXT_WIDTH-1
// 実表示に出すときは window = [LEFT_PAD .. LEFT_PAD+WIDTH-1]
// =========================
uint8_t frameExtR[EXT_WIDTH];
uint8_t frameExtG[EXT_WIDTH];

uint8_t DisplayDuty = TM1640_DUTY_14_16;

// =========================
// バッファ操作
// =========================
void clearExtFrame() {
  memset(frameExtR, 0, sizeof(frameExtR));
  memset(frameExtG, 0, sizeof(frameExtG));
}

// x: 描画座標（表示基準の論理座標） -8 .. 48
// y: 0..7  (0 = top)
// color: COLOR_NONE / COLOR_RED / COLOR_GREEN / COLOR_ORANGE
// 内部では extIndex = x + LEFT_PAD を使う
void drawDotExt(int x, int y, uint8_t color) {
  if (y < 0 || y >= HEIGHT) return;
  int extX = x + LEFT_PAD;
  if (extX < 0 || extX >= EXT_WIDTH) return;
  uint8_t mask = (1u << (7 - y));
  switch (color) {
    case COLOR_NONE:
      frameExtR[extX] &= ~mask;
      frameExtG[extX] &= ~mask;
      break;
    case COLOR_RED:
      frameExtR[extX] |= mask;
      frameExtG[extX] &= ~mask;
      break;
    case COLOR_GREEN:
      frameExtR[extX] &= ~mask;
      frameExtG[extX] |= mask;
      break;
    case COLOR_ORANGE:
      frameExtR[extX] |= mask;
      frameExtG[extX] |= mask;
      break;
    default:
      break;
  }
}


void flushFrameExtToDisplay() {
  const uint8_t visibleBase = LEFT_PAD;  // = 8 → x=0 に相当
  for (uint8_t m = 0; m < MODULE_COUNT; m++) {
    uint8_t buff[16];
    uint8_t fb = visibleBase + m * 8;  // このモジュールの左端列（0,8,16,24,32）
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t src = fb + (7 - i);    // 逆順補正
      buff[i]     = frameExtR[src];  // R-plane
      buff[i + 8] = frameExtG[src];  // G-plane
    }
    LED[m].DrawAddrInc(buff, 16);      // 1 モジュールへ 16 バイト送信
  }
}


// =========================
// font5x7 を拡張フレーム座標に描画する
// - yOffset を 1 に固定（"全部 y=1"）
// - 描画は OR（上書き）
// =========================
void drawCharToExtAt(int x, const uint8_t *bitmap, uint8_t color) {
  // top y = 1 per instruction
  const int yOffset = 1;
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t colBits = bitmap[col]; // bitmap bit0..bit6 used (top at bit0)
    // font5x7 stored as vertical bits with LSB = top? The font provided uses each byte as vertical column with top in bit0 or documentation said top ignored highest bit.
    // In earlier code we treated bit j (0..6) as mapping to y=j top->bottom. We'll use that mapping here.
    for (uint8_t bit = 0; bit < 7; bit++) {
      if (colBits & (1u << bit)) {
        int yy = yOffset + bit; // 1..7 (fits into 0..7)
        drawDotExt(x + col, yy, color);
      } else {
        // leave as-is (do not clear) — caller should clear ext buffer before placing new char if desired
      }
    }
  }
}

#define RightToLeft 0
#define LeftToRight 1

void ScrollString(const char *dispStr, uint8_t color, uint16_t scrollDelayMs) {
    const int startX = 41; // 右端スタート

    // 文字列を順に処理
    for (int i = 0; dispStr[i] != '\0'; i++) {
        appendChar(dispStr[i], startX, 1, color);  // 右端に文字を描画

        // 文字幅分（5列 + 1スペース = 6列）だけスクロール
        for (uint8_t j = 0; j < 6; j++) {
            Scroll(RightToLeft);
            flushFrameExtToDisplay(); // 表示更新
            delay(scrollDelayMs);
        }
    }

    // 文字列終了後、画面内に残った文字を左に完全に流す（40列分）
    //for (uint8_t j = 0; j < WIDTH; j++) {
    //    Scroll(RightToLeft);
    //    flushFrameExtToDisplay();
    //    delay(scrollDelayMs);
    //}
}


void appendChar(char ch, int x, int y, uint8_t color)
{
    // ASCII → UTF-8 1文字に変換
    char key[2] = { ch, 0 };

    // フォントインデックスを取得
    uint8_t idx = getFontIndex(key);
    if (idx == 0xFF) {
        return; // 未定義文字は無視
    }

    const FontChar &fc = font5x7[idx];

    // bitmap[5] をそのまま描画
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = fc.bitmap[col];

        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                drawDotExt(x + col, y + row, color);
            }
        }
    }
}




void Scroll(uint8_t dir) {
    if (dir == RightToLeft) {
        // 左に1列シフト
        for (int i = 0; i < EXT_WIDTH - 1; i++) {
            frameExtR[i] = frameExtR[i + 1];
            frameExtG[i] = frameExtG[i + 1];
        }
        frameExtR[EXT_WIDTH - 1] = 0; // 新しい右端列は空白
        frameExtG[EXT_WIDTH - 1] = 0;
    } else if (dir == LeftToRight) {
        // 右に1列シフト
        for (int i = EXT_WIDTH - 1; i > 0; i--) {
            frameExtR[i] = frameExtR[i - 1];
            frameExtG[i] = frameExtG[i - 1];
        }
        frameExtR[0] = 0; // 新しい左端列は空白
        frameExtG[0] = 0;
    }
}




void setup() {
  Serial.begin(230400);

  for (uint8_t i = 0; i < MODULE_COUNT; i++) {
    LED[i].SetDuty(DisplayDuty);
    delay(10);
  }

  clearExtFrame();     // ★まずバッファを確実に空にする


ScrollString("Hello well come to", COLOR_GREEN, 30);
ScrollString(" TM1640", COLOR_ORANGE, 30);
ScrollString(" test. This librally is more than", COLOR_GREEN, 30);
ScrollString(" 100", COLOR_RED, 30);
ScrollString(" FPS       ", COLOR_GREEN, 30);

}


// loop: 既存の描画シーケンス（行優先／列優先） + 文字スクロール処理を追加

const uint8_t ColoeNum[3] {COLOR_RED,COLOR_GREEN,COLOR_ORANGE};
uint8_t ClorCount = 0;
void loop() {
  const uint16_t stepDelay_us = 1; // ユーザ要求（サンプル描画は 5ms/コマ）
  const uint32_t scrollDelay = 40; // 文字スクロールは 300us/コマ（要求）

  // 既存サンプル描画（行優先／列優先）
  clearExtFrame();
  // 1) 行優先 (y 外側, x 内側)
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      drawDotExt(x, y, COLOR_RED);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      drawDotExt(x, y, COLOR_GREEN);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      drawDotExt(x, y, COLOR_ORANGE);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      drawDotExt(x, y, COLOR_NONE);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }

  // 2) 列優先 (x 外側, y 内側)
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      drawDotExt(x, y, COLOR_RED);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      drawDotExt(x, y, COLOR_GREEN);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      drawDotExt(x, y, COLOR_ORANGE);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      drawDotExt(x, y, COLOR_NONE);
      flushFrameExtToDisplay();
      delayMicroseconds(stepDelay_us);
    }
  }

  ScrollString("0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ", ColoeNum[ClorCount], 12);
  ScrollString("abcdefghijilmnopqrstuvwxyz <>?*+()/:;=       ", ColoeNum[ClorCount], 12);

  ClorCount ++;
  if(ClorCount > 2) ClorCount = 0;

  delay(200);

  ScrollString("Hello well come to", COLOR_GREEN, 9);
  ScrollString(" TM1640", COLOR_ORANGE, 9);
  ScrollString(" test. This librally is more than", COLOR_GREEN, 9);
  ScrollString("100", COLOR_RED, 9);
  ScrollString("FPS       ", COLOR_GREEN, 9);
}
