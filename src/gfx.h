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
extern u8 db;
//Gfx structures
typedef struct
{
	u32 tim_mode;
	RECT tim_prect, tim_crect;
	u16 tpage, clut;
	u8 pxshift;
} Gfx_Tex;

//Gfx functions
void Gfx_Init(void);
void Gfx_ScreenSetup(void);
void Gfx_Quit(void);
void Gfx_Flip(void);
void Gfx_SetClear(u8 r, u8 g, u8 b);
void Gfx_EnableClear(void);
void Gfx_DisableClear(void);

typedef u8 Gfx_LoadTex_Flag;
#define GFX_LOADTEX_FREE   (1 << 0)
#define GFX_LOADTEX_NOTEX  (1 << 1)
#define GFX_LOADTEX_NOCLUT (1 << 2)
void Gfx_LoadTex(Gfx_Tex *tex, IO_Data data, Gfx_LoadTex_Flag flag);

void Gfx_DrawRect(const RECT *rect, u8 r, u8 g, u8 b);
void Gfx_BlendRect(const RECT *rect, u8 r, u8 g, u8 b, u8 mode);
void Gfx_BlitTexCol(Gfx_Tex *tex, const RECT *src, s32 x, s32 y, u8 r, u8 g, u8 b);
void Gfx_BlitTex(Gfx_Tex *tex, const RECT *src, s32 x, s32 y);
void Gfx_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 r, u8 g, u8 b);
void Gfx_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT *dst);
void Gfx_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 angle);
void Gfx_BlendTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 angle, u8 mode);
void Gfx_BlendTex(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 mode);
void Gfx_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, u8 r, u8 g, u8 b);
void Gfx_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3);
void Gfx_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, u8 mode);

#endif
