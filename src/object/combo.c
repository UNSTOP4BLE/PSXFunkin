/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "combo.h"

#include <stdlib.h>  
#include "../timer.h"
#include "../random.h"

//Combo object functions
bool Obj_Combo_Tick(Object *obj)
{
    Obj_Combo *this = (Obj_Combo*)obj;
    
    //Tick hit type
    if (this->hit_type != 0xFF && this->ht < (FIXED_DEC(16,1) / 60))
    {
        //Get hit src and dst
        uint8_t clipp = 16;
        if (this->ht > 0)
            clipp = 16 - ((this->ht * 60) >> FIXED_SHIFT);
        
        RECT hit_src = {
            0,
            128 + (this->hit_type << 5),
            80,
            clipp << 1
        };
        RECT_FIXED hit_dst = {
            this->x - FIXED_DEC(8,1),
            this->hy - FIXED_DEC(16,1),
            FIXED_DEC(80,1),
            (FIXED_DEC(32,1) * clipp) >> 4
        };

        hit_dst.y += stage.noteshakey;
        hit_dst.x += stage.noteshakex;

        Stage_DrawTex(&stage.tex_hud0, &hit_src, &hit_dst, stage.bump);
        
        //Apply gravity
        this->hy += FIXED_MUL(this->hv, Timer_GetDT());
        this->hv += FIXED_MUL(FIXED_DEC(5,100) * 60 * 60, Timer_GetDT());
    }
    
    //Increment hit type timer
    this->ht += Timer_GetDT();
    
    //Tick combo
    if (this->num[4] != 0xFF && this->ct < (FIXED_DEC(16,1) / 60))
    {
        //Get hit src and dst
        uint8_t clipp = 16;
        if (this->ct > 0)
            clipp = 16 - ((this->ct * 60) >> FIXED_SHIFT);
        
        RECT combo_src = {
            80,
            128,
            80,
            clipp << 1
        };
        RECT_FIXED combo_dst = {
            this->x + FIXED_DEC(48,1),
            this->cy - FIXED_DEC(16,1),
            FIXED_DEC(60,1),
            (FIXED_DEC(24,1) * clipp) >> 4
        };
        
        combo_dst.y += stage.noteshakey;
        combo_dst.x += stage.noteshakex;
        
        Stage_DrawTex(&stage.tex_hud0, &combo_src, &combo_dst, stage.bump);
        
        //Apply gravity
        this->cy += FIXED_MUL(this->cv, Timer_GetDT());
        this->cv += FIXED_MUL(FIXED_DEC(3,100) * 60 * 60, Timer_GetDT());
    }
    
    //Increment combo timer
    this->ct += Timer_GetDT();
    
    //Tick numbers
    if (this->numt < (FIXED_DEC(16,1) / 60))
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            uint8_t num = this->num[i];
            if (num == 0xFF)
                continue;
            
            //Get number src and dst
            uint8_t clipp = 16;
            if (this->numt > 0)
                clipp = 16 - ((this->numt * 60) >> FIXED_SHIFT);
            
            RECT num_src = {
                80  + ((num % 5) << 5),
                160 + ((num / 5) << 5),
                32,
                clipp << 1
            };
            RECT_FIXED num_dst = {
                this->x - FIXED_DEC(32,1) + (i * FIXED_DEC(16,1)) - FIXED_DEC(12,1),
                this->numy[i] - FIXED_DEC(12,1),
                FIXED_DEC(24,1),
                (FIXED_DEC(24,1) * clipp) >> 4
            };
            
            num_dst.y += stage.noteshakey;
            num_dst.x += stage.noteshakex;
            
            Stage_DrawTex(&stage.tex_hud0, &num_src, &num_dst, stage.bump);
            
            //Apply gravity
            this->numy[i] += FIXED_MUL(this->numv[i], Timer_GetDT());
            this->numv[i] += FIXED_MUL(FIXED_DEC(3,100) * 60 * 60, Timer_GetDT());
        }
    }
    
    //Increment number timer
    this->numt += Timer_GetDT();
    
    return (this->numt >= FIXED_DEC(16,60)) && (this->ht >= FIXED_DEC(16,60)) && (this->ct >= FIXED_DEC(16,60));
}

bool Obj_Combo_Tick_Weeb(Object *obj)
{
    Obj_Combo *this = (Obj_Combo*)obj;
    
    //Tick hit type
    if (this->hit_type != 0xFF && this->ht < (FIXED_DEC(16,1) / 60))
    {
        //Get hit src and dst
        uint8_t clipp = 16;
        if (this->ht > 0)
            clipp = 16 - ((this->ht * 60) >> FIXED_SHIFT);
        
        RECT hit_src = {
            1,
            129 + (this->hit_type * 24),
            70,
            (22 * clipp) >> 4
        };
        RECT_FIXED hit_dst = {
            this->x - FIXED_DEC(8 + 9 + 50,1),
            this->hy - FIXED_DEC(16 - 20,1),
            FIXED_DEC(140 - 10,1),
            (FIXED_DEC(44 - 10,1) * clipp) >> 4
        };

        hit_dst.y += stage.noteshakey;
        hit_dst.x += stage.noteshakex;

        Stage_DrawTex(&stage.tex_hud0, &hit_src, &hit_dst, stage.bump);
        
        //Apply gravity
        this->hy += FIXED_MUL(this->hv, Timer_GetDT()) >> 1;
        this->hv += FIXED_MUL(FIXED_DEC(5,100) * 60 * 60, Timer_GetDT());
    }
    
    //Increment hit type timer
    this->ht += Timer_GetDT();
    
    //Tick combo
    if (this->num[4] != 0xFF && this->ct < (FIXED_DEC(16,1) / 60))
    {
        //Get hit src and dst
        uint8_t clipp = 16;
        if (this->ct > 0)
            clipp = 16 - ((this->ct * 60) >> FIXED_SHIFT);
        
        RECT combo_src = {
            73,
            129,
            46,
            (22 * clipp) >> 4
        };
        RECT_FIXED combo_dst = {
            this->x + FIXED_DEC(48 - 10 - 50,1),
            this->cy - FIXED_DEC(16 + 7 - 20,1),
            FIXED_DEC(92 - 10,1),
            (FIXED_DEC(44 - 10,1) * clipp) >> 4
        };

        combo_dst.y += stage.noteshakey;
        combo_dst.x += stage.noteshakex;

        Stage_DrawTex(&stage.tex_hud0, &combo_src, &combo_dst, stage.bump);
        
        //Apply gravity
        this->cy += FIXED_MUL(this->cv, Timer_GetDT()) >> 1;
        this->cv += FIXED_MUL(FIXED_DEC(3,100) * 60 * 60, Timer_GetDT());
    }
    
    //Increment combo timer
    this->ct += Timer_GetDT();
    
    //Tick numbers
    if (this->numt < (FIXED_DEC(16,1) / 60))
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            uint8_t num = this->num[i];
            if (num == 0xFF)
                continue;
            
            //Get number src and dst
            uint8_t clipp = 16;
            if (this->numt > 0)
                clipp = 16 - ((this->numt * 60) >> FIXED_SHIFT);
            
            RECT num_src = {
                72  + (num * 12),
                152,
                11,
                (12 * clipp) >> 4
            };
            RECT_FIXED num_dst = {
                this->x - FIXED_DEC(32 + 50,1) + (i * FIXED_DEC(16,1)) - FIXED_DEC(12,1),
                this->numy[i] - FIXED_DEC(12 - 20,1),
                FIXED_DEC(22,1),
                (FIXED_DEC(24,1) * clipp) >> 4
            };

            num_dst.y += stage.noteshakey;
            num_dst.x += stage.noteshakex;

            Stage_DrawTex(&stage.tex_hud0, &num_src, &num_dst, stage.bump);
            
            //Apply gravity
            this->numy[i] += FIXED_MUL(this->numv[i], Timer_GetDT()) >> 1;
            this->numv[i] += FIXED_MUL(FIXED_DEC(3,100) * 60 * 60, Timer_GetDT());
        }
    }
    
    //Increment number timer
    this->numt += Timer_GetDT();
    
    return (this->numt >= FIXED_DEC(16,60)) && (this->ht >= FIXED_DEC(16,60)) && (this->ct >= FIXED_DEC(16,60));
}

void Obj_Combo_Free(Object *obj)
{
    (void)obj;
}

Obj_Combo *Obj_Combo_New(fixed_t x, fixed_t y, uint8_t hit_type, uint16_t combo)
{
    (void)x;
    
    //Allocate new object
    Obj_Combo *this = (Obj_Combo*)malloc(sizeof(Obj_Combo));
    if (this == NULL)
        return NULL;
    
    //Set object functions and position
    if (stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_3)
    {
        //Pixel combo
        this->obj.tick = Obj_Combo_Tick_Weeb;
        if ((x >= 0) ^ (stage.mode < StageMode_2P))
            this->x = FIXED_DEC(-70,1) - FIXED_DEC(screen.SCREEN_WIDEADD,8);
        else
            this->x = FIXED_DEC(0,1) + FIXED_DEC(screen.SCREEN_WIDEADD,8);
        y = FIXED_DEC(44,1);
    }
    else
    {
        //Regular combo
        this->obj.tick = Obj_Combo_Tick;
        if ((x >= 0) ^ (stage.mode < StageMode_2P))
            this->x = FIXED_DEC(-112,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
        else
            this->x = FIXED_DEC(30,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        y = FIXED_DEC(73,1);
    }
    this->obj.free = Obj_Combo_Free;
    
    //Setup hit type
    if ((this->hit_type = hit_type) != 0xFF)
    {
        this->hy = y - FIXED_DEC(38,1);
        this->hv = -(FIXED_DEC(8,10) + RandomRange(0, FIXED_DEC(3,10))) * 60;
    }
    
    //Setup numbers
    if (combo != 0xFFFF)
    {
        //Initial numbers
        this->num[0] = this->num[1] = 0xFF;
        this->num[2] = this->num[3] = this->num[4] = 0; //MEH
        
        //Write numbers
        static const uint16_t dig[5] = {10000, 1000, 100, 10, 1};
        bool hit = false;
        
        const uint16_t *digp = dig;
        for (uint8_t i = 0; i < 5; i++, digp++)
        {
            //Get digit value
            uint8_t v = 0;
            while (combo >= *digp)
            {
                combo -= *digp;
                v++;
            }
            
            //Write digit value
            if (v || hit)
            {
                hit = true;
                this->num[i] = v;
            }
        }
        
        //Initialize number positions
        for (uint8_t i = 0; i < 5; i++)
        {
            if (this->num[i] == 0xFF)
                continue;
            this->numy[i] = y;
            this->numv[i] = -(FIXED_DEC(7,10) + RandomRange(0, FIXED_DEC(18,100))) * 60;
        }
        
        //Setup combo
        this->cy = y;
        this->cv = -(FIXED_DEC(7,10) + RandomRange(0, FIXED_DEC(16,100))) * 60;
    }
    else
    {
        //Write null numbers
        this->num[0] = this->num[1] = this->num[2] = this->num[3] = this->num[4] = 0xFF;
    }
    
    //Initialize timers
    this->ht = FIXED_DEC(-30,60);
    this->ct = FIXED_DEC(-53,60);
    this->numt = FIXED_DEC(-56,60);
    
    return this;
}
