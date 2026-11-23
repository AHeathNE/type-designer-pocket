// Type Designer Pocket: M0 hello ROM.
// Proves the three things the real game depends on:
//   1. text on screen
//   2. a glyph built at runtime from 8 bytes of 1bpp data (how the editor will show your font)
//   3. battery-backed save RAM that survives a power cycle (a boot counter)
#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char magic[4];     // "TDP0"
    uint8_t version;
    uint16_t boots;
} SaveHeader;

#define SAVE ((SaveHeader *)0xA000)
#define GLYPH_TILE 200

// A 1bpp "A": one byte per row, bit 7 = leftmost pixel.
static const uint8_t glyph_A[8] = {0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00};

// Save RAM is only touched with whole-struct memcpy into and out of a work copy in normal RAM.
// SDCC 4.5/4.6 (sm83) can mis-track HL across a store and re-read of the same SRAM field through a
// fixed pointer (SAVE->boots++ then reading SAVE->boots returned the byte after it), so don't do that.
static SaveHeader work;

static uint16_t count_boot(void) {
    ENABLE_RAM;
    SWITCH_RAM(0);
    memcpy(&work, SAVE, sizeof work);
    if (memcmp(work.magic, "TDP0", 4) != 0) {
        memcpy(work.magic, "TDP0", 4);
        work.version = 0;
        work.boots = 0;
    }
    work.boots++;
    memcpy(SAVE, &work, sizeof work);
    DISABLE_RAM;
    return work.boots;
}

void main(void) {
    uint16_t boots = count_boot();

    printf("\n TYPE DESIGNER\n     POCKET\n\n");
    printf(" hello from M0\n\n");
    printf(" boots saved: %u\n\n", boots);
    printf(" glyph from RAM:\n");

    set_bkg_1bpp_data(GLYPH_TILE, 1, glyph_A);
    set_bkg_tile_xy(17, 8, GLYPH_TILE);

    SHOW_BKG;
    DISPLAY_ON;
    while (1) {
        vsync();
    }
}
