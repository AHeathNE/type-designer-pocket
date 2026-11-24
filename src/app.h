#ifndef APP_H
#define APP_H

#include <gb/gb.h>   // BANKED
#include <stdint.h>

// screens; each *_update returns the next screen, or SCR_STAY
#define SCR_SLOTS 0
#define SCR_PICKER 1
#define SCR_CREDITS 2
#define SCR_MAP 3
#define SCR_EDITOR 4
#define SCR_TYPETEST 5
#define SCR_KEYBOARD 6
#define SCR_CHALLENGE 7   // challenge setup
#define SCR_COUNTDOWN 8
#define SCR_RESULTS 9
#define SCR_LEARN 10
#define SCR_TITLE 11
#define SCR_STAY 0xFF

extern uint8_t current;       // glyph index being viewed or edited (code - 32)
extern uint8_t picker_slot;   // slot the new-font screen will overwrite

// Challenge mode (challenge.c): the editor runs timed, one glyph from chal_list after another.
extern uint8_t chal_on;       // the editor is in a challenge
extern uint8_t chal_retry;    // ... redoing one glyph from the results screen
extern uint8_t chal_list[];   // glyph indexes, in order
extern uint8_t chal_n, chal_pos;
extern uint8_t chal_seconds;  // per glyph

// Design orders (order.c), chosen on the map and kept in the save header.
#define ORDER_COUNT 4
extern uint8_t order_list[];  // the current order: every glyph index but 127's
void order_build(void) BANKED;   // (re)build order_list for the saved order
uint8_t order_step(uint8_t i, int8_t dir) BANKED;
uint8_t order_next_undrawn(void) BANKED;   // or 0xFF when everything is drawn
void order_name(uint8_t x, uint8_t y, uint8_t win) BANKED;   // the saved order's name, on BG or window
uint16_t from_source(uint8_t code) BANKED; // START FROM: source glyph index | turn << 8 (1 flip H, 2 flip V,
                                    // 3 both), or 0xFFFF

extern uint8_t learn_back;    // the screen LEARN returns to

void slots_enter(void) BANKED; uint8_t slots_update(void) BANKED;
void picker_enter(void) BANKED; uint8_t picker_update(void) BANKED;
void credits_enter(void) BANKED; uint8_t credits_update(void) BANKED;
void map_enter(void) BANKED; uint8_t map_update(void) BANKED;
void editor_enter(void) BANKED; uint8_t editor_update(void) BANKED;
void typetest_enter(void) BANKED; uint8_t typetest_update(void) BANKED;
void keyboard_enter(void) BANKED; uint8_t keyboard_update(void) BANKED;
void challenge_enter(void) BANKED; uint8_t challenge_update(void) BANKED;
void countdown_enter(void) BANKED; uint8_t countdown_update(void) BANKED;
void results_enter(void) BANKED; uint8_t results_update(void) BANKED;
void learn_enter(void) BANKED; uint8_t learn_update(void) BANKED;
void title_enter(void) BANKED; uint8_t title_update(void) BANKED;

#endif
