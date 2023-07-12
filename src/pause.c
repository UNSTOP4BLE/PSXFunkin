/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"
#include "trans.h"
#include "audio.h"

static uint8_t pause_select = 0;

void PausedState()
{
    static const char *stage_options[] = {
        "RESUME",
        "RESTART SONG",
        "OPTIONS",
        "EXIT TO MENU"
    };

    //Select option if cross or start is pressed
    if (pad_state.press & (PAD_CROSS | PAD_START))
    {
        switch (pause_select)
        {
            case 0: //Resume
                Audio_StartStream(true);
                stage.paused = false;
                pause_select = 0;
                break;
            case 1: //Retry
                stage.trans = StageTrans_Reload;
                Trans_Start();
                pause_select = 0;
                break;
            case 2: //Settings
                stage.pause_scroll = -1;
                stage.pause_state = 1;
                break;
            case 3: //Quit
                stage.trans = StageTrans_Menu;
                Trans_Start();
                pause_select = 0;
                break;
        }
    }

    //Change option
    if (pad_state.press & PAD_UP)
    {
        if (pause_select > 0)
            pause_select--;
        else
            pause_select = COUNT_OF(stage_options) - 1;
    }
    if (pad_state.press & PAD_DOWN)
    {
        if (pause_select < COUNT_OF(stage_options) - 1)
            pause_select++;
        else
            pause_select = 0;
    }
                
    //draw options
    if (stage.pause_scroll == -1)
        stage.pause_scroll = COUNT_OF(stage_options) * FIXED_DEC(33,1);

    int32_t next_scroll = pause_select * FIXED_DEC(33,1);
    stage.pause_scroll += (next_scroll - stage.pause_scroll) >> 3;

    for (uint8_t i = 0; i < COUNT_OF(stage_options); i++)
    {
        //get position on screen
        int32_t y = (i * 33) - 8 - (stage.pause_scroll >> FIXED_SHIFT);
        if (y <= -screen.SCREEN_HEIGHT2 - 8)
            continue;
        if (y >= screen.SCREEN_HEIGHT2 + 8)
            break;
                
        //draw text
        stage.font_bold.draw_col(&stage.font_bold,
        stage_options[i],
      20 + (y >> 2),
        y + 120,
        FontAlign_Left,
        //if the option is the one you are selecting, draw in normal color, else, draw gray
        (i == pause_select) ? 128 : 100,
        (i == pause_select) ? 128 : 100,
        (i == pause_select) ? 128 : 100
        );
    }
    //pog blend
    RECT scr = {0, 0, screen.SCREEN_WIDTH, screen.SCREEN_HEIGHT};
    Gfx_BlendRect(&scr, 0, 0, 0, 0);
}

void OptionsState(int *note_x)
{
    static const char *stage_options[] = {
        "BACK",
        "",             
        "PAL REFRESH RATE",
        "GHOST TAP",
        "MISS SOUNDS",
        "DOWNSCROLL",
        "MIDDLESCROLL",
        "SHOW SONG TIME",
        "WIDESCREEN",
        "AUDIO OFFSET",
        "DEBUG MODE"
    };

    //Select option if cross or start is pressed
    if (pad_state.press & (PAD_CROSS | PAD_START))
    {
        if (stage_options[pause_select] != "") //todo remove this shit
            stage.pause_scroll = -1;
        
        switch (pause_select)
        {
            case 0: //Back
                stage.pause_state = 0;
                break;
            case 1: 
                break;
            case 2: //Pal mode
                stage.prefs.palmode = !stage.prefs.palmode;
                Gfx_ScreenSetup();
                break;
            case 3: //Ghost tap
                stage.prefs.ghost = !stage.prefs.ghost;
                break;
            case 4: //Miss sounds
                stage.prefs.sfxmiss = !stage.prefs.sfxmiss;
                break;
            case 5: //Downscroll
                stage.prefs.downscroll = !stage.prefs.downscroll;
                break;
            case 6: //Middlescroll
                stage.prefs.middlescroll = !stage.prefs.middlescroll;

                if(stage.prefs.middlescroll)
                {
                    //bf
                    note_x[0] = FIXED_DEC(26 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[1] = FIXED_DEC(60 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
                    note_x[2] = FIXED_DEC(94 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[3] = FIXED_DEC(128 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    //opponent
                    note_x[4] = FIXED_DEC(-50 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[5] = FIXED_DEC(-16 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
                    note_x[6] = FIXED_DEC(170 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[7] = FIXED_DEC(204 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                }
                else
                {
                    //bf
                    note_x[0] = FIXED_DEC(26,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[1] = FIXED_DEC(60,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
                    note_x[2] = FIXED_DEC(94,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[3] = FIXED_DEC(128,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    //opponent
                    note_x[4] = FIXED_DEC(-128,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[5] = FIXED_DEC(-94,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
                    note_x[6] = FIXED_DEC(-60,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                    note_x[7] = FIXED_DEC(-26,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
                }

                break;
            case 7: //Song timer
                stage.prefs.songtimer = !stage.prefs.songtimer;
                break;
            case 8: //Widescreen
                stage.prefs.widescreen = !stage.prefs.widescreen;
                Gfx_ScreenSetup();
                break;
            case 9:
            //audio offset
                break;
            case 10: //Debug mode
                stage.prefs.debug = !stage.prefs.debug;
                break;
        }
    }

    if (pause_select == 9 && pad_state.held & (PAD_LEFT | PAD_RIGHT))
    {
        if (pad_state.held & PAD_LEFT)
            stage.offset --;
        else if (pad_state.held & PAD_RIGHT)
            stage.offset ++;
        stage.prefs.audio_offset = stage.offset;
    }

    char offset_test[24];
    sprintf(offset_test, "AUDIO OFFSET %d", stage.offset);
    Gfx_DrawText(5, screen.SCREEN_HEIGHT - 15, 0, offset_test);

    //Change option
    if (pad_state.press & PAD_UP)
    {
        if (pause_select > 0)
            pause_select--;
        else
            pause_select = COUNT_OF(stage_options) - 1;
    }
    if (pad_state.press & PAD_DOWN)
    {
        if (pause_select < COUNT_OF(stage_options) - 1)
            pause_select++;
        else
            pause_select = 0;
    }
                
    //draw options
    if (stage.pause_scroll == -1)
        stage.pause_scroll = COUNT_OF(stage_options) * FIXED_DEC(33,1);

    int32_t next_scroll = pause_select * FIXED_DEC(33,1);
    stage.pause_scroll += (next_scroll - stage.pause_scroll) >> 3;

    for (uint8_t i = 0; i < COUNT_OF(stage_options); i++)
    {
        //get position on screen
        int32_t y = (i * 33) - 8 - (stage.pause_scroll >> FIXED_SHIFT);
        if (y <= -screen.SCREEN_HEIGHT2 - 8)
            continue;
        if (y >= screen.SCREEN_HEIGHT2 + 8)
            break;
                
        //draw text
        stage.font_bold.draw_col(&stage.font_bold,
        stage_options[i],
      20 + (y >> 2),
        y + 120,
        FontAlign_Left,
        //if the option is the one you are selecting, draw in normal color, else, draw gray
        (i == pause_select) ? 128 : 100,
        (i == pause_select) ? 128 : 100,
        (i == pause_select) ? 128 : 100
        );
    }

    //pog blend
    RECT scr = {0, 0, screen.SCREEN_WIDTH, screen.SCREEN_HEIGHT};
    Gfx_BlendRect(&scr, 0, 0, 0, 0);
}