/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "random.h"

//Random functions
static uint32_t rand_seed;

void RandomSeed(uint32_t seed)
{
    rand_seed = seed;
}

uint32_t RandomGetSeed(void)
{
    return rand_seed;
}

uint8_t Random8(void)
{
    return Random16() >> 4;
}

uint16_t Random16(void)
{
    rand_seed = rand_seed * 214013L + 2531011L;
    return rand_seed >> 16;
}

uint32_t Random32(void)
{
    return ((uint32_t)Random16() << 16) | Random16();
}

int32_t RandomRange(int32_t x, int32_t y)
{
    return x + Random16() % ((int32_t)y - (int32_t)x + 1);
}
