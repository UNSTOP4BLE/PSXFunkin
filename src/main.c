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
	Audio_Reset();

	while (1)
	{
		FntPrint(-1, "A fatal error has occured:\n\n%s\n", error_msg);
		Gfx_Flip();
	}
}

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
	Menu_Load(MenuPage_Opening);

	//Game loop
	while (PSX_Running()) {
#ifndef NDEBUG
		Timer_StartProfile();
#endif

		//Prepare frame
		Timer_Tick();
		Pad_Update();

		//Tick and draw game
		switch (gameloop)
		{
			case GameLoop_Menu:
				Menu_Tick();
				break;
			case GameLoop_Stage:
				Stage_Tick();
				break;
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
		Gfx_Flip();
	}
	
	//Deinitialize system
	Pad_Quit();
	Gfx_Quit();
	Audio_Quit();
	IO_Quit();
	
	PSX_Quit();
	return 0;
}
