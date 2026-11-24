// Sound effects: short note sequences on the square (1, 2) and noise (4) channels, stepped once a frame.
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

enum {
    SFX_MOVE,      // a menu cursor or page moves
    SFX_OK,        // something chosen
    SFX_BACK,      // a menu closes, a screen goes back
    SFX_INK,       // a pixel painted
    SFX_ERASE,     // a pixel erased
    SFX_ERROR,     // can't do that
    SFX_COUNT,     // countdown 3, 2, 1
    SFX_GO,        // countdown GO
    SFX_TICK,      // the last seconds of a challenge glyph
    SFX_NEXT,      // a challenge glyph is done
    SFX_DONE,      // the challenge is over
    SFX_COUNT_ALL
};

void sound_init(void);
void sfx(uint8_t id);        // start an effect (nothing when sound is off); a new one on a channel replaces the old
void sound_update(void);     // once per frame
void sound_apply(void);      // after the sound setting changes

#endif
