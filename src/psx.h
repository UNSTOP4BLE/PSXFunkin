/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_PSX_H
#define PSXF_GUARD_PSX_H

extern int my_argc;
extern char **my_argv;

//Headers
#include <sys/types.h>
#include <stdio.h>

#include <psxetc.h>
#include <psxgpu.h>
#include <psxspu.h>
#include <psxcd.h>
#include <psxapi.h>

#include <stddef.h>
#include <string.h>

//Fixed size types
#include <stdint.h>
#include <stdbool.h>

//Misc. functions
#define MsgPrint FntPrint

//Point type
typedef struct
{
    int16_t x, y;
} POINT;

//Common macros
#define sizeof_member(type, member) sizeof(((type *)0)->member)

#define COUNT_OF(x) (sizeof(x) / sizeof(0[x]))
#define COUNT_OF_MEMBER(type, member) (sizeof_member(type, member) / sizeof_member(type, member[0]))

#define TYPE_SIGNMIN(utype, stype) ((stype)((((utype)-1) >> 1) + 1))

//PSX functions
void PSX_Init(void);
void PSX_Quit(void);
bool PSX_Running(void);

#endif
