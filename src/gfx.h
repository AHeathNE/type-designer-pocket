// Tiles, text and glyph rendering.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

// Background tiles (GBDK's default 0x8800 addressing, so 256 BG tiles of our own):
//   0-94     the font being edited, one tile per code 32-126 (live: ink in black, guide in light grey)
//   128-222  UI text in Pocket Sans (code c at 128 + c - 32; 128 is blank)
//   224-235  editor canvas cells: state * 4 + corner
//   236-237  guide tick, canvas right edge
#define TILE_UI(c) (128u + (uint8_t)(c) - 32u)
#define TILE_BLANK 128u
#define TILE_CANVAS 224u
#define TILE_TICK 236u
#define TILE_EDGE 237u

#define CELL_OFF 0
#define CELL_GUIDE 1
#define CELL_INK 2

// Sprite tiles (0x8000, separate from the BG tiles above)
#define SPR_TILE_CORNER 0
#define SPR_TILE_BAR 1

extern const uint8_t pocket_sans[95][8];

void gfx_init(void);
void gfx_clear(void);
void text(uint8_t x, uint8_t y, const char *s);
void text_num(uint8_t x, uint8_t y, uint8_t n);  // left-aligned, returns nothing; clears 3 cells
void glyph_tile(uint8_t i, uint8_t guide);       // redraw BG tile i from font + optional guide
uint8_t glyph_width(uint8_t i);                  // the width GB Studio will use (same rule as the exporter)
void hide_sprites(void);

#endif
