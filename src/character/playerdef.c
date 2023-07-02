#include "playerdef.h"
#include "../player.h"
#include "../stage.h"
#include "chardef.h"

void Player_Generic_Tick(Character *character)
{
    if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
        (character->animatable.anim != CharAnim_Left &&
         character->animatable.anim != CharAnim_LeftAlt &&
         character->animatable.anim != CharAnim_Down &&
         character->animatable.anim != CharAnim_DownAlt &&
         character->animatable.anim != CharAnim_Up &&
         character->animatable.anim != CharAnim_UpAlt &&
         character->animatable.anim != CharAnim_Right &&
         character->animatable.anim != CharAnim_RightAlt))
        Character_CheckEndSing(character);
     
    if (stage.flag & STAGE_FLAG_JUST_STEP)
    {
        //Perform idle dance
        if (Animatable_Ended(&character->animatable) &&
            (character->animatable.anim != CharAnim_Left &&
             character->animatable.anim != CharAnim_LeftAlt &&
             character->animatable.anim != PlayerAnim_LeftMiss &&
             character->animatable.anim != CharAnim_Down &&
             character->animatable.anim != CharAnim_DownAlt &&
             character->animatable.anim != PlayerAnim_DownMiss &&
             character->animatable.anim != CharAnim_Up &&
             character->animatable.anim != CharAnim_UpAlt &&
             character->animatable.anim != PlayerAnim_UpMiss &&
             character->animatable.anim != CharAnim_Right &&
             character->animatable.anim != CharAnim_RightAlt &&
             character->animatable.anim != PlayerAnim_RightMiss) &&
            (stage.song_step & 0x7) == 0)
             character->set_anim(character, CharAnim_Idle);
            
        //Stage specific animations
        if (stage.note_scroll >= 0)
        {
            switch (stage.stage_id)
            {
                case StageId_1_4: //Tutorial peace
                    if (stage.song_step > 64 && stage.song_step < 192 && (stage.song_step & 0x3F) == 60)
                        character->set_anim(character, PlayerAnim_Peace);
                    break;
                case StageId_1_1: //Bopeebo peace
                    if ((stage.song_step & 0x1F) == 28)
                        character->set_anim(character, PlayerAnim_Peace);
                    break;
                default:
                    break;
            }
        }
    }
    
    
    //Animate and draw
    Animatable_Animate(&character->animatable, (void*)character, Char_SetFrame);
    Character_Draw(character, &character->tex, &character->frames[character->frame]);
}

void Player_Generic_SetAnim(Character *character, uint8_t anim) {
    Char_Generic_SetAnim(character, anim);
}
