/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_EVENTS_H
#define PSXF_GUARD_EVENTS_H
#include "psx.h"
#include "fixed.h"

//EVENTS
#define EVENTS_FLAG_VARIANT 0xFFFC

#define EVENTS_FLAG_SPEED     (1 << 2) //Change Scroll Speed

#define EVENTS_FLAG_PLAYED    (1 << 15) //Event has been already played

typedef struct
{
  //psych engine shit
  u16 pos; //1/12 steps
  u16 event;
  u16 value1;
  u16 value2;
} Event;

//Psych Engine Events Reader By IgorSou3000
typedef struct
{
  //Scroll Speed!!
  struct
  {
    fixed_t ogspd;
    fixed_t value1, value2;
  }speed;

}Events;

void Events_ScrollSpeed(void);

void Events_Tick(void);
void Events_StartEvents(void);
void Events_Load(void);

void Events_FG(void);
void Events_BG(void);

extern Events events;

#endif
