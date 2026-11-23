// Save RAM is only ever touched with whole-struct memcpy into and out of copies in normal RAM.
// SDCC 4.5/4.6 (sm83) can mis-track HL when a field behind a fixed SRAM pointer is written and then
// read back, so never read or write SRAM fields directly.
#include <gb/gb.h>
#include <string.h>
#include "save.h"

#define SRAM ((uint8_t *)0xA000)
#define SLOT_STRIDE 2064u

FontSlot font;
uint8_t save_status;
static SaveHeader header;

static uint8_t *slot_addr(uint8_t i) {
    return SRAM + sizeof(SaveHeader) + (uint16_t)i * SLOT_STRIDE;
}

// CRC-16/CCITT-FALSE, table driven: a bit-at-a-time loop took about a second per 2 KB slot.
static uint16_t crc_table[256];

static void crc_init(void) {
    uint16_t c;
    uint8_t bit, i = 0;
    do {
        c = (uint16_t)i << 8;
        for (bit = 0; bit < 8; bit++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1;
        crc_table[i] = c;
    } while (++i);
}

uint16_t crc16(const uint8_t *p, uint16_t n) {
    uint16_t crc = 0xFFFF;
    while (n--) crc = (crc << 8) ^ crc_table[(uint8_t)(crc >> 8) ^ *p++];
    return crc;
}

uint8_t glyph_has_ink(uint8_t i) {
    const uint8_t *g = font.glyphs[i];
    return g[0] | g[1] | g[2] | g[3] | g[4] | g[5] | g[6] | g[7];
}

static void seal_font(void) {
    uint8_t i;
    memset(font.used, 0, sizeof font.used);
    for (i = 0; i < GLYPH_COUNT; i++)
        if (glyph_has_ink(i)) font.used[i >> 3] |= 1 << (i & 7);
    font.crc = crc16((const uint8_t *)&font, sizeof font - 2);
}

static void format_save(void) {
    uint8_t i;
    for (i = 0; i < SLOT_COUNT; i++) {
        memset(&font, 0, sizeof font);
        memcpy(font.name, "FONT 1      ", 12);
        font.name[5] = '1' + i;
        font.space_width = 4;
        memset(font.widths, WIDTH_AUTO, sizeof font.widths);
        seal_font();
        memcpy(slot_addr(i), &font, sizeof font);
    }
    memset(&header, 0, sizeof header);
    memcpy(header.magic, "TDPF", 4);
    header.version = 1;
    header.slot_count = SLOT_COUNT;
    header.active = 0;
    header.crc = crc16((const uint8_t *)&header, sizeof header - 2);
    memcpy(SRAM, &header, sizeof header);
}

void save_init(void) {
    crc_init();
    ENABLE_RAM;
    SWITCH_RAM(0);
    memcpy(&header, SRAM, sizeof header);
    if (memcmp(header.magic, "TDPF", 4) != 0) {
        // nothing of ours here (new cartridge): safe to format
        format_save();
        save_status = SAVE_NEW;
    } else {
        // ours, so never wipe it, even if a checksum fails
        save_status = SAVE_OK;
        if (header.version != 1 || header.crc != crc16((const uint8_t *)&header, sizeof header - 2))
            save_status = SAVE_DAMAGED;
        if (header.active >= SLOT_COUNT) header.active = 0;
    }
    memcpy(&font, slot_addr(header.active), sizeof font);
    if (font.crc != crc16((const uint8_t *)&font, sizeof font - 2)) save_status = SAVE_DAMAGED;
    DISABLE_RAM;
}

void save_write(void) {
    seal_font();
    ENABLE_RAM;
    SWITCH_RAM(0);
    memcpy(slot_addr(header.active), &font, sizeof font);
    DISABLE_RAM;
}
