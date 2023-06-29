/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "main.h"

#include "timer.h"
#include "io.h"
#include "gfx.h"
#include "audio.h"
#include "str.h"
#include "pad.h"

#include "pause.h"
#include "menu.h"
#include "stage.h"
#include "save.h"

#include <stdlib.h>
#include <hwregs_c.h>

//Game loop
GameLoop gameloop;
SCREEN screen;

//Error handler
char error_msg[0x200];

void ErrorLock(void)
{
    while (1)
    {
        FntPrint(-1, "A fatal error has occured:\n\n%s\n", error_msg);
        Gfx_Flip();
    }
}

void Character_shitFromFile(Character *this, const char *path, int offstart)
{
    int offset = offstart;

    if (offstart != 0)
        this->file = IO_Read(path);
    CharacterFileHeader *tmphdr = (CharacterFileHeader *)this->file;
    offset += sizeof(CharacterFileHeader);
    this->animatable.anims = (const Animation *)(offset + this->file); 
    offset += sizeof(Animation) * tmphdr->size_animation;
    printf("%d size\n", sizeof(Animation));
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
int offstarts;
//Entry point                                                                             
int main(int argc, char **argv)                                                                                                                                                        
{
    //Remember arguments
    my_argc = argc;
    my_argv = argv;

    //Initialize system
    ResetGraph(0);
    PSX_Init();

    Gfx_Init();
    STR_Init();
    Pad_Init();
    InitCARD(1);
    StartPAD();
    StartCARD();
    _bu_init(); 
    ChangeClearPAD(0);
    IO_Init();
    Audio_Init();
    Timer_Init();

    if (readSaveFile() == false)
        defaultSettings();

    //Start game
    gameloop = GameLoop_Menu;
    Gfx_ScreenSetup();
  //  Menu_Load(MenuPage_Opening);

    Character testchar;
    //Game loop
    while (PSX_Running()) {
#ifndef NDEBUG
        Timer_StartProfile();
#endif

        //Prepare frame
        Timer_CalcDT();
        Audio_ProcessXA();
//        Audio_FeedStream();
        Pad_Update();

        if (testchar.animatable.anims[0].spd == 2)
        FntPrint(-1, "YAY OFFSET %d\n", offstarts);
        else
            pad_state.press |= PAD_UP;
        FntPrint(-1, "OFFSET %d", offstarts);
            if (pad_state.press & PAD_UP)
            {
                offstarts ++;
                Character_shitFromFile(&testchar, "\\CHAR\\DAD.CHR;1", offstarts);
            }
            if (pad_state.press & PAD_DOWN)
            {
                offstarts --;
                Character_shitFromFile(&testchar, "\\CHAR\\DAD.CHR;1", offstarts);
            }
        //Tick and draw game
        switch (gameloop)
        {

         //   case GameLoop_Menu:
          //      Menu_Tick();
           //     break;
            //case GameLoop_Stage:
             //   Stage_Tick();
              //  break;
            //case GameLoop_Movie:
//
  //              break;
        }

#ifndef NDEBUG
        HeapUsage heap;
        GetHeapUsage(&heap);

        int cpu = Timer_EndProfile();
        int ram = 100 * heap.alloc / heap.total;

        FntPrint(
            0, "CPU:%3d%%  HEAP:%06x\nRAM:%3d%%  MAX: %06x\n",
            cpu, heap.alloc, ram, heap.alloc_max
        );
#endif

        //Flip gfx buffers
        if (stage.str_playing)
            STR_Proccess();
        else
            Gfx_Flip();
    }
    
    return 0;
}
