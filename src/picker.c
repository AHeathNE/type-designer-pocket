// New font: pick a guide template and whether to start empty (trace over the guide) or with a copy of it.
//
//  row 0        NEW FONT IN SLOT n
//  rows 2-3     > GUIDE  < POCKET SANS >
//                 START  < EMPTY | COPY IT >
//  rows 5-15    preview of codes 32-126 (light grey = guide only, black = copied ink); all 224 are copied
//  row 17       hints, or the overwrite confirmation
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define GRID_X 2
#define GRID_Y 5

static uint8_t row, tmpl, copy, confirm, drawn;

static void draw_choices(void) {
    set_bkg_tile_xy(0, 2, row == 0 ? TILE_UI('>') : TILE_BLANK);
    set_bkg_tile_xy(0, 3, row == 1 ? TILE_UI('>') : TILE_BLANK);
    text_pad(8, 2, template_name(tmpl), 12);
    text_pad(8, 3, tmpl == TEMPLATE_NONE ? "EMPTY" : (copy ? "COPY IT" : "EMPTY"), 12);
}

static void draw_preview(void) {
    uint8_t i;
    const uint8_t *g;
    for (i = 0; i < BASIC_COUNT; i++) {
        g = tmpl < TEMPLATE_COUNT ? template_fonts[tmpl][i] : 0;
        if (copy) tile_from(i, g, 0);
        else tile_from(i, 0, g);
    }
}

static void draw_hints(void) {
    text_pad(0, 17, "A:CREATE  B:CANCEL", 20);
}

void picker_enter(void) {
    SlotInfo info;
    uint8_t i;
    save_slot_info(picker_slot, &info);
    drawn = info.drawn;
    row = 0;
    tmpl = 0;
    copy = 0;
    confirm = 0;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "NEW FONT IN SLOT");
    set_bkg_tile_xy(17, 0, TILE_UI('1' + picker_slot));
    text(2, 2, "GUIDE");
    text(2, 3, "START");
    for (i = 0; i < BASIC_COUNT; i++) set_bkg_tile_xy(GRID_X + i % 16, GRID_Y + 2 * (i / 16), i);
    draw_choices();
    draw_preview();
    draw_hints();
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t picker_update(void) {
    if (confirm) {
        if (pressed & J_B) {
            confirm = 0;
            draw_hints();
            return SCR_STAY;
        }
        if (!(pressed & J_A)) return SCR_STAY;
    } else {
        if (pressed & J_B) return SCR_SLOTS;
        if (moves & (J_UP | J_DOWN)) {
            row ^= 1;
            draw_choices();
        }
        if (moves & (J_LEFT | J_RIGHT)) {
            if (row == 0) {
                tmpl = (moves & J_RIGHT) ? template_next(tmpl) : template_next(template_next(template_next(tmpl)));
                if (tmpl == TEMPLATE_NONE) copy = 0;
            } else if (tmpl != TEMPLATE_NONE) {
                copy ^= 1;
            }
            draw_choices();
            draw_preview();
        }
        if (!(pressed & J_A)) return SCR_STAY;
        if (drawn) {
            confirm = 1;
            text_pad(0, 17, "ERASE", 20);
            text_num(6, 17, drawn);
            text(drawn >= 10 ? 8 : 7, 17, "? A:YES B:NO");
            return SCR_STAY;
        }
    }
    // create it
    save_select(picker_slot);
    save_reset(tmpl, copy && tmpl < TEMPLATE_COUNT ? template_fonts[tmpl] : 0);
    return SCR_MAP;
}
