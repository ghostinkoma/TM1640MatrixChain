#include "TM1640MatrixChain.h"

TM1640MatrixChain::TM1640MatrixChain(uint8_t sclk, const uint8_t dinPins[], uint8_t moduleCount, uint8_t displayDuty)
    : _moduleCount(moduleCount), DisplayDuty(displayDuty)
{
    _extWidth = LEFT_PAD + WIDTH + LEFT_PAD;

    // 拡張フレームバッファ確保
    _frameExtR = new uint8_t[_extWidth]();
    _frameExtG = new uint8_t[_extWidth]();

    // TM1640 インスタンス配列を確保して初期化
    _modules = new TM1640*[_moduleCount];
    for (uint8_t i = 0; i < _moduleCount; i++) {
        _modules[i] = new TM1640(sclk, dinPins[i]);
        _modules[i]->SetDuty(DisplayDuty);
    }
}

TM1640MatrixChain::~TM1640MatrixChain()
{
    if (_modules) {
        for (uint8_t i = 0; i < _moduleCount; i++) {
            delete _modules[i];
        }
        delete[] _modules;
    }
    delete[] _frameExtR;
    delete[] _frameExtG;
}

void TM1640MatrixChain::SetDuty(uint8_t duty)
{
    DisplayDuty = duty;
    for (uint8_t i = 0; i < _moduleCount; i++) {
        _modules[i]->SetDuty(DisplayDuty);
    }
}

void TM1640MatrixChain::clearExtFrame()
{
    memset(_frameExtR, 0, _extWidth);
    memset(_frameExtG, 0, _extWidth);
}

void TM1640MatrixChain::drawDotExt(int x, int y, uint8_t color)
{
    if (y < 0 || y >= HEIGHT) return;
    int extX = x + LEFT_PAD;
    if (extX < 0 || extX >= (int)_extWidth) return;

    uint8_t mask = (1u << (7 - y));
    switch (color) {
        case COLOR_NONE:
            _frameExtR[extX] &= ~mask;
            _frameExtG[extX] &= ~mask;
            break;
        case COLOR_RED:
            _frameExtR[extX] |= mask;
            _frameExtG[extX] &= ~mask;
            break;
        case COLOR_GREEN:
            _frameExtR[extX] &= ~mask;
            _frameExtG[extX] |= mask;
            break;
        case COLOR_ORANGE:
            _frameExtR[extX] |= mask;
            _frameExtG[extX] |= mask;
            break;
    }
}

void TM1640MatrixChain::flushFrameExtToDisplay()
{
    const uint8_t visibleBase = LEFT_PAD;
    for (uint8_t m = 0; m < _moduleCount; m++) {
        uint8_t buff[16];
        uint8_t fb = visibleBase + m * MODULE_RESO;
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t src = fb + (7 - i);
            buff[i]     = _frameExtR[src];
            buff[i + 8] = _frameExtG[src];
        }
        _modules[m]->DrawAddrInc(buff, 16);
    }
}

void TM1640MatrixChain::appendChar(char ch, int x, int y, uint8_t color)
{
    char key[2] = { ch, 0 };
    uint8_t idx = getFontIndex(key);
    if (idx == 0xFF) return;

    const FontChar &fc = font5x7[idx];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = fc.bitmap[col];
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                drawDotExt(x + col, y + row, color);
            }
        }
    }
}

void TM1640MatrixChain::Scroll(uint8_t dir)
{
    if (dir == RightToLeft) {
        for (int i = 0; i < (int)_extWidth - 1; i++) {
            _frameExtR[i] = _frameExtR[i + 1];
            _frameExtG[i] = _frameExtG[i + 1];
        }
        _frameExtR[_extWidth - 1] = 0;
        _frameExtG[_extWidth - 1] = 0;
    } else if (dir == LeftToRight) {
        for (int i = _extWidth - 1; i > 0; i--) {
            _frameExtR[i] = _frameExtR[i - 1];
            _frameExtG[i] = _frameExtG[i - 1];
        }
        _frameExtR[0] = 0;
        _frameExtG[0] = 0;
    }
}

void TM1640MatrixChain::ScrollString(const char *dispStr, uint8_t color, uint16_t scrollDelayMs)
{
    const int startX = 41;
    for (int i = 0; dispStr[i] != '\0'; i++) {
        appendChar(dispStr[i], startX, 1, color);
        for (uint8_t j = 0; j < 6; j++) {  // 文字幅5 + 1スペース
            Scroll(RightToLeft);
            flushFrameExtToDisplay();
            delay(scrollDelayMs);
        }
    }
}
