// Joypad input. The pad is read once per frame in the VBlank interrupt, which queues every press of every
// button in the order they happened. input_update() hands out one press per frame, so taps made while the
// game is busy for several frames (saving, redrawing a screen) are neither lost, merged nor reordered: five
// quick taps on Right are always five moves, and SELECT then Up is a page turn then a move.
#include <gb/gb.h>
#include "input.h"

#define REPEAT_DELAY 14   // frames before a held direction starts repeating
#define REPEAT_RATE 4     // frames between repeats
#define QUEUE_MAX 16      // presses remembered (a power of two)

uint8_t keys, pressed, released, moves;
static uint8_t repeat_timer;
static volatile uint8_t pad_now, pad_up;
static volatile uint8_t queue[QUEUE_MAX], q_head, q_len;   // each entry is one button's bit

static void pad_isr(void) {
    uint8_t prev = pad_now, down, bit;
    pad_now = joypad();
    down = pad_now & ~prev;
    pad_up |= prev & ~pad_now;
    for (bit = 1; down; bit <<= 1) {
        if (!(down & bit)) continue;
        down &= ~bit;
        if (q_len < QUEUE_MAX) {
            queue[(q_head + q_len) & (QUEUE_MAX - 1)] = bit;
            q_len++;
        }
    }
}

void input_init(void) {
    CRITICAL {
        add_VBL(pad_isr);
    }
}

void input_update(void) {
    pressed = 0;
    CRITICAL {
        keys = pad_now;
        released = pad_up;
        pad_up = 0;
        if (q_len) {
            pressed = queue[q_head];
            q_head = (q_head + 1) & (QUEUE_MAX - 1);
            q_len--;
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
