/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "trans.h"

#include "gfx.h"
#include "timer.h"

#define TRANS_COVER_INC 600
#define TRANS_FADE_LEN 64

//Transition state
typedef enum
{
    TransState_Idle,
    TransState_Out,
    TransState_In,
} TransState;

static struct
{
    //Transition state
    TransState state;
    fixed_t cover;
} transition;

//Transition functions
void Trans_Set(void)
{
    //Initialize transition fading in
    transition.state = TransState_In;
    transition.cover = FIXED_DEC(screen.SCREEN_HEIGHT,1);
}

void Trans_Clear(void)
{
    //Initialize transition idle
    transition.state = TransState_Idle;
    transition.cover = FIXED_DEC(-TRANS_FADE_LEN,1);
}

void Trans_Start(void)
{
    //Start fading out transition
    transition.state = TransState_Out;
}

bool Trans_Tick(void)
{
    //Handle transition by state
    switch (transition.state)
    {
        default:
            return false;
        case TransState_In:
        {
            //Decrement transition coverage
            if ((transition.cover -= (Timer_GetDT() * TRANS_COVER_INC)) <= FIXED_DEC(-TRANS_FADE_LEN,1))
            {
                transition.state = TransState_Idle;
                return false;
            }
            
            //Draw transition cover
            int16_t cover = transition.cover >> FIXED_SHIFT;
            RECT trans_rect = {
                0,
                screen.SCREEN_HEIGHT - cover,
                screen.SCREEN_WIDTH,
                cover
            };
            Gfx_DrawRect(&trans_rect, 0, 0, 0);
            
            RECT trans_fade = {
                0,
                screen.SCREEN_HEIGHT - cover - TRANS_FADE_LEN,
                screen.SCREEN_WIDTH,
                1
            };
            for (int i = 0; i < TRANS_FADE_LEN; i++)
            {
                uint8_t col = 255 * i / TRANS_FADE_LEN;
                Gfx_BlendRect(&trans_fade, col, col, col, 2);
                trans_fade.y++;
            }
            return false;
        }
        case TransState_Out:
        {
            //Increment transition coverage
            bool result;
            if (transition.cover >= FIXED_DEC(screen.SCREEN_HEIGHT,1))
            {
                transition.state = TransState_In;
                result = true;
            }
            else
            {
                result = false;
            }
            transition.cover += (Timer_GetDT() * TRANS_COVER_INC);
            
            //Draw transition cover
            int16_t cover = transition.cover >> FIXED_SHIFT;
            RECT trans_rect = {
                0,
                0,
                screen.SCREEN_WIDTH,
                cover
            };
            Gfx_DrawRect(&trans_rect, 0, 0, 0);
            
            RECT trans_fade = {
                0,
                cover + TRANS_FADE_LEN,
                screen.SCREEN_WIDTH,
                1
            };
            for (int i = 0; i < TRANS_FADE_LEN; i++)
            {
                uint8_t col = 255 * i / TRANS_FADE_LEN;
                trans_fade.y--;
                Gfx_BlendRect(&trans_fade, col, col, col, 2);
            }
            return result;
        }
    }
}

bool Trans_Idle(void)
{
    return transition.state == TransState_Idle;
}
