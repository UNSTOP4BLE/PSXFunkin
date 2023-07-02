/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_ANIMATION_H
#define PSXF_GUARD_ANIMATION_H

#include "psx.h"

#include "fixed.h"

//Animation structures
#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

typedef struct __attribute__((packed)) 
{
    //Animation data and script
    uint8_t spd;
    uint8_t script[255];
} Animation;

typedef struct
{
    //Animation state
    const Animation *anims;
    const uint8_t *anim_p;
    uint8_t anim;
    fixed_t anim_time, anim_spd;
    bool ended;
} Animatable;

//Animation functions
void Animatable_Init(Animatable *this, const Animation *anims);
void Animatable_SetAnim(Animatable *this, uint8_t anim);
void Animatable_Animate(Animatable *this, void *user, void (*set_frame)(void*, uint8_t));
bool Animatable_Ended(Animatable *this);

#endif
