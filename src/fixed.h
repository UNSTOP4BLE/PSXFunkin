/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_FIXED_H
#define PSXF_GUARD_FIXED_H

#include "psx.h"

//Fixed types and constants
typedef int32_t fixed_t;
typedef uint32_t fixedu_t;

typedef struct
{
    fixed_t x, y, w, h;
} RECT_FIXED;

typedef struct
{
    fixed_t x, y;
} POINT_FIXED;

#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)
#define FIXED_LAND  (FIXED_UNIT - 1)
#define FIXED_UAND  (~FIXED_LAND)

#define FIXED_DEC(d, f) ((fixed_t)(((int64_t)(d) * FIXED_UNIT) / (f)))

#define FIXED_MUL(x, y) ((fixed_t)(((int64_t)(x) * (y)) >> FIXED_SHIFT))
#define FIXED_DIV(x, y) ((fixed_t)(((int32_t)(x) * FIXED_UNIT) / (y)))

#define FIXEDU_DEC(d, f) (((fixedu_t)(d) * FIXED_UNIT) / (f))

#define FIXEDU_MUL(x, y) ((fixedu_t)(((uint64_t)(x) * (y)) >> FIXED_SHIFT))
#define FIXEDU_DIV(x, y) ((fixedu_t)(((uint32_t)(x) * FIXED_UNIT) / (y)))

#endif
