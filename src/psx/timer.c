/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../timer.h"
#include "../stage.h"

#define TIMER_BITS (3)

//Timer state
Timer timer;
u32 frame_count, animf_count;
u32 timer_count, timer_lcount, timer_countbase;
u32 timer_persec;

fixed_t timer_sec, timer_dt, timer_secbase;

u8 timer_brokeconf;

//Timer interface
extern void InterruptCallback(int index, void *cb);
extern void ChangeClearRCnt(int t, int m);

void Timer_Callback(void) {
	timer_count++;
}

void Timer_Init(boolean pal_console, boolean pal_video)
{
	//Initialize counters
	frame_count = animf_count = timer_count = timer_lcount = timer_countbase = 0;
	timer_sec = timer_dt = timer_secbase = 0;
	
	//Setup counter IRQ
	static const u32 hblanks_per_sec[4] = {
		15734 >> TIMER_BITS, //!console && !video => 262.5 * 59.940
		15591 >> TIMER_BITS, //!console &&  video => 262.5 * 59.393
		15769 >> TIMER_BITS, // console && !video => 312.5 * 50.460
		15625 >> TIMER_BITS, // console &&  video => 312.5 * 50.000
	};
	timer_persec = hblanks_per_sec[(pal_console << 1) | pal_video];
	
	EnterCriticalSection();
	
	SetRCnt(RCntCNT1, 1 << TIMER_BITS, RCntMdINTR);
	InterruptCallback(5, (void*)Timer_Callback); //IRQ5 is RCNT1
	StartRCnt(RCntCNT1);
	ChangeClearRCnt(1, 0);
	
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

void StageTimer_Calculate()
{
	//Calculate the minutes and seconds
	if (timer.timer >= 60) {
		timer.timermin ++;
		timer.timer -= 60;
	}	
}

void StageTimer_Tick()
{
	if (!stage.paused)
	{
		timer.secondtimer += timer_dt / 12;
		if (stage.prefs.palmode ? timer.secondtimer >= 50 : timer.secondtimer >= 60)
		{
			timer.secondtimer = 0;
			if (timer.timer <= 0)
			{		
				if (timer.timermin > 0)
					timer.timermin --;
				else
					timer.timermin = 0;
				timer.timer = 59;
			}
			else 
				timer.timer --;
		}
	}
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
	if (timer.timer >= 10)
		sprintf(timer.timer_display, "%d", timer.timer);
	else
		sprintf(timer.timer_display, "0%d", timer.timer);

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
