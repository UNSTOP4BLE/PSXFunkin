#ifndef PSXF_GUARD_CHARDEF_H
#define PSXF_GUARD_CHARDEF_H

#include "../character.h"

void Char_Generic_Tick(Character *character);
void Char_Generic_SetAnim(Character *character, uint8_t anim);


void Char_Ghost_Tick(Character *character);                                                                                                                
void Char_Ghost_SetAnim(Character *character, uint8_t anim);

#endif
