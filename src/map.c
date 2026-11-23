// Character map: every glyph in a 16-wide grid, the same order as GB Studio's font sheet.
//
//  row 0   TYPE DESIGNER POCKET
//  row 1   slot name, guide font
//  rows 3-13 (odd)  glyph rows, columns 2-17
//  row 15  selected: reference char, your glyph, code, width
//  row 16  DRAWN n/95, B hint
//  row 17  hints (A edit, START type test)
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define GRID_X 2
#define GRID_Y 3

static void place_bars(void) {
    uint8_t x = (GRID_X + current % 16) * 8 + 8;          // OAM x = screen x + 8
    uint8_t y = (GRID_Y + 2 * (current / 16)) * 8 + 16;   // OAM y = screen y + 16
    move_sprite(0, x, y - 8);
    move_sprite(1, x, y + 8);
}

static void draw_info(void) {
    set_bkg_tile_xy(2, 15, TILE_UI(FIRST_CODE + current));
    set_bkg_tile_xy(4, 15, current);
    text(6, 15, "#");
    text_num(7, 15, FIRST_CODE + current);
    text(11, 15, "W");
    text_num(12, 15, glyph_width(current));
}

static void draw_count(void) {
    uint8_t i, n = 0;
    for (i = 0; i < EDIT_COUNT; i++)
        if (glyph_has_ink(i)) n++;
    text(2, 16, "DRAWN");
    text_num(8, 16, n);
    text(n >= 10 ? 10 : 9, 16, "/95");
}

void map_enter(void) {
    uint8_t i;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "TYPE DESIGNER POCKET");
    for (i = 0; i < 12 && font.name[i]; i++) set_bkg_tile_xy(i, 1, TILE_UI(font.name[i]));
    text(14, 1, font.template_id < TEMPLATE_COUNT ? template_short[font.template_id] : "-");
    for (i = 0; i < EDIT_COUNT; i++) {
        glyph_tile(i, !glyph_has_ink(i));
        set_bkg_tile_xy(GRID_X + i % 16, GRID_Y + 2 * (i / 16), i);
    }
    draw_info();
    draw_count();
    text(0, 17, "A:EDIT  START:TEST");
    text(13, 16, "B:FONTS");
    if (save_status == SAVE_DAMAGED) text(0, 2, "SAVE CHECK!");

    set_sprite_tile(0, SPR_TILE_BAR);
    set_sprite_prop(0, 0);
    set_sprite_tile(1, SPR_TILE_BAR);
    set_sprite_prop(1, S_FLIPY);
    place_bars();
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t map_update(void) {
    uint8_t was = current;
    if ((moves & J_LEFT) && current > 0) current--;
    if ((moves & J_RIGHT) && current < EDIT_COUNT - 1) current++;
    if ((moves & J_UP) && current >= 16) current -= 16;
    if ((moves & J_DOWN) && current + 16 < EDIT_COUNT) current += 16;
    if (current != was) {
        place_bars();
        draw_info();
    }
    if (pressed & J_A) return SCR_EDITOR;
    if (pressed & J_B) return SCR_SLOTS;
    if (pressed & J_START) return SCR_TYPETEST;
    return SCR_STAY;
}
