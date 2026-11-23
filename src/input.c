#include <gb/gb.h>
#include "input.h"

#define REPEAT_DELAY 14   // frames before a held direction starts repeating
#define REPEAT_RATE 4     // frames between repeats

uint8_t keys, pressed, released, moves;
static uint8_t repeat_timer;

void input_update(void) {
    uint8_t prev = keys;
    keys = joypad();
    pressed = keys & ~prev;
    released = prev & ~keys;

    moves = 0;
    if (pressed & J_DPAD) {
        moves = pressed & J_DPAD;
        repeat_timer = REPEAT_DELAY;
    } else if (keys & J_DPAD) {
        if (--repeat_timer == 0) {
            moves = keys & J_DPAD;
            repeat_timer = REPEAT_RATE;
        }
    }
}
