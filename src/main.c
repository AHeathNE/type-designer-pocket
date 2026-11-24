// Type Designer Pocket
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

uint8_t current = 'A' - FIRST_CODE;
uint8_t picker_slot;

typedef void (*enter_fn)(void);
typedef uint8_t (*update_fn)(void);
static const enter_fn enters[] = {slots_enter, picker_enter, credits_enter, map_enter, editor_enter,
                                  typetest_enter, keyboard_enter, challenge_enter, countdown_enter,
                                  results_enter};
static const update_fn updates[] = {slots_update, picker_update, credits_update, map_update, editor_update,
                                    typetest_update, keyboard_update, challenge_update, countdown_update,
                                    results_update};

void main(void) {
    uint8_t screen = SCR_SLOTS, next;

    DISPLAY_OFF;
    gfx_init();
    save_init();
    input_init();
    slots_enter();

    while (1) {
        vsync();
        input_update();
        next = updates[screen]();
        if (next != SCR_STAY) {
            screen = next;
            enters[screen]();
        }
    }
}
