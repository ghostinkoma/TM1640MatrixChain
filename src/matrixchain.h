// matrixchain.h - Chain multiple TM1640 modules with U8g2 integration
#ifndef MATRIXCHAIN_H
#define MATRIXCHAIN_H

#include "tm1640.h"
#include <U8g2lib.h>
#include <stdint.h>

enum Orientation { HORIZONTAL = 0, VERTICAL = 1 };
enum Direction { LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3 };
enum FontType { FONT_8X8, FONT_5X7 };

class MatrixChain {
public:
    // Constructor: array of modules, count, orientation, layout columns
    MatrixChain(TM1640* modules[], uint8_t count, Orientation orient, uint8_t layoutCols = 0);
    ~MatrixChain();

    // Control
    void initialize_all();           // Initialize all modules
    void set_duty(uint8_t val);      // Set global brightness

    // Global drawing
    void draw_dot_global(uint16_t globalX, uint8_t globalY, uint8_t color);
    void drawbin_global(const uint8_t bytes[], uint32_t byteLen, uint8_t color);
    void get_gram_linear(uint8_t out[], uint32_t out_len) const;

    // Text & icons
    void print_matrix(const char* str, uint16_t x, uint8_t y, FontType font_type, uint8_t color); // Legacy
    void scroll(Direction dir, uint16_t start_x=0, uint8_t start_y=0, uint16_t end_x=0, uint8_t end_y=0);
    void icon_print(const uint8_t* icon_data, uint16_t start_column, uint8_t color);

    void force_update_all();         // Force update all modules

    // Dimensions
    uint16_t total_width()  const;
    uint16_t total_height() const;

    // U8g2 virtual display
    void beginU8g2Virtual();         // Initialize virtual buffer
    void setFont(const uint8_t* font);
    int16_t drawUTF8(uint16_t x, uint8_t y, const char* str, uint8_t color = ORANGE);

    // Smooth scroll animation
    void start_scroll_animation(Direction dir, uint16_t start_x = 0, uint8_t start_y = 0, uint16_t end_x = 0, uint8_t end_y = 0, uint16_t duration_ms = 1000, uint8_t pixels = 0);
    void update_scroll_animation();
    bool is_scroll_animating() const { return _scroll_active; }

private:
    TM1640** _modules;
    uint8_t _count;
    Orientation _orient;
    uint8_t _layoutCols;

    static constexpr uint8_t _module_width = 8;
    static constexpr uint8_t _module_height = 8;

    uint16_t _ext_width;
    uint8_t _ext_height = 10;
    uint8_t* _ext_plane_red = nullptr;
    uint8_t* _ext_plane_green = nullptr;

    u8g2_t u8g2_virt;
    uint8_t* _temp_buffer = nullptr;
    uint8_t _current_color = ORANGE;

    bool _scroll_active = false;
    Direction _scroll_dir;
    uint16_t _scroll_sx, _scroll_sy, _scroll_ex, _scroll_ey;
    uint32_t _scroll_start_time;
    uint16_t _scroll_duration_ms;
    int16_t _scroll_total_pixels;
    float _scroll_progress = 0.0f;

    bool map_global_to_module(uint16_t gx, uint8_t gy, uint8_t& moduleIndex, uint8_t& localX, uint8_t& localY) const;
    void transpose_for_rotation90(const uint8_t src[], uint8_t dst[], uint8_t w, uint8_t h) const;
    void transpose_for_rotation270(const uint8_t src[], uint8_t dst[], uint8_t w, uint8_t h) const;

    void sync_extended_to_modules();  // Sync virtual plane to modules
    void apply_scroll_shift(int16_t shift);
};

#endif