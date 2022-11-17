/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "monster.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../main.h"

//Monster character structure
enum
{
	Monster_ArcMain_Idle0,
	Monster_ArcMain_Idle1,
	Monster_ArcMain_Left,
	Monster_ArcMain_Down,
	Monster_ArcMain_Up,
	Monster_ArcMain_Right,
	
	Monster_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[Monster_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_Monster;

//Monster character definitions
static const CharFrame char_monster_frame[] = {
	{Monster_ArcMain_Idle0, {  0,   0, 106, 159}, { 52, 159}}, //0 idle 1
	{Monster_ArcMain_Idle0, {107,   0, 105, 162}, { 53, 162}}, //1 idle 2
	{Monster_ArcMain_Idle1, {  0,   0, 110, 160}, { 54, 160}}, //2 idle 3
	{Monster_ArcMain_Idle1, {111,   0, 121, 175}, { 63, 175}}, //3 idle 4
	
	{Monster_ArcMain_Left, {  0,   0,  115, 178}, { 47, 178}}, //4 left 1
	{Monster_ArcMain_Left, {117,   0,  119, 176}, { 52, 176}}, //5 left 2
	
	{Monster_ArcMain_Down, {  0,   0, 107, 153}, { 49, 153}}, //6 down 1
	{Monster_ArcMain_Down, {108,   0, 108, 153}, { 51, 152}}, //7 down 2
	
	{Monster_ArcMain_Up, {  0,   0, 102, 188}, { 52, 188}}, //8 up 1
	{Monster_ArcMain_Up, {103,   0, 105, 188}, { 55, 181}}, //9 up 2
	
	{Monster_ArcMain_Right, {  0,   0, 93, 180}, { 49, 180}}, //10 right 1
	{Monster_ArcMain_Right, { 93,   0, 95, 180}, { 52, 178}}, //11 right 2
};

static const Animation char_monster_anim[CharAnim_Max] = {
	{2, (const u8[]){ 0, 1, 2, 3, ASCR_BACK, 0}}, //CharAnim_Idle
	{2, (const u8[]){ 4,  5, ASCR_BACK, 0}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{2, (const u8[]){ 6,  7, ASCR_BACK, 0}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 0}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{2, (const u8[]){10, 11, ASCR_BACK, 0}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

//Monster character functions
void Char_Monster_SetFrame(void *user, u8 frame)
{
	Char_Monster *this = (Char_Monster*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_monster_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_Monster_Tick(Character *character)
{
	Char_Monster *this = (Char_Monster*)character;
	
	//Perform idle dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
		Character_PerformIdle(character);
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_Monster_SetFrame);
	Character_Draw(character, &this->tex, &char_monster_frame[this->frame]);
}

void Char_Monster_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_Monster_Free(Character *character)
{
	Char_Monster *this = (Char_Monster*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_Monster_New(fixed_t x, fixed_t y)
{
	//Allocate monster object
	Char_Monster *this = Mem_Alloc(sizeof(Char_Monster));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_Monster_New] Failed to allocate monster object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_Monster_Tick;
	this->character.set_anim = Char_Monster_SetAnim;
	this->character.free = Char_Monster_Free;
	
	Animatable_Init(&this->character.animatable, char_monster_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 2;

	//health bar color
	this->character.health_bar = 0xFFF7FF6B;
	
	this->character.focus_x =  FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-80,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	if (stage.stage_id == StageId_5_3)
	{
		//Load art
		this->arc_main = IO_Read("\\CHAR\\MONSTERX.ARC;1");
		
		const char **pathp = (const char *[]){
			"idleb0.tim", //Monster_ArcMain_Idle0
			"idleb1.tim", //Monster_ArcMain_Idle1
			"leftb.tim",  //Monster_ArcMain_Left
			"downb.tim",  //Monster_ArcMain_Down
			"upb.tim",    //Monster_ArcMain_Up
			"rightb.tim", //Monster_ArcMain_Right
			NULL
		};
		IO_Data *arc_ptr = this->arc_ptr;
		for (; *pathp != NULL; pathp++)
			*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	}
	else
	{
		//Load art
		this->arc_main = IO_Read("\\CHAR\\MONSTER.ARC;1");
		
		const char **pathp = (const char *[]){
			"idle0.tim", //Monster_ArcMain_Idle0
			"idle1.tim", //Monster_ArcMain_Idle1
			"left.tim",  //Monster_ArcMain_Left
			"down.tim",  //Monster_ArcMain_Down
			"up.tim",    //Monster_ArcMain_Up
			"right.tim", //Monster_ArcMain_Right
			NULL
		};
		IO_Data *arc_ptr = this->arc_ptr;
		for (; *pathp != NULL; pathp++)
			*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	}
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
