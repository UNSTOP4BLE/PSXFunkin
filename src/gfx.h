/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_GFX_H
#define PSXF_GUARD_GFX_H

#include "psx.h"
#include "io.h"
#include "fixed.h"

//Gfx constants
typedef struct 
{
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;
    int SCREEN_WIDTH2;
    int SCREEN_HEIGHT2;

    int SCREEN_WIDEADD;
    int SCREEN_TALLADD;
    int SCREEN_WIDEADD2;
    int SCREEN_TALLADD2;

    int SCREEN_WIDEOADD;
    int SCREEN_TALLOADD;
    int SCREEN_WIDEOADD2;
    int SCREEN_TALLOADD2;   
} SCREEN;

extern SCREEN screen;
extern uint8_t db;
//Gfx structures
typedef struct
{
    uint32_t tim_mode;
    RECT tim_prect, tim_crect;
    uint16_t tpage, clut;
    uint8_t pxshift;
} Gfx_Tex;

//Gfx functions
void Gfx_Init(void);
void Gfx_ScreenSetup(void);
void Gfx_DrawText(int x, int y, int z, const char *text);
void Gfx_Quit(void);
void Gfx_Flip(void);
void Gfx_SetClear(uint8_t r, uint8_t g, uint8_t b);
void Gfx_EnableClear(void);
void Gfx_DisableClear(void);

typedef uint8_t Gfx_LoadTex_Flag;
#define GFX_LOADTEX_FREE   (1 << 0)
#define GFX_LOADTEX_NOTEX  (1 << 1)
#define GFX_LOADTEX_NOCLUT (1 << 2)
void Gfx_LoadTex(Gfx_Tex *tex, IO_Data data, Gfx_LoadTex_Flag flag);

void Gfx_DrawRect(const RECT *rect, uint8_t r, uint8_t g, uint8_t b);
void Gfx_BlendRect(const RECT *rect, uint8_t r, uint8_t g, uint8_t b, uint8_t mode);
void Gfx_BlitTexCol(Gfx_Tex *tex, const RECT *src, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);
void Gfx_BlitTex(Gfx_Tex *tex, const RECT *src, int32_t x, int32_t y);
void Gfx_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t r, uint8_t g, uint8_t b);
void Gfx_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT *dst);
void Gfx_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t angle);
void Gfx_BlendTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t angle, uint8_t mode);
void Gfx_BlendTex(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t mode);
void Gfx_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, uint8_t r, uint8_t g, uint8_t b);
void Gfx_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3);
void Gfx_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, uint8_t mode);

#endif
