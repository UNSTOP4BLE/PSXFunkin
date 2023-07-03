/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_TIMER_H
#define PSXF_GUARD_TIMER_H

#include "psx.h"
#include "fixed.h"

#define TIMER_SHIFT   4
#define TICKS_PER_SEC ((F_CPU / 8) >> (16 - TIMER_SHIFT))

typedef struct
{
    int secondtimer;
    int timer;
    int timersec;
    int timermin;
    char timer_display[13];
    volatile uint64_t timer_irq_count;
} Timer;

extern Timer timer;

//Timer interface 
void Timer_Init(void);
void Timer_incrementFrameCount(void);
void Timer_CalcFPS(void);
int Timer_GetFPS(void);
uint32_t Timer_GetAnimfCount(void);
uint64_t Timer_GetTime(void);
uint32_t Timer_GetTimeint32(void);
uint64_t Timer_GetTimeMS(void);
void Timer_Reset(void);
void Timer_CalcDT();
int Timer_GetDT();
void StageTimer_Tick();
void StageTimer_Draw();

void Timer_StartProfile(void);
int Timer_EndProfile(void);

#endif
