// Tiles, text, templates and glyph rendering.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "save.h"   // glyph_t, font

// Background tiles (GBDK's default 0x8800 addressing, so 256 BG tiles of our own):
//   0-127    per screen: glyphs of the font being edited (the map shows one page at a time in 0-94),
//            text set in the font, reference characters drawn from Pocket Sans
//   128-222  UI text in Pocket Sans (code c at 128 + c - 32; 128 is blank)
//   224-235  editor canvas cells: state * 4 + corner
//   236-240  guide tick, canvas edge, menu frame (vertical, horizontal, corner)
//   241-249  challenge timer bar, 0-8 columns filled
#define TILE_UI(c) (128u + (uint8_t)(c) - 32u)
#define TILE_BLANK 128u
#define TILE_CANVAS 224u
#define TILE_TICK 236u
#define TILE_EDGE 237u
#define TILE_FRAME_V 238u
#define TILE_FRAME_H 239u
#define TILE_FRAME_C 240u
#define TILE_BAR 241u

#define CELL_OFF 0
#define CELL_GUIDE 1
#define CELL_INK 2

// Sprite tiles (0x8000, separate from the BG tiles above)
#define SPR_TILE_CORNER 0
#define SPR_TILE_BAR 1
#define SPR_TILE_MARK 2   // dashed vertical line: the width markers in the editor

// Starter templates, in template_id order (the id stored in each save slot)
#define TEMPLATE_COUNT 3
#define TEMPLATE_NONE 0xFF
extern const uint8_t pocket_sans[GLYPH_COUNT][8];   // in ROM bank 1, which stays mapped
extern const glyph_t *const template_fonts[TEMPLATE_COUNT];
extern const char *const template_names[TEMPLATE_COUNT];   // up to 12 characters
extern const char *const template_short[TEMPLATE_COUNT];   // up to 5 characters
const char *template_name(uint8_t id);                     // "NO GUIDE" for TEMPLATE_NONE
uint8_t template_next(uint8_t id);                         // cycles 0, 1, 2, none, 0...

// The character set comes in three pages, each shown as a 16-wide grid (map, keyboard, challenge results):
//   0  BASIC    codes 32-126 (95)
//   1  SYMBOLS  codes 128-191 (64): arrows, card suits, quotes, then Latin-1 punctuation
//   2  ACCENTS  codes 192-255 (64): accented capitals, then lowercase
#define PAGE_COUNT 3
extern const uint8_t page_first[PAGE_COUNT];    // glyph index of the page's first cell
extern const uint8_t page_cells[PAGE_COUNT];
extern const char *const page_titles[PAGE_COUNT];
uint8_t page_of(uint8_t i);
uint8_t glyph_step(uint8_t i, int8_t dir);       // next/previous glyph index, skipping 127, wrapping
uint8_t other_case(uint8_t code);                // the same letter in the other case (ASCII, Latin-1), or 0

void gfx_init(void);
void gfx_clear(void);
void text(uint8_t x, uint8_t y, const char *s);
void text_pad(uint8_t x, uint8_t y, const char *s, uint8_t width);  // pads with blanks
void win_text(uint8_t x, uint8_t y, const char *s);
void text_num(uint8_t x, uint8_t y, uint8_t n);   // left-aligned in 3 cells
char *num_cat(char *p, uint8_t n);                // write n in decimal at p, return the end (no NUL)
void text_frac(uint8_t x, uint8_t y, uint8_t a, uint8_t b, uint8_t width);   // "a/b", padded with blanks
void tile_from(uint8_t tile, const uint8_t *ink, const uint8_t *guide);  // either may be 0
void glyph_tile(uint8_t tile, uint8_t i, uint8_t guide);  // glyph i (+ its template guide) into a BG tile
uint8_t ref_tile(uint8_t i, uint8_t spare);       // tile showing character i in Pocket Sans: the UI tile for
                                                  // ASCII, else drawn into `spare`
void bars_show(void);                             // sprites 0-1: the bars above and below a grid pick
void bars_place(uint8_t x0, uint8_t y0, uint8_t k);  // cell k of a 16-wide grid at (x0, y0), every other row
const uint8_t *guide_rows(uint8_t i);             // template rows for glyph i, or 0
uint8_t glyph_width(uint8_t i);                   // the width GB Studio will use (same rule as the exporter)
uint8_t glyph_ink_left(uint8_t i);                // first column with ink (1 if blank)
void hide_sprites(void);                          // all 40

// Text in the font being made, exactly as GB Studio will set it (same rules as tools/sav2font.py):
// variable width, ink moved to start at column 1, letters drawn in one case used for both.
#define TEXT_LINE_PX 144                          // 18 tiles
uint8_t text_glyph(uint8_t c, const uint8_t **rows, uint8_t *lo, uint8_t *left);  // returns advance width
uint8_t text_layout(const char *s, uint8_t len, uint8_t max_lines, uint8_t *starts, uint8_t *lens);
void text_line_tiles(const char *s, uint8_t len, uint8_t tile0);   // render one line into 18 BG tiles
void text_block(const char *s, uint8_t len, uint8_t x, uint8_t y, uint8_t lines, uint8_t tile0);
                                                  // wrap and draw on every other row from (x, y)

#endif
