// Joypad input. The pad is read once per frame in the VBlank interrupt, which counts every press of every
// button. input_update() hands out one queued press per button per frame, so taps made while the game is
// busy for several frames (saving, redrawing a screen) are neither lost nor merged: five quick taps on Right
// are always five moves.
#include <gb/gb.h>
#include "input.h"

#define REPEAT_DELAY 14   // frames before a held direction starts repeating
#define REPEAT_RATE 4     // frames between repeats
#define QUEUE_MAX 8       // presses remembered per button

uint8_t keys, pressed, released, moves;
static uint8_t repeat_timer;
static volatile uint8_t pad_now, pad_up;
static volatile uint8_t queued[8];   // presses not yet handed out, per button bit

static void pad_isr(void) {
    uint8_t prev = pad_now, down, i;
    pad_now = joypad();
    down = pad_now & ~prev;
    pad_up |= prev & ~pad_now;
    for (i = 0; down; i++, down >>= 1)
        if ((down & 1) && queued[i] < QUEUE_MAX) queued[i]++;
}

void input_init(void) {
    CRITICAL {
        add_VBL(pad_isr);
    }
}

void input_update(void) {
    uint8_t i, bit;
    pressed = 0;
    CRITICAL {
        keys = pad_now;
        released = pad_up;
        pad_up = 0;
        for (i = 0, bit = 1; i < 8; i++, bit <<= 1) {
            if (queued[i]) {
                queued[i]--;
                pressed |= bit;
            }
        }
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
