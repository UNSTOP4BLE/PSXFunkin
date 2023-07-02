#ifndef PSXF_GUARD_GFDEF_H
#define PSXF_GUARD_GFDEF_H

#include "speaker.h"
#include "../character.h"

extern Speaker speaker;

void GirlFriend_Generic_Tick(Character *character);
void GirlFriend_Generic_SetAnim(Character *character, uint8_t anim);

#endif
