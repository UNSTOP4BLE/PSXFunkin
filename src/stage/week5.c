/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week5.h"

#include "../archive.h"
#include "../mem.h"
#include "../mutil.h"

//Week 5 background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
	
	//Textures
	Gfx_Tex tex_back0; //Back wall
	Gfx_Tex tex_back1; //Second floor
	Gfx_Tex tex_back2; //Lower bop
	Gfx_Tex tex_back3; //Santa
	Gfx_Tex tex_back4; //Upper bop
} Back_Week5;

int particx;
int particy[2];
boolean movepart;

static void drawparticle(Gfx_Tex *tex)
{
	if (stage.song_step > 0)	
	{
		if (!stage.paused)
		{
			if (stage.stage_id == StageId_5_2)
				particx += 4;
			else
				particx += 3;
			particy[0] -= 5;
			particy[1] += 1;
			if(particy[0] > 250)
				particy[0] -= 250;
			if(particy[1] > 250)
				particy[1] -= 250;
		}
	}
	RECT bit_src = {4, 172, 4, 4};
	RECT_FIXED bit_dst = {
		FIXED_DEC(particx - 380,1) - stage.camera.x,
		FIXED_DEC(MUtil_Sin(particy[0]) / 9 + -40,1) - stage.camera.y,
		FIXED_DEC(4,1),
		FIXED_DEC(4,1)
	};

	Stage_DrawTex(tex, &bit_src, &bit_dst, stage.camera.bzoom);
	if (!stage.paused)
		bit_dst.y = FIXED_DEC(MUtil_Sin(particy[1]) / 8 + -30,1) - stage.camera.y;
	Stage_DrawTex(tex, &bit_src, &bit_dst, stage.camera.bzoom);
}

//Week 5 background functions
void Back_Week5_DrawBG(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	fixed_t fx, fy;
	
	fixed_t beat_bop;
	if ((stage.song_step & 0x3) == 0) {
		beat_bop = FIXED_UNIT - ((stage.note_scroll / 24) & FIXED_LAND);
		movepart = 1;
	}
	else
		beat_bop = 0;
	fx = stage.camera.x;
	fy = stage.camera.y;

	if (particx > 94 || stage.song_step < 0)
	{
		movepart = 0;
		particx = 10;
		particy[0] = 20;
		particy[1] = 0;

	}

	//Draw Santa
	struct Back_Week5_Santa
	{
		RECT src;
		RECT_FIXED dst;
	} santap[] = {
		{{0, 0, 117, 150}, {FIXED_DEC(-446,1), FIXED_DEC(-60,1), FIXED_DEC(169,1), FIXED_DEC(188,1)}},
	};
	
	const struct Back_Week5_Santa *santa = santap;
	for (size_t i = 0; i < COUNT_OF(santap); i++, santa++)
	{
		RECT_FIXED santa_dst = {
			santa->dst.x - fx - (beat_bop << 1),
			santa->dst.y - fy + (beat_bop << 3),
			santa->dst.w + (beat_bop << 2),
			santa->dst.h - (beat_bop << 3),
		};
		Debug_StageMoveDebug(&santa_dst, 10, fx, fy);
		Stage_DrawTex(&this->tex_back3, &santa->src, &santa_dst, stage.camera.bzoom);
	}

	if (movepart)
		drawparticle(&this->tex_back3);

	//Draw snow	
	RECT snow_src = {120, 155, 136, 101};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-350,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(570,1),
		FIXED_DEC(27,1)
	};

	if (stage.prefs.widescreen)
	{
		snow_dst.x = FIXED_DEC(-429,1) - fx;
		snow_dst.w = FIXED_DEC(736,1);
	}
	Debug_StageMoveDebug(&snow_dst, 4, fx, fy);
	Stage_DrawTex(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	snow_src.y = 255; snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 3;
	Stage_DrawTex(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	
	//Draw boppers
	static const struct Back_Week5_LowerBop
	{
		RECT src;
		RECT_FIXED dst;
	} lbop_piece[] = {
		{{0, 0, 80, 102}, {FIXED_DEC(-315,1), FIXED_DEC(-30,1), FIXED_DEC(80,1), FIXED_DEC(102,1)}},
		{{175, 3, 74, 151}, {FIXED_DEC(-120,1), FIXED_DEC(-80,1), FIXED_DEC(74,1), FIXED_DEC(151,1)}},
		{{81, 0, 70, 128}, {FIXED_DEC(30,1), FIXED_DEC(-70,1), FIXED_DEC(70,1), FIXED_DEC(128,1)}},
		{{151, 0, 23, 132}, {FIXED_DEC(100,1), FIXED_DEC(-70,1), FIXED_DEC(23,1), FIXED_DEC(132,1)}},
		{{0, 109, 41, 139}, {FIXED_DEC(123,1), FIXED_DEC(-69,1), FIXED_DEC(41,1), FIXED_DEC(139,1)}},
		{{41, 126, 69, 130}, {FIXED_DEC(164,1), FIXED_DEC(-52,1), FIXED_DEC(69,1), FIXED_DEC(130,1)}},
	};
	
	const struct Back_Week5_LowerBop *lbop_p = lbop_piece;
	for (size_t i = 0; i < COUNT_OF(lbop_piece); i++, lbop_p++)
	{
		RECT_FIXED lbop_dst = {
			lbop_p->dst.x - fx - (beat_bop << 1),
			lbop_p->dst.y - fy + (beat_bop << 3),
			lbop_p->dst.w + (beat_bop << 2),
			lbop_p->dst.h - (beat_bop << 3),
		};
		Debug_StageMoveDebug(&lbop_dst, 5, fx, fy);
		Stage_DrawTex(&this->tex_back2, &lbop_p->src, &lbop_dst, stage.camera.bzoom);
	}
	
	//Draw tree
	fx = stage.camera.x * 2 / 5;
	fy = stage.camera.y * 2 / 5;
	
	RECT tree_src = {118, 0, 138, 210};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-86,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(174,1),
		FIXED_DEC(210,1)
	};
	Debug_StageMoveDebug(&tree_dst, 6, fx, fy);
	Stage_DrawTex(&this->tex_back3, &tree_src, &tree_dst, stage.camera.bzoom);
	
	//Draw second floor
	fx = stage.camera.x >> 2;
	fy = stage.camera.y >> 2;
	
	struct Back_Week5_FloorPiece
	{
		RECT src;
		fixed_t scale;
	} floor_piece[] = {
		{{  0, 0, 161, 256}, FIXED_DEC(14,10)},
		{{161, 0,   9, 256}, FIXED_DEC(7,1)},
		{{171, 0,  85, 256}, FIXED_DEC(14,10)},
	};
	RECT_FIXED floor_dst = {
		FIXED_DEC(-220,1) - fx,
		FIXED_DEC(-115,1) - fy,
		0,
		FIXED_DEC(180,1)
	};
	if (stage.prefs.widescreen)
	{
		floor_dst.x = FIXED_DEC(-299,1) - fx;
		floor_piece[0].scale += FIXED_DEC(1,1);
		floor_piece[1].scale += FIXED_DEC(1,1);
		floor_piece[2].scale += FIXED_DEC(1,1);
	}

	Debug_StageMoveDebug(&floor_dst, 7, fx, fy);

	const struct Back_Week5_FloorPiece *floor_p = floor_piece;
	for (size_t i = 0; i < COUNT_OF(floor_piece); i++, floor_p++)
	{
		floor_dst.w = floor_p->src.w ? (floor_p->src.w * floor_p->scale) : floor_p->scale;
		Stage_DrawTex(&this->tex_back1, &floor_p->src, &floor_dst, stage.camera.bzoom);
		floor_dst.x += floor_dst.w;
	}
	
	//Draw boppers
	struct Back_Week5_UpperBop
	{
		RECT src;
		RECT_FIXED dst;
	} ubop_piece[] = {
		{{0, 0, 256, 76}, {FIXED_DEC(-200,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}},
		{{0, 76, 256, 76}, {FIXED_DEC(50,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}}
	};
	if (stage.prefs.widescreen)
	{
		ubop_piece[1].dst.x += FIXED_DEC(70,1);
		ubop_piece[1].dst.y += FIXED_DEC(2,1);

	}
	const struct Back_Week5_UpperBop *ubop_p = ubop_piece;
	for (size_t i = 0; i < COUNT_OF(ubop_piece); i++, ubop_p++)
	{
		RECT_FIXED ubop_dst = {
			ubop_p->dst.x - fx,
			ubop_p->dst.y - fy + (beat_bop << 2),
			ubop_p->dst.w,
			ubop_p->dst.h - (beat_bop << 2),
		};
		Debug_StageMoveDebug(&ubop_dst, 8, fx, fy);
		Stage_DrawTex(&this->tex_back4, &ubop_p->src, &ubop_dst, stage.camera.bzoom);
	}
	
	//Draw back wall
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	struct Back_Week5_WallPiece
	{
		RECT src;
		fixed_t scale;
	} wall_piece[] = {
		{{  0, 0, 113, 256}, FIXED_DEC(1,1)},
		{{113, 0,   6, 256}, FIXED_DEC(17,1)},
		{{119, 0, 137, 256}, FIXED_DEC(1,1)},
	};
	
	if (stage.prefs.widescreen) {
		wall_piece[0].scale += FIXED_DEC(1,1);
		wall_piece[1].scale += FIXED_DEC(1,1);
		wall_piece[2].scale += FIXED_DEC(1,1);
	}
	RECT_FIXED wall_dst = {
		FIXED_DEC(-186,1) - fx,
		FIXED_DEC(-130,1) - fy,
		0,
		FIXED_DEC(190,1)
	};
	

	RECT wall_src = {0, 255, 0, 0};
	RECT_FIXED wall_fill;
	wall_fill.x = wall_dst.x - FIXED_DEC(60,1);
	wall_fill.y = wall_dst.y + wall_dst.h - FIXED_UNIT;
	wall_fill.w = FIXED_DEC(600,1);
	wall_fill.h = FIXED_DEC(100,1);
	if (stage.prefs.widescreen)
	{
		wall_dst.x = FIXED_DEC(-284,1) - fx;
		wall_fill.x = FIXED_DEC(-284,1) - fx;
		wall_fill.y = FIXED_DEC(55,1) - fy;
		wall_fill.w = FIXED_DEC(605,1);
	}
	Debug_StageMoveDebug(&wall_dst, 9, fx, fy);
	Stage_DrawTex(&this->tex_back0, &wall_src, &wall_fill, stage.camera.bzoom);
	
	const struct Back_Week5_WallPiece *wall_p = wall_piece;
	for (size_t i = 0; i < COUNT_OF(wall_piece); i++, wall_p++)
	{
		wall_dst.w = wall_p->src.w ? (wall_p->src.w * wall_p->scale) : wall_p->scale;
		Stage_DrawTex(&this->tex_back0, &wall_p->src, &wall_dst, stage.camera.bzoom);
		wall_dst.x += wall_dst.w;
	}
}


//Week 5 background functions
void Back_Week5_DrawBGHorror(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	fixed_t fx, fy;

	fx = stage.camera.x;
	fy = stage.camera.y;

	//Draw snow	
	RECT snow_src = {120, 155, 136, 101};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-376,1) - fx,
		FIXED_DEC(55,1) - fy,
		FIXED_DEC(736,1),
		FIXED_DEC(27,1)
	};

	Debug_StageMoveDebug(&snow_dst, 4, fx, fy);
	Stage_DrawTexCol(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 255, 40, 60);
	snow_src.y = 255; snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 3;
	Stage_DrawTexCol(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 255, 40, 60);

	fx = stage.camera.x * 2 / 10;
	fy = stage.camera.y * 2 / 10;

	//Draw tree
	RECT tree_src = {0, 0, 156, 256};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-79,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(154,1),
		FIXED_DEC(270,1)
	};
	Debug_StageMoveDebug(&tree_dst, 5, fx, fy);
	Stage_DrawTex(&this->tex_back1, &tree_src, &tree_dst, stage.camera.bzoom);

	RECT back_src = {0, 0, 256, 256};
	RECT_FIXED back_dst = {
		FIXED_DEC(-267,1) - fx,
		FIXED_DEC(-205,1) - fy,
		FIXED_DEC(541,1),
		FIXED_DEC(321,1)
	};

	Debug_StageMoveDebug(&back_dst, 6, fx, fy);
	Stage_DrawTex(&this->tex_back0, &back_src, &back_dst, stage.camera.bzoom);
}

void Back_Week5_Free(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Week5_New()
{
	//Allocate background structure
	Back_Week5 *this = (Back_Week5*)Mem_Alloc(sizeof(Back_Week5));
	if (this == NULL)
		return NULL;
	
	if (stage.stage_id != StageId_5_3)
	{
		//Set background functions
		this->back.draw_fg = NULL;
		this->back.draw_md = NULL;
		this->back.draw_bg = Back_Week5_DrawBG;
		this->back.free = Back_Week5_Free;
		
		//Load background textures
		IO_Data arc_back = IO_Read("\\WEEK5\\BACK.ARC;1");
		Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
		Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
		Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
		Gfx_LoadTex(&this->tex_back3, Archive_Find(arc_back, "back3.tim"), 0);
		Gfx_LoadTex(&this->tex_back4, Archive_Find(arc_back, "back4.tim"), 0);
		Mem_Free(arc_back);
	}
	else
	{
		//Set background functions
		this->back.draw_fg = NULL;
		this->back.draw_md = NULL;
		this->back.draw_bg = Back_Week5_DrawBGHorror;
		this->back.free = Back_Week5_Free;
		
		//Load background textures
		IO_Data arc_back = IO_Read("\\WEEK5\\BACK.ARC;1");
		Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back5.tim"), 0);
		Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back6.tim"), 0);
		Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
		Mem_Free(arc_back);
	}

	return (StageBack*)this;
}
