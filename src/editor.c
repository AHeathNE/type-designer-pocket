// Glyph editor: the 8x8 glyph blown up to 16x16 px per pixel.
//
//  row 0        reference char, code, width, SEL hint
//  rows 1-16    canvas (columns 2-17), guide ticks in column 1, edge in column 18, live preview at (19, 1)
//  row 17       hints
//
//  D-pad move (hold to repeat)     A paint (toggles; hold A and move to keep painting)
//  B erase (hold and move)         SELECT + Left/Right previous/next glyph
//  SELECT tap  guide on/off        START save and back to the map
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define CANVAS_X 2
#define CANVAS_Y 1

static uint8_t cx = 1, cy = 1;   // cursor, in glyph pixels
static uint8_t paint;            // value A paints while held
static uint8_t guide = 1;        // show Pocket Sans under the canvas
static uint8_t select_combo;     // SELECT was used with the d-pad, so its release isn't a tap
static uint8_t dirty;

static uint8_t pix(uint8_t x, uint8_t y) {
    return (font.glyphs[current][y] >> (7 - x)) & 1;
}

static void draw_cell(uint8_t x, uint8_t y) {
    uint8_t state, base, t[4];
    if (pix(x, y)) state = CELL_INK;
    else if (guide && current < 95 && ((pocket_sans[current][y] >> (7 - x)) & 1)) state = CELL_GUIDE;
    else state = CELL_OFF;
    base = TILE_CANVAS + state * 4;
    t[0] = base; t[1] = base + 1; t[2] = base + 2; t[3] = base + 3;
    set_bkg_tiles(CANVAS_X + 2 * x, CANVAS_Y + 2 * y, 2, 2, t);
}

static void draw_canvas(void) {
    uint8_t x, y;
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) draw_cell(x, y);
}

static void draw_width(void) {
    text(7, 0, "W");
    text_num(8, 0, glyph_width(current));
}

static void draw_header(void) {
    set_bkg_tile_xy(0, 0, TILE_UI(FIRST_CODE + current));
    text(2, 0, "#");
    text_num(3, 0, FIRST_CODE + current);
    draw_width();
    text(14, 0, "SEL<>");
    glyph_tile(current, 0);
    set_bkg_tile_xy(19, 1, current);
}

static void place_cursor(void) {
    uint8_t x = (CANVAS_X + 2 * cx) * 8 + 8;   // OAM x = screen x + 8
    uint8_t y = (CANVAS_Y + 2 * cy) * 8 + 16;  // OAM y = screen y + 16
    move_sprite(0, x, y);
    move_sprite(1, x + 8, y);
    move_sprite(2, x, y + 8);
    move_sprite(3, x + 8, y + 8);
}

static void set_pix(uint8_t x, uint8_t y, uint8_t v) {
    uint8_t bit = 0x80 >> x;
    if (pix(x, y) == v) return;
    if (v) font.glyphs[current][y] |= bit;
    else font.glyphs[current][y] &= ~bit;
    dirty = 1;
    draw_cell(x, y);
    glyph_tile(current, 0);
    draw_width();
}

static void save_if_dirty(void) {
    if (dirty) {
        save_write();
        dirty = 0;
    }
}

static void switch_glyph(int8_t dir) {
    save_if_dirty();
    if (dir < 0) current = current ? current - 1 : EDIT_COUNT - 1;
    else current = (current + 1 < EDIT_COUNT) ? current + 1 : 0;
    draw_header();
    draw_canvas();
}

void editor_enter(void) {
    uint8_t y;
    DISPLAY_OFF;
    gfx_clear();
    for (y = CANVAS_Y; y < CANVAS_Y + 16; y++) set_bkg_tile_xy(CANVAS_X + 16, y, TILE_EDGE);
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 1, TILE_TICK);   // cap line
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 2, TILE_TICK);   // x-height
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 7, TILE_TICK);   // baseline
    text(0, 17, "A:INK B:ERASE ST:OK");
    draw_header();
    draw_canvas();

    set_sprite_tile(0, SPR_TILE_CORNER); set_sprite_prop(0, 0);
    set_sprite_tile(1, SPR_TILE_CORNER); set_sprite_prop(1, S_FLIPX);
    set_sprite_tile(2, SPR_TILE_CORNER); set_sprite_prop(2, S_FLIPY);
    set_sprite_tile(3, SPR_TILE_CORNER); set_sprite_prop(3, S_FLIPX | S_FLIPY);
    place_cursor();
    dirty = 0;
    select_combo = 0;
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t editor_update(void) {
    uint8_t moved = 0;

    if (released & J_SELECT) {
        if (!select_combo) {
            guide ^= 1;
            draw_canvas();
        }
        select_combo = 0;
    }
    if (keys & J_SELECT) {
        if (pressed & J_LEFT) { switch_glyph(-1); select_combo = 1; }
        if (pressed & J_RIGHT) { switch_glyph(1); select_combo = 1; }
        return 0;
    }

    if ((moves & J_LEFT) && cx > 0) { cx--; moved = 1; }
    if ((moves & J_RIGHT) && cx < 7) { cx++; moved = 1; }
    if ((moves & J_UP) && cy > 0) { cy--; moved = 1; }
    if ((moves & J_DOWN) && cy < 7) { cy++; moved = 1; }
    if (moved) place_cursor();

    if (pressed & J_A) {
        paint = !pix(cx, cy);
        set_pix(cx, cy, paint);
    } else if ((keys & J_A) && moved) {
        set_pix(cx, cy, paint);
    }
    if ((pressed & J_B) || ((keys & J_B) && moved)) set_pix(cx, cy, 0);

    if (pressed & J_START) {
        save_if_dirty();
        return 1;
    }
    return 0;
}
