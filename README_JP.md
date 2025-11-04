```markdown
# TM1640MatrixChain

TM1640 ベースの **バイカラー 8x8 LED マトリクス** 用高性能 Arduino ライブラリ。  
**チェーン接続**、**回転**、**U8g2 フォント（日本語対応）**、**スムーズスクロール** 対応。

---

## 機能

- 赤/緑/オレンジ/消灯
- 最大255モジュールのチェーン（グローバル座標）
- モジュールごとの回転（0°/90°/270°）
- UTF-8 テキスト描画（日本語対応）
- イージング付きスムーズスクロール（60FPS）
- 高性能モード（TM16xx 風レジスタ操作）
- メモリ安全設計

---

## 依存ライブラリ

- [U8g2](https://github.com/olikraus/u8g2)

---

## インストール

1. ZIP でダウンロード or `git clone`
2. `Arduino/libraries/TM1640MatrixChain` に配置
3. Arduino IDE 再起動

---

## 使い方

```cpp
#include "matrixchain.h"

TM1640* mods[8];
MatrixChain chain(mods, 8, HORIZONTAL);

void setup() {
  for (int i = 0; i < 8; i++) mods[i] = new TM1640(3+i, 2);
  chain.initialize_all();
  chain.beginU8g2Virtual();
  chain.setFont(u8g2_font_japanese1_tf);
  chain.drawUTF8(0, 0, "こんにちは", ORANGE);
  chain.start_scroll_animation(LEFT, 0, 0, 0, 0, 1500);
}

void loop() {
  chain.update_scroll_animation();
  delay(16);
}