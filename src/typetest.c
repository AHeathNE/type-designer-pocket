#pragma bank 2   // a BANKED screen or helper: see "ROM banks" in the Makefile
// Type test: sample text set in the font being made, exactly as GB Studio will set it.
//
//  row 0        TYPE TEST       <n/7>
//  rows 2-12    six lines of text (every other row), tiles 0-107
//  row 14       characters the sample needs that aren't drawn yet (tiles 108-115 for codes 128-255)
//  rows 16-17   hints
//
// Keyboard: type your own sample, or (from the map's RENAME) the font's name.
//  row 0        hints
//  rows 2, 4    your text so far, set in your font (tiles 0-35)
//  rows 6-16    one page of the character set in Pocket Sans (every other row), bars above and below the
//               pick; moving off the top or bottom row turns the page (codes 128-255 use tiles 40-103)
//  row 17       length, page
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"
#include "sound.h"

#define SAMPLE_COUNT 7
#define CUSTOM_MAX 40
#define LINES 6

static const char *const samples[SAMPLE_COUNT - 1] = {
    "The quick brown fox jumps over the lazy dog.",
    "SPHINX OF BLACK QUARTZ, JUDGE MY VOW!",
    "0123456789 +-=*/ ()[]{}<> .,:;!? '\"` @#$%&^_|~\\",
    "Hello! Welcome to my shop. What can I get you today?",
    // Latin-1, written as escapes (split where the next letter would read as a hex digit)
    "\xA1Ol\xE9! \xC7" "a va? O\xF9 est le caf\xE9? Gr\xF6\xDF" "e \xBD \xA3" "5 \xA9 \xC6sir \xD8re Se\xF1or",
    // symbols, codes 128-159
    "\x88 \x89 \x8A \x8B \x8C \x8D \x92Hi!\x93 5\x98 \x99 \x95 \x9C \x9D \x84\x85\x86\x87 \x80\x81\x82\x83 \x8E\x8F\x9E\x9F",
};

static char custom[CUSTOM_MAX + 1];
static uint8_t custom_len;
static uint8_t sample;            // 0-3 built in, 4 your text
static uint8_t key = 'A' - 32;    // keyboard selection, within the page
static uint8_t kpage;

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
    uint8_t list[8], seen[GLYPH_COUNT / 8], n = 0, shown = 0, c, i;
    memset(seen, 0, sizeof seen);
    for (; *s; s++) {
        c = (uint8_t)*s;
        if (c == ' ' || c == 160 || c < 32 || c == 127 || (seen[(c - 32) >> 3] & (1 << ((c - 32) & 7)))) continue;
        seen[(c - 32) >> 3] |= 1 << ((c - 32) & 7);
        if (!is_blank(c)) continue;
        n++;
        if (shown < 8) list[shown++] = c;
    }
    text_pad(0, 14, "", 20);
    if (!n) {
        text(0, 14, "ALL GLYPHS DRAWN");
        return;
    }
    text(0, 14, "MISSING");
    text_num(8, 14, n);
    for (i = 0; i < shown; i++) set_bkg_tile_xy(12 + i, 14, ref_tile(list[i] - FIRST_CODE, 108 + i));
}

static void draw_sample(void) {
    const char *s = sample_text();
    text_block(s, strlen(s), 1, 2, LINES, 0);
    set_bkg_tile_xy(16, 0, TILE_UI('1' + sample));
    if (sample == SAMPLE_COUNT - 1 && !custom_len) text(1, 2, "(A: WRITE YOUR OWN)");
    draw_missing();
}

void typetest_enter(void) BANKED {
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "TYPE TEST");
    text(15, 0, "< /7>");
    draw_sample();
    text(0, 16, "<>:SAMPLE A:WRITE");
    text(0, 17, "B:BACK");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t typetest_update(void) BANKED {
    if (moves & J_LEFT) { sample = sample ? sample - 1 : SAMPLE_COUNT - 1; draw_sample(); sfx(SFX_MOVE); }
    if (moves & J_RIGHT) { sample = (sample + 1) % SAMPLE_COUNT; draw_sample(); sfx(SFX_MOVE); }
    if (pressed & J_A) return SCR_KEYBOARD;
    if (pressed & J_B) return SCR_MAP;
    return SCR_STAY;
}

// ---- keyboard ----

#define KEY_X 2
#define KEY_Y 6
#define NAME_MAX 12

static uint8_t naming;        // the keyboard is naming the font (ASCII only, page 1), not writing a sample
static char name[NAME_MAX + 1];
static uint8_t name_len;

static void draw_typed(void) {
    if (naming) {             // the name in the game's own lettering, as the fonts screen shows it
        text(0, 2, "NAME:");
        text_pad(6, 2, name, NAME_MAX);
        if (name_len < NAME_MAX) set_bkg_tile_xy(6 + name_len, 2, TILE_UI('_'));
        text_num(0, 17, name_len);
        text(name_len >= 10 ? 2 : 1, 17, "/12 ");
        return;
    }
    text_block(custom, custom_len, 1, 2, 2, 0);
    text_num(0, 17, custom_len);
    text(custom_len >= 10 ? 2 : 1, 17, "/40 ");
}

static void draw_keys(void) {
    uint8_t k, cells = page_cells[kpage];
    fill_bkg_rect(KEY_X, KEY_Y, 16, 11, TILE_BLANK);
    for (k = 0; k < cells; k++)
        set_bkg_tile_xy(KEY_X + k % 16, KEY_Y + 2 * (k / 16), ref_tile(page_first[kpage] + k, 40 + k));
    if (naming) return;
    set_bkg_tile_xy(6, 17, TILE_UI('1' + kpage));
    text(7, 17, "/3");
}

static void keyboard_open(void) {
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, naming ? "A:ADD B:DEL ST:SAVE" : "A:ADD B:DEL ST:DONE");
    text(11, 17, "SEL:SPACE");
    draw_keys();
    draw_typed();
    bars_show();
    bars_place(KEY_X, KEY_Y, key);
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

void keyboard_enter(void) BANKED {
    naming = 0;
    keyboard_open();
}

void rename_enter(void) BANKED {
    naming = 1;
    kpage = 0;   // names are ASCII: the fonts screen shows them in Pocket Sans
    if (key >= BASIC_COUNT) key = 'A' - FIRST_CODE;
    for (name_len = 0; name_len < NAME_MAX && font.name[name_len]; name_len++) name[name_len] = font.name[name_len];
    while (name_len && name[name_len - 1] == ' ') name_len--;
    name[name_len] = 0;
    keyboard_open();
}

static void key_page(int8_t dir, uint8_t k) {
    kpage = dir < 0 ? (kpage ? kpage - 1 : PAGE_COUNT - 1) : (kpage + 1) % PAGE_COUNT;
    key = k < page_cells[kpage] ? k : page_cells[kpage] - 1;
    draw_keys();
}

uint8_t keyboard_update(void) BANKED {
    uint8_t was = key, page_was = kpage, cells = page_cells[kpage];
    if ((moves & J_LEFT) && key > 0) key--;
    if ((moves & J_RIGHT) && key + 1 < cells) key++;
    if (moves & J_UP) {
        if (key >= 16) key -= 16;
        else if (!naming) key_page(-1, ((page_cells[kpage ? kpage - 1 : PAGE_COUNT - 1] - 1) / 16) * 16 + key);
    }
    if (moves & J_DOWN) {
        if (key / 16 < (cells - 1) / 16) key = key + 16 < cells ? key + 16 : cells - 1;
        else if (!naming) key_page(1, key % 16);
    }
    if (key != was || kpage != page_was) {
        bars_place(KEY_X, KEY_Y, key);
        sfx(SFX_MOVE);
    }
    if (naming) {
        if ((pressed & (J_A | J_SELECT)) && name_len < NAME_MAX) {
            name[name_len++] = (pressed & J_SELECT) ? ' ' : (char)(FIRST_CODE + key);
            name[name_len] = 0;
            draw_typed();
            sfx(SFX_INK);
        }
        if ((pressed & J_B) && name_len) {
            name[--name_len] = 0;
            draw_typed();
            sfx(SFX_ERASE);
        }
        if (pressed & J_START) {
            uint8_t i;
            while (name_len && name[name_len - 1] == ' ') name_len--;
            if (!name_len) {   // a font needs a name
                sfx(SFX_ERROR);
                return SCR_STAY;
            }
            for (i = 0; i < NAME_MAX; i++) font.name[i] = i < name_len ? name[i] : ' ';
            save_write();
            sfx(SFX_OK);
            return SCR_MAP;
        }
        return SCR_STAY;
    }
    if ((pressed & (J_A | J_SELECT)) && custom_len < CUSTOM_MAX) {
        custom[custom_len++] = (pressed & J_SELECT) ? ' ' : (char)(FIRST_CODE + page_first[kpage] + key);
        custom[custom_len] = 0;
        draw_typed();
        sfx(SFX_INK);
    }
    if ((pressed & J_B) && custom_len) {
        custom[--custom_len] = 0;
        draw_typed();
        sfx(SFX_ERASE);
    }
    if (pressed & J_START) {
        sample = SAMPLE_COUNT - 1;
        return SCR_TYPETEST;
    }
    return SCR_STAY;
}
