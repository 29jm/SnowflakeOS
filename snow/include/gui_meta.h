#pragma once

/* title bar height
*/
#define tb_height 30
#define tb_padding 8

typedef struct {
    int64_t bg_color, // background color
    tb_color, tb_bcolor, tb_tcolor, tb_highlight, // title bar colors
    w_bcolor; // window border color
    bool is_hovered; // enables tb highlight
} color_scheme_t;

typedef struct position_t {
    int32_t x;
    int32_t y;
} pos_t;