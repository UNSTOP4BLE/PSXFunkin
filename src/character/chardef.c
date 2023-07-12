#include "chardef.h"
#include "../stage.h"
#include "../mutil.h"
#include "../random.h"

void Char_Generic_Tick(Character *character)
{
    switch (character->spec) {
        case CHAR_SPEC_SPOOKIDLE:
            if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
            {
                Character_CheckEndSing(character);
                
                if (stage.flag & STAGE_FLAG_JUST_STEP)
                {
                    if ((Animatable_Ended(&character->animatable) || character->animatable.anim == CharAnim_LeftAlt || character->animatable.anim == CharAnim_RightAlt) &&
                        (character->animatable.anim != CharAnim_Left &&
                         character->animatable.anim != CharAnim_Down &&
                         character->animatable.anim != CharAnim_Up &&
                         character->animatable.anim != CharAnim_Right) &&
                        (stage.song_step & 0x3) == 0)
                        character->set_anim(character, CharAnim_Idle);
                }
            }
            break;
        default:
            if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
                Character_PerformIdle(character);
            break;

    }   
    
    
    //Animate and draw
    Animatable_Animate(&character->animatable, (void*)character, Char_SetFrame);
    Character_Draw(character, &character->tex, &character->frames[character->frame]);
}

void Char_Generic_SetAnim(Character *character, uint8_t anim)
{
    switch (character->spec) {
        case CHAR_SPEC_SPOOKIDLE:
            if (anim == CharAnim_Idle)
            {
                if (character->animatable.anim == CharAnim_LeftAlt)
                    anim = CharAnim_RightAlt;
                else
                    anim = CharAnim_LeftAlt;
                character->sing_end = FIXED_DEC(0x7FFF,1);
            }
            else
            {
                Character_CheckStartSing(character);
            }
            Animatable_SetAnim(&character->animatable, anim);
            break;
        default:
            Animatable_SetAnim(&character->animatable, anim);
            Character_CheckStartSing(character);
            break;

    }   
}

//ghost
void Char_Ghost_SetFrame(void *user, uint8_t frame)
{
    Character *this = (Character*)user;
    
    //Check if this is a new frame
    if (frame != this->frame)
    {
        //Check if new art shall be loaded
        const CharFrame *cframe = &this->frames[this->frame = frame];
        if (cframe->tex != this->tex_id)
            Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
    }
    
    //Process distortion
    this->distort_ang += this->distort_spd;
    this->distort_pow += (FIXED_UNIT - this->distort_pow) >> 2;
    this->distort_spd += (FIXED_UNIT - this->distort_spd) >> 1;
    
    this->ghost_x += (FIXED_UNIT - this->ghost_x) >> 2;
    this->ghost_y += (FIXED_UNIT - this->ghost_y) >> 2;
}
                                                                                                                                                                              
void Char_Ghost_SetAnim(Character *character, uint8_t anim)                                                                                                                                                
{                                                                                                                                                                                                          
    Character *this = (Character*)character;                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                           
    Animatable_SetAnim(&character->animatable, anim);                                                                                                                                                       
    Character_CheckStartSing(character);                                                                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                       
    this->distort_pow += FIXED_DEC(14,10);                                                                                                                                                                  
    this->distort_spd += FIXED_DEC(145,10);                                                                                                                                                                 
                                                                                                                                                                                                            
    switch (anim)                                                                                                                                                                                           
    {                                                                                                                                                                                               
        case CharAnim_Idle:                                                                                                                                                                                 
            this->ghost_x += RandomRange(FIXED_DEC(-9,1), FIXED_DEC(9,1));                                                                                                                                  
            this->ghost_y += RandomRange(FIXED_DEC(-13,1), FIXED_DEC(13,1));                                                                                                                                
            break;                                                                                                                                                                                          
        case CharAnim_Left:                                                                                                                                                                                 
            this->ghost_x += RandomRange(FIXED_DEC(2,1), FIXED_DEC(16,1));                                                                                                                                  
            break;                                                                                                                                                                                          
        case CharAnim_Down:                                                                                                                                                                                 
            this->ghost_y -= RandomRange(FIXED_DEC(2,1), FIXED_DEC(16,1));                                                                                                                                  
            break;                                                                                                                                                                                          
        case CharAnim_Up:                                                                                                                                                                                   
            this->ghost_y += RandomRange(FIXED_DEC(2,1), FIXED_DEC(16,1));                                                                                                                                  
            break;                                                                                                                                                                                          
        case CharAnim_Right:                                                                                                                                                                                
            this->ghost_x -= RandomRange(FIXED_DEC(2,1), FIXED_DEC(16,1));                                                                                                                                  
            break;                                                                                                                                                                                          
    }                                                                                                                                                                                                       
}                                                                                                                                                                                                         

static void Char_Ghost_Draw(Character *this, fixed_t x, fixed_t y, fixed_t phase, bool mode)
{
    //Get character state stuff
    const CharFrame *cframe = &this->frames[this->frame];
    
    //Get offset coordinates
    fixed_t ox = x - stage.camera.x - FIXED_DEC(cframe->off[0],1);
    fixed_t oy = y - stage.camera.y - FIXED_DEC(cframe->off[1],1);
    
    //Get distorted points
    fixed_t pang = FIXED_MUL(this->distort_ang, phase) >> FIXED_SHIFT;
    uint8_t a0 = pang *  9 / 10;
    uint8_t a1 = pang * 11 / 10;
    uint8_t a2 = pang * 10 / 10;
    uint8_t a3 = pang * 12 / 10;
    
    fixed_t pow = this->distort_pow * 2;
    
    POINT_FIXED d0 = {ox + ((MUtil_Cos(a0) * pow) >> 8),                               oy + ((MUtil_Sin(a0) * pow) >> 8)};
    POINT_FIXED d1 = {ox + ((MUtil_Cos(a1) * pow) >> 8) + FIXED_DEC(cframe->src[2],1), oy + ((MUtil_Sin(a1) * pow) >> 8)};
    POINT_FIXED d2 = {ox + ((MUtil_Cos(a2) * pow) >> 8),                               oy + ((MUtil_Sin(a2) * pow) >> 8) + FIXED_DEC(cframe->src[3],1)};
    POINT_FIXED d3 = {ox + ((MUtil_Cos(a3) * pow) >> 8) + FIXED_DEC(cframe->src[2],1), oy + ((MUtil_Sin(a3) * pow) >> 8) + FIXED_DEC(cframe->src[3],1)};
    
    RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
    if (mode)
        Stage_BlendTexArb(&this->tex, &src, &d0, &d1, &d2, &d3, stage.camera.bzoom, 0);
    else
        Stage_DrawTexArb(&this->tex, &src, &d0, &d1, &d2, &d3, stage.camera.bzoom);
}

void Char_Ghost_Tick(Character *character)
{
    Character *this = character;
    
    //Perform idle dance
    if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
        Character_PerformIdle(character);
    
    //Animate
    Animatable_Animate(&character->animatable, (void*)this, Char_Ghost_SetFrame);
    
    //Draw body and ghost
    Char_Ghost_Draw(this, character->x, character->y, FIXED_DEC(25,10), false);
    Char_Ghost_Draw(this, character->x + this->ghost_x, character->y + this->ghost_y, FIXED_DEC(15,10), true);
}
