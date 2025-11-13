#pragma once
#include <Arduino.h>
#include "TM1640.h"

// --- モジュール物理回転方向を定義 ---
enum TM1640Rotation : uint16_t {
  TM1640_ANGLE_0   = 0,   // 通常方向
  TM1640_ANGLE_90  = 90,  // 時計回り90°
  TM1640_ANGLE_180 = 180, // 180°
  TM1640_ANGLE_270 = 270  // 時計回り270°
};

// --- 1モジュール単位の情報 ---
struct TM1640Unit {
  TM1640* module;           // 実体ポインタ
  TM1640Rotation rotation;  // 実装角度
};

// --- モジュールチェイン管理クラス ---
class TM1640MatrixChain {
public:
  // コンストラクタ
  TM1640MatrixChain(TM1640Unit* units, uint8_t unitCount);

  // デストラクタ
  ~TM1640MatrixChain() = default;

private:
  TM1640Unit* _units = nullptr;  // モジュール配列
  uint8_t     _count = 0;        // モジュール数
  uint8_t     _duty  = TM1640_DUTY_10_16; // 輝度デフォルト
};

