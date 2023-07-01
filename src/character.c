/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "character.h"

#include "main.h"
#include "archive.h"
#include <stdlib.h>      
#include "stage.h"

typedef struct TimPath{
    char path[32];
} TimPath;

//Character functions
void Char_Generic_SetFrame(void *user, uint8_t frame)
{
//    printf("generic setframe \n");
    Character *this = (Character*)user;

    //Check if this is a new frame
    if (frame != this->frame)
    {
        //Check if new art shall be loaded
        const CharFrame *cframe = &this->frames[this->frame = frame];
        if (cframe->tex != this->tex_id)
            Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
    }
}

void Char_Generic_Tick(Character *character)
{
  //  printf("generic tick \n");
    //Perform idle dance
    if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
        Character_PerformIdle(character);
    
    //Animate and draw
    Animatable_Animate(&character->animatable, (void*)character, Char_Generic_SetFrame);
    Character_Draw(character, &character->tex, &character->frames[character->frame]);
}

void Char_Generic_SetAnim(Character *character, uint8_t anim)
{
  //  printf("generic setanim \n");
    //Set animation
    Animatable_SetAnim(&character->animatable, anim);
    Character_CheckStartSing(character);
}

void Char_Generic_Free(Character *character)
{
    //Free art
    free(character->arc_main);
}

Character *Character_FromFile(Character *this, const char *path, fixed_t x, fixed_t y)
{
    uint32_t offset = 0;

    if (this != NULL)
        free(this);
    this = malloc(sizeof(Character));

    //load the actual character
    if (this == NULL)
    {
        sprintf(error_msg, "[%s] Failed to allocate object", path);
        ErrorLock();
        return NULL;
    }

    //Initialize character
    this->tick = Char_Generic_Tick;
    this->set_anim = Char_Generic_SetAnim;
    this->free = Char_Generic_Free;
    
    this->file = IO_Read(path);
    CharacterFileHeader *tmphdr = (CharacterFileHeader *)this->file;    
    offset += sizeof(CharacterFileHeader) / 4;
    Animatable_Init(&this->animatable, (const Animation *)(offset + this->file));
    offset += (sizeof(Animation) * tmphdr->size_animation) / 4;
    this->frames = (const CharFrame *)(offset + this->file);
    offset += (tmphdr->size_frames * sizeof(CharFrame)) / 4;
    TimPath *tex_paths = (TimPath *)(offset + this->file);

    Character_Init(this, x, y);
    
    //Set character information
    this->spec = tmphdr->spec;
    
    this->health_i = tmphdr->health_i;

    //health bar color
    this->health_bar = tmphdr->health_bar;
    
    this->focus_x = tmphdr->focus_x;
    this->focus_y = tmphdr->focus_y;
    this->focus_zoom = tmphdr->focus_zoom;
    
    //Load art //todo
    this->arc_main = IO_Read(tmphdr->archive_path);

    IO_Data *arc_ptr = this->arc_ptr;
    for (int i = 0; i < tmphdr->size_textures; i++) {
        printf("%s\n", tex_paths[i].path);
        *arc_ptr++ = Archive_Find(this->arc_main, tex_paths[i].path);
    }
    //Initialize render state
    this->tex_id = this->frame = 0xFF;
    /*
    printf("struct %d, \n", tmphdr->size_struct);
    printf("frames %d, \n", tmphdr->size_frames);
    printf("animation %d, \n", tmphdr->size_animation);
    
    printf("scripts %d %d %d %d %d %d %d %d %d, \n", tmphdr->sizes_scripts[0], tmphdr->sizes_scripts[1], tmphdr->sizes_scripts[2], tmphdr->sizes_scripts[3], tmphdr->sizes_scripts[4], tmphdr->sizes_scripts[5], tmphdr->sizes_scripts[6], tmphdr->sizes_scripts[7], tmphdr->sizes_scripts[8]);

    printf("spec %d, \n", this->spec);
    printf("health %d, \n", this->health_i);
    printf("bar %d, \n", (uint32_t)this->health_bar);
    printf("focus %d %d %d, \n", this->focus_x, this->focus_y, this->focus_zoom);

    for (int i = 0; i < tmphdr->size_animation; i++) {
        printf("sped %d frames", this->animatable.anims[i].spd);
        for (int i2 = 0; i2 < tmphdr->sizes_scripts[i]; i2 ++)
            printf(" %d ", this->animatable.anims[i].script[i2]);

        printf("\n");
    }

    for (int i = 0; i < tmphdr->size_frames; ++i) {
        printf("tex %d, frames %d %d %d %d offsets %d %d\n", (unsigned int)this->frames[i].tex, this->frames[i].src[0], this->frames[i].src[1], this->frames[i].src[2], this->frames[i].src[3], this->frames[i].off[0], this->frames[i].off[1] ); 
    }   
    */
    return this;
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
