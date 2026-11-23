#ifndef APP_H
#define APP_H

#include <stdint.h>

extern uint8_t current;   // glyph index being viewed or edited (code - 32)

void map_enter(void);
uint8_t map_update(void);      // 1 = open the editor

void editor_enter(void);
uint8_t editor_update(void);   // 1 = back to the map

#endif
