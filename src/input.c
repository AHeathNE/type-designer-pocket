// Joypad input. The pad is read once per frame in the VBlank interrupt, which also records every button
// that went down or up since the last input_update(). A quick tap made while the game is busy for several
// frames (saving, redrawing a screen) still counts as a press instead of being lost, even if the same
// button was already down the last time the main loop looked.
#include <gb/gb.h>
#include "input.h"

#define REPEAT_DELAY 14   // frames before a held direction starts repeating
#define REPEAT_RATE 4     // frames between repeats

uint8_t keys, pressed, released, moves;
static uint8_t repeat_timer;
static volatile uint8_t pad_now, pad_down, pad_up;

static void pad_isr(void) {
    uint8_t prev = pad_now;
    pad_now = joypad();
    pad_down |= pad_now & ~prev;
    pad_up |= prev & ~pad_now;
}

void input_init(void) {
    CRITICAL {
        add_VBL(pad_isr);
    }
}

void input_update(void) {
    CRITICAL {
        keys = pad_now;
        pressed = pad_down;
        released = pad_up;
        pad_down = 0;
        pad_up = 0;
    }

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
