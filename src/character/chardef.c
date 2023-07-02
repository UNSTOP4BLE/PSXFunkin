#include "chardef.h"
#include "../stage.h"

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