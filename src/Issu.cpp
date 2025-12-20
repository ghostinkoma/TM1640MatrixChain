void TM1640MatrixChain::drawDotExt(int x, int y, uint8_t color) {
    if (x < 0 || x >= _extWidth || y < 0 || y >= HEIGHT) return;

    int byteIndex = x;
    uint8_t bit = (1 << y);

    // まず両方のビットをクリア（重要！）
    _frameExtR[byteIndex] &= ~bit;
    _frameExtG[byteIndex] &= ~bit;

    // その後、指定色に応じてセット
    if (color == COLOR_RED || color == COLOR_ORANGE) {
        _frameExtR[byteIndex] |= bit;
    }
    if (color == COLOR_GREEN || color == COLOR_ORANGE) {
        _frameExtG[byteIndex] |= bit;
    }
}
