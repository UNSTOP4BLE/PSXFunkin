/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_FONT_H
#define PSXF_GUARD_FONT_H

#include "gfx.h"

//Font types
typedef enum
{
    Font_Bold,
    Font_Arial,
    Font_CDR,
} Font;

typedef enum
{
    FontAlign_Left,
    FontAlign_Center,
    FontAlign_Right,
} FontAlign;

typedef struct FontData
{
    //Font functions and data
    int32_t (*get_width)(struct FontData *this, const char *text);
    void (*draw_col)(struct FontData *this, const char *text, int32_t x, int32_t y, FontAlign align, uint8_t r, uint8_t g, uint8_t b);
    void (*draw)(struct FontData *this, const char *text, int32_t x, int32_t y, FontAlign align);
    
    Gfx_Tex tex;
} FontData;

//Font functions
void FontData_Load(FontData *this, Font font);

#endif
