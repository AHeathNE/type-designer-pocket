#pragma bank 1   // ROM bank 1 is always mapped (the game never switches banks): see Makefile
// LEARN: short illustrated pages on the parts of a letter, how designers work, and where fonts come from.
//
//  row 0        title, page n/N
//  rows 2-9     up to two letters drawn big (a tile per pixel) from a template, with guide lines and the
//               part being explained in dark grey (or, for a space like a counter, light grey)
//  rows 11-16   up to six lines of text, 20 characters each
//  row 17       hints
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define LINE_CAP 1    // guide lines across the drawings: along the top of glyph row 1 (cap height),
#define LINE_X 2      // row 2 (x-height)
#define LINE_BASE 4   // and row 7 (the baseline is under row 6)

#define SANS 0
#define SERIF 1
#define CLASSIC 2

typedef struct {
    const char *title;    // up to 14 characters
    uint8_t code[2];      // the two letters (0: none), drawn at columns 2 and 10
    uint8_t font[2];      // their template
    uint8_t lines;        // LINE_* bits
    uint8_t mark[2][8];   // the part to point out, a bit per pixel, in each letter
    const char *text;     // word-wrapped to 20 columns, at most 6 lines (tests/test_m6.py checks)
} Page;

#define NONE {0, 0, 0, 0, 0, 0, 0, 0}

static const Page pages[] = {
    // ---- the parts of a letter ----
    {"BASELINE", {'H', 'x'}, {SANS, SANS}, LINE_BASE, {NONE, NONE},
     "Letters stand on the baseline, the line under them. In 8x8 fonts it runs under row 6, which leaves row 7 for tails."},
    {"CAP HEIGHT", {'H', 'h'}, {SANS, SANS}, LINE_CAP | LINE_BASE, {NONE, NONE},
     "Capitals reach the cap height: row 1 in Pocket Sans, so capitals are 6 pixels tall. Row 0 is for accents."},
    {"X-HEIGHT", {'x', 'n'}, {SANS, SANS}, LINE_X | LINE_BASE, {NONE, NONE},
     "The x-height is how tall a lowercase x is: 5 pixels here, rows 2-6. A big x-height keeps small text easy to read."},
    {"ASCENDERS", {'h', 'p'}, {SANS, SANS}, LINE_X | LINE_BASE,
     {{0, 0x40, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0x40, 0x40}},
     "Ascenders rise above the x-height: b d h k l. Descenders drop below the baseline: g j p q y."},
    {"STEMS", {'n', 'H'}, {SANS, SANS}, 0,
     {{0, 0, 0x40, 0x40, 0x40, 0x40, 0x40, 0}, {0, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0}},
     "A stem is a main upright stroke. Its thickness sets the weight of the font: 1 pixel is light, 2 pixels is bold."},
    {"BOWL, COUNTER", {'b', 'o'}, {SANS, SANS}, 0,
     {{0, 0, 0x38, 0x04, 0x04, 0x04, 0x38, 0}, {0, 0, 0, 0x38, 0x38, 0x38, 0, 0}},
     "A bowl is a curve that closes a shape (b d o p q). The space inside is the counter. Keep counters open."},
    {"CROSSBAR", {'A', 'H'}, {SANS, SANS}, 0,
     {{0, 0, 0, 0, 0x38, 0, 0, 0}, {0, 0, 0, 0x38, 0, 0, 0, 0}},
     "A crossbar joins two strokes (A H) or crosses one (f t). Sitting a little above the middle, it looks centred."},
    {"APEX, VERTEX", {'A', 'V'}, {SANS, SANS}, 0,
     {{0, 0x38, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0x28, 0x10, 0}},
     "The apex is where strokes meet at the top (A), the vertex where they meet at the bottom (V W). In pixels they go flat."},
    {"SERIFS", {'n', 'n'}, {SANS, SERIF}, LINE_X | LINE_BASE,
     {NONE, {0, 0, 0x80, 0, 0, 0, 0xAA, 0}},
     "Serifs are the small feet at the ends of strokes. Pocket Serif has slab serifs. Pocket Sans has none."},
    {"ACCENTS", {193, 233}, {SANS, SANS}, LINE_CAP | LINE_X | LINE_BASE,
     {{0x08, 0x10, 0, 0, 0, 0, 0, 0}, {0x08, 0x10, 0, 0, 0, 0, 0, 0}},
     "Accents sit above a letter, or under it, as in c-cedilla. With just 8 rows, accented capitals give up a row."},
    // ---- designing ----
    {"SPACING", {'n', 'o'}, {SANS, SANS}, 0, {NONE, NONE},
     "Space matters as much as shape. GB Studio adds a blank column each side of a letter. Check real words in TYPE TEST."},
    {"CONTROL", {'n', 'o'}, {SANS, SANS}, LINE_X | LINE_BASE, {NONE, NONE},
     "Designers often start with n and o, then H and O: they set the stems, curves and spacing. Try ORDER: CONTROL."},
    {"FAMILIES", {'n', 'h'}, {SANS, SANS}, 0,
     {NONE, {0, 0, 0x78, 0x44, 0x44, 0x44, 0x44, 0}},
     "Letters share parts: n makes h m r u, and o makes c e d. The editor's FROM (in the START menu) copies one in."},
    {"TEST WORDS", {'h', 'g'}, {SANS, SANS}, 0, {NONE, NONE},
     "Hamburgefonstiv is a classic test word: its letters show a font's style fast. ORDER: HAMBURG draws them first."},
    {"FREQUENCY", {'e', 't'}, {SANS, SANS}, 0, {NONE, NONE},
     "In English, e t a o i n are the most used letters. ORDER: FREQUENT draws those first, so real words read soonest."},
    // ---- where fonts come from ----
    {"MOVABLE TYPE", {'A', 'a'}, {SERIF, SERIF}, 0, {NONE, NONE},
     "Movable type began in China around 1040. Korea printed with metal type by 1377, Gutenberg around 1450."},
    {"ROMAN AND SANS", {'H', 'H'}, {SERIF, SANS}, 0, {NONE, NONE},
     "Roman type with serifs took shape in Venice around 1470. Sans serif type came much later, in London in the 1810s."},
    {"BITMAP FONTS", {'A', 'a'}, {CLASSIC, CLASSIC}, 0, {NONE, NONE},
     "Screens draw letters from a grid of pixels. Early PCs kept 8x8 fonts in ROM, like the IBM font in Classic 8X8."},
    {"GAME BOY TILES", {'G', 'B'}, {SANS, SANS}, 0, {NONE, NONE},
     "A Game Boy screen is built from 8x8 tiles, 20 across and 18 down. A letter per tile makes 8x8 fonts a natural fit."},
    {"PROPORTIONAL", {'i', 'm'}, {SANS, SANS}, 0, {NONE, NONE},
     "Monospace fonts give each letter one width. Proportional ones let i be thin and m wide. GB Studio does both."},
};
#define PAGE_COUNT_L (sizeof pages / sizeof pages[0])

#define TILE_MARK_INK 250    // a pointed-out pixel: dark grey
#define TILE_LINE 251        // an empty cell with a guide line along its top

uint8_t learn_back = SCR_SLOTS;
static uint8_t page;

static void make_tiles(void) {
    uint8_t t[16], y;
    // a pointed-out pixel: dark grey (colour 2) with the canvas's light-grey grid lines along top and left
    t[0] = 0xFF; t[1] = 0x00;
    for (y = 1; y < 8; y++) { t[y * 2] = 0x80; t[y * 2 + 1] = 0x7F; }
    set_bkg_data(TILE_MARK_INK, 1, t);
    // an empty cell with a black guide line along the top
    t[0] = 0xFF; t[1] = 0xFF;
    for (y = 1; y < 8; y++) { t[y * 2] = 0x80; t[y * 2 + 1] = 0x00; }
    set_bkg_data(TILE_LINE, 1, t);
}

static void draw_letter(uint8_t k) {
    const Page *p = &pages[page];
    const uint8_t *rows = template_fonts[p->font[k]][p->code[k] - FIRST_CODE];
    uint8_t r, b, ink, mark, tile, x0 = k ? 10 : 2, line;
    for (r = 0; r < 8; r++) {
        line = (r == 1 && (p->lines & LINE_CAP)) || (r == 2 && (p->lines & LINE_X)) ||
               (r == 7 && (p->lines & LINE_BASE));
        for (b = 0; b < 8; b++) {
            ink = (rows[r] >> (7 - b)) & 1;
            mark = (p->mark[k][r] >> (7 - b)) & 1;
            if (ink) tile = mark ? TILE_MARK_INK : TILE_CANVAS + CELL_INK * 4;
            else if (mark) tile = TILE_CANVAS + CELL_GUIDE * 4;
            else if (line) tile = TILE_LINE;
            else tile = TILE_CANVAS + CELL_OFF * 4;
            set_bkg_tile_xy(x0 + b, 2 + r, tile);
        }
    }
}

static void draw_page(void) {
    const Page *p = &pages[page];
    const char *s = p->text;
    char line[21];
    uint8_t y = 11, n, brk;
    fill_bkg_rect(0, 0, 20, 17, TILE_BLANK);
    text(0, 0, p->title);
    text_frac(15, 0, page + 1, PAGE_COUNT_L, 5);
    if (p->code[0]) draw_letter(0);
    if (p->code[1]) draw_letter(1);
    while (*s && y < 17) {   // greedy word wrap at 20 columns
        for (n = 0, brk = 0; s[n] && n < 20; n++)
            if (s[n] == ' ') brk = n;
        if (s[n] && s[n] != ' ' && brk) n = brk;
        memcpy(line, s, n);
        line[n] = 0;
        text(0, y++, line);
        s += n;
        while (*s == ' ') s++;
    }
}

void learn_enter(void) {
    DISPLAY_OFF;
    gfx_clear();
    make_tiles();
    draw_page();
    text(0, 17, "<>:PAGE  B:BACK");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t learn_update(void) {
    if (moves & J_LEFT) { page = page ? page - 1 : PAGE_COUNT_L - 1; draw_page(); }
    if (moves & J_RIGHT) { page = (page + 1) % PAGE_COUNT_L; draw_page(); }
    if (pressed & J_B) return learn_back;
    return SCR_STAY;
}
