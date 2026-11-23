#ifndef APP_H
#define APP_H

#include <stdint.h>

// screens; each *_update returns the next screen, or SCR_STAY
#define SCR_SLOTS 0
#define SCR_PICKER 1
#define SCR_CREDITS 2
#define SCR_MAP 3
#define SCR_EDITOR 4
#define SCR_STAY 0xFF

extern uint8_t current;       // glyph index being viewed or edited (code - 32)
extern uint8_t picker_slot;   // slot the new-font screen will overwrite

void slots_enter(void);   uint8_t slots_update(void);
void picker_enter(void);  uint8_t picker_update(void);
void credits_enter(void); uint8_t credits_update(void);
void map_enter(void);     uint8_t map_update(void);
void editor_enter(void);  uint8_t editor_update(void);

#endif
