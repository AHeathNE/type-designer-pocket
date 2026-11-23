// Tiles, text, templates and glyph rendering.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "save.h"   // glyph_t, font

// Background tiles (GBDK's default 0x8800 addressing, so 256 BG tiles of our own):
//   0-94     the font being edited, one tile per code 32-126 (live: ink in black, guide in light grey)
//   128-222  UI text in Pocket Sans (code c at 128 + c - 32; 128 is blank)
//   224-235  editor canvas cells: state * 4 + corner
//   236-240  guide tick, canvas edge, menu frame (vertical, horizontal, corner)
#define TILE_UI(c) (128u + (uint8_t)(c) - 32u)
#define TILE_BLANK 128u
#define TILE_CANVAS 224u
#define TILE_TICK 236u
#define TILE_EDGE 237u
#define TILE_FRAME_V 238u
#define TILE_FRAME_H 239u
#define TILE_FRAME_C 240u

#define CELL_OFF 0
#define CELL_GUIDE 1
#define CELL_INK 2

// Sprite tiles (0x8000, separate from the BG tiles above)
#define SPR_TILE_CORNER 0
#define SPR_TILE_BAR 1

// Starter templates, in template_id order (the id stored in each save slot)
#define TEMPLATE_COUNT 3
#define TEMPLATE_NONE 0xFF
extern const uint8_t pocket_sans[95][8];
extern const glyph_t *const template_fonts[TEMPLATE_COUNT];
extern const char *const template_names[TEMPLATE_COUNT];   // up to 12 characters
extern const char *const template_short[TEMPLATE_COUNT];   // up to 5 characters
const char *template_name(uint8_t id);                     // "NO GUIDE" for TEMPLATE_NONE
uint8_t template_next(uint8_t id);                         // cycles 0, 1, 2, none, 0...

void gfx_init(void);
void gfx_clear(void);
void text(uint8_t x, uint8_t y, const char *s);
void text_pad(uint8_t x, uint8_t y, const char *s, uint8_t width);  // pads with blanks
void win_text(uint8_t x, uint8_t y, const char *s);
void text_num(uint8_t x, uint8_t y, uint8_t n);   // left-aligned in 3 cells
void tile_from(uint8_t tile, const uint8_t *ink, const uint8_t *guide);  // either may be 0
void glyph_tile(uint8_t i, uint8_t guide);        // BG tile i from the font + its template guide
const uint8_t *guide_rows(uint8_t i);             // template rows for glyph i, or 0
uint8_t glyph_width(uint8_t i);                   // the width GB Studio will use (same rule as the exporter)
void hide_sprites(void);

#endif
