/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "speaker.h"

#include "../io.h"
#include "../stage.h"
#include "../timer.h"

//Speaker functions
void Speaker_Init(Speaker *this)
{
	//Initialize speaker state
	this->bump = 0;
	
	//Load speaker graphics
	if (stage.stage_id >= StageId_5_1 && stage.stage_id <= StageId_5_3)
		Gfx_LoadTex(&this->tex, IO_Read("\\CHAR\\SPEAKERX.TIM;1"), GFX_LOADTEX_FREE);
	else
		Gfx_LoadTex(&this->tex, IO_Read("\\CHAR\\SPEAKER.TIM;1"), GFX_LOADTEX_FREE);
}

void Speaker_Bump(Speaker *this)
{
	//Set bump
	this->bump = FIXED_DEC(4,1) / 24 - 1;
}

void Speaker_Tick(Speaker *this, fixed_t x, fixed_t y, fixed_t parallax)
{
	//Get frame to use according to bump
	u8 frame;
	if (this->bump > 0)
	{
		frame = (this->bump * 24) >> FIXED_SHIFT;
		this->bump -= timer_dt;
	}
	else
	{
		frame = 0;
	}
	
	//Draw speakers
	static const struct SpeakerPiece
	{
		u8 rect[4];
		u8 ox, oy;
	} speaker_draw[4][2] = {
		{ //bump 0
			{{ 97,  88, 158, 88},   0,  0},
			{{  0, 176,  18, 56}, 158, 32},
		},
		{ //bump 1
			{{176,   0,  79, 88},   0,  0},
			{{  0,  88,  97, 88},  79,  0},
		},
		{ //bump 2
			{{  0,   0,  88, 88},   0,  0},
			{{ 88,   0,  88, 88},  88,  0},
		},
		{ //bump 3
			{{  0,   0,  88, 88},   0,  0},
			{{ 88,   0,  88, 88},  88,  0},
		}
	};

	static const struct SpeakerPiecex
	{
		u8 rect[4];
		u8 ox, oy;
	} speaker_drawx[4][2] = {
		{ //bump 0
			{{134, 100, 121, 90},   0,  1},
			{{ 0, 100,  134, 100}, 121 - 60, 0},
		},
		{ //bump 1
			{{195,   0,  60, 91},   0 + 1,  0},
			{{  0, 100, 134, 100},  60 + 1,  0},
		},
		{ //bump 2
			{{  0,   0, 159, 100},   0,  0},
			{{159,   0,  36, 100}, 159,  0},
		},
		{ //bump 3
			{{  0,   0, 159, 100},   0,  0},
			{{159,   0,  36, 100}, 159,  0},
		}
	};

	const struct SpeakerPiece *piece = speaker_draw[frame];
	const struct SpeakerPiecex *piecex = speaker_drawx[frame];

	if (stage.stage_id >= StageId_5_1 && stage.stage_id <= StageId_5_3)
	{
		
		for (int i = 0; i < 2; i++, piecex++)
		{
			//Draw piece
			RECT piecex_src = {piecex->rect[0], piecex->rect[1], piecex->rect[2], piecex->rect[3]};
			RECT_FIXED piecex_dst = {
				x - FIXED_DEC(88,1) + ((fixed_t)piecex->ox << FIXED_SHIFT) - FIXED_MUL(stage.camera.x, parallax),
				y + ((fixed_t)piecex->oy << FIXED_SHIFT) - FIXED_MUL(stage.camera.y, parallax),
				(fixed_t)piecex->rect[2] << FIXED_SHIFT,
				(fixed_t)piecex->rect[3] << FIXED_SHIFT,
			};
			
			Stage_DrawTex(&this->tex, &piecex_src, &piecex_dst, stage.camera.bzoom);
		}
	}
	else 	
	{
		
		for (int i = 0; i < 2; i++, piece++)
		{
			//Draw piece
			RECT piece_src = {piece->rect[0], piece->rect[1], piece->rect[2], piece->rect[3]};
			RECT_FIXED piece_dst = {
				x - FIXED_DEC(88,1) + ((fixed_t)piece->ox << FIXED_SHIFT) - FIXED_MUL(stage.camera.x, parallax),
				y + ((fixed_t)piece->oy << FIXED_SHIFT) - FIXED_MUL(stage.camera.y, parallax),
				(fixed_t)piece->rect[2] << FIXED_SHIFT,
				(fixed_t)piece->rect[3] << FIXED_SHIFT,
			};
			
			Stage_DrawTex(&this->tex, &piece_src, &piece_dst, stage.camera.bzoom);
		}
	}
}
