/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_MUTIL_H
#define PSXF_GUARD_MUTIL_H

#include "psx.h"

#include "fixed.h"

//Math utility functions
#define lerp(position, target, speed) position + FIXED_MUL((target - position),speed)

int16_t MUtil_Sin(uint8_t x);
int16_t MUtil_Cos(uint8_t x);
void MUtil_RotatePoint(POINT *p, int16_t s, int16_t c);
fixed_t MUtil_Pull(fixed_t a, fixed_t b, fixed_t t);

#endif
