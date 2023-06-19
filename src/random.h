/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_RANDOM_H
#define PSXF_GUARD_RANDOM_H

#include "psx.h"

//Random functions
void RandomSeed(uint32_t seed);
uint32_t RandomGetSeed();
uint8_t Random8();
uint16_t Random16();
uint32_t Random32();
int32_t RandomRange(int32_t x, int32_t y);

#endif
