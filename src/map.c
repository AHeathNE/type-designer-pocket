#pragma bank 1   // a BANKED screen or helper: see "ROM banks" in the Makefile
// Character map: one page of the character set at a time, in a 16-wide grid in GB Studio's sheet order.
//
//  row 0   page title, page number
//  row 1   slot name, guide font
//  row 2   SAVE CHECK! (left), NEXT: the next undrawn glyph in the design order (right)
//  rows 3-13 (odd)  glyph rows, columns 2-17
//  row 15  selected: reference char, your glyph, code, width
//  row 16  DRAWN n/95 on this page, SEL hint
//  row 17  hints; START opens a menu in the window over rows 10-17: type test, challenge, next glyph,
//          design order, learn, fonts
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"
#include "sound.h"

#define GRID_X 2
#define GRID_Y 3
#define REF_TILE 100
#define NEXT_TILE 101
#define MENU_X 4
#define MENU_Y 10
#define MENU_W 16
enum { MI_TEST, MI_CHALLENGE, MI_NEXT, MI_ORDER, MI_LEARN, MI_FONTS, MAP_MENU_COUNT };
static const char *const menu_labels[MAP_MENU_COUNT] = {"TYPE TEST", "CHALLENGE", "NEXT GLYPH", "ORDER",
                                                        "LEARN", "FONTS"};
static const uint8_t menu_targets[MAP_MENU_COUNT] = {SCR_TYPETEST, SCR_CHALLENGE, SCR_STAY, SCR_STAY,
                                                     SCR_LEARN, SCR_SLOTS};

static uint8_t page, menu_open, menu_sel;

static void draw_info(void) {
    uint8_t k = current - page_first[page];
    set_bkg_tile_xy(2, 15, ref_tile(current, REF_TILE));
    set_bkg_tile_xy(4, 15, k);
    text(6, 15, "#");
    text_num(7, 15, FIRST_CODE + current);
    text(11, 15, "W");
    text_num(12, 15, glyph_width(current));
    bars_place(GRID_X, GRID_Y, k);
}

static void draw_count(void) {
    uint8_t i, n = 0, cells = page_cells[page];
    for (i = 0; i < cells; i++)
        if (glyph_has_ink(page_first[page] + i)) n++;
    text(0, 16, "DRAWN");   // columns 0-10 at most ("DRAWN 95/95"), clear of "SEL:PAGE" at 12
    text_frac(6, 16, n, cells, 5);
}

static void draw_next(void) {
    uint8_t n = order_next_undrawn();
    if (n == 0xFF) {
        text_pad(13, 2, "", 7);
        return;
    }
    text(14, 2, "NEXT:");
    set_bkg_tile_xy(19, 2, ref_tile(n, NEXT_TILE));
}

static void draw_page(void) {
    uint8_t k, cells = page_cells[page];
    text_pad(0, 0, page_titles[page], 17);
    set_bkg_tile_xy(17, 0, TILE_UI('1' + page));
    text(18, 0, "/3");
    fill_bkg_rect(GRID_X, GRID_Y, 16, 11, TILE_BLANK);
    for (k = 0; k < cells; k++) {
        glyph_tile(k, page_first[page] + k, !glyph_has_ink(page_first[page] + k));
        set_bkg_tile_xy(GRID_X + k % 16, GRID_Y + 2 * (k / 16), k);
    }
    draw_count();
}

// ---- START menu, in the window layer ----

static void menu_draw(void) {
    uint8_t i;
    fill_win_rect(0, 0, MENU_W, MAP_MENU_COUNT + 2, TILE_BLANK);
    set_win_tile_xy(0, 0, TILE_FRAME_C);
    for (i = 1; i < MENU_W; i++) set_win_tile_xy(i, 0, TILE_FRAME_H);
    for (i = 1; i < MAP_MENU_COUNT + 2; i++) set_win_tile_xy(0, i, TILE_FRAME_V);
    for (i = 0; i < MAP_MENU_COUNT; i++) {
        win_text(1, 1 + i, i == menu_sel ? ">" : " ");
        win_text(2, 1 + i, menu_labels[i]);
    }
    order_name(8, 1 + MI_ORDER, 1);   // A cycles it
    win_text(2, MAP_MENU_COUNT + 1, "B:CLOSE");
}

static uint8_t menu_update(void) {
    if (pressed & (J_B | J_START)) {
        sfx(SFX_BACK);
        menu_open = 0;
        HIDE_WIN;
        bars_show();
        draw_info();
        return SCR_STAY;
    }
    if (moves & J_UP) { menu_sel = menu_sel ? menu_sel - 1 : MAP_MENU_COUNT - 1; menu_draw(); sfx(SFX_MOVE); }
    if (moves & J_DOWN) { menu_sel = (menu_sel + 1) % MAP_MENU_COUNT; menu_draw(); sfx(SFX_MOVE); }
    if (pressed & J_A) {
        sfx(SFX_OK);
        if (menu_sel == MI_ORDER) {   // stays open, so you can keep cycling
            save_set_order((save_order() + 1) % ORDER_COUNT);
            menu_draw();
            draw_next();
            return SCR_STAY;
        }
        if (menu_sel == MI_NEXT) {    // jump to the next undrawn glyph in the order
            uint8_t n = order_next_undrawn();
            menu_open = 0;
            HIDE_WIN;
            bars_show();
            if (n != 0xFF) {
                current = n;
                if (page_of(n) != page) {
                    page = page_of(n);
                    draw_page();
                }
            }
            draw_info();
            return SCR_STAY;
        }
        menu_open = 0;
        learn_back = SCR_MAP;
        return menu_targets[menu_sel];
    }
    return SCR_STAY;
}

// ---- screen ----

void map_enter(void) BANKED {
    uint8_t i;
    if (current == UNUSED_GLYPH) current = UNUSED_GLYPH - 1;
    page = page_of(current);
    menu_open = 0;
    DISPLAY_OFF;
    gfx_clear();
    for (i = 0; i < 12 && font.name[i]; i++) set_bkg_tile_xy(i, 1, TILE_UI(font.name[i]));
    text(14, 1, font.template_id < TEMPLATE_COUNT ? template_short[font.template_id] : "-");
    if (save_status == SAVE_DAMAGED) text(0, 2, "SAVE CHECK!");
    draw_next();
    draw_page();
    text(12, 16, "SEL:PAGE");
    text(0, 17, "A:EDIT  START:MENU");
    bars_show();
    draw_info();
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

static void go_page(uint8_t p, uint8_t k) {
    page = p;
    if (k >= page_cells[p]) k = page_cells[p] - 1;
    current = page_first[p] + k;
    draw_page();
}

uint8_t map_update(void) BANKED {
    uint8_t k = current - page_first[page], cells = page_cells[page], was = current;
    uint8_t last_row = (cells - 1) / 16;

    if (menu_open) return menu_update();

    if (moves & J_LEFT) {
        if (k) current--;
        else go_page(page ? page - 1 : PAGE_COUNT - 1, 0xFF);
    }
    if (moves & J_RIGHT) {
        if (k + 1 < cells) current++;
        else go_page((page + 1) % PAGE_COUNT, 0);
    }
    if (moves & J_UP) {
        if (k >= 16) current -= 16;
        else {   // up from the top row: the bottom row of the page before, same column
            uint8_t p = page ? page - 1 : PAGE_COUNT - 1;
            go_page(p, ((page_cells[p] - 1) / 16) * 16 + k % 16);
        }
    }
    if (moves & J_DOWN) {
        if (k / 16 < last_row) current = (k + 16 < cells) ? current + 16 : page_first[page] + cells - 1;
        else go_page((page + 1) % PAGE_COUNT, k % 16);
    }
    if (pressed & J_SELECT) go_page((page + 1) % PAGE_COUNT, k);
    if (current != was || (pressed & J_SELECT)) {
        draw_info();
        sfx(SFX_MOVE);
    }

    if (pressed & J_A) { sfx(SFX_OK); return SCR_EDITOR; }
    if (pressed & J_B) { sfx(SFX_BACK); return SCR_SLOTS; }
    if (pressed & J_START) {
        sfx(SFX_OK);
        menu_open = 1;
        menu_sel = 0;
        hide_sprites();
        menu_draw();
        move_win(MENU_X * 8 + 7, MENU_Y * 8);
        SHOW_WIN;
    }
    return SCR_STAY;
}
