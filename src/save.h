// Battery save layout. Must match tools/tdpsave.py (which documents every byte).
#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

#define GLYPH_COUNT 224   // codes 32-255, GB Studio's full range
#define FIRST_CODE 32
#define EDIT_COUNT 95     // codes 32-126 are editable in M1
#define SLOT_COUNT 3
#define WIDTH_AUTO 0xFF

typedef struct {
    char magic[4];        // "TDPF"
    uint8_t version;      // 1
    uint8_t slot_count;   // 3
    uint8_t active;       // slot shown at boot
    uint8_t reserved[7];
    uint16_t crc;         // CRC-16/CCITT-FALSE of the 14 bytes above
} SaveHeader;             // 16 bytes

typedef struct {
    char name[12];
    uint8_t template_id;
    uint8_t space_width;
    uint8_t used[GLYPH_COUNT / 8];
    uint8_t widths[GLYPH_COUNT];
    uint8_t glyphs[GLYPH_COUNT][8];
    uint16_t crc;         // CRC-16/CCITT-FALSE of everything above
} FontSlot;               // 2060 bytes

#define SAVE_OK 0
#define SAVE_NEW 1        // no save found, formatted a fresh one
#define SAVE_DAMAGED 2    // a checksum failed; data kept as-is, never wiped

extern FontSlot font;     // working copy of the active slot, in normal RAM
extern uint8_t save_status;

void save_init(void);
void save_write(void);
uint8_t glyph_has_ink(uint8_t i);
uint16_t crc16(const uint8_t *p, uint16_t n);

#endif
