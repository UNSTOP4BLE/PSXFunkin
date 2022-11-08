/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <psxetc.h>
#include <psxapi.h>
#include <hwregs_c.h>
#include "../timer.h"
#include "../stage.h"
#include "../audio.h"

//Timer state
Timer timer;
volatile u32 timer_count;
u32 frame_count, animf_count;
u32 timer_lcount, timer_countbase;
u32 timer_persec;

fixed_t timer_sec, timer_dt, timer_secbase;

u8 timer_brokeconf;

u16 profile_start, total_time;

void Timer_StartProfile(void) {
	total_time = (TIMER_VALUE(1) - profile_start) & 0xffff;
	profile_start = TIMER_VALUE(1);
}

// returns cpu usage percentage
int Timer_EndProfile(void) {
	u16 cpu_time = (TIMER_VALUE(1) - profile_start) & 0xffff;
	return 100 * cpu_time / (total_time + 1);
}

void Timer_Callback(void) {
	timer_count++;
}

void Timer_Init(void) {
    //Initialize counters
    frame_count = animf_count = timer_count = timer_lcount = timer_countbase = 0;
    timer_sec = timer_dt = timer_secbase = 0;
    
    //Setup counter IRQ
    timer_persec = 100;
    
    EnterCriticalSection();

    TIMER_CTRL(2) = 0x0258; // F_CPU/8 input, IRQ on reload
    TIMER_RELOAD(2) = (F_CPU / 8) / timer_persec; // 100 Hz

    ChangeClearRCnt(2, 0);
    InterruptCallback(6, &Timer_Callback); //IRQ6 is RCNT2

    ExitCriticalSection();
    timer_brokeconf = 0;
}

void Timer_Tick(void)
{
	u32 status = *((volatile const u32*)0x1f801814);
	
	//Increment frame count
	frame_count++;
	
	//Update counter time
	if (timer_count == timer_lcount)
	{
		if (timer_brokeconf != 0xFF)
			timer_brokeconf++;
		if (timer_brokeconf >= 10)
		{
			if ((status & 0x00100000) != 0)
				timer_count += timer_persec / 50;
			else
				timer_count += timer_persec / 60;
		}
	}
	else
	{
		if (timer_brokeconf != 0)
			timer_brokeconf--;
	}
	
	if (timer_count < timer_lcount)
	{
		timer_secbase = timer_sec;
		timer_countbase = timer_lcount;
	}
	fixed_t next_sec = timer_secbase + FIXED_DIV(timer_count - timer_countbase, timer_persec);
	
	timer_dt = next_sec - timer_sec;
	timer_sec = next_sec;
	
	if (!stage.paused)
		animf_count = (timer_sec * 24) >> FIXED_SHIFT;
	
	timer_lcount = timer_count;
}

void Timer_Reset(void)
{
	Timer_Tick();
	timer_dt = 0;
}

void StageTimer_Tick()
{
	//has the song started?
	if (stage.song_step > 0) //if song starts decrease the timer
   		timer.timer = (Audio_GetLength(stage.stage_def->music_track)+1) - (stage.song_time / 1000); //seconds (ticks down)
    else //if not keep the timer at the song starting length	
 	    timer.timer = (Audio_GetLength(stage.stage_def->music_track)+1); //seconds (ticks down)
    timer.timermin = timer.timer / 60; //minutes left till song ends
    timer.timersec = timer.timer % 60; //seconds left till song ends
}

void StageTimer_Draw()
{
	RECT bar_fill = {252, 252, 1, 1};
	RECT_FIXED bar_dst = {FIXED_DEC(-70,1), FIXED_DEC(-110,1), FIXED_DEC(140,1), FIXED_DEC(11,1)};
	//Draw timer
	sprintf(timer.timer_display, "%d", timer.timermin);
	stage.font_cdr.draw(&stage.font_cdr,
		timer.timer_display,
		FIXED_DEC(-1 - 10,1) + stage.noteshakex, 
		FIXED_DEC(-109,1) + stage.noteshakey,
		FontAlign_Left
	);
	sprintf(timer.timer_display, ":");
	stage.font_cdr.draw(&stage.font_cdr,
		timer.timer_display,

		FIXED_DEC(-1,1) + stage.noteshakex,
		FIXED_DEC(-109,1) + stage.noteshakey,
		FontAlign_Left
	);
	if (timer.timersec >= 10)
		sprintf(timer.timer_display, "%d", timer.timersec);
	else
		sprintf(timer.timer_display, "0%d", timer.timersec);

	stage.font_cdr.draw(&stage.font_cdr,
		timer.timer_display,
		FIXED_DEC(-1 + 7,1) + stage.noteshakex,
		FIXED_DEC(-109,1) + stage.noteshakey,
		FontAlign_Left
	);
	if (stage.prefs.downscroll)
		bar_dst.y = FIXED_DEC(99,1); 

	Stage_BlendTex(&stage.tex_hud0, &bar_fill, &bar_dst, stage.bump, 1);
}
