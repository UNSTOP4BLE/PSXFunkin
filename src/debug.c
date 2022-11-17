/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"
#include "debug.h"

static void Drawdebugtxt()
{
	sprintf(debug.debugtext, "Welcome to PSXFunkin debug mode!");
	stage.font_cdr.draw(&stage.font_cdr,
		debug.debugtext,
		FIXED_DEC(-158, 1),
		FIXED_DEC(-105, 1),
		FontAlign_Left
	);
	if (debug.mode == 0)
	{
		sprintf(debug.debugtext, "Transform objects with dpad and face buttons, switch objects");
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 10, 1),
			FontAlign_Left
		);
		sprintf(debug.debugtext, "with L1 and R1, switch debug modes with L2 and R2,");
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 20, 1),
			FontAlign_Left
		);
		sprintf(debug.debugtext, "Focus camera with select");
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 30, 1),
			FontAlign_Left
		);
		sprintf(debug.debugtext, "x %d, y %d, w %d, h %d, selection %d, mode %d", debug.debugx, debug.debugy, debug.debugw, debug.debugh, debug.selection, debug.mode);
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 40, 1),
			FontAlign_Left
		);
		stage.freecam = 0;
	}
	else
	{
		sprintf(debug.debugtext, "Move the camera with the dpad,");
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 10, 1),
			FontAlign_Left
		);
		sprintf(debug.debugtext, "switch debug modes with L2 and R2");
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 20, 1),
			FontAlign_Left
		);
		sprintf(debug.debugtext, "x %d, y %d, zoom %d, mode %d", stage.camera.x / 1024, stage.camera.y / 1024, stage.camera.bzoom, debug.mode);
		stage.font_cdr.draw(&stage.font_cdr,
			debug.debugtext,
			FIXED_DEC(-158, 1),
			FIXED_DEC(-105 + 30, 1),
			FontAlign_Left
		);
		stage.freecam = 1;
	}
}

void Debug_StageDebug()
{
	if (stage.prefs.debug)
	{
		stage.prefs.botplay = 1; 
		
		if (debug.selection < 1)
			debug.selection = 0;

		if (debug.mode > 1 || debug.mode < 1)
			debug.mode = 0;
		Drawdebugtxt();

		if (debug.mode == 0)
		{
			switch(debug.selection)
			{
				case 0:
				if (debug.switchcooldown == 0)
				{
					debug.debugx = stage.opponent->x / 1024; 
					debug.debugy = stage.opponent->y / 1024; 
				}
				debug.switchcooldown ++;
				stage.opponent->x = FIXED_DEC(debug.debugx,1);
				stage.opponent->y = FIXED_DEC(debug.debugy,1);
				break;
				case 1:
				if (debug.switchcooldown == 0)
				{
					debug.debugx = stage.opponent2->x / 1024; 
					debug.debugy = stage.opponent2->y / 1024; 
				}
				debug.switchcooldown ++;
				stage.opponent2->x = FIXED_DEC(debug.debugx,1);
				stage.opponent2->y = FIXED_DEC(debug.debugy,1);
				break;
				case 2:
				if (debug.switchcooldown == 0)
				{
					debug.debugx = stage.gf->x / 1024; 
					debug.debugy = stage.gf->y / 1024; 
				}
				debug.switchcooldown ++;
				stage.gf->x = FIXED_DEC(debug.debugx,1);
				stage.gf->y = FIXED_DEC(debug.debugy,1);
				break;
				case 3:
				if (debug.switchcooldown == 0)
				{
					debug.debugx = stage.player->x / 1024; 
					debug.debugy = stage.player->y / 1024; 
				}
				debug.switchcooldown ++;
				stage.player->x = FIXED_DEC(debug.debugx,1);
				stage.player->y = FIXED_DEC(debug.debugy,1);
				break;
			}

			switch(pad_state.held)
			{
				case PAD_LEFT: 
					debug.debugx --;
				break;
				case PAD_RIGHT: 
					debug.debugx ++;
				break;
				case PAD_UP: 
					debug.debugy --;
				break;
				case PAD_DOWN: 
					debug.debugy ++;
				break;
				case PAD_SQUARE: 
					debug.debugw --;
				break;
				case PAD_CIRCLE: 
					debug.debugw ++;
				break;
				case PAD_TRIANGLE: 
					debug.debugh --;
				break;
				case PAD_CROSS: 
					debug.debugh ++;
				break;
			}
		}
		switch(pad_state.press)
		{
			case PAD_L1: 
				debug.selection --;
				debug.switchcooldown = 0;
			break;
			case PAD_R1: 
				debug.selection ++;
				debug.switchcooldown = 0;
			break;
			case PAD_L2: 
				debug.mode --;
			break;
			case PAD_R2: 
				debug.mode ++;
			break;
		}
	}
}

void Debug_StageMoveDebug(RECT_FIXED *dst, int dacase, fixed_t fx, fixed_t fy)
{
	if (debug.mode == 0 && stage.prefs.debug)
	{
		if (debug.selection == dacase)
		{
			if (debug.switchcooldown == 0)
			{
				debug.debugx = dst->x / 1024 + fx / 1024; 
				debug.debugy = dst->y / 1024 + fy / 1024; 
				debug.debugw = dst->w / 1024; 
				debug.debugh = dst->h / 1024; 
			}
			debug.switchcooldown ++;
			dst->x = FIXED_DEC(debug.debugx,1) - fx;
			dst->y = FIXED_DEC(debug.debugy,1) - fy;
			dst->w = FIXED_DEC(debug.debugw,1);
			dst->h = FIXED_DEC(debug.debugh,1);
		}
	}
}

void Debug_GfxMoveDebug(RECT *dst, int dacase)
{
	if (debug.mode == 0 && stage.prefs.debug)
	{
		if (debug.selection == dacase)
		{
			if (debug.switchcooldown == 0)
			{
				debug.debugx = dst->x; 
				debug.debugy = dst->y; 
				debug.debugw = dst->w; 
				debug.debugh = dst->h; 
			}
			debug.switchcooldown ++;
			dst->x = debug.debugx;
			dst->y = debug.debugy;
			dst->w = debug.debugw;
			dst->h = debug.debugh;
		}
	}
}

void Debug_ScrollCamera(void)
{
	if (stage.freecam)
	{
		if (pad_state.held & PAD_LEFT)
			stage.camera.x -= FIXED_DEC(2,1);
		if (pad_state.held & PAD_UP)
			stage.camera.y -= FIXED_DEC(2,1);
		if (pad_state.held & PAD_RIGHT)
			stage.camera.x += FIXED_DEC(2,1);
		if (pad_state.held & PAD_DOWN)
			stage.camera.y += FIXED_DEC(2,1);
		if (pad_state.held & PAD_TRIANGLE)
			stage.camera.zoom -= FIXED_DEC(1,100);
		if (pad_state.held & PAD_CROSS)
			stage.camera.zoom += FIXED_DEC(1,100);
	}
	else if (pad_state.held & PAD_SELECT)
	{
		//Get delta position
		fixed_t dx = stage.camera.tx - stage.camera.x;
		fixed_t dy = stage.camera.ty - stage.camera.y;
		fixed_t dz = stage.camera.tz - stage.camera.zoom;
		
		//Scroll based off current divisor
		stage.camera.x += FIXED_MUL(dx, stage.camera.td);
		stage.camera.y += FIXED_MUL(dy, stage.camera.td);
		stage.camera.zoom += FIXED_MUL(dz, stage.camera.td);
	}
}