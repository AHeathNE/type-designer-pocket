#include <gb/gb.h>
#include <string.h>
#include "gfx.h"
#include "save.h"

// put colour c (0-3) at (x, y) of a 2bpp tile
static void tile_px(uint8_t *t, uint8_t x, uint8_t y, uint8_t c) {
    uint8_t bit = 0x80 >> x;
    if (c & 1) t[y * 2] |= bit;
    if (c & 2) t[y * 2 + 1] |= bit;
}

// Editor cells are 2x2 tiles per pixel; grid lines run along the top and left of each pixel.
static void make_canvas_tiles(void) {
    static const uint8_t fill[3] = {0, 1, 3};   // off, guide, ink
    static const uint8_t line[3] = {1, 2, 2};
    uint8_t t[16], s, corner, x, y, c;
    for (s = 0; s < 3; s++) {
        for (corner = 0; corner < 4; corner++) {   // 0 TL, 1 TR, 2 BL, 3 BR
            memset(t, 0, sizeof t);
            for (y = 0; y < 8; y++) {
                for (x = 0; x < 8; x++) {
                    c = fill[s];
                    if ((y == 0 && corner < 2) || (x == 0 && (corner & 1) == 0)) c = line[s];
                    tile_px(t, x, y, c);
                }
            }
            set_bkg_data(TILE_CANVAS + s * 4 + corner, 1, t);
        }
    }
    memset(t, 0, sizeof t);
    for (x = 2; x < 8; x++) tile_px(t, x, 0, 3);
    set_bkg_data(TILE_TICK, 1, t);
    memset(t, 0, sizeof t);
    for (y = 0; y < 8; y++) tile_px(t, 0, y, 1);
    set_bkg_data(TILE_EDGE, 1, t);
}

static void make_sprite_tiles(void) {
    uint8_t t[16], i;
    // cursor corner: a black L with a white inner L, so it shows on both ink and paper
    memset(t, 0, sizeof t);
    for (i = 0; i < 5; i++) { tile_px(t, i, 0, 3); tile_px(t, 0, i, 3); }
    for (i = 1; i < 4; i++) { tile_px(t, i, 1, 1); tile_px(t, 1, i, 1); }
    set_sprite_data(SPR_TILE_CORNER, 1, t);
    // map selection bar: two black rows at the bottom of the tile
    memset(t, 0, sizeof t);
    for (i = 0; i < 8; i++) { tile_px(t, i, 6, 3); tile_px(t, i, 7, 3); }
    set_sprite_data(SPR_TILE_BAR, 1, t);
}

void gfx_init(void) {
    BGP_REG = 0xE4;                 // 0 white, 1 light, 2 dark, 3 black
    OBP0_REG = 0xE0;                // sprites: 1 white, 2 dark, 3 black
    set_1bpp_colors(3, 0);
    set_bkg_1bpp_data(TILE_UI(32), 95, (const uint8_t *)pocket_sans);
    make_canvas_tiles();
    make_sprite_tiles();
    SPRITES_8x8;
}

void gfx_clear(void) {
    fill_bkg_rect(0, 0, 20, 18, TILE_BLANK);
    hide_sprites();
}

void hide_sprites(void) {
    uint8_t i;
    for (i = 0; i < 8; i++) move_sprite(i, 0, 0);
}

void text(uint8_t x, uint8_t y, const char *s) {
    char c;
    while ((c = *s++)) {
        if (c < 32 || c > 126) c = '?';
        set_bkg_tile_xy(x++, y, TILE_UI(c));
    }
}

void text_num(uint8_t x, uint8_t y, uint8_t n) {
    char buf[4];
    uint8_t i = 0;
    if (n >= 100) buf[i++] = '0' + n / 100;
    if (n >= 10) buf[i++] = '0' + (n / 10) % 10;
    buf[i++] = '0' + n % 10;
    while (i < 3) buf[i++] = ' ';
    buf[3] = 0;
    text(x, y, buf);
}

void glyph_tile(uint8_t i, uint8_t guide) {
    uint8_t t[16], r, ink, g;
    for (r = 0; r < 8; r++) {
        ink = font.glyphs[i][r];
        g = (guide && i < 95) ? pocket_sans[i][r] : 0;
        t[r * 2] = ink | g;   // ink -> colour 3, guide only -> colour 1
        t[r * 2 + 1] = ink;
    }
    set_bkg_data(i, 1, t);
}

uint8_t glyph_width(uint8_t i) {
    uint8_t cols = 0, r, lo, hi, w;
    if (font.widths[i] != WIDTH_AUTO) return font.widths[i];
    for (r = 0; r < 8; r++) cols |= font.glyphs[i][r];
    if (!cols) return font.space_width;
    for (lo = 0; !(cols & (0x80 >> lo)); lo++) {}
    for (hi = 7; !(cols & (0x80 >> hi)); hi--) {}
    w = hi - lo + 1;                       // ink width
    w = (w >= 8) ? 8 : w + 2;              // one blank column each side, capped at 8
    return w > 8 ? 8 : w;
}
