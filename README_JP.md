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

# TM1640MatrixChain – World Fastest TM1640 Matrix Driver

Copyright © 2025 ghostinkoma (ghostinkoma@gmail.com)

This work is licensed under  
**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License**  
(CC BY-NC-SA 4.0)

You are free to:
- Share — copy and redistribute the material in any medium or format
- Adapt — remix, transform, and build upon the material

Under the following terms:
- Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
- NonCommercial — You may not use the material for commercial purposes.
- ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.

Full license text: https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode  
Human-readable summary: https://creativecommons.org/licenses/by-nc-sa/4.0/

商用利用をご希望の方は、以下のメールアドレスまでご連絡ください。  
For commercial licensing inquiries: ghostinkoma@gmail.com
