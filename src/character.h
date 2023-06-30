/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_CHARACTER_H
#define PSXF_GUARD_CHARACTER_H

#include "io.h"
#include "gfx.h"

#include "fixed.h"
#include "animation.h"

//Character specs
typedef uint8_t CharSpec;
#define CHAR_SPEC_MISSANIM (1 << 0) //Has miss animations

typedef enum
{
    CharAnim_Idle,
    CharAnim_Left,  CharAnim_LeftAlt,
    CharAnim_Down,  CharAnim_DownAlt,
    CharAnim_Up,    CharAnim_UpAlt,
    CharAnim_Right, CharAnim_RightAlt,
    
    CharAnim_Max //Max standard/shared animation
} CharAnim;

//Character structures
typedef struct CharFrame
{
    uint8_t tex;
    uint16_t src[4];
    int16_t off[2];
} CharFrame;

typedef struct Character
{
    //Character functions
    void (*tick)(struct Character*);
    void (*set_anim)(struct Character*, uint8_t);
    void (*free)(struct Character*);
    
    //Position
    fixed_t x, y;
    
    //Character information
    CharSpec spec;
    uint8_t health_i; //hud1.tim
    uint32_t health_bar; //hud1.tim
    fixed_t focus_x, focus_y, focus_zoom;
    
    //Animation state
    const CharFrame *frames;
    Animatable animatable;
    fixed_t sing_end;
    uint16_t pad_held;
    uint32_t *file;

    //Render data and state
    IO_Data arc_main;
    IO_Data *arc_ptr;
    
    Gfx_Tex tex;
    uint8_t frame, tex_id;
} Character;

typedef struct CharacterFileHeader
{
    int32_t size_struct;
    int32_t size_frames;
    int32_t size_animation;
    int32_t sizes_scripts[9]; // size of charAnim vector

    //Character information
    uint16_t spec;
    uint16_t health_i; //hud1.tim
    uint32_t health_bar; //hud1.tim
    fixed_t focus_x, focus_y, focus_zoom;
} CharacterFileHeader;

//Character functions
void Character_FromFile(Character *this, const char *path, fixed_t x, fixed_t y);
void Character_Free(Character *this);
void Character_Init(Character *this, fixed_t x, fixed_t y);
void Character_DrawParallax(Character *this, Gfx_Tex *tex, const CharFrame *cframe, fixed_t parallax);
void Character_DrawParallaxFlipped(Character *this, Gfx_Tex *tex, const CharFrame *cframe, fixed_t parallax);
void Character_Draw(Character *this, Gfx_Tex *tex, const CharFrame *cframe);
void Character_DrawFlipped(Character *this, Gfx_Tex *tex, const CharFrame *cframe);

void Character_CheckStartSing(Character *this);
void Character_CheckEndSing(Character *this);
void Character_PerformIdle(Character *this);

#endif
