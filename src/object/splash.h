/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_SPLASH_H
#define PSXF_GUARD_SPLASH_H

#include "../object.h"
#include "../stage.h"

//Splash object structure
typedef struct
{
    //Object base structure
    Object obj;
    
    //Splash state
    uint8_t colour;
    
    fixed_t x, y, xsp, ysp, size, sin, cos;
} Obj_Splash;

//Splash object functions
Obj_Splash *Obj_Splash_New(fixed_t x, fixed_t y, uint8_t colour);

#endif
