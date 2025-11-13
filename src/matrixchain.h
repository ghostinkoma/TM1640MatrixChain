#pragma once
#include <Arduino.h>
#include "TM1640.h"

// --- モジュールの物理回転方向 ---
enum TM1640Rotation : uint16_t {
  TM1640_ANGLE_0   = 0,
  TM1640_ANGLE_90  = 90,
  TM1640_ANGLE_180 = 180,
  TM1640_ANGLE_270 = 270
};

// --- カラー定義（マジックナンバー排除） ---
#define COLOR_OFF     0u
#define COLOR_RED     1u
#define COLOR_GREEN   2u
#define COLOR_ORANGE  3u   // RED + GREEN 同時点灯

// --- 1モジュール単位情報 ---
struct TM1640Unit {
  TM1640* module;           // 実モジュールインスタンス
  TM1640Rotation rotation;  // 実装角度（0,90,180,270）
};

// --- マトリクスチェイン全体管理クラス ---
class TM1640MatrixChain {
public:
  TM1640MatrixChain(TM1640Unit* units, uint8_t rows, uint8_t cols);
  ~TM1640MatrixChain();

  // --- ユーザAPI ---
  void drawPixel(uint16_t x, uint16_t y, uint8_t color);
  void clear();
  void update();

  uint16_t width() const { return _width; }
  uint16_t height() const { return _height; }

private:
  TM1640Unit* _units;
  uint8_t _rows;
  uint8_t _cols;
  uint16_t _width;   // cols * 8
  uint16_t _height;  // rows * 8

  // バイカラー用シャドウGRAM
  uint8_t* _red;
  uint8_t* _green;

  // 内部座標マッピング
  void _mapToModule(uint16_t x, uint16_t y,
                    uint8_t& moduleIndex,
                    uint8_t& localX,
                    uint8_t& localY);
};

