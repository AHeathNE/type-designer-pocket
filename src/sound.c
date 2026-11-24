// Sound effects (see sound.h). Bank 0: called from every screen, and the tables are small.
#include <gb/gb.h>
#include "sound.h"
#include "save.h"

// One step: which channel, then for channels 1-2 the 11-bit frequency value (2048 - 131072 / Hz), for
// channel 4 the NR43 noise setting; the volume envelope (volume << 4 | direction << 3 | pace); frames to
// hold before the next step. A step with frames = 0 ends the effect.
typedef struct {
    uint8_t ch;
    uint16_t freq;
    uint8_t env;
    uint8_t frames;
} Step;

#define C4 1548u
#define G4 1714u
#define A4 1750u
#define C5 1797u
#define E5 1849u
#define G5 1881u
#define C6 1923u
#define E6 1949u
#define END {0, 0, 0, 0}

static const Step s_move[] = {{2, G5, 0x51, 2}, END};
static const Step s_ok[] = {{2, C5, 0x81, 3}, {2, G5, 0x81, 4}, END};
static const Step s_back[] = {{2, G5, 0x61, 3}, {2, C5, 0x61, 4}, END};
static const Step s_ink[] = {{2, C6, 0x31, 1}, END};
static const Step s_erase[] = {{4, 0x31, 0x31, 1}, END};
static const Step s_error[] = {{1, C4, 0x92, 6}, {1, C4 - 60, 0x92, 8}, END};
static const Step s_count[] = {{1, A4, 0xA3, 12}, END};
static const Step s_go[] = {{1, C6, 0xC4, 24}, END};
static const Step s_tick[] = {{4, 0x10, 0x41, 1}, END};
static const Step s_next[] = {{2, E5, 0x82, 4}, {2, C6, 0x82, 6}, END};
static const Step s_done[] = {{1, C5, 0xB2, 6}, {1, E5, 0xB2, 6}, {1, G5, 0xB2, 6}, {1, C6, 0xB4, 18}, END};
static const Step *const effects[SFX_COUNT_ALL] = {s_move, s_ok, s_back, s_ink, s_erase, s_error,
                                                   s_count, s_go, s_tick, s_next, s_done};

static const Step *playing;
static uint8_t wait;

static void play_step(const Step *s) {
    switch (s->ch) {
    case 1:
        NR10_REG = 0x00;                       // no sweep
        NR11_REG = 0x80;                       // 50% duty
        NR12_REG = s->env;
        NR13_REG = (uint8_t)s->freq;
        NR14_REG = 0x80 | (uint8_t)(s->freq >> 8);   // trigger
        break;
    case 2:
        NR21_REG = 0x40;                       // 25% duty: a thinner blip
        NR22_REG = s->env;
        NR23_REG = (uint8_t)s->freq;
        NR24_REG = 0x80 | (uint8_t)(s->freq >> 8);
        break;
    case 4:
        NR41_REG = 0x00;
        NR42_REG = s->env;
        NR43_REG = (uint8_t)s->freq;
        NR44_REG = 0x80;
        break;
    }
    wait = s->frames;
}

void sound_apply(void) {
    if (save_sound()) {
        NR52_REG = 0x80;   // sound on (must come first)
        NR50_REG = 0x77;   // full volume, both sides
        NR51_REG = 0xFF;   // every channel to both sides
    } else {
        playing = 0;
        NR52_REG = 0x00;   // all sound off
    }
}

void sound_init(void) {
    sound_apply();
}

void sfx(uint8_t id) {
    if (!save_sound()) return;
    playing = effects[id];
    play_step(playing);
}

void sound_update(void) {
    if (!playing || --wait) return;
    playing++;
    if (!playing->frames) {
        playing = 0;
        return;
    }
    play_step(playing);
}
