/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../gfx.h"

#include <stdlib.h>                  
#include "../main.h"
#include "../mutil.h"
#include "../stage.h"

//Gfx constants
#define OTLEN 8

//Gfx state
uint8_t db;

static uint32_t ot[2][OTLEN];    //Ordering table length
static uint8_t pribuff[2][32768]; //Primitive buffer
static uint8_t *nextpri;          //Next primitive pointer

//Gfx functions
void Gfx_Init(void)
{
    int width = stage.prefs.widescreen ? 512 : 320;

    //Initialize display environment
    SetDefDispEnv(&stage.disp[0], 0, 0, width, 240);
    SetDefDispEnv(&stage.disp[1], 0, 240, width, 240);              
    //Initialize draw environment
    SetDefDrawEnv(&stage.draw[0], 0, 240, width, 240);
    SetDefDrawEnv(&stage.draw[1], 0, 0, width, 240);

    //Set draw background
    stage.draw[0].isbg = 1;
    stage.draw[1].isbg = 1;
    setRGB0(&stage.draw[0], 0, 0, 0);
    setRGB0(&stage.draw[1], 0, 0, 0);

    PutDispEnv(&stage.disp[0]);
    PutDrawEnv(&stage.draw[0]);
    SetDispMask(1);

    //Initialize drawing state
    nextpri = pribuff[0];
    db = 0;

    ClearOTagR((uint32_t *)ot[0], OTLEN);
    ClearOTagR((uint32_t *)ot[1], OTLEN);

    //Load font
    FntLoad(960, 0);
    FntOpen(0, 8, 320, 224, 0, 100);
}

void Gfx_ScreenSetup(void) {
    screen.SCREEN_WIDTH   = stage.prefs.widescreen ? 512 : 320;
    screen.SCREEN_HEIGHT  = 240;
    screen.SCREEN_WIDTH2  = (screen.SCREEN_WIDTH >> 1);
    screen.SCREEN_HEIGHT2 = (screen.SCREEN_HEIGHT >> 1);

    screen.SCREEN_WIDEADD = 0; // ???
    screen.SCREEN_TALLADD = 0; // ???
    screen.SCREEN_WIDEADD2 = (screen.SCREEN_WIDEADD >> 1);
    screen.SCREEN_TALLADD2 = (screen.SCREEN_TALLADD >> 1);

    screen.SCREEN_WIDEOADD = (screen.SCREEN_WIDEADD > 0 ? screen.SCREEN_WIDEADD : 0);
    screen.SCREEN_TALLOADD = (screen.SCREEN_TALLADD > 0 ? screen.SCREEN_TALLADD : 0);
    screen.SCREEN_WIDEOADD2 = (screen.SCREEN_WIDEOADD >> 1);
    screen.SCREEN_TALLOADD2 = (screen.SCREEN_TALLOADD >> 1);

    SetVideoMode(stage.prefs.palmode ? MODE_PAL : MODE_NTSC);

    Gfx_Init();

    //screen borders
    if (stage.prefs.widescreen)
    {
        if (stage.prefs.scr_x > 217)
            stage.prefs.scr_x = 217;

        if (stage.prefs.scr_x < -134)
            stage.prefs.scr_x = -134;
    }
    stage.disp[0].screen.x = stage.prefs.scr_x;
    stage.disp[1].screen.x = stage.prefs.scr_x;
    stage.disp[0].screen.y = stage.prefs.scr_y;
    stage.disp[1].screen.y = stage.prefs.scr_y;
}

void Gfx_DrawText(int x, int y, int z, const char *text) 
{
    nextpri = FntSort(&ot[db][z], nextpri, x, y, text);
}

void Gfx_Quit(void)
{
    SetDispMask(0); // turn screen off
}

void Gfx_Flip(void)
{
    FntFlush(-1);

    //Sync
    //DrawSync(0); // not required, FntFlush already does it
    VSync(0);

    //Flip buffers
    db ^= 1;
    nextpri = pribuff[db];
    ClearOTagR((uint32_t *)ot[db], OTLEN);

    //Apply environments
    PutDispEnv(&stage.disp[db]);
    PutDrawEnv(&stage.draw[db]);

    //Draw screen
    DrawOTag((uint32_t *)&(ot[db ^ 1])[OTLEN - 1]);
}

void Gfx_SetClear(uint8_t r, uint8_t g, uint8_t b)
{
    setRGB0(&stage.draw[0], r, g, b);
    setRGB0(&stage.draw[1], r, g, b);
}

void Gfx_EnableClear(void)
{
    stage.draw[0].isbg = stage.draw[1].isbg = 1;
}

void Gfx_DisableClear(void)
{
    stage.draw[0].isbg = stage.draw[1].isbg = 0;
}

void Gfx_LoadTex(Gfx_Tex *tex, IO_Data data, Gfx_LoadTex_Flag flag)
{
    //Catch NULL data
    if (data == NULL)
    {
        sprintf(error_msg, "[Gfx_LoadTex] data is NULL");
        ErrorLock();
    }
    
    //Read TIM information
    TIM_IMAGE tparam;
    GetTimInfo((uint32_t *)data, &tparam);

    //Upload pixel data to framebuffer
    if (!(flag & GFX_LOADTEX_NOTEX))
    {
        if (tex != NULL)
        {
            tex->tim_prect = *tparam.prect;
            tex->tpage = getTPage(tparam.mode & 0x3, 0, tparam.prect->x, tparam.prect->y);
        }
        LoadImage(tparam.prect, (uint32_t *)tparam.paddr);
    }
    
    //Upload CLUT to framebuffer if present
    if ((tparam.mode & 0x8) && !(flag & GFX_LOADTEX_NOCLUT))
    {
        if (tex != NULL)
        {
            tex->tim_crect = *tparam.crect;
            tex->clut = getClut(tparam.crect->x, tparam.crect->y);
        }
        LoadImage(tparam.crect, (uint32_t *)tparam.caddr);
    }
    
    //Free data
    if (flag & GFX_LOADTEX_FREE)
        free(data);
}

void Gfx_DrawRect(const RECT *rect, uint8_t r, uint8_t g, uint8_t b)
{
    //Add quad
    POLY_F4 *quad = (POLY_F4*)nextpri;
    setPolyF4(quad);
    setXYWH(quad, rect->x, rect->y, rect->w, rect->h);
    setRGB0(quad, r, g, b);
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_F4);
}

void Gfx_BlendRect(const RECT *rect, uint8_t r, uint8_t g, uint8_t b, uint8_t mode)
{
    //Add quad
    POLY_F4 *quad = (POLY_F4*)nextpri;
    setPolyF4(quad);
    setXYWH(quad, rect->x, rect->y, rect->w, rect->h);
    setRGB0(quad, r, g, b);
    setSemiTrans(quad, 1);
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_F4);
    
    //Add tpage change (this controls transparency mode)
    DR_TPAGE *tpage = (DR_TPAGE*)nextpri;
    setDrawTPage(tpage, 0, 1, getTPage(0, mode, 0, 0));
    
    addPrim(ot[db], tpage);
    nextpri += sizeof(DR_TPAGE);
}

void Gfx_BlendTex(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t mode)
{
    //Manipulate rects to comply with GPU restrictions
    RECT csrc, cdst;
    csrc = *src;
    cdst = *dst;
    
    if (dst->w < 0)
        csrc.x--;
    if (dst->h < 0)
        csrc.y--;
    
    if ((csrc.x + csrc.w) >= 0x100)
    {
        csrc.w = 0xFF - csrc.x;
        cdst.w = cdst.w * csrc.w / src->w;
    }
    if ((csrc.y + csrc.h) >= 0x100)
    {
        csrc.h = 0xFF - csrc.y;
        cdst.h = cdst.h * csrc.h / src->h;
    }
    
    //Add quad
    POLY_FT4 *quad = (POLY_FT4*)nextpri;
    setPolyFT4(quad);
    setUVWH(quad, csrc.x, csrc.y, csrc.w, csrc.h);
    setXYWH(quad, cdst.x, cdst.y, cdst.w, cdst.h);
    setRGB0(quad, 0x80, 0x80, 0x80);
    setSemiTrans(quad, mode);
    quad->tpage = tex->tpage;
    quad->clut = tex->clut;
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_FT4);
}

void Gfx_BlitTexCol(Gfx_Tex *tex, const RECT *src, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    //Add sprite
    SPRT *sprt = (SPRT*)nextpri;
    setSprt(sprt);
    setXY0(sprt, x, y);
    setWH(sprt, src->w, src->h);
    setUV0(sprt, src->x, src->y);
    setRGB0(sprt, r, g, b);
    sprt->clut = tex->clut;
    
    addPrim(ot[db], sprt);
    nextpri += sizeof(SPRT);
    
    //Add tpage change (TODO: reduce tpage changes)
    DR_TPAGE *tpage = (DR_TPAGE*)nextpri;
    setDrawTPage(tpage, 0, 1, tex->tpage);
    
    addPrim(ot[db], tpage);
    nextpri += sizeof(DR_TPAGE);
}

void Gfx_BlitTex(Gfx_Tex *tex, const RECT *src, int32_t x, int32_t y)
{
    Gfx_BlitTexCol(tex, src, x, y, 0x80, 0x80, 0x80);
}

void Gfx_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t r, uint8_t g, uint8_t b)
{
    //Manipulate rects to comply with GPU restrictions
    RECT csrc, cdst;
    csrc = *src;
    cdst = *dst;
    
    if (dst->w < 0)
        csrc.x--;
    if (dst->h < 0)
        csrc.y--;
    
    if ((csrc.x + csrc.w) >= 0x100)
    {
        csrc.w = 0xFF - csrc.x;
        cdst.w = cdst.w * csrc.w / src->w;
    }
    if ((csrc.y + csrc.h) >= 0x100)
    {
        csrc.h = 0xFF - csrc.y;
        cdst.h = cdst.h * csrc.h / src->h;
    }
    
    //Add quad
    POLY_FT4 *quad = (POLY_FT4*)nextpri;
    setPolyFT4(quad);
    setUVWH(quad, src->x, csrc.y, csrc.w, csrc.h);
    setXYWH(quad, cdst.x, cdst.y, cdst.w, cdst.h);
    setRGB0(quad, r, g, b);
    quad->tpage = tex->tpage;
    quad->clut = tex->clut;
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_FT4);
}

void Gfx_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT *dst)
{
    Gfx_DrawTexCol(tex, src, dst, 0x80, 0x80, 0x80);
}

void Gfx_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, uint8_t r, uint8_t g, uint8_t b)
{
    //Add quad
    POLY_FT4 *quad = (POLY_FT4*)nextpri;
    setPolyFT4(quad);
    setUVWH(quad, src->x, src->y, src->w, src->h);
    setXY4(quad, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
    setRGB0(quad, r, g, b);
    quad->tpage = tex->tpage;
    quad->clut = tex->clut;
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_FT4);
}

void Gfx_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3)
{
    Gfx_DrawTexArbCol(tex, src, p0, p1, p2, p3, 0x80, 0x80, 0x80);
}

void Gfx_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t angle)
{   
    int16_t sin = MUtil_Sin(angle);
    int16_t cos = MUtil_Cos(angle);
    int pw = dst->w / 2;
    int ph = dst->h / 2;

    //Get tank rotated points
    POINT p0 = {-pw, -ph};
    MUtil_RotatePoint(&p0, sin, cos);
    
    POINT p1 = { pw, -ph};
    MUtil_RotatePoint(&p1, sin, cos);
    
    POINT p2 = {-pw,  ph};
    MUtil_RotatePoint(&p2, sin, cos);
    
    POINT p3 = { pw,  ph};
    MUtil_RotatePoint(&p3, sin, cos);
    
    POINT d0 = {
        dst->x + p0.x,
        dst->y + p0.y
    };
    POINT d1 = {
        dst->x + p1.x,
        dst->y + p1.y
    };
    POINT d2 = {
        dst->x + p2.x,
        dst->y + p2.y
    };
    POINT d3 = {
        dst->x + p3.x,
        dst->y + p3.y
    };
    
    Gfx_DrawTexArb(tex, src, &d0, &d1, &d2, &d3);
}

void Gfx_BlendTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, uint8_t angle, uint8_t mode)
{   
    int16_t sin = MUtil_Sin(angle);
    int16_t cos = MUtil_Cos(angle);
    int pw = dst->w / 2;
    int ph = dst->h / 2;

    //Get tank rotated points
    POINT p0 = {-pw, -ph};
    MUtil_RotatePoint(&p0, sin, cos);
    
    POINT p1 = { pw, -ph};
    MUtil_RotatePoint(&p1, sin, cos);
    
    POINT p2 = {-pw,  ph};
    MUtil_RotatePoint(&p2, sin, cos);
    
    POINT p3 = { pw,  ph};
    MUtil_RotatePoint(&p3, sin, cos);
    
    POINT d0 = {
        dst->x + p0.x,
        dst->y + p0.y
    };
    POINT d1 = {
        dst->x + p1.x,
        dst->y + p1.y
    };
    POINT d2 = {
        dst->x + p2.x,
        dst->y + p2.y
    };
    POINT d3 = {
        dst->x + p3.x,
        dst->y + p3.y
    };
    
    Gfx_BlendTexArb(tex, src, &d0, &d1, &d2, &d3, mode);
}

void Gfx_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, uint8_t mode)
{
    //Add quad
    POLY_FT4 *quad = (POLY_FT4*)nextpri;
    setPolyFT4(quad);
    setUVWH(quad, src->x, src->y, src->w, src->h);
    setXY4(quad, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
    setRGB0(quad, 0x80, 0x80, 0x80);
    setSemiTrans(quad, 1);
    quad->tpage = tex->tpage | getTPage(0, mode, 0, 0);
    quad->clut = tex->clut;
    
    addPrim(ot[db], quad);
    nextpri += sizeof(POLY_FT4);
}
