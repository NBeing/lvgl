/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --bpp 1 --size 10 --no-compress --font PressStart2P-Regular.ttf --range 32-127 --format lvgl -o PressStart2P_12.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef PRESSSTART2P_12
#define PRESSSTART2P_12 1
#endif

#if PRESSSTART2P_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xee, 0xe0, 0xe,

    /* U+0022 "\"" */
    0xff, 0xff, 0xff,

    /* U+0023 "#" */
    0x77, 0x3b, 0xbf, 0xee, 0xe7, 0x73, 0xb9, 0xdd,
    0xff, 0x77, 0x3b, 0x80,

    /* U+0024 "$" */
    0x8, 0x4, 0x1f, 0x9d, 0x6, 0x83, 0xf0, 0x2f,
    0xfc, 0x8, 0x4, 0x0,

    /* U+0025 "%" */
    0x70, 0x88, 0x24, 0x98, 0x80, 0x0, 0x40, 0x4e,
    0x89, 0x4, 0x43, 0x0,

    /* U+0026 "&" */
    0x78, 0x3c, 0x3f, 0x1f, 0x87, 0x83, 0xc3, 0xf3,
    0xce, 0x67, 0x3f, 0xc0,

    /* U+0027 "'" */
    0xff, 0xf0,

    /* U+0028 "(" */
    0x18, 0x9d, 0xce, 0x73, 0x8e, 0x10, 0xc0,

    /* U+0029 ")" */
    0xe3, 0x1c, 0x31, 0x8c, 0x6e, 0x67, 0x0,

    /* U+002A "*" */
    0x77, 0x1e, 0x3f, 0xe7, 0x83, 0xc3, 0xb8,

    /* U+002B "+" */
    0x38, 0x38, 0xff, 0x38, 0x38, 0x38,

    /* U+002C "," */
    0x77, 0xe0,

    /* U+002D "-" */
    0xff,

    /* U+002E "." */
    0xfc,

    /* U+002F "/" */
    0x0, 0x80, 0x0, 0x80, 0x80, 0x0, 0x40, 0x40,
    0x80, 0x0, 0x40, 0x0,

    /* U+0030 "0" */
    0x3c, 0x26, 0x13, 0x98, 0x7c, 0x3e, 0x1f, 0xe,
    0xe4, 0x34, 0x1e, 0x0,

    /* U+0031 "1" */
    0x38, 0x38, 0x78, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38, 0xff,

    /* U+0032 "2" */
    0x7f, 0x31, 0xb8, 0xe0, 0xf0, 0x70, 0xf9, 0xf1,
    0xe0, 0xf0, 0x7f, 0xc0,

    /* U+0033 "3" */
    0x7f, 0x87, 0x3, 0x83, 0x81, 0xc0, 0xf0, 0xf,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0034 "4" */
    0xf, 0x83, 0xe1, 0x39, 0xce, 0xe3, 0xb8, 0xef,
    0xfc, 0xe, 0x3, 0x80, 0xe0,

    /* U+0035 "5" */
    0xfe, 0x70, 0x38, 0x1f, 0xc0, 0x38, 0x1c, 0xf,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0036 "6" */
    0x1f, 0x38, 0x1c, 0x1c, 0xe, 0x7, 0xfb, 0x8f,
    0xc7, 0x63, 0x3f, 0x80,

    /* U+0037 "7" */
    0xff, 0xf1, 0xf8, 0xe0, 0xe1, 0xc0, 0xc0, 0x60,
    0x30, 0x18, 0xc, 0x0,

    /* U+0038 "8" */
    0x7c, 0x32, 0x38, 0x9e, 0x47, 0x44, 0x62, 0x3f,
    0x7, 0x82, 0x3f, 0x0,

    /* U+0039 "9" */
    0x7f, 0x31, 0xb8, 0xfc, 0x76, 0x3b, 0xfc, 0xe,
    0xe, 0x7, 0x3e, 0x0,

    /* U+003A ":" */
    0xfc, 0xf, 0xc0,

    /* U+003B ";" */
    0x77, 0x0, 0x77, 0x6e,

    /* U+003C "<" */
    0xc, 0x21, 0x9c, 0x63, 0x87, 0x6, 0x8, 0x30,

    /* U+003D "=" */
    0xff, 0x80, 0x0, 0x1f, 0xf0,

    /* U+003E ">" */
    0xe1, 0x87, 0x6, 0x8, 0x31, 0x9c, 0x63, 0x80,

    /* U+003F "?" */
    0x7e, 0x3f, 0x38, 0xfc, 0x70, 0xe0, 0x70, 0x70,
    0x0, 0x0, 0xe, 0x0,

    /* U+0040 "@" */
    0x7e, 0x0, 0x20, 0x33, 0x99, 0x4c, 0xa6, 0x7f,
    0x0, 0x0, 0x3f, 0x0,

    /* U+0041 "A" */
    0x1c, 0x3b, 0x9d, 0xdc, 0x7e, 0x3f, 0x1f, 0xff,
    0xc7, 0xe3, 0xf1, 0xc0,

    /* U+0042 "B" */
    0xff, 0x71, 0xb8, 0xfc, 0x7e, 0x37, 0xfb, 0x8f,
    0xc7, 0xe3, 0x7f, 0x80,

    /* U+0043 "C" */
    0x1f, 0x9, 0x9c, 0xfc, 0xe, 0x7, 0x3, 0x80,
    0xe7, 0x13, 0xf, 0x80,

    /* U+0044 "D" */
    0xfc, 0x72, 0x39, 0xdc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xce, 0xe4, 0x7e, 0x0,

    /* U+0045 "E" */
    0xff, 0xf0, 0x38, 0x1c, 0xe, 0x7, 0xfb, 0x81,
    0xc0, 0xe0, 0x7f, 0xc0,

    /* U+0046 "F" */
    0xff, 0xf0, 0x38, 0x1c, 0xe, 0x7, 0xfb, 0x81,
    0xc0, 0xe0, 0x70, 0x0,

    /* U+0047 "G" */
    0x1f, 0x88, 0x1c, 0x1c, 0xe, 0x7, 0x3f, 0x8e,
    0xe7, 0x13, 0x8f, 0xc0,

    /* U+0048 "H" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0xff, 0x8f,
    0xc7, 0xe3, 0xf1, 0xc0,

    /* U+0049 "I" */
    0xff, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38, 0xff,

    /* U+004A "J" */
    0x3, 0x81, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0xf,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+004B "K" */
    0xe3, 0xf1, 0xb9, 0xdf, 0x8f, 0x87, 0xc3, 0xf1,
    0xfe, 0xe7, 0x73, 0xc0,

    /* U+004C "L" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xff,

    /* U+004D "M" */
    0xe3, 0xf1, 0xfd, 0xff, 0xfe, 0xbf, 0x5f, 0xaf,
    0xc7, 0xe3, 0xf1, 0xc0,

    /* U+004E "N" */
    0xe3, 0xf1, 0xfc, 0xff, 0x7e, 0xbf, 0x3f, 0x9f,
    0xc7, 0xe3, 0xf1, 0xc0,

    /* U+004F "O" */
    0x7e, 0x31, 0x38, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0050 "P" */
    0xff, 0x71, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0xfd,
    0xc0, 0xe0, 0x70, 0x0,

    /* U+0051 "Q" */
    0x7f, 0x31, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0xbf,
    0xce, 0x64, 0x3e, 0x40,

    /* U+0052 "R" */
    0xff, 0x71, 0xb8, 0xfc, 0x7e, 0x3f, 0x3f, 0xf1,
    0xde, 0xef, 0x73, 0xc0,

    /* U+0053 "S" */
    0x7e, 0x31, 0x38, 0xfc, 0x6, 0x3, 0xf0, 0xf,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0054 "T" */
    0xff, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38, 0x38,

    /* U+0055 "U" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0056 "V" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3b, 0xb1, 0xf8,
    0xfc, 0x1c, 0x4, 0x0,

    /* U+0057 "W" */
    0xcb, 0xe5, 0xf2, 0xf9, 0x7c, 0xbe, 0x5f, 0xdf,
    0xef, 0x42, 0x21, 0x0,

    /* U+0058 "X" */
    0xe3, 0xf1, 0xd8, 0xce, 0xe7, 0x70, 0xe1, 0xdc,
    0xc6, 0xe3, 0xf1, 0xc0,

    /* U+0059 "Y" */
    0xe7, 0xe7, 0xe7, 0xe7, 0x64, 0x7c, 0x18, 0x18,
    0x18, 0x18,

    /* U+005A "Z" */
    0xff, 0x83, 0xc1, 0xe1, 0xe3, 0xc1, 0xe1, 0xe1,
    0xe0, 0xf0, 0x7f, 0xc0,

    /* U+005B "[" */
    0xff, 0x8e, 0x38, 0xe3, 0x8e, 0x38, 0xe3, 0xf0,

    /* U+005C "\\" */
    0x80, 0x0, 0x10, 0x2, 0x0, 0x0, 0x40, 0x10,
    0x4, 0x0, 0x0, 0x40,

    /* U+005D "]" */
    0xfc, 0x71, 0xc7, 0x1c, 0x71, 0xc7, 0x1f, 0xf0,

    /* U+005E "^" */
    0x3c, 0xff, 0xc0,

    /* U+005F "_" */
    0xff, 0x80,

    /* U+0060 "`" */
    0x84,

    /* U+0061 "a" */
    0x7e, 0x1, 0xc0, 0xef, 0xfe, 0x3b, 0x1d, 0xfe,

    /* U+0062 "b" */
    0xe0, 0x70, 0x38, 0x1f, 0xce, 0x3f, 0x1f, 0x8f,
    0xc7, 0x62, 0x3f, 0x0,

    /* U+0063 "c" */
    0x7f, 0xb0, 0x38, 0x1c, 0xe, 0x7, 0x1, 0xfe,

    /* U+0064 "d" */
    0x3, 0x81, 0xc0, 0xef, 0xf6, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0xbf, 0xc0,

    /* U+0065 "e" */
    0x7e, 0x31, 0x38, 0xff, 0xfe, 0x7, 0x1, 0xf8,

    /* U+0066 "f" */
    0xf, 0x8, 0x38, 0xff, 0x38, 0x38, 0x38, 0x38,
    0x38, 0x38,

    /* U+0067 "g" */
    0x7f, 0xb1, 0xf8, 0xfc, 0x77, 0xf8, 0x1c, 0xe,
    0xfc,

    /* U+0068 "h" */
    0xe0, 0x70, 0x38, 0x1f, 0xee, 0x37, 0x1f, 0x8f,
    0xc7, 0xe3, 0xf1, 0xc0,

    /* U+0069 "i" */
    0x38, 0x0, 0x0, 0x78, 0x38, 0x38, 0x38, 0x38,
    0x38, 0xff,

    /* U+006A "j" */
    0x1c, 0x0, 0xf, 0x1c, 0x71, 0xc7, 0x1c, 0x7f,
    0x80,

    /* U+006B "k" */
    0xe0, 0x70, 0x38, 0x1c, 0x7e, 0x37, 0x3b, 0xf1,
    0xce, 0xe7, 0x71, 0xc0,

    /* U+006C "l" */
    0x78, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38, 0xff,

    /* U+006D "m" */
    0xff, 0x5d, 0xae, 0xf7, 0x7b, 0xbd, 0xde, 0xee,

    /* U+006E "n" */
    0xff, 0x71, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8e,

    /* U+006F "o" */
    0x7e, 0x31, 0x38, 0xfc, 0x7e, 0x3f, 0x1d, 0xf8,

    /* U+0070 "p" */
    0xff, 0x71, 0xb8, 0xfc, 0x7f, 0xf7, 0x3, 0x81,
    0xc0,

    /* U+0071 "q" */
    0x7f, 0xb1, 0xf8, 0xfc, 0x77, 0xf8, 0x1c, 0xe,
    0x7,

    /* U+0072 "r" */
    0xef, 0xe0, 0xf0, 0xe0, 0xe0, 0xe0, 0xe0,

    /* U+0073 "s" */
    0x7e, 0x30, 0x38, 0xf, 0xc0, 0x38, 0x13, 0xf8,

    /* U+0074 "t" */
    0x38, 0x38, 0x38, 0xff, 0x38, 0x38, 0x38, 0x38,
    0x38, 0x38,

    /* U+0075 "u" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3b, 0x1d, 0xfe,

    /* U+0076 "v" */
    0xe7, 0xe7, 0xe7, 0xe7, 0x64, 0x7c, 0x18,

    /* U+0077 "w" */
    0xeb, 0xf5, 0xfa, 0xfd, 0x7e, 0xbb, 0x19, 0xdc,

    /* U+0078 "x" */
    0xe3, 0xb1, 0x9d, 0xc3, 0x87, 0x73, 0xbb, 0x8e,

    /* U+0079 "y" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x77, 0xf8, 0x1c, 0xe,
    0xfc,

    /* U+007A "z" */
    0xff, 0x87, 0x83, 0xc7, 0x87, 0x83, 0xc3, 0xfe,

    /* U+007B "{" */
    0x18, 0x9c, 0xe7, 0x71, 0xce, 0x10, 0xc0,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xfc,

    /* U+007D "}" */
    0xe3, 0x1c, 0xe7, 0xd, 0xce, 0x67, 0x0,

    /* U+007E "~" */
    0x78, 0x5e, 0x43, 0x1, 0xe0,

    /* U+007F "" */
    0xff, 0xf0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 160, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 160, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 6, .adv_w = 160, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 9, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 21, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 33, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 45, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 57, .adv_w = 160, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 59, .adv_w = 160, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 66, .adv_w = 160, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 73, .adv_w = 160, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 80, .adv_w = 160, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 86, .adv_w = 160, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 160, .box_w = 8, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 89, .adv_w = 160, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 90, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 102, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 114, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 124, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 136, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 148, .adv_w = 160, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 161, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 173, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 185, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 197, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 209, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 221, .adv_w = 160, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 224, .adv_w = 160, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 228, .adv_w = 160, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 236, .adv_w = 160, .box_w = 9, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 241, .adv_w = 160, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 249, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 261, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 273, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 285, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 297, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 309, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 321, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 333, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 345, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 357, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 369, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 379, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 391, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 403, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 413, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 425, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 437, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 449, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 461, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 473, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 485, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 497, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 507, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 519, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 531, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 543, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 555, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 565, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 577, .adv_w = 160, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 585, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 597, .adv_w = 160, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 605, .adv_w = 160, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 608, .adv_w = 160, .box_w = 9, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 160, .box_w = 2, .box_h = 3, .ofs_x = 4, .ofs_y = 8},
    {.bitmap_index = 611, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 619, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 631, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 639, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 651, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 659, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 669, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 690, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 700, .adv_w = 160, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 721, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 731, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 739, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 747, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 755, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 773, .adv_w = 160, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 780, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 788, .adv_w = 160, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 798, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 806, .adv_w = 160, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 813, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 821, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 829, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 838, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 846, .adv_w = 160, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 853, .adv_w = 160, .box_w = 3, .box_h = 10, .ofs_x = 4, .ofs_y = 1},
    {.bitmap_index = 857, .adv_w = 160, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 864, .adv_w = 160, .box_w = 9, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 869, .adv_w = 160, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 96, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lv_font_montserrat_10;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t PressStart2P_10 = {
#else
lv_font_t PressStart2P_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_10,
#endif
    .user_data = NULL,
};



#endif /*#if PRESSSTART2P_12*/

