// Challenge mode, like the web Type Making Game: pick the time per glyph and which characters, count down
// 3-2-1-GO, then draw each glyph before its timer runs out (the editor runs the timer: see editor.c).
//
// Setup:
//  row 0        CHALLENGE
//  row 2        > TIME        < 7S >
//  rows 4-9       [X] A-Z / a-z / 0-9 / PUNCTUATION / SYMBOLS / ACCENTS
//  row 11         [X] ONLY UNDRAWN
//  row 13       24 GLYPHS  (or NOTHING TO DRAW)
//  rows 16-17   hints
//
// Results:
//  row 0        CHALLENGE DONE!
//  row 1        DRAWN n/m
//  rows 3-13    the challenge's glyphs, 16 to a row (more than 96: SELECT pages through them)
//  row 15       selected: reference char, your glyph, code
//  rows 16-17   hints
#pragma bank 1   // ROM bank 1 is always mapped (the game never switches banks): see Makefile
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

uint8_t chal_on, chal_retry;
uint8_t chal_list[GLYPH_COUNT];
uint8_t chal_n, chal_pos;
uint8_t chal_seconds;

#define TIME_COUNT 8
static const uint8_t time_options[TIME_COUNT] = {3, 5, 7, 10, 15, 20, 30, 60};   // the web game's choices
static uint8_t time_idx = 2;   // 7 seconds

enum { SET_UPPER, SET_LOWER, SET_DIGITS, SET_PUNCT, SET_SYMBOLS, SET_ACCENTS, SET_COUNT };
static const char *const set_labels[SET_COUNT] = {"A-Z", "a-z", "0-9", "PUNCTUATION", "SYMBOLS", "ACCENTS"};
static uint8_t sets = (1 << SET_UPPER) | (1 << SET_DIGITS) | (1 << SET_PUNCT);   // the web game's default
static uint8_t only_undrawn = 1;

#define ROW_TIME 0
#define ROW_UNDRAWN (1 + SET_COUNT)
#define ROWS (2 + SET_COUNT)
static const uint8_t row_y[ROWS] = {2, 4, 5, 6, 7, 8, 9, 11};
static uint8_t row;

static uint8_t set_of(uint8_t code) {
    if (code >= 'A' && code <= 'Z') return SET_UPPER;
    if (code >= 'a' && code <= 'z') return SET_LOWER;
    if (code >= '0' && code <= '9') return SET_DIGITS;
    if (code < 127) return SET_PUNCT;
    if (code < 192) return SET_SYMBOLS;
    return SET_ACCENTS;
}

// Fill chal_list from the settings, in the design order. Spaces (32, 160) and the soft hyphen (173) are
// never drawn.
static void build_list(void) {
    uint8_t k, i, code;
    chal_n = 0;
    order_build();
    for (k = 0; k < GLYPH_COUNT - 1; k++) {   // in the design order chosen on the map
        i = order_list[k];
        code = FIRST_CODE + i;
        if (code == 32 || code == 160 || code == 173) continue;
        if (!(sets & (1 << set_of(code)))) continue;
        if (only_undrawn && glyph_has_ink(i)) continue;
        chal_list[chal_n++] = i;
    }
}

static void tick(uint8_t y, uint8_t on) {
    text(3, y, on ? "[X]" : "[ ]");
}

static void draw_count(void) {
    char buf[12], *p;
    build_list();
    text_pad(0, 13, "", 20);
    if (!chal_n) {
        text(0, 13, "NOTHING TO DRAW");
        return;
    }
    p = num_cat(buf, chal_n);
    strcpy(p, chal_n == 1 ? " GLYPH" : " GLYPHS");
    text(0, 13, buf);
}

static void draw_setup(void) {
    uint8_t i;
    for (i = 0; i < ROWS; i++) set_bkg_tile_xy(0, row_y[i], i == row ? TILE_UI('>') : TILE_BLANK);
    text(13, 2, "<");
    text_num(15, 2, time_options[time_idx]);
    set_bkg_tile_xy(time_options[time_idx] >= 10 ? 17 : 16, 2, TILE_UI('S'));
    set_bkg_tile_xy(time_options[time_idx] >= 10 ? 18 : 17, 2, TILE_BLANK);
    text(19, 2, ">");
    for (i = 0; i < SET_COUNT; i++) tick(row_y[1 + i], sets & (1 << i));
    tick(row_y[ROW_UNDRAWN], only_undrawn);
    draw_count();
}

void challenge_enter(void) {
    uint8_t i;
    chal_on = 0;
    chal_retry = 0;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "CHALLENGE");
    text(2, 2, "TIME");
    for (i = 0; i < SET_COUNT; i++) text(7, row_y[1 + i], set_labels[i]);
    text(7, row_y[ROW_UNDRAWN], "ONLY UNDRAWN");
    text(0, 15, "ORDER:");
    text(7, 15, order_names[save_order()]);
    text(0, 16, "A:TICK  START:GO");
    text(0, 17, "B:BACK");
    draw_setup();
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t challenge_update(void) {
    uint8_t changed = 0;
    if (pressed & J_B) return SCR_MAP;
    if (moves & J_UP) { row = row ? row - 1 : ROWS - 1; changed = 1; }
    if (moves & J_DOWN) { row = (row + 1) % ROWS; changed = 1; }
    if (row == ROW_TIME) {
        if ((moves & J_LEFT) && time_idx) { time_idx--; changed = 1; }
        if ((moves & (J_RIGHT)) && time_idx + 1 < TIME_COUNT) { time_idx++; changed = 1; }
        if ((pressed & J_A)) { time_idx = (time_idx + 1) % TIME_COUNT; changed = 1; }
    } else if (pressed & (J_A | J_LEFT | J_RIGHT)) {
        if (row == ROW_UNDRAWN) only_undrawn ^= 1;
        else sets ^= 1 << (row - 1);
        changed = 1;
    }
    if (changed) draw_setup();
    if (pressed & J_START) {
        if (!chal_n) return SCR_STAY;   // "NOTHING TO DRAW" is already showing
        chal_seconds = time_options[time_idx];
        chal_pos = 0;
        return SCR_COUNTDOWN;
    }
    return SCR_STAY;
}

// ---- countdown: 3, 2, 1, GO, a second each, drawn big from Pocket Sans ----

static uint8_t step;
static uint16_t step_start;

// one Pocket Sans character with each pixel a tile, its 8x8 box at (x, y)
static void big_char(uint8_t c, uint8_t x, uint8_t y) {
    const uint8_t *rows = pocket_sans[c - FIRST_CODE];
    uint8_t r, b;
    for (r = 0; r < 8; r++)
        for (b = 0; b < 8; b++)
            set_bkg_tile_xy(x + b, y + r, ((rows[r] >> (7 - b)) & 1) ? TILE_CANVAS + CELL_INK * 4 : TILE_BLANK);
}

static void draw_step(void) {
    fill_bkg_rect(0, 4, 20, 9, TILE_BLANK);
    if (step < 3) {
        big_char('3' - step, 6, 4);
    } else {
        big_char('G', 2, 4);
        big_char('O', 10, 4);
    }
}

void countdown_enter(void) {
    char buf[21], *p;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "GET READY!");
    p = num_cat(buf, chal_n);
    strcpy(p, chal_n == 1 ? " GLYPH, " : " GLYPHS, ");
    p = num_cat(p + strlen(p), chal_seconds);
    strcpy(p, "S EACH");
    text(0, 14, buf);
    text(0, 17, "B:CANCEL");
    step = 0;
    step_start = sys_time;
    draw_step();
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t countdown_update(void) {
    if (pressed & J_B) return SCR_CHALLENGE;
    if ((uint16_t)(sys_time - step_start) < 60) return SCR_STAY;
    step_start += 60;
    if (++step > 3) {
        chal_on = 1;
        chal_retry = 0;
        return SCR_EDITOR;
    }
    draw_step();
    return SCR_STAY;
}

// ---- results ----

#define GRID_X 2
#define GRID_Y 3
#define PAGE 96
#define REF_TILE 100
static uint8_t sel, rpage;

static void draw_results_page(void) {
    uint8_t k, n = chal_n - rpage * PAGE;
    if (n > PAGE) n = PAGE;
    fill_bkg_rect(GRID_X, GRID_Y, 16, 11, TILE_BLANK);
    for (k = 0; k < n; k++) {
        glyph_tile(k, chal_list[rpage * PAGE + k], !glyph_has_ink(chal_list[rpage * PAGE + k]));   // missed: guide
        set_bkg_tile_xy(GRID_X + k % 16, GRID_Y + 2 * (k / 16), k);
    }
}

static void draw_pick(void) {
    uint8_t i = chal_list[sel];
    set_bkg_tile_xy(2, 15, ref_tile(i, REF_TILE));
    set_bkg_tile_xy(4, 15, sel - rpage * PAGE);
    text(6, 15, "#");
    text_num(7, 15, FIRST_CODE + i);
    bars_place(GRID_X, GRID_Y, sel - rpage * PAGE);
}

void results_enter(void) {
    uint8_t k, drawn = 0;
    if (chal_retry) sel = chal_pos - 1;
    else sel = 0;
    chal_on = 0;
    chal_retry = 0;
    rpage = sel / PAGE;
    for (k = 0; k < chal_n; k++)
        if (glyph_has_ink(chal_list[k])) drawn++;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "CHALLENGE DONE!");
    text(0, 1, "DRAWN");
    text_frac(6, 1, drawn, chal_n, 7);
    draw_results_page();
    text(0, 16, "A:TRY AGAIN");
    if (chal_n > PAGE) text(12, 16, "SEL:PAGE");
    text(0, 17, "B:DONE");
    bars_show();
    draw_pick();
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t results_update(void) {
    uint8_t was = sel, page_was = rpage;
    if ((moves & J_LEFT) && sel) sel--;
    if ((moves & J_RIGHT) && sel + 1 < chal_n) sel++;
    if ((moves & J_UP) && sel >= 16) sel -= 16;
    if ((moves & J_DOWN) && sel + 16 < chal_n) sel += 16;
    if ((pressed & J_SELECT) && chal_n > PAGE) {
        rpage = (rpage + 1) % ((chal_n + PAGE - 1) / PAGE);
        sel = rpage * PAGE;
    }
    rpage = sel / PAGE;
    if (rpage != page_was) draw_results_page();
    if (sel != was || rpage != page_was) draw_pick();
    if (pressed & J_A) {   // redo this one glyph, timed, then come back here
        chal_pos = sel;
        chal_on = 1;
        chal_retry = 1;
        return SCR_EDITOR;
    }
    if (pressed & (J_B | J_START)) {
        current = chal_list[sel];
        return SCR_MAP;
    }
    return SCR_STAY;
}
