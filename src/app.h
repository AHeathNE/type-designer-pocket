#ifndef APP_H
#define APP_H

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
#define SCR_STAY 0xFF

extern uint8_t current;       // glyph index being viewed or edited (code - 32)
extern uint8_t picker_slot;   // slot the new-font screen will overwrite

// Challenge mode (challenge.c): the editor runs timed, one glyph from chal_list after another.
extern uint8_t chal_on;       // the editor is in a challenge
extern uint8_t chal_retry;    // ... redoing one glyph from the results screen
extern uint8_t chal_list[];   // glyph indexes, in order
extern uint8_t chal_n, chal_pos;
extern uint8_t chal_seconds;  // per glyph

void slots_enter(void);   uint8_t slots_update(void);
void picker_enter(void);  uint8_t picker_update(void);
void credits_enter(void); uint8_t credits_update(void);
void map_enter(void);     uint8_t map_update(void);
void editor_enter(void);  uint8_t editor_update(void);
void typetest_enter(void); uint8_t typetest_update(void);
void keyboard_enter(void); uint8_t keyboard_update(void);
void challenge_enter(void); uint8_t challenge_update(void);
void countdown_enter(void); uint8_t countdown_update(void);
void results_enter(void); uint8_t results_update(void);

#endif
