// TM1640.cpp
#include "TM1640.h"

TM1640::TM1640(uint8_t gpio_sclk, 
               uint8_t gpio_din, 
               int frequency_khz)
  : _sclk_pin(gpio_sclk),
    _din_pin(gpio_din),
    _frequency_khz(frequency_khz){
  // frequency_khz が 0 または負値ならデフォルト値（CPUクロック由来）を使用
  if (_frequency_khz <= 0) {
    _frequency_khz = (int)(TM1640_FCPU_HZ / 1000UL);  // Hz → kHz 換算
  }

  pinMode(_sclk_pin, OUTPUT);
  pinMode(_din_pin, OUTPUT);

  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);
}


int TM1640::Display(bool on_off){
  // デフォルト duty（必要に応じて private メンバで管理可）
  static uint8_t currentDuty = 0x07;  // 14/16 duty（データシート推奨の最大輝度）

  uint8_t cmd = 0x80u;  // Display Control Command base

  if (on_off) {
    cmd |= 0x08u;  // bit3 = D = 1 → Display ON
  }

  cmd |= (currentDuty & 0x07u);  // Duty bits設定 (C2–C0)

  char sendBuf[1];
  sendBuf[0] = static_cast<char>(cmd);

  return _sendChars(sendBuf, false, false, 1);
}


int TM1640::Test(bool start){
  // start=true → Display Test Mode (0x01)
  // start=false → Normal Operation (0x00)
  uint8_t cmd = start ? 0x01 : 0x00;

  // コマンド送信用バッファ
  char sendBuf[1];
  sendBuf[0] = cmd;

  // TM1640 データシート準拠: コマンド送信 (1バイト)
  int result = _sendChars(sendBuf, false, false, 1);

  // 念のためラインをHighでアイドルに戻す
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);

  return result;
}


int TM1640::SetDuty(uint8_t duty){
  if (duty > 0x07)
    return TM1640_NG;  // データシート範囲外ならエラー

  // Display ON固定でDuty設定（ユーザーがDisplay()で制御しているなら引数追加も可）
  uint8_t cmd = 0x88 | (duty & 0x07);  // 0b10001000 | duty

  // コマンドを送信
  char sendBuf[1];
  sendBuf[0] = static_cast<char>(cmd);

  // 1バイト送信、ストップビット不要（Display Control Commandは単独命令）
  return _sendChars(sendBuf, false, false, 1);
}


int TM1640::DrawAddrInc(const uint8_t *chars, uint16_t len)
{
  if (!chars || len == 0)
    return TM1640_NG;
  if (_frequency_khz <= 0)
    return TM1640_NG;

  // --- 1) データ設定コマンド: アドレス自動加算モード (0x40) ---
  {
    char cmdDataMode[1];
    cmdDataMode[0] = static_cast<char>(0x40);  // アドレス自動加算モード
    int res = _sendChars(cmdDataMode, false, true, 1);
    if (res != TM1640_OK) return res;
  }

  // --- 2) 書き込み開始アドレス (通常は0xC0 + 0x00) ---
  {
    char addrCmd[1];
    addrCmd[0] = static_cast<char>(0xC0);  // 開始アドレス = 0
    int res = _sendChars(addrCmd, false, false, 1);
    if (res != TM1640_OK) return res;
  }

  // --- 3) データ送信 (自動アドレス加算: STOPなしで連続送信) ---
  int res = _sendChars(reinterpret_cast<char*>(const_cast<uint8_t*>(chars)),
                       false, true, len);
  if (res != TM1640_OK) return res;

  return TM1640_OK;
}


int TM1640::DrawAddrFix(uint8_t addr, const uint8_t *chars, uint16_t len)
{
  if (!chars || len == 0)
    return TM1640_NG;
  if (addr > TM1640_ADDR_MAX)
    return TM1640_NG;
  if (_frequency_khz <= 0)
    return TM1640_NG;

  // --- 1) データ設定コマンド: 固定アドレスモード (0x44) ---
  {
    char cmdDataMode[1];
    cmdDataMode[0] = static_cast<char>(0x44);  // 固定アドレスモード
    int res = _sendChars(cmdDataMode, false, true, 1);
    if (res != TM1640_OK) return res;
  }

  // --- 2) 書き込み先アドレス指定 ---
  {
    char addrCmd[1];
    addrCmd[0] = static_cast<char>(0xC0u | (addr & TM1640_ADDR_MASK));
    int res = _sendChars(addrCmd, false, true, 1);
    if (res != TM1640_OK) return res;
  }

  // --- 3) データ送信 (固定アドレスなので1バイトごと STOP 発生) ---
  for (uint16_t i = 0; i < len; ++i) {
    char dataByte[1];
    dataByte[0] = static_cast<char>(chars[i]);

    // 固定アドレスなので毎回 STOP 条件を伴う
    int res = _sendChars(dataByte, false, true, 1);
    if (res != TM1640_OK) return res;
  }

  return TM1640_OK;
}


int TM1640::_sendChars(
  char * sendChars,
  bool addStopBitAfter1stChar,
  bool addStopBitBeforLastChar,
  int charLength){
  if (!sendChars || charLength <= 0)
    return TM1640_NG;

  if (_frequency_khz <= 0)
    return TM1640_NG;

  for (int i = 0; i < charLength; ++i) {
    uint8_t data = (uint8_t)sendChars[i];
    bool addStopAfter = (addStopBitAfter1stChar && i == 0);
    bool addStopBefore = (addStopBitBeforLastChar && i == (charLength - 1));
    
   if(i == charLength -1){
      if(addStopBitBeforLastChar){
        _sendBit(1);
      }
    }

    for (int j = 0; j < 8; j++) {
      bool bitVal = (data & 0x01);  // LSB 抽出
      _sendBit(bitVal);             // この関数が実際の物理ライン操作
      data >>= 1;                   // LSB-first のため右シフト
    }

    if(i==0){
      if(addStopBitAfter1stChar){
        _sendBit(1);
      }
    }
    
  }

  // 通信終了時、ラインを安定化
  digitalWrite(_sclk_pin, HIGH);
  digitalWrite(_din_pin, HIGH);

  return TM1640_OK;
}

void TM1640::_sendBit(bool bitVal){
  digitalWrite(_din_pin, bitVal ? HIGH : LOW);

  // 周波数に基づいた1周期の半分のディレイを設定
  if (_frequency_khz <= 0) return;  // 周波数が未設定なら即終了（上位が責任を持つ）

  uint32_t halfPeriod_us = 500U / static_cast<uint32_t>(_frequency_khz);
  if (halfPeriod_us < 1U) halfPeriod_us = 1U;

  // SCLK Low → High トグルで1bitラッチ
  digitalWrite(_sclk_pin, LOW);
  delayMicroseconds(halfPeriod_us);
  digitalWrite(_sclk_pin, HIGH);
  delayMicroseconds(halfPeriod_us);
}







