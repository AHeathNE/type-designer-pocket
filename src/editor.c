// Glyph editor: the 8x8 glyph blown up to 16x16 px per pixel.
//
//  row 0        reference char, code, width, SEL hint
//  rows 1-16    canvas (columns 2-17), guide ticks in column 1, edge in column 18, live preview at (19, 1)
//  row 17       hints, or a short message
//
//  D-pad move (hold to repeat)     A paint (toggles; hold A and move to keep painting)
//  B erase (hold and move)         SELECT + Left/Right previous/next glyph
//  SELECT tap  guide on/off        SELECT + B undo
//  START       menu: undo, copy, paste, flip, nudge, width, clear, start from, guide font, done
//
//  SELECT + Left/Right follows the design order chosen on the map (order.c). START FROM copies a related
//  glyph in to adapt (h from n, b from d mirrored, é from e...).
//
//  Width mode (from the menu): Left/Right set a fixed GB Studio width, B goes back to automatic, A done.
//  Two dashed lines show where the glyph starts and where the next one begins once it's set in text.
//
//  In a challenge (chal_on) row 0 is a timer bar, START pauses (resume, next glyph, quit), the guide starts
//  off, and the glyph starts blank: if time runs out with nothing drawn, the old drawing comes back.
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
#define PREVIEW_TILE 0
#define REF_TILE 1

#define UNDO_MAX 16   // power of two

#define MENU_X 11     // window position, in tiles
#define MENU_Y 5
enum { M_UNDO, M_COPY, M_PASTE, M_FLIPH, M_FLIPV, M_NUDGE, M_WIDTH, M_CLEAR, M_FROM, M_GUIDE, M_DONE,
       MENU_COUNT };
static const char *const menu_labels[MENU_COUNT] = {
    "UNDO", "COPY", "PASTE", "FLIP H", "FLIP V", "NUDGE", "WIDTH", "CLEAR", "FROM", 0, "DONE"};
#define FROM_TILE 2
#define MARK_SPRITE 4   // sprites 4-11 start line, 12-19 advance line

static uint8_t cx = 1, cy = 1;   // cursor, in glyph pixels
static uint8_t paint;            // value A paints while held
static uint8_t guide = 1;        // show the template under the canvas
static uint8_t select_combo;     // SELECT was used with another button, so its release isn't a tap
static uint8_t dirty;
static uint8_t stroke_pending;   // A/B went down: take an undo snapshot before the first change
static uint8_t menu_open, menu_sel, nudging, widthing, msg_timer;

static uint8_t undo_buf[UNDO_MAX][8];
static uint8_t undo_top, undo_count;
static uint8_t clip[8], has_clip;

// challenge
static uint16_t deadline, total, paused_left;
static uint8_t bar_px, saved_guide;
static uint8_t backup[8];

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
    if (chal_on) return;   // row 0 is the timer bar
    text(7, 0, "W");
    text_num(8, 0, glyph_width(current));
    if (font.widths[current] != WIDTH_AUTO) set_bkg_tile_xy(9, 0, TILE_UI('*'));
}

static void draw_header(void) {
    set_bkg_tile_xy(0, 0, ref_tile(current, REF_TILE));
    if (!chal_on) {
        text(2, 0, "#");
        text_num(3, 0, FIRST_CODE + current);
        draw_width();
        text(14, 0, "SEL<>");
    }
    glyph_tile(PREVIEW_TILE, current, 0);
    set_bkg_tile_xy(19, 1, PREVIEW_TILE);
}

static void show_hints(void) {
    if (chal_on) {   // "12/24     ST:PAUSE"
        text_pad(0, 17, "", 20);
        text_frac(0, 17, chal_retry ? 1 : chal_pos + 1, chal_retry ? 1 : chal_n, 7);
        text(12, 17, "ST:PAUSE");
    } else {
        text_pad(0, 17, HINTS, 20);
    }
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
    glyph_tile(PREVIEW_TILE, current, 0);
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
    glyph_tile(PREVIEW_TILE, current, 0);
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
    current = order_step(current, dir);
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

// ---- width mode ----

// A dashed vertical line at the boundary before glyph column `col` (may be -1 .. 9).
static void place_mark(uint8_t first_sprite, int8_t col) {
    uint8_t k, x = (uint8_t)(CANVAS_X * 8 + col * 16 - 1 + 8);   // OAM x = screen x + 8
    for (k = 0; k < 8; k++) {
        set_sprite_tile(first_sprite + k, SPR_TILE_MARK);
        set_sprite_prop(first_sprite + k, 0);
        move_sprite(first_sprite + k, x, (CANVAS_Y * 8) + k * 16 + 16 + 4);
    }
}

// Where GB Studio will put this glyph: its ink moves to start at column 1 (0 if 8 wide), and the next
// glyph starts `width` columns after the left edge of the cell.
static void place_width_marks(void) {
    uint8_t lo = glyph_ink_left(current), w = glyph_width(current), inkw = 0, cols = 0, r, hi;
    int8_t start;
    for (r = 0; r < 8; r++) cols |= font.glyphs[current][r];
    if (cols) {
        for (hi = 7; !(cols & (0x80 >> hi)); hi--) {}
        inkw = hi - lo + 1;
    }
    start = (int8_t)lo - (cols && inkw >= 8 ? 0 : 1);
    place_mark(MARK_SPRITE, start);
    place_mark(MARK_SPRITE + 8, start + w);
}

static void width_hint(void) {
    text_pad(0, 17, "<>:WIDTH B:AUTO A:OK", 20);
}

static void width_update(void) {
    uint8_t w = glyph_width(current);
    if (moves & (J_LEFT | J_RIGHT)) {
        if ((moves & J_LEFT) && w > 1) w--;
        if ((moves & J_RIGHT) && w < 8) w++;
        font.widths[current] = w;
        dirty = 1;
    }
    if (pressed & J_B) {
        font.widths[current] = WIDTH_AUTO;
        dirty = 1;
    }
    if (moves & (J_LEFT | J_RIGHT) || pressed & J_B) {
        draw_width();
        place_width_marks();
    }
    if (pressed & (J_A | J_START)) {
        widthing = 0;
        hide_sprites();
        show_hints();
        place_cursor();
    }
}

// ---- menu (in the window layer, over the right side of the canvas) ----

static void menu_draw(void) {
    uint8_t i;
    const char *label;
    uint16_t from = from_source(FIRST_CODE + current);
    fill_win_rect(0, 0, 9, MENU_COUNT + 2, TILE_BLANK);
    set_win_tile_xy(0, 0, TILE_FRAME_C);
    for (i = 1; i < 9; i++) set_win_tile_xy(i, 0, TILE_FRAME_H);
    for (i = 1; i < MENU_COUNT + 2; i++) set_win_tile_xy(0, i, TILE_FRAME_V);
    for (i = 0; i < MENU_COUNT; i++) {
        label = menu_labels[i];
        if (i == M_FROM) {   // "FROM n", the source drawn in Pocket Sans
            win_text(2, 1 + i, label);
            if (from == 0xFFFF) win_text(7, 1 + i, "-");
            else set_win_tile_xy(7, 1 + i, ref_tile((uint8_t)from, FROM_TILE));
        } else if (i == M_GUIDE) {
            win_text(2, 1 + i, "G:");
            label = font.template_id < TEMPLATE_COUNT ? template_short[font.template_id] : "OFF";
            win_text(4, 1 + i, label);
        } else {
            win_text(2, 1 + i, label);
        }
        if (i == menu_sel) win_text(1, 1 + i, ">");
    }
    win_text(1, MENU_COUNT + 1, "B:CLOSE");
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
    case M_WIDTH:
        widthing = 1;
        menu_close();
        hide_sprites();
        place_width_marks();
        width_hint();
        return SCR_STAY;
    case M_CLEAR:
        snapshot();
        memset(font.glyphs[current], 0, 8);
        glyph_changed();
        message("CLEARED");
        break;
    case M_FROM: {
        uint16_t from = from_source(FIRST_CODE + current);
        uint8_t src = (uint8_t)from;
        if (from == 0xFFFF) {
            message("NO SOURCE FOR THIS");
            break;
        }
        if (!glyph_has_ink(src)) {
            message("SOURCE NOT DRAWN");
            break;
        }
        snapshot();
        memcpy(font.glyphs[current], font.glyphs[src], 8);
        if (from & 0x100) flip_h();
        if (from & 0x200) flip_v();
        glyph_changed();
        message("STARTED FROM IT");
        break;
    }
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

// ---- challenge: timer bar, pause menu, one glyph after another ----

static void bar_draw(uint16_t left) {
    uint8_t px = (uint8_t)(((uint32_t)left * 128u) / total), t, f;
    if (px == bar_px) return;
    bar_px = px;
    for (t = 0; t < 16; t++) {
        f = px > t * 8 ? px - t * 8 : 0;
        set_bkg_tile_xy(2 + t, 0, TILE_BAR + (f > 8 ? 8 : f));
    }
}

static void chal_glyph_start(void) {
    current = chal_list[chal_pos];
    memcpy(backup, font.glyphs[current], 8);
    memset(font.glyphs[current], 0, 8);   // start blank, as in the web game
    undo_count = 0;
    stroke_pending = 0;
    draw_header();
    draw_canvas();
    show_hints();
    total = (uint16_t)chal_seconds * 60u;
    deadline = sys_time + total;
    bar_px = 0xFF;
    bar_draw(total);
}

// This glyph is over (time ran out, NEXT, or QUIT): keep what was drawn, or the old drawing if nothing was.
static uint8_t chal_finish(void) {
    if (!glyph_has_ink(current)) memcpy(font.glyphs[current], backup, 8);
    if (memcmp(backup, font.glyphs[current], 8)) save_write();
    dirty = 0;
    chal_pos++;
    if (chal_retry || chal_pos >= chal_n) {
        guide = saved_guide;
        return SCR_RESULTS;
    }
    chal_glyph_start();
    return SCR_STAY;
}

enum { P_RESUME, P_NEXT, P_QUIT, PAUSE_COUNT };
static const char *const pause_labels[PAUSE_COUNT] = {"RESUME", "NEXT", "QUIT"};

static void pause_draw(void) {
    uint8_t i;
    fill_win_rect(0, 0, 9, 6, TILE_BLANK);
    set_win_tile_xy(0, 0, TILE_FRAME_C);
    for (i = 1; i < 9; i++) set_win_tile_xy(i, 0, TILE_FRAME_H);
    for (i = 1; i < 6; i++) set_win_tile_xy(0, i, TILE_FRAME_V);
    win_text(2, 0, "PAUSED");
    for (i = 0; i < PAUSE_COUNT; i++) {
        win_text(1, 1 + i, i == menu_sel ? ">" : " ");
        win_text(2, 1 + i, pause_labels[i]);
    }
}

static void pause_show(void) {
    paused_left = deadline - sys_time;
    menu_open = 1;
    menu_sel = 0;
    hide_sprites();
    pause_draw();
    move_win(MENU_X * 8 + 7, MENU_Y * 8);
    SHOW_WIN;
}

static uint8_t pause_update(void) {
    uint8_t pick = 0xFF;
    if (pressed & (J_B | J_START)) pick = P_RESUME;
    if (moves & J_UP) { menu_sel = menu_sel ? menu_sel - 1 : PAUSE_COUNT - 1; pause_draw(); }
    if (moves & J_DOWN) { menu_sel = (menu_sel + 1) % PAUSE_COUNT; pause_draw(); }
    if (pressed & J_A) pick = menu_sel;
    if (pick == 0xFF) return SCR_STAY;
    menu_close();
    deadline = sys_time + paused_left;
    if (pick == P_NEXT) return chal_finish();
    if (pick == P_QUIT) {
        chal_n = chal_pos + 1;   // the results show the glyphs reached so far
        return chal_finish();
    }
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
    widthing = 0;
    undo_count = 0;
    if (chal_on) {
        saved_guide = guide;
        guide = 0;
        chal_glyph_start();
    }
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t editor_update(void) {
    uint8_t moved = 0;

    if (msg_timer && --msg_timer == 0) show_hints();

    if (chal_on) {
        uint16_t left;
        if (menu_open) return pause_update();
        left = deadline - sys_time;
        if ((int16_t)left <= 0) return chal_finish();
        bar_draw(left);
    }
    if (menu_open) return menu_update();
    if (widthing) {
        width_update();
        return SCR_STAY;
    }

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
        if ((pressed & J_LEFT) && !chal_on) { switch_glyph(-1); select_combo = 1; }
        if ((pressed & J_RIGHT) && !chal_on) { switch_glyph(1); select_combo = 1; }
        if (pressed & J_B) { undo(); select_combo = 1; }
        return SCR_STAY;
    }

    if (pressed & J_START) {
        if (chal_on) pause_show();
        else menu_show();
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
