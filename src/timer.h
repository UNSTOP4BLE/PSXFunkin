/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_TIMER_H
#define PSXF_GUARD_TIMER_H

#include "psx.h"
#include "fixed.h"

//Timer state
extern u32 frame_count, animf_count;
extern fixed_t timer_sec, timer_dt;

typedef struct
{
	u32 cursonglength;
	int secondtimer;
	int timer;
	int timermin;
	char timer_display[13];
} Timer;

extern Timer timer;

//Timer interface 
void Timer_Init(boolean pal_console, boolean pal_video);
void Timer_Tick(void);
void Timer_Reset(void);
void StageTimer_Calculate();
void StageTimer_Tick();
void StageTimer_Draw();

#endif
