#pragma once
#include <Arduino.h>
#include "TM1640.h"

class TM1640MatrixChain {
public:
  // --- コンストラクタ ---
  TM1640MatrixChain(TM1640** modules, uint8_t moduleCount);

  // --- デストラクタ ---
  ~TM1640MatrixChain() = default;

private:
  // --- メンバ変数 ---
  TM1640** _modules = nullptr;   // TM1640インスタンスポインタ配列
  uint8_t  _count   = 0;         // モジュール数
  uint8_t  _duty    = TM1640_DUTY_10_16; // デフォルト輝度Duty
};
