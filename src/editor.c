// Glyph editor: the 8x8 glyph blown up to 16x16 px per pixel.
//
//  row 0        reference char, code, width, SEL hint
//  rows 1-16    canvas (columns 2-17), guide ticks in column 1, edge in column 18, live preview at (19, 1)
//  row 17       hints, or a short message
//
//  D-pad move (hold to repeat)     A paint (toggles; hold A and move to keep painting)
//  B erase (hold and move)         SELECT + Left/Right previous/next glyph
//  SELECT tap  guide on/off        SELECT + B undo
//  START       menu: undo, copy, paste, flip, nudge, clear, guide font, done
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define CANVAS_X 2
#define CANVAS_Y 1
#define HINTS "A:INK B:RUB ST:MENU"
#define MSG_FRAMES 90

#define UNDO_MAX 16   // power of two

#define MENU_X 11     // window position, in tiles
#define MENU_Y 6
enum { M_UNDO, M_COPY, M_PASTE, M_FLIPH, M_FLIPV, M_NUDGE, M_CLEAR, M_GUIDE, M_DONE, MENU_COUNT };
static const char *const menu_labels[MENU_COUNT] = {
    "UNDO", "COPY", "PASTE", "FLIP H", "FLIP V", "NUDGE", "CLEAR", 0, "DONE"};

static uint8_t cx = 1, cy = 1;   // cursor, in glyph pixels
static uint8_t paint;            // value A paints while held
static uint8_t guide = 1;        // show the template under the canvas
static uint8_t select_combo;     // SELECT was used with another button, so its release isn't a tap
static uint8_t dirty;
static uint8_t stroke_pending;   // A/B went down: take an undo snapshot before the first change
static uint8_t menu_open, menu_sel, nudging, msg_timer;

static uint8_t undo_buf[UNDO_MAX][8];
static uint8_t undo_top, undo_count;
static uint8_t clip[8], has_clip;

static uint8_t pix(uint8_t x, uint8_t y) {
    return (font.glyphs[current][y] >> (7 - x)) & 1;
}

static void draw_cell(uint8_t x, uint8_t y) {
    uint8_t state, base, t[4];
    const uint8_t *g = guide ? guide_rows(current) : 0;
    if (pix(x, y)) state = CELL_INK;
    else if (g && ((g[y] >> (7 - x)) & 1)) state = CELL_GUIDE;
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

static void show_hints(void) {
    text_pad(0, 17, HINTS, 20);
    msg_timer = 0;
}

static void message(const char *s) {
    text_pad(0, 17, s, 20);
    msg_timer = MSG_FRAMES;
}

static void place_cursor(void) {
    uint8_t x = (CANVAS_X + 2 * cx) * 8 + 8;   // OAM x = screen x + 8
    uint8_t y = (CANVAS_Y + 2 * cy) * 8 + 16;  // OAM y = screen y + 16
    move_sprite(0, x, y);
    move_sprite(1, x + 8, y);
    move_sprite(2, x, y + 8);
    move_sprite(3, x + 8, y + 8);
}

// the whole glyph changed (undo, paste, flip, nudge, clear)
static void glyph_changed(void) {
    dirty = 1;
    draw_canvas();
    glyph_tile(current, 0);
    draw_width();
}

static void snapshot(void) {
    memcpy(undo_buf[undo_top], font.glyphs[current], 8);
    undo_top = (undo_top + 1) & (UNDO_MAX - 1);
    if (undo_count < UNDO_MAX) undo_count++;
}

static void undo(void) {
    if (!undo_count) {
        message("NOTHING TO UNDO");
        return;
    }
    undo_top = (undo_top - 1) & (UNDO_MAX - 1);
    undo_count--;
    memcpy(font.glyphs[current], undo_buf[undo_top], 8);
    glyph_changed();
    message("UNDONE");
}

static void set_pix(uint8_t x, uint8_t y, uint8_t v) {
    uint8_t bit = 0x80 >> x;
    if (pix(x, y) == v) return;
    if (stroke_pending) {
        snapshot();
        stroke_pending = 0;
    }
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
    undo_count = 0;   // undo history belongs to one glyph
    draw_header();
    draw_canvas();
}

// ---- whole-glyph tools ----

static void flip_h(void) {
    uint8_t r, b, out, i;
    for (r = 0; r < 8; r++) {
        b = font.glyphs[current][r];
        out = 0;
        for (i = 0; i < 8; i++) {
            out = (out << 1) | (b & 1);
            b >>= 1;
        }
        font.glyphs[current][r] = out;
    }
}

static void flip_v(void) {
    uint8_t r, t, *g = font.glyphs[current];
    for (r = 0; r < 4; r++) {
        t = g[r];
        g[r] = g[7 - r];
        g[7 - r] = t;
    }
}

static void nudge(uint8_t dir) {
    uint8_t r, *g = font.glyphs[current];
    if (dir & J_LEFT) for (r = 0; r < 8; r++) g[r] <<= 1;
    if (dir & J_RIGHT) for (r = 0; r < 8; r++) g[r] >>= 1;
    if (dir & J_UP) { for (r = 0; r < 7; r++) g[r] = g[r + 1]; g[7] = 0; }
    if (dir & J_DOWN) { for (r = 7; r > 0; r--) g[r] = g[r - 1]; g[0] = 0; }
}

// ---- menu (in the window layer, over the right side of the canvas) ----

static void menu_draw(void) {
    uint8_t i;
    const char *label;
    fill_win_rect(0, 0, 9, 12, TILE_BLANK);
    set_win_tile_xy(0, 0, TILE_FRAME_C);
    for (i = 1; i < 9; i++) set_win_tile_xy(i, 0, TILE_FRAME_H);
    for (i = 1; i < 12; i++) set_win_tile_xy(0, i, TILE_FRAME_V);
    for (i = 0; i < MENU_COUNT; i++) {
        label = menu_labels[i];
        if (i == M_GUIDE) {
            win_text(2, 1 + i, "G:");
            label = font.template_id < TEMPLATE_COUNT ? template_short[font.template_id] : "OFF";
            win_text(4, 1 + i, label);
        } else {
            win_text(2, 1 + i, label);
        }
        if (i == menu_sel) win_text(1, 1 + i, ">");
    }
    win_text(1, 11, "B:CLOSE");
}

static void menu_show(void) {
    menu_open = 1;
    menu_sel = 0;
    hide_sprites();
    menu_draw();
    move_win(MENU_X * 8 + 7, MENU_Y * 8);
    SHOW_WIN;
}

static void menu_close(void) {
    menu_open = 0;
    HIDE_WIN;
    place_cursor();
}

static uint8_t menu_update(void) {
    if (pressed & (J_B | J_START)) {
        menu_close();
        return SCR_STAY;
    }
    if (moves & J_UP) {
        menu_sel = menu_sel ? menu_sel - 1 : MENU_COUNT - 1;
        menu_draw();
    }
    if (moves & J_DOWN) {
        menu_sel = (menu_sel + 1 < MENU_COUNT) ? menu_sel + 1 : 0;
        menu_draw();
    }
    if (!(pressed & J_A)) return SCR_STAY;

    switch (menu_sel) {
    case M_UNDO:
        undo();
        break;
    case M_COPY:
        memcpy(clip, font.glyphs[current], 8);
        has_clip = 1;
        message("COPIED");
        break;
    case M_PASTE:
        if (!has_clip) {
            message("NOTHING COPIED");
            break;
        }
        snapshot();
        memcpy(font.glyphs[current], clip, 8);
        glyph_changed();
        message("PASTED");
        break;
    case M_FLIPH:
        snapshot();
        flip_h();
        glyph_changed();
        break;
    case M_FLIPV:
        snapshot();
        flip_v();
        glyph_changed();
        break;
    case M_NUDGE:
        snapshot();
        nudging = 1;
        menu_close();
        hide_sprites();
        text_pad(0, 17, "NUDGE: D-PAD  A:OK", 20);
        return SCR_STAY;
    case M_CLEAR:
        snapshot();
        memset(font.glyphs[current], 0, 8);
        glyph_changed();
        message("CLEARED");
        break;
    case M_GUIDE:
        font.template_id = template_next(font.template_id);
        dirty = 1;
        guide = 1;
        draw_canvas();
        menu_draw();
        return SCR_STAY;   // stay open so you can keep cycling
    case M_DONE:
        menu_close();
        save_if_dirty();
        return SCR_MAP;
    }
    menu_close();
    return SCR_STAY;
}

// ---- screen ----

void editor_enter(void) {
    uint8_t y;
    DISPLAY_OFF;
    gfx_clear();
    for (y = CANVAS_Y; y < CANVAS_Y + 16; y++) set_bkg_tile_xy(CANVAS_X + 16, y, TILE_EDGE);
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 1, TILE_TICK);   // cap line
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 2, TILE_TICK);   // x-height
    set_bkg_tile_xy(1, CANVAS_Y + 2 * 7, TILE_TICK);   // baseline
    show_hints();
    draw_header();
    draw_canvas();

    set_sprite_tile(0, SPR_TILE_CORNER); set_sprite_prop(0, 0);
    set_sprite_tile(1, SPR_TILE_CORNER); set_sprite_prop(1, S_FLIPX);
    set_sprite_tile(2, SPR_TILE_CORNER); set_sprite_prop(2, S_FLIPY);
    set_sprite_tile(3, SPR_TILE_CORNER); set_sprite_prop(3, S_FLIPX | S_FLIPY);
    place_cursor();
    dirty = 0;
    select_combo = 0;
    stroke_pending = 0;
    menu_open = 0;
    nudging = 0;
    undo_count = 0;
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t editor_update(void) {
    uint8_t moved = 0;

    if (msg_timer && --msg_timer == 0) show_hints();

    if (menu_open) return menu_update();

    if (nudging) {
        if (moves & J_DPAD) {
            nudge(moves);
            glyph_changed();
        }
        if (pressed & (J_A | J_B | J_START)) {
            nudging = 0;
            show_hints();
            place_cursor();
        }
        return SCR_STAY;
    }

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
        if (pressed & J_B) { undo(); select_combo = 1; }
        return SCR_STAY;
    }

    if (pressed & J_START) {
        menu_show();
        return SCR_STAY;
    }

    if ((moves & J_LEFT) && cx > 0) { cx--; moved = 1; }
    if ((moves & J_RIGHT) && cx < 7) { cx++; moved = 1; }
    if ((moves & J_UP) && cy > 0) { cy--; moved = 1; }
    if ((moves & J_DOWN) && cy < 7) { cy++; moved = 1; }
    if (moved) place_cursor();

    if (pressed & (J_A | J_B)) stroke_pending = 1;
    if (pressed & J_A) {
        paint = !pix(cx, cy);
        set_pix(cx, cy, paint);
    } else if ((keys & J_A) && moved) {
        set_pix(cx, cy, paint);
    }
    if ((pressed & J_B) || ((keys & J_B) && moved)) set_pix(cx, cy, 0);
    if (!(keys & (J_A | J_B))) stroke_pending = 0;
    return SCR_STAY;
}
