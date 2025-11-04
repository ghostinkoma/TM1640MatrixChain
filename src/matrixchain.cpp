// matrixchain.cpp - Implementation of chained TM1640 control
#include "matrixchain.h"
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include <U8g2lib.h> 

MatrixChain::MatrixChain(TM1640* modules[], uint8_t count, Orientation orient, uint8_t layoutCols)
    : _modules(modules), _count(count), _orient(orient) {
    if (layoutCols == 0 || layoutCols > count) _layoutCols = count;
    else _layoutCols = layoutCols;

    _ext_width = (_module_width + 2) * _count;
    _ext_plane_red = (uint8_t*)malloc(_ext_height * _ext_width);
    _ext_plane_green = (uint8_t*)malloc(_ext_height * _ext_width);
    if (_ext_plane_red) memset(_ext_plane_red, 0, _ext_height * _ext_width);
    if (_ext_plane_green) memset(_ext_plane_green, 0, _ext_height * _ext_width);
}

MatrixChain::~MatrixChain() {
    if (_ext_plane_red) { free(_ext_plane_red); _ext_plane_red = nullptr; }
    if (_ext_plane_green) { free(_ext_plane_green); _ext_plane_green = nullptr; }
    if (_temp_buffer) { free(_temp_buffer); _temp_buffer = nullptr; }
}

void MatrixChain::initialize_all() {
    for (uint8_t i = 0; i < _count; ++i) {
        if (_modules[i]) _modules[i]->initialize();
    }
}

void MatrixChain::set_duty(uint8_t val) {
    for (uint8_t i = 0; i < _count; ++i) {
        if (_modules[i]) _modules[i]->set_duty(val);
    }
}

bool MatrixChain::map_global_to_module(uint16_t gx, uint8_t gy, uint8_t& moduleIndex, uint8_t& localX, uint8_t& localY) const {
    uint16_t totalW = total_width();
    uint16_t totalH = total_height();
    if (gx >= totalW || gy >= totalH) return false;

    uint16_t cols = (_orient == HORIZONTAL) ? _layoutCols : 1;
    uint16_t col = gx / _module_width;
    uint16_t row = gy / _module_height;
    uint16_t idx = row * cols + col;
    if (idx >= _count) return false;

    moduleIndex = idx;
    TM1640* m = _modules[moduleIndex];
    if (!m) return false;

    uint8_t lx0 = gx % _module_width;
    uint8_t ly0 = gy % _module_height;
    uint16_t rot = m->get_rotation();
    uint8_t lx = lx0, ly = ly0;

    if (rot == 90) { lx = ly0; ly = 7 - lx0; }
    else if (rot == 270) { lx = 7 - ly0; ly = lx0; }

    localX = lx; localY = ly;
    return true;
}

void MatrixChain::draw_dot_global(uint16_t globalX, uint8_t globalY, uint8_t color) {
    uint8_t mi, lx, ly;
    if (map_global_to_module(globalX, globalY, mi, lx, ly)) {
        if (_modules[mi]) _modules[mi]->draw_dot_local(lx, ly, color);
    }
}

void MatrixChain::drawbin_global(const uint8_t bytes[], uint32_t byteLen, uint8_t color) {
    uint32_t expected = _count * 8;
    if (byteLen < expected) return;

    uint32_t pos = 0;
    for (uint8_t i = 0; i < _count; ++i) {
        if (!_modules[i]) continue;
        const uint8_t* chunk = &bytes[pos];
        pos += 8;
        uint8_t tmp[8];
        memcpy(tmp, chunk, 8);
        uint16_t rot = _modules[i]->get_rotation();
        if (rot == 90) transpose_for_rotation90(tmp, tmp, 8, 8);
        else if (rot == 270) transpose_for_rotation270(tmp, tmp, 8, 8);
        _modules[i]->drawbin_local(tmp, 8, color);
    }
}

void MatrixChain::get_gram_linear(uint8_t out[], uint32_t out_len) const {
    uint32_t needed = _count * 16;
    if (out_len < needed) return;

    uint32_t pos = 0;
    for (uint16_t row = 0; row < total_height() / 8; ++row) {
        for (uint16_t col = 0; col < _layoutCols && (row * _layoutCols + col) < _count; ++col) {
            uint8_t idx = row * _layoutCols + col;
            if (_modules[idx]) {
                _modules[idx]->get_gram_local(&out[pos]);
                pos += 16;
            }
        }
    }
}

void MatrixChain::print_matrix(const char* str, uint16_t x, uint8_t y, FontType font_type, uint8_t color) {
    const uint8_t* font = (font_type == FONT_8X8) ? u8g2_font_maniac_tf : u8g2_font_5x7_tf;
    setFont(font);
    drawUTF8(x, y, str, color);
}

void MatrixChain::scroll(Direction dir, uint16_t start_x, uint8_t start_y, uint16_t end_x, uint8_t end_y) {
    start_scroll_animation(dir, start_x, start_y, end_x, end_y, 1000, 0);
}

void MatrixChain::icon_print(const uint8_t* icon_data, uint16_t start_column, uint8_t color) {
    if (!icon_data) return;
    for (uint8_t y = 0; y < 8; ++y) {
        uint8_t row = icon_data[y];
        for (uint8_t x = 0; x < 8; ++x) {
            if (row & (1 << x)) {
                draw_dot_global(start_column + x, y, color);
            }
        }
    }
}

void MatrixChain::force_update_all() {
    sync_extended_to_modules();
    for (uint8_t i = 0; i < _count; ++i) {
        if (_modules[i]) _modules[i]->force_update();
    }
}

uint16_t MatrixChain::total_width() const {
    uint16_t cols = (_orient == HORIZONTAL) ? _layoutCols : ((_count + _layoutCols - 1) / _layoutCols);
    return _module_width * cols;
}

uint16_t MatrixChain::total_height() const {
    uint16_t rows = (_orient == HORIZONTAL) ? ((_count + _layoutCols - 1) / _layoutCols) : _layoutCols;
    return _module_height * rows;
}

void MatrixChain::transpose_for_rotation90(const uint8_t src[], uint8_t dst[], uint8_t w, uint8_t h) const {
    memset(dst, 0, h);
    for (uint8_t y = 0; y < h; ++y) {
        for (uint8_t x = 0; x < w; ++x) {
            if (src[y] & (1 << x)) {
                dst[(w - 1 - x)] |= (1 << y);
            }
        }
    }
}

void MatrixChain::transpose_for_rotation270(const uint8_t src[], uint8_t dst[], uint8_t w, uint8_t h) const {
    memset(dst, 0, h);
    for (uint8_t y = 0; y < h; ++y) {
        for (uint8_t x = 0; x < w; ++x) {
            if (src[y] & (1 << x)) {
                dst[x] |= (1 << (h - 1 - y));
            }
        }
    }
}

void MatrixChain::sync_extended_to_modules() {
    if (!_ext_plane_red || !_ext_plane_green) return;

    for (uint8_t mod_idx = 0; mod_idx < _count; ++mod_idx) {
        uint16_t base_x = 1 + (mod_idx % _layoutCols) * 10;
        uint16_t base_y = 1 + (mod_idx / _layoutCols) * 10;

        uint8_t red_buf[8] = {0}, green_buf[8] = {0};

        for (uint8_t y = 0; y < 8; ++y) {
            for (uint8_t x = 0; x < 8; ++x) {
                uint16_t ext_x = base_x + x;
                uint16_t ext_y = base_y + y;
                uint16_t byte_idx = ext_y * _ext_width + (ext_x / 8);
                uint8_t bit_pos = ext_x % 8;

                if (byte_idx >= _ext_height * _ext_width) continue;

                if (_ext_plane_red[byte_idx] & (1 << bit_pos)) red_buf[y] |= (1 << x);
                if (_ext_plane_green[byte_idx] & (1 << bit_pos)) green_buf[y] |= (1 << x);
            }
        }

        if (_modules[mod_idx]) {
            uint16_t rot = _modules[mod_idx]->get_rotation();
            uint8_t tmp_red[8], tmp_green[8];
            memcpy(tmp_red, red_buf, 8);
            memcpy(tmp_green, green_buf, 8);

            if (rot == 90) {
                transpose_for_rotation90(tmp_red, red_buf, 8, 8);
                transpose_for_rotation90(tmp_green, green_buf, 8, 8);
            } else if (rot == 270) {
                transpose_for_rotation270(tmp_red, red_buf, 8, 8);
                transpose_for_rotation270(tmp_green, green_buf, 8, 8);
            }

            _modules[mod_idx]->drawbin_local(red_buf, 8, RED);
            _modules[mod_idx]->drawbin_local(green_buf, 8, GREEN);
        }
    }
}
void MatrixChain::beginU8g2Virtual() {
    if (_temp_buffer) return;

    uint16_t virt_w = total_width();
    uint16_t virt_h = total_height();
    uint16_t buf_size = (virt_w * virt_h + 7) / 8;
    _temp_buffer = (uint8_t*)malloc(buf_size);
    if (!_temp_buffer) return;

    u8g2_SetupBuffer(&u8g2_virt, _temp_buffer, virt_h, u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_SetFontMode(&u8g2_virt, 1);
    u8g2_SetFontDirection(&u8g2_virt, 0);
#if defined(u8g2_SetUserPtr)
    u8g2_SetUserPtr(&u8g2_virt, this);
#elif defined(u8g2_SetUserPointer)
    u8g2_SetUserPointer(&u8g2_virt, this);
#endif
}

void MatrixChain::setFont(const uint8_t* font) {
    if (!font || !_temp_buffer) return;
    u8g2_SetFont(&u8g2_virt, (const uint8_t*)font);
}

int16_t MatrixChain::drawUTF8(uint16_t x, uint8_t y, const char* str, uint8_t color) {
    if (!str || !_temp_buffer) return 0;
    _current_color = color;

    u8g2_ClearBuffer(&u8g2_virt);
    u8g2_DrawUTF8(&u8g2_virt, x, y, str);

    uint16_t virt_w = total_width();
    uint16_t virt_h = total_height();

    memset(_ext_plane_red, 0, _ext_height * _ext_width);
    memset(_ext_plane_green, 0, _ext_height * _ext_width);

    for (uint16_t py = 0; py < virt_h; ++py) {
        for (uint16_t px = 0; px < virt_w; ++px) {
            uint16_t buf_idx = (py * virt_w + px) / 8;
            uint8_t bit = 7 - (px % 8);
            if (_temp_buffer[buf_idx] & (1 << bit)) {
                uint16_t ext_x = 1 + px;
                uint16_t ext_y = 1 + py;
                uint16_t byte_idx = ext_y * _ext_width + (ext_x / 8);
                uint8_t bit_pos = ext_x % 8;
                if (byte_idx < _ext_height * _ext_width) {
                    if (color & RED) _ext_plane_red[byte_idx] |= (1 << bit_pos);
                    if (color & GREEN) _ext_plane_green[byte_idx] |= (1 << bit_pos);
                }
            }
        }
    }

    sync_extended_to_modules();
    return u8g2_GetUTF8Width(&u8g2_virt, str);
}

void MatrixChain::start_scroll_animation(Direction dir, uint16_t start_x, uint8_t start_y, uint16_t end_x, uint8_t end_y, uint16_t duration_ms, uint8_t pixels) {
    if (_scroll_active) return;

    _scroll_dir = dir;
    _scroll_sx = start_x; _scroll_sy = start_y;
    _scroll_ex = end_x ? end_x : total_width();
    _scroll_ey = end_y ? end_y : total_height();
    _scroll_duration_ms = duration_ms;
    _scroll_start_time = millis();

    if (pixels == 0) {
        switch (dir) {
            case LEFT: case RIGHT: pixels = _scroll_ex - _scroll_sx; break;
            case UP: case DOWN: pixels = _scroll_ey - _scroll_sy; break;
        }
    }
    _scroll_total_pixels = (dir == RIGHT || dir == DOWN) ? pixels : -pixels;
    _scroll_active = true;
    _scroll_progress = 0.0f;
}

void MatrixChain::update_scroll_animation() {
    if (!_scroll_active) return;

    uint32_t now = millis();
    float elapsed = now - _scroll_start_time;
    _scroll_progress = elapsed / _scroll_duration_ms;
    if (_scroll_progress >= 1.0f) {
        _scroll_progress = 1.0f;
        _scroll_active = false;
    }

    float eased = _scroll_progress * _scroll_progress * (3.0f - 2.0f * _scroll_progress);
    int16_t shift = (int16_t)(eased * _scroll_total_pixels);

    apply_scroll_shift(shift);

    if (!_scroll_active) {
        force_update_all();
    }
}

void MatrixChain::apply_scroll_shift(int16_t shift) {
    if (shift == 0) return;

    uint16_t w = total_width();
    uint16_t h = total_height();
    uint8_t* temp_red = (uint8_t*)malloc(_ext_height * _ext_width);
    uint8_t* temp_green = (uint8_t*)malloc(_ext_height * _ext_width);
    if (!temp_red || !temp_green) {
        if (temp_red) free(temp_red);
        if (temp_green) free(temp_green);
        return;
    }

    memcpy(temp_red, _ext_plane_red, _ext_height * _ext_width);
    memcpy(temp_green, _ext_plane_green, _ext_height * _ext_width);
    memset(_ext_plane_red, 0, _ext_height * _ext_width);
    memset(_ext_plane_green, 0, _ext_height * _ext_width);

    int16_t dx = 0, dy = 0;
    switch (_scroll_dir) {
        case LEFT:  dx = shift; break;
        case RIGHT: dx = -shift; break;
        case UP:    dy = shift; break;
        case DOWN:  dy = -shift; break;
    }

    for (uint16_t y = 0; y < h; ++y) {
        for (uint16_t x = 0; x < w; ++x) {
            int16_t src_x = x - dx;
            int16_t src_y = y - dy;
            if (src_x >= 0 && src_x < (int16_t)w && src_y >= 0 && src_y < (int16_t)h) {
                uint16_t src_ext_x = 1 + src_x;
                uint16_t src_ext_y = 1 + src_y;
                uint16_t dst_ext_x = 1 + x;
                uint16_t dst_ext_y = 1 + y;

                uint16_t src_idx = src_ext_y * _ext_width + (src_ext_x / 8);
                uint8_t src_bit = src_ext_x % 8;
                uint16_t dst_idx = dst_ext_y * _ext_width + (dst_ext_x / 8);
                uint8_t dst_bit = dst_ext_x % 8;

                if (src_idx < _ext_height * _ext_width && dst_idx < _ext_height * _ext_width) {
                    if (temp_red[src_idx] & (1 << src_bit)) {
                        _ext_plane_red[dst_idx] |= (1 << dst_bit);
                    }
                    if (temp_green[src_idx] & (1 << src_bit)) {
                        _ext_plane_green[dst_idx] |= (1 << dst_bit);
                    }
                }
            }
        }
    }

    free(temp_red);
    free(temp_green);
    sync_extended_to_modules();
}
