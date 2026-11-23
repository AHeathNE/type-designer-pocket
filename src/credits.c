// Credits: who made what, and under which licence.
#include <gb/gb.h>
#include "app.h"
#include "gfx.h"
#include "input.h"

void credits_enter(void) {
    DISPLAY_OFF;
    gfx_clear();
    text(0, 0, "CREDITS");
    text(0, 2, "TYPE DESIGNER POCKET");
    text(1, 3, "by AHeathNE  (MIT)");
    text(0, 5, "POCKET SANS + SERIF");
    text(1, 6, "AHeathNE (OFL 1.1)");
    text(0, 8, "CLASSIC 8X8");
    text(1, 9, "font8x8 by Daniel");
    text(1, 10, "Hepper, from IBM");
    text(1, 11, "VGA. Public domain");
    text(0, 13, "MADE WITH GBDK-2020");
    text(0, 17, "B:BACK");
    SHOW_BKG;
    DISPLAY_ON;
}

uint8_t credits_update(void) {
    return (pressed & (J_A | J_B | J_START | J_SELECT)) ? SCR_SLOTS : SCR_STAY;
}
