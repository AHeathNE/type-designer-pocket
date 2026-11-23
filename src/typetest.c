// Type test: sample text set in the font being made, exactly as GB Studio will set it.
//
//  row 0        TYPE TEST       <n/5>
//  rows 2-12    six lines of text (every other row), tiles 0-107
//  row 14       characters the sample needs that aren't drawn yet
//  rows 16-17   hints
//
// Keyboard: type your own sample.
//  row 0        hints
//  rows 2, 4    your text so far, set in your font
//  rows 6-16    character grid in Pocket Sans (every other row), bars above and below the pick
//  row 17       length
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

#define SAMPLE_COUNT 5
#define CUSTOM_MAX 40
#define LINES 6

static const char *const samples[SAMPLE_COUNT - 1] = {
    "The quick brown fox jumps over the lazy dog.",
    "SPHINX OF BLACK QUARTZ, JUDGE MY VOW!",
    "0123456789 +-=*/ ()[]{}<> .,:;!? '\"` @#$%&^_|~\\",
    "Hello! Welcome to my shop. What can I get you today?",
};

static char custom[CUSTOM_MAX + 1];
static uint8_t custom_len;
static uint8_t sample;            // 0-3 built in, 4 your text
static uint8_t key = 'A' - 32;    // keyboard selection

static const char *sample_text(void) {
    return sample < SAMPLE_COUNT - 1 ? samples[sample] : custom;
}

static uint8_t is_blank(uint8_t c) {
    const uint8_t *rows;
    uint8_t lo, left;
    text_glyph(c, &rows, &lo, &left);
    return rows == 0;
}

static void draw_missing(void) {
    const char *s = sample_text();
    char list[10];
    uint8_t seen[12], n = 0, shown = 0, c, i;
    memset(seen, 0, sizeof seen);
    for (; *s; s++) {
        c = *s;
        if (c == ' ' || c < 32 || c > 126 || (seen[(c - 32) >> 3] & (1 << ((c - 32) & 7)))) continue;
        seen[(c - 32) >> 3] |= 1 << ((c - 32) & 7);
        if (!is_blank(c)) continue;
        n++;
        if (shown < 8) list[shown++] = c;
    }
    list[shown] = 0;
    text_pad(0, 14, "", 20);
    if (!n) {
        text(0, 14, "ALL GLYPHS DRAWN");
        return;
    }
    text(0, 14, "MISSING");
    text_num(8, 14, n);
    for (i = 0; i < shown; i++) set_bkg_tile_xy(12 + i, 14, TILE_UI(list[i]));
}

static void draw_sample(void) {
    const char *s = sample_text();
    text_block(s, strlen(s), 1, 2, LINES, 0);
    set_bkg_tile_xy(16, 0, TILE_UI('1' + sample));
    if (sample == SAMPLE_COUNT - 1 && !custom_len) text(1, 2, "(A: WRITE YOUR OWN)");
    draw_missing();
}

void typetest_enter(void) {
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "TYPE TEST");
    text(15, 0, "< /5>");
    draw_sample();
    text(0, 16, "<>:SAMPLE A:WRITE");
    text(0, 17, "B:BACK");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t typetest_update(void) {
    if (moves & J_LEFT) { sample = sample ? sample - 1 : SAMPLE_COUNT - 1; draw_sample(); }
    if (moves & J_RIGHT) { sample = (sample + 1) % SAMPLE_COUNT; draw_sample(); }
    if (pressed & J_A) return SCR_KEYBOARD;
    if (pressed & J_B) return SCR_MAP;
    return SCR_STAY;
}

// ---- keyboard ----

#define KEY_X 2
#define KEY_Y 6

static void draw_typed(void) {
    text_block(custom, custom_len, 1, 2, 2, 0);
    text_num(0, 17, custom_len);
    text(custom_len >= 10 ? 2 : 1, 17, "/40 ");
}

static void place_key_bars(void) {
    uint8_t x = (KEY_X + key % 16) * 8 + 8;
    uint8_t y = (KEY_Y + 2 * (key / 16)) * 8 + 16;
    move_sprite(0, x, y - 8);
    move_sprite(1, x, y + 8);
}

void keyboard_enter(void) {
    uint8_t i;
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "A:ADD B:DEL ST:DONE");
    for (i = 0; i < EDIT_COUNT; i++) set_bkg_tile_xy(KEY_X + i % 16, KEY_Y + 2 * (i / 16), TILE_UI(32 + i));
    text(11, 17, "SEL:SPACE");
    draw_typed();
    set_sprite_tile(0, SPR_TILE_BAR); set_sprite_prop(0, 0);
    set_sprite_tile(1, SPR_TILE_BAR); set_sprite_prop(1, S_FLIPY);
    place_key_bars();
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t keyboard_update(void) {
    uint8_t was = key;
    if ((moves & J_LEFT) && key > 0) key--;
    if ((moves & J_RIGHT) && key < EDIT_COUNT - 1) key++;
    if ((moves & J_UP) && key >= 16) key -= 16;
    if ((moves & J_DOWN) && key + 16 < EDIT_COUNT) key += 16;
    if (key != was) place_key_bars();
    if ((pressed & (J_A | J_SELECT)) && custom_len < CUSTOM_MAX) {
        custom[custom_len++] = (pressed & J_SELECT) ? ' ' : 32 + key;
        custom[custom_len] = 0;
        draw_typed();
    }
    if ((pressed & J_B) && custom_len) {
        custom[--custom_len] = 0;
        draw_typed();
    }
    if (pressed & J_START) {
        sample = SAMPLE_COUNT - 1;
        return SCR_TYPETEST;
    }
    return SCR_STAY;
}
