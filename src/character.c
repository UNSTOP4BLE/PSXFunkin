/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "character.h"

#include <stdlib.h>      
#include "stage.h"

//Character functions
void Character_FromFile(Character *this, const char *path)
{
    int offset = 0;

    if (this->file != NULL)
        free(this->file);
    this->file = IO_Read(path);
    CharacterFileHeader *tmphdr = (CharacterFileHeader *)this->file;
    offset += sizeof(CharacterFileHeader);
    this->animatable.anims = (const Animation *)(offset + this->file); 
    offset += sizeof(Animation) * tmphdr->size_animation;
    this->frames = (const CharFrame *)(offset + this->file);

/*
    printf("struct %d, \n", tmphdr->size_struct);
    printf("frames %d, \n", tmphdr->size_frames);
    printf("animation %d, \n", tmphdr->size_animation);
    
    printf("scripts %d %d %d %d %d %d %d %d %d, \n", tmphdr->sizes_scripts[0], tmphdr->sizes_scripts[1], tmphdr->sizes_scripts[2], tmphdr->sizes_scripts[3], tmphdr->sizes_scripts[4], tmphdr->sizes_scripts[5], tmphdr->sizes_scripts[6], tmphdr->sizes_scripts[7], tmphdr->sizes_scripts[8]);

    printf("spec %d, \n", tmphdr->spec);
    printf("health %d, \n", tmphdr->health_i);
    printf("bar %d, \n", (uint32_t)tmphdr->health_bar);
    printf("focus %d %d %d, \n", tmphdr->focus_x, tmphdr->focus_y, tmphdr->focus_zoom);

        */

    for (int i = 0; i < tmphdr->size_animation; i++) {
        printf("sped %d frames", this->animatable.anims[i].spd);
        for (int i2 = 0; i2 < tmphdr->sizes_scripts[i]; i2 ++)
            printf(" %d ", this->animatable.anims->script[i2]);

        printf("\n");
    }

        for (int i = 0; i < tmphdr->size_frames; ++i)
        {
            printf("tex %d, frames %d %d %d %d offsets %d %d\n", (unsigned int)this->frames[i].tex, this->frames[i].src[0], this->frames[i].src[1], this->frames[i].src[2], this->frames[i].src[3], this->frames[i].off[0], this->frames[i].off[1] ); 
        }   

}

void Character_Free(Character *this)
{
    //Check if NULL
    if (this == NULL)
        return;
    
    //Free character
    this->free(this);
    free(this);
}

void Character_Init(Character *this, fixed_t x, fixed_t y)
{
    //Perform common character initialization
    this->x = x;
    this->y = y;
    
    this->set_anim(this, CharAnim_Idle);
    this->pad_held = 0;
    
    this->sing_end = 0;
}

void Character_DrawParallax(Character *this, Gfx_Tex *tex, const CharFrame *cframe, fixed_t parallax)
{
    //Draw character
    fixed_t x = this->x - FIXED_MUL(stage.camera.x, parallax) - FIXED_DEC(cframe->off[0],1);
    fixed_t y = this->y - FIXED_MUL(stage.camera.y, parallax) - FIXED_DEC(cframe->off[1],1);
    
    RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
    RECT_FIXED dst = {x, y, src.w << FIXED_SHIFT, src.h << FIXED_SHIFT};
    Stage_DrawTex(tex, &src, &dst, stage.camera.bzoom);
}

void Character_DrawParallaxFlipped(Character *this, Gfx_Tex *tex, const CharFrame *cframe, fixed_t parallax)
{
    //Draw character
    fixed_t x = this->x - FIXED_MUL(stage.camera.x, parallax) - FIXED_DEC(-cframe->off[0],1);
    fixed_t y = this->y - FIXED_MUL(stage.camera.y, parallax) - FIXED_DEC(cframe->off[1],1);
    
    RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
    RECT_FIXED dst = {x, y, -src.w << FIXED_SHIFT, src.h << FIXED_SHIFT};
    Stage_DrawTex(tex, &src, &dst, stage.camera.bzoom);
}

void Character_Draw(Character *this, Gfx_Tex *tex, const CharFrame *cframe)
{
    Character_DrawParallax(this, tex, cframe, FIXED_UNIT);
}

void Character_DrawFlipped(Character *this, Gfx_Tex *tex, const CharFrame *cframe)
{
    Character_DrawParallaxFlipped(this, tex, cframe, FIXED_UNIT);
}

void Character_CheckStartSing(Character *this)
{
    //Update sing end if singing animation
    if (this->animatable.anim == CharAnim_Left ||
        this->animatable.anim == CharAnim_LeftAlt ||
        this->animatable.anim == CharAnim_Down ||
        this->animatable.anim == CharAnim_DownAlt ||
        this->animatable.anim == CharAnim_Up ||
        this->animatable.anim == CharAnim_UpAlt ||
        this->animatable.anim == CharAnim_Right ||
        this->animatable.anim == CharAnim_RightAlt ||
        ((this->spec & CHAR_SPEC_MISSANIM) &&
        (this->animatable.anim == PlayerAnim_LeftMiss ||
         this->animatable.anim == PlayerAnim_DownMiss ||
         this->animatable.anim == PlayerAnim_UpMiss ||
         this->animatable.anim == PlayerAnim_RightMiss)))
        this->sing_end = stage.note_scroll + (FIXED_DEC(12,1) << 2); //1 beat
}

void Character_CheckEndSing(Character *this)
{
    if ((this->animatable.anim == CharAnim_Left ||
         this->animatable.anim == CharAnim_LeftAlt ||
         this->animatable.anim == CharAnim_Down ||
         this->animatable.anim == CharAnim_DownAlt ||
         this->animatable.anim == CharAnim_Up ||
         this->animatable.anim == CharAnim_UpAlt ||
         this->animatable.anim == CharAnim_Right ||
         this->animatable.anim == CharAnim_RightAlt ||
        ((this->spec & CHAR_SPEC_MISSANIM) &&
        (this->animatable.anim == PlayerAnim_LeftMiss ||
         this->animatable.anim == PlayerAnim_DownMiss ||
         this->animatable.anim == PlayerAnim_UpMiss ||
         this->animatable.anim == PlayerAnim_RightMiss))) &&
        stage.note_scroll >= this->sing_end)
        this->set_anim(this, CharAnim_Idle);
}

void Character_PerformIdle(Character *this)
{
    Character_CheckEndSing(this);
    if (stage.flag & STAGE_FLAG_JUST_STEP)
    {
        if (Animatable_Ended(&this->animatable) &&
            (this->animatable.anim != CharAnim_Left &&
             this->animatable.anim != CharAnim_LeftAlt &&
             this->animatable.anim != CharAnim_Down &&
             this->animatable.anim != CharAnim_DownAlt &&
             this->animatable.anim != CharAnim_Up &&
             this->animatable.anim != CharAnim_UpAlt &&
             this->animatable.anim != CharAnim_Right &&
             this->animatable.anim != CharAnim_RightAlt) &&
            (stage.song_step & 0x7) == 0)
            this->set_anim(this, CharAnim_Idle);
    }
}
