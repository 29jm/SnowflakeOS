#pragma once

/* title bar height
*/
#define tb_height 30
#define tb_padding 8

typedef struct {
    int64_t bg_color, // background color
    base_color, border_color, text_color, highlight, // title bar colors
    border_color2; // window border color
    bool is_hovered; // enables tb highlight
} color_scheme_t;