#pragma bank 2   // a BANKED screen or helper: see "ROM banks" in the Makefile
// Design orders: the sequence the editor's SELECT + Left/Right, the map's NEXT GLYPH and the challenge
// follow. Each order lists some characters first; everything else follows in code order.
#include <gb/gb.h>
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "save.h"

static const char *const order_names[ORDER_COUNT] = {"A-Z", "CONTROL", "HAMBURG", "FREQUENT"};

void order_name(uint8_t x, uint8_t y, uint8_t win) BANKED {
    if (win) win_text(x, y, order_names[save_order()]);
    else text_pad(x, y, order_names[save_order()], 8);
}
static const char *const order_first[ORDER_COUNT] = {
    "",
    // n and o set the stems, spacing and curves, then H and O for capitals; then each family:
    // straights, rounds, diagonals, the rest
    "nohmurlicedbpqvwxykzagfstj" "HOEFLTICGDQBPRUJAVWXYKMNZS",
    // the classic test word, lowercase then capitals
    "hamburgefonstiv" "HAMBURGEFONSTIV",
    // English letters, most used first
    "etaoinshrdlcumwfgypbvkjxqz" "ETAOINSHRDLCUMWFGYPBVKJXQZ",
};

uint8_t order_list[GLYPH_COUNT - 1];   // glyph indexes, every one but 127
static uint8_t built = 0xFF;

void order_build(void) BANKED {
    uint8_t seen[GLYPH_COUNT / 8], n = 0, i;
    const char *p;
    if (built == save_order()) return;
    built = save_order();
    memset(seen, 0, sizeof seen);
    for (p = order_first[built]; *p; p++) {
        i = (uint8_t)*p - FIRST_CODE;
        if (seen[i >> 3] & (1 << (i & 7))) continue;
        seen[i >> 3] |= 1 << (i & 7);
        order_list[n++] = i;
    }
    for (i = 0; i < GLYPH_COUNT; i++) {
        if (i == UNUSED_GLYPH || (seen[i >> 3] & (1 << (i & 7)))) continue;
        order_list[n++] = i;
    }
}

uint8_t order_step(uint8_t i, int8_t dir) BANKED {
    uint8_t k;
    order_build();
    for (k = 0; order_list[k] != i; k++) {}
    if (dir < 0) k = k ? k - 1 : GLYPH_COUNT - 2;
    else k = (k + 1 < GLYPH_COUNT - 1) ? k + 1 : 0;
    return order_list[k];
}

// The first glyph in the order that isn't drawn yet (spaces and the soft hyphen don't count), or 0xFF.
uint8_t order_next_undrawn(void) BANKED {
    uint8_t k, i, code;
    order_build();
    for (k = 0; k < GLYPH_COUNT - 1; k++) {
        i = order_list[k];
        code = FIRST_CODE + i;
        if (code == 32 || code == 160 || code == 173) continue;
        if (!glyph_has_ink(i)) return i;
    }
    return 0xFF;
}

// ---- START FROM: a related glyph to begin from ----
// Each entry: the character, the one to copy, and how to turn it (FROM_*). Families follow the control
// order above (n makes h m u r; o makes c e d; d makes b, and so on); accented letters start from their
// base letter, mirrored pairs from their twin.
#define FROM_COPY 0
#define FROM_FLIP_H 1
#define FROM_FLIP_V 2
#define FROM_TURN 3   // both: half a turn
static const uint8_t from_table[][3] = {
    {'h', 'n', FROM_COPY}, {'m', 'n', FROM_COPY}, {'u', 'n', FROM_TURN}, {'r', 'n', FROM_COPY},
    {'i', 'l', FROM_COPY}, {'j', 'i', FROM_COPY}, {'t', 'l', FROM_COPY}, {'f', 't', FROM_COPY},
    {'k', 'l', FROM_COPY}, {'c', 'o', FROM_COPY}, {'e', 'o', FROM_COPY}, {'d', 'o', FROM_COPY},
    {'b', 'd', FROM_FLIP_H}, {'p', 'b', FROM_COPY}, {'q', 'p', FROM_FLIP_H}, {'a', 'o', FROM_COPY},
    {'g', 'q', FROM_COPY}, {'w', 'v', FROM_COPY}, {'y', 'v', FROM_COPY}, {'x', 'v', FROM_COPY},
    {'E', 'H', FROM_COPY}, {'F', 'E', FROM_COPY}, {'L', 'E', FROM_COPY}, {'I', 'T', FROM_COPY},
    {'C', 'O', FROM_COPY}, {'G', 'C', FROM_COPY}, {'D', 'O', FROM_COPY}, {'Q', 'O', FROM_COPY},
    {'P', 'B', FROM_COPY}, {'R', 'P', FROM_COPY}, {'U', 'H', FROM_COPY}, {'J', 'U', FROM_COPY},
    {'V', 'A', FROM_FLIP_V}, {'W', 'V', FROM_COPY}, {'Y', 'V', FROM_COPY}, {'X', 'V', FROM_COPY},
    {'K', 'H', FROM_COPY}, {'M', 'H', FROM_COPY}, {'N', 'H', FROM_COPY},
    {'0', 'O', FROM_COPY}, {'1', 'I', FROM_COPY}, {'5', 'S', FROM_COPY}, {'9', '6', FROM_TURN},
    {')', '(', FROM_FLIP_H}, {']', '[', FROM_FLIP_H}, {'}', '{', FROM_FLIP_H}, {'>', '<', FROM_FLIP_H},
    {'\\', '/', FROM_FLIP_H}, {';', ':', FROM_COPY}, {'"', '\'', FROM_COPY},
    {129, 128, FROM_FLIP_H}, {131, 130, FROM_FLIP_V}, {133, 132, FROM_FLIP_V}, {135, 134, FROM_FLIP_H},
    {143, 142, FROM_COPY}, {145, 144, FROM_TURN}, {147, 146, FROM_TURN}, {151, 150, FROM_COPY},
    {155, 'o', FROM_COPY}, {154, 'O', FROM_COPY}, {157, 'x', FROM_COPY}, {159, 158, FROM_COPY},
    {161, '!', FROM_TURN}, {191, '?', FROM_TURN}, {187, 171, FROM_FLIP_H}, {189, 188, FROM_COPY},
    {190, 188, FROM_COPY}, {215, 'x', FROM_COPY}, {198, 'E', FROM_COPY}, {230, 'e', FROM_COPY},
    {208, 'D', FROM_COPY}, {222, 'P', FROM_COPY}, {254, 'p', FROM_COPY}, {223, 'B', FROM_COPY},
    {240, 'o', FROM_COPY}, {216, 'O', FROM_COPY}, {248, 'o', FROM_COPY}, {199, 'C', FROM_COPY},
    {231, 'c', FROM_COPY}, {209, 'N', FROM_COPY}, {241, 'n', FROM_COPY}, {221, 'Y', FROM_COPY},
    {253, 'y', FROM_COPY}, {255, 'y', FROM_COPY},
};

// accented vowels, in blocks: À-Å à-å È-Ë è-ë Ì-Ï ì-ï Ò-Ö ò-ö Ù-Ü ù-ü
static const uint8_t accent_blocks[][3] = {
    {192, 197, 'A'}, {224, 229, 'a'}, {200, 203, 'E'}, {232, 235, 'e'}, {204, 207, 'I'},
    {236, 239, 'i'}, {210, 214, 'O'}, {242, 246, 'o'}, {217, 220, 'U'}, {249, 252, 'u'},
};

// The glyph index to start `code` from, with its FROM_* turn in the high byte, or 0xFFFF for none.
uint16_t from_source(uint8_t code) BANKED {
    uint8_t k;
    for (k = 0; k < sizeof accent_blocks / 3; k++)
        if (code >= accent_blocks[k][0] && code <= accent_blocks[k][1])
            return accent_blocks[k][2] - FIRST_CODE;
    for (k = 0; k < sizeof from_table / 3; k++)
        if (from_table[k][0] == code)
            return ((uint16_t)from_table[k][2] << 8) | (uint8_t)(from_table[k][1] - FIRST_CODE);
    return 0xFFFF;
}
