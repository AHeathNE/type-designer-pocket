#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "gfx.h"
#include "save.h"

extern const uint8_t pocket_sans[GLYPH_COUNT][8];   // all three in ROM bank FONT_BANK
extern const uint8_t pocket_serif[GLYPH_COUNT][8];
extern const uint8_t classic_8x8[GLYPH_COUNT][8];
static const glyph_t *const template_fonts[TEMPLATE_COUNT] = {pocket_sans, pocket_serif, classic_8x8};
static uint8_t rows_buf[8];

// Bank 0 code, so it can map the font bank in, copy, and map the caller's bank back.
const uint8_t *template_rows(uint8_t tmpl, uint8_t i) {
    uint8_t was = CURRENT_BANK;
    SWITCH_ROM(FONT_BANK);
    memcpy(rows_buf, template_fonts[tmpl][i], 8);
    SWITCH_ROM(was);
    return rows_buf;
}
const char *const template_names[TEMPLATE_COUNT] = {"POCKET SANS", "POCKET SERIF", "CLASSIC 8X8"};
const char *const template_short[TEMPLATE_COUNT] = {"SANS", "SERIF", "8X8"};

const char *template_name(uint8_t id) {
    return id < TEMPLATE_COUNT ? template_names[id] : "NO GUIDE";
}

uint8_t template_next(uint8_t id) {
    if (id == TEMPLATE_NONE) return 0;
    return (id + 1 < TEMPLATE_COUNT) ? id + 1 : TEMPLATE_NONE;
}

const uint8_t page_first[PAGE_COUNT] = {0, 96, 160};   // codes 32, 128, 192
const uint8_t page_cells[PAGE_COUNT] = {BASIC_COUNT, 64, 64};
const char *const page_titles[PAGE_COUNT] = {"BASIC 32-126", "SYMBOLS 128-191", "ACCENTS 192-255"};

uint8_t page_of(uint8_t i) {
    return i < page_first[1] ? 0 : (i < page_first[2] ? 1 : 2);
}

uint8_t glyph_step(uint8_t i, int8_t dir) {
    do {
        if (dir < 0) i = i ? i - 1 : GLYPH_COUNT - 1;
        else i = (i + 1 < GLYPH_COUNT) ? i + 1 : 0;
    } while (i == UNUSED_GLYPH);
    return i;
}

uint8_t other_case(uint8_t c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 192 && c <= 222 && c != 215)) return c + 32;
    if ((c >= 'a' && c <= 'z') || (c >= 224 && c <= 254 && c != 247)) return c - 32;
    return 0;
}

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
    // menu frame: black lines along the left and top of the window
    memset(t, 0, sizeof t);
    for (y = 0; y < 8; y++) tile_px(t, 0, y, 3);
    set_bkg_data(TILE_FRAME_V, 1, t);
    memset(t, 0, sizeof t);
    for (x = 0; x < 8; x++) tile_px(t, x, 0, 3);
    set_bkg_data(TILE_FRAME_H, 1, t);
    for (y = 0; y < 8; y++) tile_px(t, 0, y, 3);
    set_bkg_data(TILE_FRAME_C, 1, t);
    // timer bar: the first s columns solid black, the rest a light-grey track
    for (s = 0; s <= 8; s++) {
        memset(t, 0, sizeof t);
        for (x = 0; x < 8; x++)
            for (y = 1; y < 7; y++)
                if (x < s) tile_px(t, x, y, 3);
                else if (y == 3 || y == 4) tile_px(t, x, y, 1);
        set_bkg_data(TILE_BAR + s, 1, t);
    }
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
    // width marker: a 2 px dash down the left edge (one sprite every 16 px makes a dashed line)
    memset(t, 0, sizeof t);
    for (i = 0; i < 8; i++) { tile_px(t, 0, i, 3); tile_px(t, 1, i, 3); }
    set_sprite_data(SPR_TILE_MARK, 1, t);
}

uint8_t is_color;
static const char *const theme_names[THEME_COUNT] = {"GREY", "GREEN", "PAPER", "NIGHT"};
static const palette_color_t themes[THEME_COUNT][4] = {
    {RGB8(255, 255, 255), RGB8(170, 170, 170), RGB8(85, 85, 85), RGB8(0, 0, 0)},
    {RGB8(224, 248, 208), RGB8(136, 192, 112), RGB8(52, 104, 86), RGB8(8, 24, 32)},       // DMG green
    {RGB8(243, 240, 230), RGB8(201, 198, 184), RGB8(107, 115, 102), RGB8(29, 35, 28)},    // the website's
    {RGB8(16, 24, 42), RGB8(52, 80, 110), RGB8(110, 143, 176), RGB8(240, 246, 255)},      // light on dark
};

void theme_apply(void) {
    palette_color_t spr[4];
    const palette_color_t *t;
    if (!is_color) return;
    t = themes[save_theme() < THEME_COUNT ? save_theme() : 0];
    if (LCDC_REG & LCDCF_ON) vsync();   // palette RAM can't be written while the screen is being drawn
    set_bkg_palette(0, 1, t);
    spr[0] = t[0];   // sprites: colour 1 is the paper (OBP0 0xE0 on the original), 2 and 3 as the BG
    spr[1] = t[0];
    spr[2] = t[2];
    spr[3] = t[3];
    set_sprite_palette(0, 1, spr);
}

void theme_name(uint8_t x, uint8_t y) {
    text_pad(x, y, theme_names[save_theme() < THEME_COUNT ? save_theme() : 0], 6);
}

void gfx_init(void) {
    uint8_t was = CURRENT_BANK;
    is_color = (_cpu == CGB_TYPE);
    BGP_REG = 0xE4;                 // 0 white, 1 light, 2 dark, 3 black
    OBP0_REG = 0xE0;                // sprites: 1 white, 2 dark, 3 black
    set_1bpp_colors(3, 0);
    SWITCH_ROM(FONT_BANK);
    set_bkg_1bpp_data(TILE_UI(32), BASIC_COUNT, (const uint8_t *)pocket_sans);
    SWITCH_ROM(was);
    make_canvas_tiles();
    make_sprite_tiles();
    SPRITES_8x8;
}

void gfx_clear(void) {
    HIDE_WIN;
    fill_bkg_rect(0, 0, 20, 18, TILE_BLANK);
    hide_sprites();
}

void hide_sprites(void) {
    uint8_t i;
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);
}

void text(uint8_t x, uint8_t y, const char *s) {
    char c;
    while ((c = *s++)) {
        if (c < 32 || c > 126) c = '?';
        set_bkg_tile_xy(x++, y, TILE_UI(c));
    }
}

void text_pad(uint8_t x, uint8_t y, const char *s, uint8_t width) {
    while (width && *s) {
        set_bkg_tile_xy(x++, y, TILE_UI(*s >= 32 && *s <= 126 ? *s : '?'));
        s++;
        width--;
    }
    while (width--) set_bkg_tile_xy(x++, y, TILE_BLANK);
}

void win_text(uint8_t x, uint8_t y, const char *s) {
    char c;
    while ((c = *s++)) set_win_tile_xy(x++, y, TILE_UI(c >= 32 && c <= 126 ? c : '?'));
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

char *num_cat(char *p, uint8_t n) {
    if (n >= 100) *p++ = '0' + n / 100;
    if (n >= 10) *p++ = '0' + (n / 10) % 10;
    *p++ = '0' + n % 10;
    return p;
}

void text_frac(uint8_t x, uint8_t y, uint8_t a, uint8_t b, uint8_t width) {
    char buf[8], *p = num_cat(buf, a);
    *p++ = '/';
    *num_cat(p, b) = 0;
    text_pad(x, y, buf, width);
}

void tile_from(uint8_t tile, const uint8_t *ink, const uint8_t *guide) {
    uint8_t t[16], r, k, g;
    for (r = 0; r < 8; r++) {
        k = ink ? ink[r] : 0;
        g = guide ? guide[r] : 0;
        t[r * 2] = k | g;   // ink -> colour 3, guide only -> colour 1
        t[r * 2 + 1] = k;
    }
    set_bkg_data(tile, 1, t);
}

const uint8_t *guide_rows(uint8_t i) {
    if (font.template_id >= TEMPLATE_COUNT) return 0;
    return template_rows(font.template_id, i);
}

void glyph_tile(uint8_t tile, uint8_t i, uint8_t guide) {
    tile_from(tile, font.glyphs[i], guide ? guide_rows(i) : 0);
}

uint8_t ref_tile(uint8_t i, uint8_t spare) {
    if (i < BASIC_COUNT) return TILE_UI(FIRST_CODE + i);
    set_bkg_1bpp_data(spare, 1, template_rows(0, i));
    return spare;
}

void bars_show(void) {
    set_sprite_tile(0, SPR_TILE_BAR);
    set_sprite_prop(0, 0);
    set_sprite_tile(1, SPR_TILE_BAR);
    set_sprite_prop(1, S_FLIPY);
}

void bars_place(uint8_t x0, uint8_t y0, uint8_t k) {
    uint8_t x = (x0 + k % 16) * 8 + 8;          // OAM x = screen x + 8
    uint8_t y = (y0 + 2 * (k / 16)) * 8 + 16;   // OAM y = screen y + 16
    move_sprite(0, x, y - 8);
    move_sprite(1, x, y + 8);
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

uint8_t glyph_ink_left(uint8_t i) {
    uint8_t cols = 0, r, lo;
    for (r = 0; r < 8; r++) cols |= font.glyphs[i][r];
    if (!cols) return 1;
    for (lo = 0; !(cols & (0x80 >> lo)); lo++) {}
    return lo;
}

// ---- text set in the font being made ----

static uint8_t line_buf[TEXT_LINE_PX];   // 18 tiles x 8 rows, 1bpp

uint8_t text_glyph(uint8_t c, const uint8_t **rows, uint8_t *lo, uint8_t *left) {
    uint8_t i, j, cols = 0, r, hi, w;
    *rows = 0;
    if (c < 32 || c == 127) return font.space_width;
    i = j = c - 32;
    if (!glyph_has_ink(i) && (r = other_case(c))) {
        j = r - 32;
        if (!glyph_has_ink(j)) j = i;
    }
    for (r = 0; r < 8; r++) cols |= font.glyphs[j][r];
    if (!cols) return font.widths[i] != WIDTH_AUTO ? font.widths[i] : font.space_width;
    for (*lo = 0; !(cols & (0x80 >> *lo)); (*lo)++) {}
    for (hi = 7; !(cols & (0x80 >> hi)); hi--) {}
    w = hi - *lo + 1;
    *left = (w >= 8) ? 0 : 1;
    *rows = font.glyphs[j];
    if (font.widths[j] != WIDTH_AUTO) return font.widths[j];
    w += *left + 1;
    return w > 8 ? 8 : w;
}

static uint8_t advance(uint8_t c) {
    const uint8_t *rows;
    uint8_t lo, left;
    return text_glyph(c, &rows, &lo, &left);
}

// Greedy word wrap: break at the last space that fits; a word longer than a line breaks where it must.
uint8_t text_layout(const char *s, uint8_t len, uint8_t max_lines, uint8_t *starts, uint8_t *lens) {
    uint8_t i = 0, n = 0, start, j, w, brk;
    uint16_t x;
    while (i < len && n < max_lines) {
        while (i < len && s[i] == ' ') i++;          // no spaces at the start of a line
        if (i >= len) break;
        start = i;
        x = 0;
        brk = 0xFF;
        for (j = i; j < len; j++) {
            w = advance((uint8_t)s[j]);
            if (x + w > TEXT_LINE_PX) break;
            if (s[j] == ' ') brk = j;
            x += w;
        }
        if (j < len && brk != 0xFF && brk > start) {
            lens[n] = brk - start;
            i = brk + 1;
        } else {
            lens[n] = j - start;
            i = j;
        }
        starts[n++] = start;
    }
    return n;
}

void text_line_tiles(const char *s, uint8_t len, uint8_t tile0) {
    const uint8_t *rows;
    uint8_t k, r, w, lo, left, b, t, sh;
    uint16_t x = 0, px;
    memset(line_buf, 0, sizeof line_buf);
    for (k = 0; k < len && x < TEXT_LINE_PX; k++) {
        w = text_glyph((uint8_t)s[k], &rows, &lo, &left);
        if (rows) {
            px = x + left;
            t = px >> 3;
            sh = px & 7;
            if (px < TEXT_LINE_PX) {
                for (r = 0; r < 8; r++) {
                    b = rows[r] << lo;
                    line_buf[t * 8 + r] |= b >> sh;
                    if (sh && t < 17) line_buf[(t + 1) * 8 + r] |= b << (8 - sh);
                }
            }
        }
        x += w;
    }
    set_bkg_1bpp_data(tile0, TEXT_LINE_PX / 8, line_buf);
}

void text_block(const char *s, uint8_t len, uint8_t x, uint8_t y, uint8_t lines, uint8_t tile0) {
    uint8_t starts[8], lens[8], n, k, c;
    n = text_layout(s, len, lines, starts, lens);
    for (k = 0; k < lines; k++) {
        text_line_tiles(k < n ? s + starts[k] : s, k < n ? lens[k] : 0, tile0 + k * 18);
        for (c = 0; c < 18; c++) set_bkg_tile_xy(x + c, y + 2 * k, tile0 + k * 18 + c);
    }
}
