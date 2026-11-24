// Type Designer Pocket
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"
#include "sound.h"

uint8_t current = 'A' - FIRST_CODE;
uint8_t picker_slot;

// The screens are BANKED functions in ROM banks 1-3, so they're called by name (a banked call goes through
// a trampoline that maps the right bank in), not through a table of plain function pointers.
static void enter(uint8_t screen) {
    switch (screen) {
    case SCR_SLOTS: slots_enter(); break;
    case SCR_PICKER: picker_enter(); break;
    case SCR_CREDITS: credits_enter(); break;
    case SCR_MAP: map_enter(); break;
    case SCR_EDITOR: editor_enter(); break;
    case SCR_TYPETEST: typetest_enter(); break;
    case SCR_KEYBOARD: keyboard_enter(); break;
    case SCR_CHALLENGE: challenge_enter(); break;
    case SCR_COUNTDOWN: countdown_enter(); break;
    case SCR_RESULTS: results_enter(); break;
    case SCR_LEARN: learn_enter(); break;
    case SCR_TITLE: title_enter(); break;
    }
}

static uint8_t update(uint8_t screen) {
    switch (screen) {
    case SCR_SLOTS: return slots_update();
    case SCR_PICKER: return picker_update();
    case SCR_CREDITS: return credits_update();
    case SCR_MAP: return map_update();
    case SCR_EDITOR: return editor_update();
    case SCR_TYPETEST: return typetest_update();
    case SCR_KEYBOARD: return keyboard_update();
    case SCR_CHALLENGE: return challenge_update();
    case SCR_COUNTDOWN: return countdown_update();
    case SCR_RESULTS: return results_update();
    case SCR_LEARN: return learn_update();
    case SCR_TITLE: return title_update();
    }
    return SCR_STAY;
}

void main(void) {
    uint8_t screen = SCR_TITLE, next;

    DISPLAY_OFF;
    gfx_init();
    save_init();
    theme_apply();
    sound_init();
    input_init();
    enter(SCR_TITLE);

    while (1) {
        vsync();
        input_update();
        sound_update();
        next = update(screen);
        if (next != SCR_STAY) {
            screen = next;
            enter(screen);
        }
    }
}
