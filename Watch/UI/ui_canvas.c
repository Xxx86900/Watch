#include "ui_canvas.h"
#include <stddef.h>
#include <string.h>

/*
 * 128 × 64 单色屏幕：
 * 每个像素占 1 bit，因此一共需要 1024 字节。
 * static 表示该缓冲区只允许本文件内部直接访问。
 */
static uint8_t s_canvas_buffer[UI_CANVAS_BUFFER_SIZE];

static void ui_canvas_draw_pixel_checked(int16_t x,int16_t y,ui_color_t color)
{
  if ((x >= 0) && (x < (int16_t)UI_CANVAS_WIDTH) &&
      (y >= 0) && (y < (int16_t)UI_CANVAS_HEIGHT))
  {
    ui_canvas_draw_pixel((uint8_t)x,(uint8_t)y,color);
  }
}

static uint8_t ui_canvas_decode_utf8(const char *text,uint32_t *codepoint)
{
  const uint8_t *bytes = (const uint8_t *)text;
  uint32_t value;

  if (bytes[0] < 0x80U)
  {
    *codepoint = bytes[0];
    return 1U;
  }

  if ((bytes[0] >= 0xC2U) && (bytes[0] <= 0xDFU) &&
      ((bytes[1] & 0xC0U) == 0x80U))
  {
    *codepoint = ((uint32_t)(bytes[0] & 0x1FU) << 6U) |
                 (uint32_t)(bytes[1] & 0x3FU);
    return 2U;
  }

  if ((bytes[0] >= 0xE0U) && (bytes[0] <= 0xEFU) &&
      (bytes[1] != 0U) && (bytes[2] != 0U) &&
      ((bytes[1] & 0xC0U) == 0x80U) && ((bytes[2] & 0xC0U) == 0x80U))
  {
    value = ((uint32_t)(bytes[0] & 0x0FU) << 12U) |
            ((uint32_t)(bytes[1] & 0x3FU) << 6U) |
            (uint32_t)(bytes[2] & 0x3FU);
    if ((value >= 0x800U) && !((value >= 0xD800U) && (value <= 0xDFFFU)))
    {
      *codepoint = value;
      return 3U;
    }
  }

  if ((bytes[0] >= 0xF0U) && (bytes[0] <= 0xF4U) &&
      (bytes[1] != 0U) && (bytes[2] != 0U) && (bytes[3] != 0U) &&
      ((bytes[1] & 0xC0U) == 0x80U) && ((bytes[2] & 0xC0U) == 0x80U) &&
      ((bytes[3] & 0xC0U) == 0x80U))
  {
    value = ((uint32_t)(bytes[0] & 0x07U) << 18U) |
            ((uint32_t)(bytes[1] & 0x3FU) << 12U) |
            ((uint32_t)(bytes[2] & 0x3FU) << 6U) |
            (uint32_t)(bytes[3] & 0x3FU);
    if ((value >= 0x10000U) && (value <= 0x10FFFFU))
    {
      *codepoint = value;
      return 4U;
    }
  }

  *codepoint = 0xFFFDU;
  return 1U;
}

static int32_t ui_canvas_find_unicode_glyph(const ui_unicode_font_t *font,uint32_t codepoint)
{
  int32_t left = 0;
  int32_t right = (int32_t)font->glyph_count - 1;

  while (left <= right)
  {
    int32_t middle = left + ((right - left) / 2);
    uint32_t current = font->codepoints[middle];

    if (current == codepoint)
    {
      return middle;
    }
    if (current < codepoint)
    {
      left = middle + 1;
    }
    else
    {
      right = middle - 1;
    }
  }
  return -1;
}

static void ui_canvas_draw_unicode_char(uint8_t x,uint8_t y,uint32_t codepoint,
                                        const ui_unicode_font_t *font,ui_color_t color)
{
  int32_t glyph_index;
  uint8_t column;
  uint8_t row;
  uint8_t bytes_per_column;
  uint32_t glyph_size;
  uint32_t glyph_offset;
  uint32_t bitmap_index;

  if ((font == NULL) || (font->codepoints == NULL) || (font->bitmap == NULL))
  {
    return;
  }

  glyph_index = ui_canvas_find_unicode_glyph(font,codepoint);
  if (glyph_index < 0)
  {
    ui_canvas_draw_rectangle(x,y,font->width,font->height,color);
    return;
  }

  bytes_per_column = (uint8_t)((font->height + 7U) / 8U);
  glyph_size = (uint32_t)font->width * bytes_per_column;
  glyph_offset = (uint32_t)glyph_index * glyph_size;

  for (column = 0U; column < font->width; column++)
  {
    for (row = 0U; row < font->height; row++)
    {
      bitmap_index = glyph_offset + ((uint32_t)column * bytes_per_column) + (row / 8U);
      if ((font->bitmap[bitmap_index] & (uint8_t)(1U << (row % 8U))) != 0U)
      {
        ui_canvas_draw_pixel((uint8_t)(x + column),(uint8_t)(y + row),color);
      }
    }
  }
}

void ui_canvas_init(void)
{
  ui_canvas_clear(UI_COLOR_BLACK);
}

void ui_canvas_clear(ui_color_t color)
{
  uint8_t fill_value;

  if (color == UI_COLOR_WHITE)
  {
    fill_value = 0xFFU;
  }
  else
  {
    fill_value = 0x00U;
  }

  memset(s_canvas_buffer, fill_value, sizeof(s_canvas_buffer));
}

void ui_canvas_draw_pixel(uint8_t x,uint8_t y,ui_color_t color)
{
  uint16_t byte_index;
  uint8_t bit_mask;

  /* 防止坐标超出屏幕范围 */
  if ((x >= UI_CANVAS_WIDTH) || (y >= UI_CANVAS_HEIGHT))
  {
    return;
  }
  /*
   * SH1106 按页存储：
   * 每一页高 8 个像素，同一列的 8 个像素存放在一个字节中。
   */
  byte_index = (uint16_t)x +
               ((uint16_t)(y / 8U) * UI_CANVAS_WIDTH);

  bit_mask = (uint8_t)(1U << (y % 8U));

  if (color == UI_COLOR_WHITE)
  {
    s_canvas_buffer[byte_index] |= bit_mask;
  }
  else
  {
    s_canvas_buffer[byte_index] &= (uint8_t)(~bit_mask);
  }
}
void ui_canvas_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, ui_color_t color)
{
  int16_t current_x = x0;
  int16_t current_y = y0;
  int16_t end_x = x1;
  int16_t end_y = y1;

  int16_t delta_x;
  int16_t delta_y;
  int16_t step_x;
  int16_t step_y;
  int16_t error;
  int16_t error_double;

  delta_x = end_x - current_x;
  if (delta_x < 0)
  {
    delta_x = -delta_x;
  }

  delta_y = end_y - current_y;
  if (delta_y < 0)
  {
    delta_y = -delta_y;
  }

  step_x = (current_x < end_x) ? 1 : -1;
  step_y = (current_y < end_y) ? 1 : -1;
  error = delta_x - delta_y;
  while (1)
  {
    ui_canvas_draw_pixel(current_x, current_y, color);
    if (current_x == end_x && current_y == end_y)
    {
      break;
    }
    error_double = (int16_t)(2*error);
    if (error_double > -delta_y)
    {
      error -= delta_y;
      current_x += step_x;
    }

    if (error_double < delta_x)
    {
      error += delta_x;
      current_y += step_y;
    }

  }
}
void ui_canvas_draw_rectangle(uint8_t x,uint8_t y,uint8_t width,uint8_t height,ui_color_t color)
{
    uint16_t right;
    uint16_t bottom;

    /* 宽度或高度为 0 时不需要绘制 */
    if ((width == 0U) || (height == 0U))
    {
        return;
    }

    /* 左上角已经超出屏幕 */
    if ((x >= UI_CANVAS_WIDTH) || (y >= UI_CANVAS_HEIGHT))
    {
        return;
    }

    right = (uint16_t)x + width - 1U;
    bottom = (uint16_t)y + height - 1U;

    /* 将矩形裁剪在屏幕范围内 */
    if (right >= UI_CANVAS_WIDTH)
    {
        right = UI_CANVAS_WIDTH - 1U;
    }

    if (bottom >= UI_CANVAS_HEIGHT)
    {
        bottom = UI_CANVAS_HEIGHT - 1U;
    }

    /* 上边和下边 */
    ui_canvas_draw_line(x, y,
                        (uint8_t)right, y,
                        color);

    ui_canvas_draw_line(x, (uint8_t)bottom,
                        (uint8_t)right, (uint8_t)bottom,
                        color);

    /* 左边和右边 */
    ui_canvas_draw_line(x, y,
                        x, (uint8_t)bottom,
                        color);

    ui_canvas_draw_line((uint8_t)right, y,
                        (uint8_t)right, (uint8_t)bottom,
                        color);
}

void ui_canvas_fill_rectangle(uint8_t x,uint8_t y,uint8_t width,uint8_t height,ui_color_t color)
{
    uint16_t right;
    uint16_t bottom;
    uint16_t current_y;

    if ((width == 0U) || (height == 0U))
    {
        return;
    }

    if ((x >= UI_CANVAS_WIDTH) || (y >= UI_CANVAS_HEIGHT))
    {
        return;
    }

    right = (uint16_t)x + width - 1U;
    bottom = (uint16_t)y + height - 1U;

    if (right >= UI_CANVAS_WIDTH)
    {
        right = UI_CANVAS_WIDTH - 1U;
    }

    if (bottom >= UI_CANVAS_HEIGHT)
    {
        bottom = UI_CANVAS_HEIGHT - 1U;
    }

    /*
     * 从上到下画多条水平线，
     * 多条水平线组合起来就是实心矩形。
     */
    for (current_y = y; current_y <= bottom; current_y++)
    {
        ui_canvas_draw_line(x,
                            (uint8_t)current_y,
                            (uint8_t)right,
                            (uint8_t)current_y,
                            color);
    }
}
void ui_canvas_draw_char(uint8_t x,uint8_t y,char character,const ui_font_t *font,ui_color_t color)
{
  ui_canvas_draw_char_scaled(x,y,character,font,1U,color);
}

void ui_canvas_draw_circle(int16_t center_x,int16_t center_y,uint8_t radius,ui_color_t color)
{
  int16_t x = radius;
  int16_t y = 0;
  int16_t error = (int16_t)(1 - x);

  while (x >= y)
  {
    ui_canvas_draw_pixel_checked(center_x + x,center_y + y,color);
    ui_canvas_draw_pixel_checked(center_x + y,center_y + x,color);
    ui_canvas_draw_pixel_checked(center_x - y,center_y + x,color);
    ui_canvas_draw_pixel_checked(center_x - x,center_y + y,color);
    ui_canvas_draw_pixel_checked(center_x - x,center_y - y,color);
    ui_canvas_draw_pixel_checked(center_x - y,center_y - x,color);
    ui_canvas_draw_pixel_checked(center_x + y,center_y - x,color);
    ui_canvas_draw_pixel_checked(center_x + x,center_y - y,color);
    y++;
    if (error < 0)
    {
      error += (int16_t)(2 * y + 1);
    }
    else
    {
      x--;
      error += (int16_t)(2 * (y - x) + 1);
    }
  }
}

void ui_canvas_fill_circle(int16_t center_x,int16_t center_y,uint8_t radius,ui_color_t color)
{
  int16_t x;
  int16_t y;
  int32_t radius_squared = (int32_t)radius * radius;

  for (y = -(int16_t)radius; y <= (int16_t)radius; y++)
  {
    for (x = -(int16_t)radius; x <= (int16_t)radius; x++)
    {
      if (((int32_t)x * x + (int32_t)y * y) <= radius_squared)
      {
        ui_canvas_draw_pixel_checked(center_x + x,center_y + y,color);
      }
    }
  }
}

void ui_canvas_draw_char_scaled(uint8_t x,uint8_t y,char character,const ui_font_t *font,uint8_t scale,ui_color_t color)
{
    uint8_t character_code;
    uint8_t column;
    uint8_t row;
    uint8_t bytes_per_column;
    uint32_t glyph_size;
    uint32_t glyph_offset;
    uint32_t bitmap_index;
    uint16_t pixel_x;
    uint16_t pixel_y;
    uint8_t bit_mask;

    if ((font == NULL) || (font->bitmap == NULL) || (scale == 0U))
    {
        return;
    }

    if ((font->width == 0U) || (font->height == 0U))
    {
        return;
    }

    character_code = (uint8_t)character;

    /* 当前字体中没有这个字符 */
    if ((character_code < font->first_character) ||
        (character_code > font->last_character))
    {
        return;
    }

    bytes_per_column = (uint8_t)((font->height + 7U) / 8U);
    glyph_size = (uint32_t)font->width * bytes_per_column;

    glyph_offset =
        ((uint32_t)character_code - font->first_character) * glyph_size;

    for (column = 0U; column < font->width; column++)
    {
        for (row = 0U; row < font->height; row++)
        {
            bitmap_index =
                glyph_offset +
                ((uint32_t)column * bytes_per_column) +
                (row / 8U);

            bit_mask = (uint8_t)(1U << (row % 8U));

            if ((font->bitmap[bitmap_index] & bit_mask) != 0U)
            {
                pixel_x = (uint16_t)x + ((uint16_t)column * scale);
                pixel_y = (uint16_t)y + ((uint16_t)row * scale);

                if ((pixel_x < UI_CANVAS_WIDTH) &&
                    (pixel_y < UI_CANVAS_HEIGHT))
                {
                    ui_canvas_fill_rectangle((uint8_t)pixel_x,(uint8_t)pixel_y,scale,scale,color);
                }
            }
        }
    }
}

void ui_canvas_draw_text(uint8_t x,uint8_t y,const char *text,const ui_font_t *font,ui_color_t color)
{
    uint16_t cursor_x;

    if ((text == NULL) || (font == NULL))
    {
        return;
    }

    cursor_x = x;

    while (*text != '\0')
    {
        if (cursor_x >= UI_CANVAS_WIDTH)
        {
            break;
        }

        ui_canvas_draw_char((uint8_t)cursor_x,
                            y,
                            *text,
                            font,
                            color);

        /* 每个字符后面留出一个像素作为间隔 */
        cursor_x += (uint16_t)font->width + 1U;
        text++;
    }
}

void ui_canvas_draw_text_scaled(uint8_t x,uint8_t y,const char *text,const ui_font_t *font,uint8_t scale,ui_color_t color)
{
  uint16_t cursor_x;
  uint16_t advance;

  if ((text == NULL) || (font == NULL) || (scale == 0U))
  {
    return;
  }

  cursor_x = x;
  advance = ((uint16_t)font->width + 1U) * scale;

  while ((*text != '\0') && (cursor_x < UI_CANVAS_WIDTH))
  {
    ui_canvas_draw_char_scaled((uint8_t)cursor_x,y,*text,font,scale,color);
    cursor_x += advance;
    text++;
  }
}

void ui_canvas_draw_utf8(uint8_t x,uint8_t y,const char *text,ui_color_t color)
{
  uint16_t cursor_x = x;
  uint32_t codepoint;
  uint8_t consumed;

  if (text == NULL)
  {
    return;
  }

  while ((*text != '\0') && (cursor_x < UI_CANVAS_WIDTH))
  {
    consumed = ui_canvas_decode_utf8(text,&codepoint);
    if (codepoint <= 0x7FU)
    {
      ui_canvas_draw_char((uint8_t)cursor_x,y,(char)codepoint,&ui_font_5x7,color);
      cursor_x += (uint16_t)ui_font_5x7.width + 1U;
    }
    else
    {
      ui_canvas_draw_unicode_char((uint8_t)cursor_x,y,codepoint,&ui_font_zh_16x16,color);
      cursor_x += (uint16_t)ui_font_zh_16x16.width + 1U;
    }
    text += consumed;
  }
}

uint16_t ui_canvas_measure_utf8(const char *text)
{
  uint16_t width = 0U;
  uint32_t codepoint;
  uint8_t consumed;

  if (text == NULL)
  {
    return 0U;
  }

  while (*text != '\0')
  {
    consumed = ui_canvas_decode_utf8(text,&codepoint);
    width += (codepoint <= 0x7FU) ?
             ((uint16_t)ui_font_5x7.width + 1U) :
             ((uint16_t)ui_font_zh_16x16.width + 1U);
    text += consumed;
  }

  return (width > 0U) ? (uint16_t)(width - 1U) : 0U;
}

const uint8_t *ui_canvas_get_buffer(void)
{
  return s_canvas_buffer;
}
