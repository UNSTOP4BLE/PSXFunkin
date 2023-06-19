/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "font.h"

#include "timer.h"
#include "stage.h"

#include <string.h>

//Font_Bold
int32_t Font_Bold_GetWidth(struct FontData *this, const char *text)
{
    (void)this;
    return strlen(text) * 13;
}

void Font_Bold_DrawCol(struct FontData *this, const char *text, int32_t x, int32_t y, FontAlign align, uint8_t r, uint8_t g, uint8_t b)
{
    //Offset position based off alignment
    switch (align)
    {
        case FontAlign_Left:
            break;
        case FontAlign_Center:
            x -= Font_Bold_GetWidth(this, text) >> 1;
            break;
        case FontAlign_Right:
            x -= Font_Bold_GetWidth(this, text);
            break;
    }
    
    //Get animation offsets
    uint32_t v0 = 0;
    uint8_t v1 = (Timer_GetAnimfCount() >> 1) & 1;
    
    //Draw string character by character
    uint8_t c;
    while ((c = *text++) != '\0')
    {
        //Draw character
        if ((c -= 'A') <= 'z' - 'A') //Lower-case will show inverted colours
        {
            RECT src = {((c & 0x7) << 5) + ((((v0 >> (c & 0x1F)) & 1) ^ v1) << 4), (c & ~0x7) << 1, 16, 16};
            Gfx_BlitTexCol(&this->tex, &src, x, y, r, g, b);
            v0 ^= 1 << (c & 0x1F);
        }
        x += 13;
    }
}

//Font_Arial
#include "font_arialmap.h"

int32_t Font_Arial_GetWidth(struct FontData *this, const char *text)
{
    (void)this;
    
    //Draw string width character by character
    int32_t width = 0;
    
    uint8_t c;
    while ((c = *text++) != '\0')
    {
        //Shift and validate character
        if ((c -= 0x20) >= 0x60)
            continue;
        
        //Add width
        width += font_arialmap[c].gw;
    }
    
    return width;
}

void Font_Arial_DrawCol(struct FontData *this, const char *text, int32_t x, int32_t y, FontAlign align, uint8_t r, uint8_t g, uint8_t b)
{
    //Offset position based off alignment
    switch (align)
    {
        case FontAlign_Left:
            break;
        case FontAlign_Center:
            x -= Font_Arial_GetWidth(this, text) >> 1;
            break;
        case FontAlign_Right:
            x -= Font_Arial_GetWidth(this, text);
            break;
    }
    
    //Draw string character by character
    uint8_t c;
    while ((c = *text++) != '\0')
    {
        //Shift and validate character
        if ((c -= 0x20) >= 0x60)
            continue;
        
        //Draw character
        RECT src = {font_arialmap[c].ix, font_arialmap[c].iy, font_arialmap[c].iw, font_arialmap[c].ih};
        Gfx_BlitTexCol(&this->tex, &src, x + font_arialmap[c].gx, y + font_arialmap[c].gy, r, g, b);
        
        //Increment X
        x += font_arialmap[c].gw;
    }
}

//CD-R font by bilious
#include "font_cdrmap.h"

int32_t Font_CDR_GetWidth(struct FontData *this, const char *text)
{
    (void)this;
    
    //Draw string width character by character
    int32_t width = 0;
    
    uint8_t c;
    while ((c = *text++) != '\0')
    {
        //Shift and validate character
        if ((c -= 0x20) >= 0x60)
            continue;
        
        //Add width
        width += font_cdrmap[c].charW;
    }
    
    return width;
}

void Font_CDR_DrawCol(struct FontData *this, const char *text, fixed_t x, fixed_t y, FontAlign align, uint8_t r, uint8_t g, uint8_t b)
{
    //Offset position based off alignment
    switch (align)
    {
        case FontAlign_Left:
            break;
        case FontAlign_Center:
            x -= Font_CDR_GetWidth(this, text) >> 1;
            break;
        case FontAlign_Right:
            x -= Font_CDR_GetWidth(this, text);
            break;
    }
    
    //Draw string character by character
    uint8_t c;
    int16_t xhold = x;
    while ((c = *text++) != '\0')
    {
        if (c == '\n')
        {
        x = xhold;
        y += 11;
        }
        //Shift and validate character
        if ((c -= 0x20) >= 0x60)
            continue;
        
        //Draw character
        RECT src = {font_cdrmap[c].charX, font_cdrmap[c].charY, font_cdrmap[c].charW, font_cdrmap[c].charL};
        RECT_FIXED dst = {x, y, src.w << FIXED_SHIFT, src.h << FIXED_SHIFT};

        if (stage.prefs.downscroll)
        dst.y = -dst.y - dst.h;

        Stage_DrawTexCol(&this->tex, &src, &dst, stage.bump, r, g, b);
        
        //Increment X
        x += (font_cdrmap[c].charW - 1) << FIXED_SHIFT;
    }
}

//Common font functions
void Font_Draw(struct FontData *this, const char *text, int32_t x, int32_t y, FontAlign align)
{
    this->draw_col(this, text, x, y, align, 0x80, 0x80, 0x80);
}

//Font functions
void FontData_Load(FontData *this, Font font)
{
    //Load the given font
    switch (font)
    {
        case Font_Bold:
            //Load texture and set functions
            Gfx_LoadTex(&this->tex, IO_Read("\\FONT\\BOLDFONT.TIM;1"), GFX_LOADTEX_FREE);
            this->get_width = Font_Bold_GetWidth;
            this->draw_col = Font_Bold_DrawCol;
            break;
        case Font_Arial:
            //Load texture and set functions
            Gfx_LoadTex(&this->tex, IO_Read("\\FONT\\ARIAL.TIM;1"), GFX_LOADTEX_FREE);
            this->get_width = Font_Arial_GetWidth;
            this->draw_col = Font_Arial_DrawCol;
            break;
        case Font_CDR:
            //Load texture and set functions
            Gfx_LoadTex(&this->tex, IO_Read("\\FONT\\CDR.TIM;1"), GFX_LOADTEX_FREE);
            this->get_width = Font_CDR_GetWidth;
            this->draw_col = Font_CDR_DrawCol;
            break;
    }
    this->draw = Font_Draw;
}
