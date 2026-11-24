// Fonts screen (shown at boot): the three save slots.
//
//  row 0        TYPE DESIGNER POCKET
//  rows 3-4, 6-7, 9-10   > FONT 1        12/95
//                          POCKET SANS     +3   (or SAVE CHECK!; +n counts codes 128-255)
//  rows 13-15   hints
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

static uint8_t sel = 0xFF;

static void draw_cursor(void) {
    uint8_t i;
    for (i = 0; i < SLOT_COUNT; i++) set_bkg_tile_xy(0, 3 + i * 3, i == sel ? TILE_UI('>') : TILE_BLANK);
}

void slots_enter(void) {
    SlotInfo info;
    uint8_t i, y;
    if (sel == 0xFF) sel = save_active();
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "TYPE DESIGNER POCKET");
    for (i = 0; i < SLOT_COUNT; i++) {
        y = 3 + i * 3;
        save_slot_info(i, &info);
        text_pad(2, y, info.name, 12);
        text_num(14, y, info.drawn);
        text(info.drawn >= 10 ? 16 : 15, y, "/95");
        text(3, y + 1, info.ok ? template_name(info.template_id) : "SAVE CHECK!");
        if (info.drawn_ext) {   // codes 128-255 drawn too: "+12"
            set_bkg_tile_xy(16, y + 1, TILE_UI('+'));
            text_num(17, y + 1, info.drawn_ext);
        }
    }
    draw_cursor();
    text(1, 13, "A:OPEN");
    text(1, 14, "START:NEW FONT");
    text(1, 15, "SELECT:CREDITS");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t slots_update(void) {
    if ((moves & J_UP) && sel > 0) { sel--; draw_cursor(); }
    if ((moves & J_DOWN) && sel + 1 < SLOT_COUNT) { sel++; draw_cursor(); }
    if (pressed & J_A) {
        save_select(sel);
        return SCR_MAP;
    }
    if (pressed & J_START) {
        picker_slot = sel;
        return SCR_PICKER;
    }
    if (pressed & J_SELECT) return SCR_CREDITS;
    return SCR_STAY;
}
