#ifndef INPUT_H
#define INPUT_H

#include <gb/gb.h>
#include <stdint.h>

#define J_DPAD (J_UP | J_DOWN | J_LEFT | J_RIGHT)

extern uint8_t keys;      // held this frame
extern uint8_t pressed;   // went down this frame
extern uint8_t released;  // went up this frame
extern uint8_t moves;     // d-pad directions that fire this frame, with auto-repeat

void input_update(void);

#endif
