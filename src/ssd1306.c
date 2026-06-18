#include "ssd1306.h"
#include "config.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>

#define I2C_PORT (I2C_PORT_NUM == 0 ? i2c0 : i2c1)

static uint8_t fb[OLED_W * OLED_H / 8];

static void cmd(uint8_t c)
{
    uint8_t buf[2] = { 0x00, c };
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

void ssd1306_init(void)
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    static const uint8_t init_seq[] = {
        0xAE,             /* display off */
        0xD5, 0x80,       /* clock divide */
        0xA8, 0x3F,       /* multiplex 64 */
        0xD3, 0x00,       /* display offset */
        0x40,             /* start line 0 */
        0x8D, 0x14,       /* charge pump on */
        0xA1,             /* segment remap */
        0xC8,             /* COM scan dir */
        0xDA, 0x12,       /* COM pins */
        0x81, 0x7F,       /* contrast */
        0xD9, 0xF1,       /* precharge */
        0xDB, 0x40,       /* vcom deselect */
        0xA4,             /* entire display from RAM */
        0xA6,             /* normal (not inverted) */
        0x20, 0x00,       /* horizontal addressing mode */
        0xAF              /* display on */
    };
    for (unsigned i = 0; i < sizeof(init_seq); i++) cmd(init_seq[i]);
    ssd1306_clear();
    ssd1306_display();
}

void ssd1306_clear(void) { memset(fb, 0, sizeof(fb)); }

void ssd1306_set_pixel(int x, int y, int on)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    uint32_t idx = x + (y / 8) * OLED_W;
    uint8_t bit = 1u << (y % 8);
    if (on) fb[idx] |= bit; else fb[idx] &= ~bit;
}

void ssd1306_draw_line(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0, dy = y1 - y0;
    int sx = (dx > 0) - (dx < 0);
    int sy = (dy > 0) - (dy < 0);
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    int x = x0, y = y0;
    while (1) {
        ssd1306_set_pixel(x, y, 1);
        if (x == x1 && y == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
}

void ssd1306_display(void)
{
    cmd(0x21); cmd(0); cmd(OLED_W - 1);
    cmd(0x22); cmd(0); cmd((OLED_H / 8) - 1);
    uint8_t buf[1 + sizeof(fb)];
    buf[0] = 0x40;
    memcpy(&buf[1], fb, sizeof(fb));
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, sizeof(buf), false);
}

/* ---- compact stroke font: 5x7 cell, nodes TL,TM,TR / ML,MM,MR / BL,BM,BR ---- */
typedef struct { int8_t x0, y0, x1, y1; } seg_t;

/* x: 0,2,4   y: 0,3,6  (within a 5x7 glyph cell) */
static const seg_t SEG[] = {
    {0,0,2,0}, {2,0,4,0},   /* a, b : top */
    {0,0,0,3}, {4,0,4,3},   /* c, d : upper verticals */
    {0,3,0,6}, {4,3,4,6},   /* e, f : lower verticals */
    {0,6,2,6}, {2,6,4,6},   /* g, h : bottom */
    {0,3,2,3}, {2,3,4,3},   /* i, j : middle */
    {0,0,2,3}, {4,0,2,3},   /* k, l : upper diag to mid */
    {2,3,0,6}, {2,3,4,6},   /* m, n : mid to lower diag */
    {2,0,0,3}, {2,0,4,3},   /* o, p : top-mid to mid-side */
    {2,0,2,3}, {2,3,2,6},   /* q, r : vertical stem halves */
    {0,0,4,6}, {0,0,2,6}, {4,0,2,6} /* s, t, u : long diagonals */
};
#define SEG_A (1u<<0)
#define SEG_B (1u<<1)
#define SEG_C (1u<<2)
#define SEG_D (1u<<3)
#define SEG_E (1u<<4)
#define SEG_F (1u<<5)
#define SEG_G (1u<<6)
#define SEG_H (1u<<7)
#define SEG_I (1u<<8)
#define SEG_J (1u<<9)
#define SEG_K (1u<<10)
#define SEG_L (1u<<11)
#define SEG_M (1u<<12)
#define SEG_N (1u<<13)
#define SEG_O (1u<<14)
#define SEG_P (1u<<15)
#define SEG_Q (1u<<16)
#define SEG_R (1u<<17)
#define SEG_S (1u<<18)
#define SEG_T (1u<<19)
#define SEG_U (1u<<20)

static uint32_t glyph_mask(char c)
{
    switch (c) {
    case '0': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_H;
    case '1': return SEG_D|SEG_F;
    case '2': return SEG_A|SEG_B|SEG_D|SEG_I|SEG_J|SEG_E|SEG_G|SEG_H;
    case '3': return SEG_A|SEG_B|SEG_D|SEG_I|SEG_J|SEG_F|SEG_G|SEG_H;
    case '4': return SEG_C|SEG_D|SEG_I|SEG_J|SEG_F;
    case '5': return SEG_A|SEG_B|SEG_C|SEG_I|SEG_J|SEG_F|SEG_G|SEG_H;
    case '6': return SEG_A|SEG_B|SEG_C|SEG_I|SEG_J|SEG_E|SEG_F|SEG_G|SEG_H;
    case '7': return SEG_A|SEG_B|SEG_D|SEG_F;
    case '8': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_H|SEG_I|SEG_J;
    case '9': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_I|SEG_J|SEG_F|SEG_G|SEG_H;
    case 'A': return SEG_O|SEG_P|SEG_I|SEG_J|SEG_E|SEG_F;
    case 'C': return SEG_A|SEG_B|SEG_C|SEG_E|SEG_G|SEG_H;
    case 'D': case 'O': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_H;
    case 'E': return SEG_A|SEG_B|SEG_C|SEG_I|SEG_E|SEG_G|SEG_H;
    case 'F': return SEG_A|SEG_B|SEG_C|SEG_I|SEG_E;
    case 'H': return SEG_C|SEG_D|SEG_I|SEG_J|SEG_E|SEG_F;
    case 'I': return SEG_A|SEG_B|SEG_G|SEG_H|SEG_Q|SEG_R;
    case 'J': return SEG_D|SEG_F|SEG_M;
    case 'K': return SEG_C|SEG_E|SEG_L|SEG_N;
    case 'L': return SEG_C|SEG_E|SEG_G|SEG_H;
    case 'M': return SEG_C|SEG_E|SEG_D|SEG_F|SEG_K|SEG_L;
    case 'N': return SEG_C|SEG_E|SEG_D|SEG_F|SEG_S;
    case 'P': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_I|SEG_J;
    case 'R': return SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_I|SEG_J|SEG_N;
    case 'S': return SEG_A|SEG_B|SEG_C|SEG_I|SEG_J|SEG_F|SEG_G|SEG_H;
    case 'T': return SEG_A|SEG_B|SEG_Q|SEG_R;
    case 'U': return SEG_C|SEG_E|SEG_D|SEG_F|SEG_G|SEG_H;
    case 'V': return SEG_T|SEG_U;
    case '-': return SEG_I|SEG_J;
    case '%': return SEG_S;
    default:  return 0; /* space, '.', ':' handled separately below */
    }
}

void ssd1306_draw_char(int x, int y, char c)
{
    if (c == '.') { ssd1306_set_pixel(x + 1, y + 6, 1); return; }
    if (c == ':') { ssd1306_set_pixel(x + 1, y + 2, 1); ssd1306_set_pixel(x + 1, y + 4, 1); return; }
    uint32_t m = glyph_mask(c >= 'a' && c <= 'z' ? c - 32 : c); /* fold lowercase */
    for (unsigned i = 0; i < sizeof(SEG) / sizeof(SEG[0]); i++) {
        if (m & (1u << i))
            ssd1306_draw_line(x + SEG[i].x0, y + SEG[i].y0, x + SEG[i].x1, y + SEG[i].y1);
    }
}

void ssd1306_draw_string(int x, int y, const char *s)
{
    int cx = x;
    while (*s) {
        ssd1306_draw_char(cx, y, *s);
        cx += 6;
        s++;
    }
}
