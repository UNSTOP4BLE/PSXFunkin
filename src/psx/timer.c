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

uint16_t profile_start, total_time;
int curfps, next_run, framecount;

void Timer_StartProfile(void) {
    total_time = (TIMER_VALUE(1) - profile_start) & 0xffff;
    profile_start = TIMER_VALUE(1);
}

// returns cpu usage percentage
int Timer_EndProfile(void) {
    uint16_t cpu_time = (TIMER_VALUE(1) - profile_start) & 0xffff;
    return 100 * cpu_time / (total_time + 1);
}

static void timer_irq_handler(void) {
    timer.timer_irq_count++;
}

void Timer_Init(void) {
    EnterCriticalSection();
    ChangeClearRCnt(2, 0);
    InterruptCallback(IRQ_TIMER2, &timer_irq_handler);

    TIMER_CTRL(2)   = 0x0260; // SYSCLK/8, no reload, IRQ on overflow
    timer.timer_irq_count = 0;
    ExitCriticalSection();
}

void Timer_incrementFrameCount(void)
{
    framecount ++;
}

void Timer_CalcFPS(void)
{
    int cur_t = Timer_GetTime();
    if (cur_t > next_run) {
        curfps = framecount;
        framecount = 0;
        next_run = cur_t + TICKS_PER_SEC;
    }
}

int Timer_GetFPS(void)
{
    return curfps;
}

uint32_t Timer_GetAnimfCount(void)
{
    if (stage.paused)
        return 0;
    else
        return (Timer_GetTime() * 24) / TICKS_PER_SEC;
}

uint64_t Timer_GetTime(void) {
    return
    ((uint64_t) timer.timer_irq_count << TIMER_SHIFT) |
    ((uint64_t) TIMER_VALUE(2) >> (16 - TIMER_SHIFT));
}

uint32_t Timer_GetTimeint32(void) {
    return
    ((uint32_t) timer.timer_irq_count << TIMER_SHIFT) |
    ((uint32_t) TIMER_VALUE(2) >> (16 - TIMER_SHIFT));
}

uint64_t Timer_GetTimeMS(void) {
    return (Timer_GetTime() * 1000) / TICKS_PER_SEC;
}

void Timer_Reset(void)
{
    TIMER_VALUE(2)  = 0;
    timer.timer_irq_count = 0;
    next_run = 0;
    framecount = 0;
    curfps = 0;
}

int last_time = 0;
int delta = 0;

void Timer_CalcDT()
{
    int time = Timer_GetTime();
    delta = time - last_time;
    last_time = time;
}

int Timer_GetDT()
{
    if (delta > 0)
        return delta;
    else return 0;
}

void StageTimer_Tick()
{
    //im deeply sorry for anyone reading this code
    timer.timer = Audio_GetInitialTime() - ((stage.song_time >= 0) ? (stage.song_time / 1000) : 0); //seconds (initial)
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
        sprintf(timer.timer_display, "0%d", (timer.timersec > 0 ? timer.timersec : 0));
    
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
