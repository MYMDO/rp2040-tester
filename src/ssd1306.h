#pragma once
#include <stdint.h>

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_display(void);
void ssd1306_set_pixel(int x, int y, int on);
void ssd1306_draw_line(int x0, int y0, int x1, int y1);
void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *s);
