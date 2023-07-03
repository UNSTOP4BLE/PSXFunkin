#include "gfdef.h"
#include "../stage.h"

Speaker speaker; //sorry about global vars

//hardcoding cus im stupdi
void GirlFriend_Generic_Tick(Character *character)
{
    if (stage.flag & STAGE_FLAG_JUST_STEP)
    {
        if (stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_3)
        {
            //Perform dance
            if ((stage.song_step % stage.gf_speed) == 0)
            {
                //Switch animation
                if (character->animatable.anim == CharAnim_Left)
                    character->set_anim(character, CharAnim_Right);
                else
                    character->set_anim(character, CharAnim_Left);
            }
        }
        else
        {
            //Stage specific animations
            if (stage.note_scroll >= 0)
            {
                switch (stage.stage_id)
                {
                    case StageId_1_4: //Tutorial cheer
                        if (stage.song_step > 64 && stage.song_step < 192 && (stage.song_step & 0x3F) == 60)
                            character->set_anim(character, CharAnim_UpAlt);
                        break;
                    default:
                        break;
                }
            }
                
            //Perform dance
            if (stage.note_scroll >= character->sing_end && (stage.song_step % stage.gf_speed) == 0)
            {
                //Switch animation
                if (character->animatable.anim == CharAnim_LeftAlt || character->animatable.anim == CharAnim_Right)
                    character->set_anim(character, CharAnim_RightAlt);
                else
                    character->set_anim(character, CharAnim_LeftAlt);
                
                //Bump speakers
                Speaker_Bump(&speaker);
            }
        }
    }

    //Get parallax
    fixed_t parallax;
    if (stage.stage_id >= StageId_1_1 && stage.stage_id <= StageId_1_4)
        parallax = FIXED_DEC(7,10);
    else if (stage.stage_id == StageId_6_3)
        parallax = FIXED_DEC(7,10);
    else if (stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_2)
        parallax = FIXED_DEC(85,100);
    else
        parallax = FIXED_UNIT;
    
    //Animate and draw
    Animatable_Animate(&character->animatable, (void*)character, Char_SetFrame);
    Character_DrawParallax(character, &character->tex, &character->frames[character->frame], parallax);
    
    //Tick speakers
    if (!(stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_3))
    {
        if (stage.stage_id >= StageId_5_1 && stage.stage_id <= StageId_5_3)
            Speaker_Tick(&speaker, character->x - FIXED_DEC(13,1), character->y, parallax);
        else
            Speaker_Tick(&speaker, character->x, character->y, parallax);
    }
}

void GirlFriend_Generic_SetAnim(Character *character, uint8_t anim)
{
    if (stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_3) {
        if (anim != CharAnim_Idle && anim != CharAnim_Left && anim != CharAnim_Right)
            return;
    }
    else {
    if (anim == CharAnim_Left || anim == CharAnim_Down || anim == CharAnim_Up || anim == CharAnim_Right || anim == CharAnim_UpAlt)
        character->sing_end = stage.note_scroll + FIXED_DEC(22,1); //Nearly 2 steps
    }
    Animatable_SetAnim(&character->animatable, anim);
}