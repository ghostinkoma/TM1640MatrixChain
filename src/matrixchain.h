#ifndef TM1640_MATRIX_CHAIN_H
#define TM1640_MATRIX_CHAIN_H

#include <Arduino.h>
#include "TM1640.h"

// =============================================================
//  定数定義
// =============================================================

// --- 回転角度 ---
#define ANGLE_0     0
#define ANGLE_90    1
#define ANGLE_180   2
#define ANGLE_270   3

// --- カラー定義 ---
#define COLOR_NONE  0
#define COLOR_RED   1
#define COLOR_GREEN 2
#define COLOR_ORANGE 3   // RED + GREEN

// =============================================================
//  モジュール構造体
// =============================================================
struct TM1640ModuleInfo {
    TM1640* dev;     // 対応するTM1640インスタンス
    uint8_t rotation; // ANGLE_0〜270
};

// =============================================================
//  TM1640MatrixChain クラス宣言
// =============================================================
class TM1640MatrixChain {
public:
    // ---------------------------------------------------------
    // コンストラクタ／デストラクタ
    // ---------------------------------------------------------
    TM1640MatrixChain(TM1640ModuleInfo* modules,
                      uint8_t moduleCount,
                      uint8_t rows,
                      uint8_t cols);
    ~TM1640MatrixChain();

    // ---------------------------------------------------------
    // 初期化・状態制御
    // ---------------------------------------------------------
    void begin();             // メモリ初期化など
    void clearVirtual();      // 仮想VRAM全消去
    void clearDisplay();      // 表示領域のみ消去

    // ---------------------------------------------------------
    // 描画操作（仮想バッファ側）
    // ---------------------------------------------------------
    void drawPixelVirtual(int16_t x, int16_t y, uint8_t color);

    // 仮想→表示バッファ転送（必要領域のみコピー）
    void blitToDisplay();

    // 実ハードウェア更新
    void update();

    // ---------------------------------------------------------
    // 情報取得
    // ---------------------------------------------------------
    uint16_t width()  const { return _virtWidth; }
    uint16_t height() const { return _virtHeight; }

private:
    // ---------------------------------------------------------
    // 内部処理
    // ---------------------------------------------------------
    uint8_t _mapToModule(int16_t x, int16_t y,
                         int16_t& localX, int16_t& localY) const;

    // ---------------------------------------------------------
    // メンバ変数
    // ---------------------------------------------------------
    TM1640ModuleInfo* _modules;
    uint8_t  _moduleCount;

    uint8_t  _rows;           // モジュール縦配置数
    uint8_t  _cols;           // モジュール横配置数

    // 仮想バッファ（周囲±1マージン付き）
    uint16_t _virtWidth;      // cols * 8 + 2
    uint16_t _virtHeight;     // rows * 8 + 2
    uint8_t* _vramRed;
    uint8_t* _vramGreen;

    // 実表示バッファ
    uint16_t _dispWidth;      // cols * 8
    uint16_t _dispHeight;     // rows * 8
    uint8_t* _dispRed;
    uint8_t* _dispGreen;
};

#endif // TM1640_MATRIX_CHAIN_H


