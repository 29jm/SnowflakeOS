/* title bar height
*/
#define tb_height 30
#define tb_padding 4

typedef struct {
    int64_t bg_color, // background color
    tb_color, tb_bcolor, tb_tcolor, // title bar colors
    w_bcolor; // window border color
} color_scheme_t;