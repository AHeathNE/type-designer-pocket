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
// Checksums are verified once per slot at power-on and remembered: the game is the only writer, and a full
// CRC of a 2 KB slot costs about 9 frames, too slow to repeat every time the fonts screen is drawn.
static uint8_t slot_ok[SLOT_COUNT];

static uint8_t *slot_addr(uint8_t i) {
    return SRAM + sizeof(SaveHeader) + (uint16_t)i * SLOT_STRIDE;
}

// CRC-16/CCITT-FALSE, table driven, with the table split into high and low bytes so the inner loop is
// all 8-bit operations (SDCC's 16-bit shifts made the plain version several times slower).
static uint8_t crc_hi[256], crc_lo[256];

static void crc_init(void) {
    uint16_t c;
    uint8_t bit, i = 0;
    do {
        c = (uint16_t)i << 8;
        for (bit = 0; bit < 8; bit++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1;
        crc_hi[i] = c >> 8;
        crc_lo[i] = c & 0xFF;
    } while (++i);
}

static uint16_t crc16_add(uint16_t crc, const uint8_t *p, uint16_t n) {
    uint8_t hi = crc >> 8, lo = crc & 0xFF, idx;
    while (n--) {
        idx = hi ^ *p++;
        hi = lo ^ crc_hi[idx];
        lo = crc_lo[idx];
    }
    return ((uint16_t)hi << 8) | lo;
}

uint16_t crc16(const uint8_t *p, uint16_t n) {
    return crc16_add(0xFFFF, p, n);
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

static void write_header(void) {
    header.crc = crc16((const uint8_t *)&header, sizeof header - 2);
    memcpy(SRAM, &header, sizeof header);
}

static void blank_font(uint8_t slot) {
    memset(&font, 0, sizeof font);
    memcpy(font.name, "FONT 1      ", 12);
    font.name[5] = '1' + slot;
    font.template_id = 0;
    font.space_width = 4;
    memset(font.widths, WIDTH_AUTO, sizeof font.widths);
}

static void format_save(void) {
    uint8_t i;
    for (i = 0; i < SLOT_COUNT; i++) {
        blank_font(i);
        seal_font();
        memcpy(slot_addr(i), &font, sizeof font);
    }
    memset(&header, 0, sizeof header);
    memcpy(header.magic, "TDPF", 4);
    header.version = 1;
    header.slot_count = SLOT_COUNT;
    header.active = 0;
    write_header();
}

// CRC a slot straight from save RAM, a piece at a time (no second 2 KB copy in RAM). RAM must be enabled.
static uint8_t slot_crc_ok(uint8_t slot) {
    uint8_t buf[64], n;
    uint16_t off, crc = 0xFFFF, stored;
    const uint8_t *src = slot_addr(slot);
    for (off = 0; off < sizeof(FontSlot) - 2; off += n) {
        n = (sizeof(FontSlot) - 2 - off) < sizeof buf ? (uint8_t)(sizeof(FontSlot) - 2 - off) : sizeof buf;
        memcpy(buf, src + off, n);
        crc = crc16_add(crc, buf, n);
    }
    memcpy(&stored, src + sizeof(FontSlot) - 2, 2);
    return crc == stored;
}

void save_init(void) {
    uint8_t i;
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
    for (i = 0; i < SLOT_COUNT; i++) slot_ok[i] = slot_crc_ok(i);
    memcpy(&font, slot_addr(header.active), sizeof font);
    if (!slot_ok[header.active]) save_status = SAVE_DAMAGED;
    DISABLE_RAM;
}

void save_write(void) {
    seal_font();
    ENABLE_RAM;
    SWITCH_RAM(0);
    memcpy(slot_addr(header.active), &font, sizeof font);
    DISABLE_RAM;
    slot_ok[header.active] = 1;
}

uint8_t save_active(void) {
    return header.active;
}

void save_select(uint8_t slot) {
    ENABLE_RAM;
    SWITCH_RAM(0);
    if (header.active != slot) {
        header.active = slot;
        write_header();
    }
    memcpy(&font, slot_addr(slot), sizeof font);
    DISABLE_RAM;
    save_status = slot_ok[slot] ? SAVE_OK : SAVE_DAMAGED;
}

// Name, guide and glyph count from the first 42 bytes of a slot (name .. used bitmap).
void save_slot_info(uint8_t slot, SlotInfo *out) {
    uint8_t buf[42], i;
    ENABLE_RAM;
    SWITCH_RAM(0);
    memcpy(buf, slot_addr(slot), sizeof buf);
    DISABLE_RAM;
    memcpy(out->name, buf, 12);
    out->name[12] = 0;
    for (i = 12; i && out->name[i - 1] == ' '; i--) out->name[i - 1] = 0;
    out->template_id = buf[12];
    out->drawn = out->drawn_ext = 0;
    for (i = 0; i < GLYPH_COUNT; i++)   // used bitmap starts at byte 14
        if (buf[14 + (i >> 3)] & (1 << (i & 7))) {
            if (i < BASIC_COUNT) out->drawn++;
            else out->drawn_ext++;
        }
    out->ok = slot_ok[slot];
}

void save_reset(uint8_t template_id, const glyph_t *copy_from) {
    char name[12];
    memcpy(name, font.name, 12);
    blank_font(header.active);
    memcpy(font.name, name, 12);
    font.template_id = template_id;
    if (copy_from) memcpy(font.glyphs, copy_from, sizeof font.glyphs);
    save_write();
}
