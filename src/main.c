// Type Designer Pocket
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"
#include "save.h"

uint8_t current = 'A' - FIRST_CODE;

void main(void) {
    uint8_t editing = 0;

    DISPLAY_OFF;
    gfx_init();
    save_init();
    map_enter();

    while (1) {
        vsync();
        input_update();
        if (!editing) {
            if (map_update()) { editor_enter(); editing = 1; }
        } else {
            if (editor_update()) { map_enter(); editing = 0; }
        }
    }
}
