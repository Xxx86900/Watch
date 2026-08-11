#ifndef WATCH_ASSETS_FONTS_H
#define WATCH_ASSETS_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * 固定宽度字体描述。
 *
 * bitmap 中的数据按以下顺序存放：
 * 1. 字符按照 ASCII 编码依次排列；
 * 2. 每个字符从左到右按列排列；
 * 3. 每列从上到下，每 8 个像素占一个字节；
 * 4. 字节最低位表示最上方的像素。
 */
typedef struct
{
  uint8_t width;
  uint8_t height;
  uint8_t first_character;
  uint8_t last_character;
  const uint8_t *bitmap;
} ui_font_t;

/* Sparse Unicode font used for the Chinese UI vocabulary. */
typedef struct
{
  uint8_t width;
  uint8_t height;
  uint16_t glyph_count;
  const uint32_t *codepoints;
  const uint8_t *bitmap;
} ui_unicode_font_t;

/* 后续将在 fonts.c 中定义这个字体 */
extern const ui_font_t ui_font_5x7;
extern const ui_unicode_font_t ui_font_zh_16x16;

#ifdef __cplusplus
}
#endif

#endif /* WATCH_ASSETS_FONTS_H */
