#pragma bank 2   // a BANKED screen or helper: see "ROM banks" in the Makefile
// Title screen (at power-on): a big A and a half-drawn a, the name, and the settings.
//
//  rows 1-8     A (Pocket Serif, all ink) and a (top drawn, the rest still guide), a tile per pixel
//  row 10       TYPE DESIGNER POCKET
//  row 12       PRESS START
//  rows 14-15   > SOUND   ON
//                 COLORS  GREEN   (Game Boy Color only)
//  row 17       hints
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"
#include "sound.h"

#define T_INK ((uint8_t)(TILE_CANVAS + CELL_INK * 4))
#define T_GUIDE ((uint8_t)(TILE_CANVAS + CELL_GUIDE * 4))
#define T_OFF ((uint8_t)(TILE_CANVAS + CELL_OFF * 4))
#define DRAWN_ROWS 5   // how much of the a is drawn so far

static uint8_t row;

static void big_letter(uint8_t c, uint8_t x0, uint8_t drawn_rows) {
    const uint8_t *rows = template_rows(1, c - FIRST_CODE);   // Pocket Serif
    uint8_t r, b;
    for (r = 0; r < 8; r++)
        for (b = 0; b < 8; b++)
            set_bkg_tile_xy(x0 + b, 1 + r,
                            !((rows[r] >> (7 - b)) & 1) ? T_OFF : (r < drawn_rows ? T_INK : T_GUIDE));
}

static void draw_settings(void) {
    uint8_t rows = is_color ? 2 : 1;
    set_bkg_tile_xy(0, 14, row == 0 ? TILE_UI('>') : TILE_BLANK);
    text(2, 14, "SOUND");
    text_pad(10, 14, save_sound() ? "ON" : "OFF", 3);
    if (rows > 1) {
        set_bkg_tile_xy(0, 15, row == 1 ? TILE_UI('>') : TILE_BLANK);
        text(2, 15, "COLORS");
        theme_name(10, 15);
    }
}

void title_enter(void) BANKED {
    DISPLAY_OFF;
    gfx_clear();
    big_letter('A', 2, 8);
    big_letter('a', 10, DRAWN_ROWS);
    text(0, 10, "TYPE DESIGNER POCKET");
    text(4, 12, "PRESS START");
    draw_settings();
    text(0, 17, "<>:CHANGE");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t title_update(void) BANKED {
    uint8_t rows = is_color ? 2 : 1;
    if (moves & (J_UP | J_DOWN) && rows > 1) {
        row ^= 1;
        sfx(SFX_MOVE);
        draw_settings();
    }
    if (moves & (J_LEFT | J_RIGHT) || (pressed & J_A)) {
        if (row == 0) {
            save_set_sound(!save_sound());
            sound_apply();
        } else {
            save_set_theme((save_theme() + ((moves & J_LEFT) ? THEME_COUNT - 1 : 1)) % THEME_COUNT);
            theme_apply();
        }
        sfx(SFX_MOVE);
        draw_settings();
    }
    if (pressed & J_START) {
        sfx(SFX_OK);
        return SCR_SLOTS;
    }
    return SCR_STAY;
}
