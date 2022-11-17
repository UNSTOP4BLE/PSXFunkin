/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "events.h"
#include "stage.h"
#include "timer.h"
#include "random.h"

 Events events;

void Events_Tick(void)
{
	//Scroll Speed!
	stage.speed += (FIXED_MUL(events.speed.ogspd, events.speed.value1) - stage.speed) / ((events.speed.value2) / 64 + 1);
}

void Events_StartEvents(void)
{
	for (Event *event = stage.cur_event; event->pos != 0xFFFF; event++)
	{
		//Update event pointer
		if (event->pos > (stage.note_scroll >> FIXED_SHIFT))
			break;

		else
			stage.cur_event++;

		if (event->event & EVENTS_FLAG_PLAYED)
			continue;

		//Events
		switch(event->event & EVENTS_FLAG_VARIANT)
		{
			case EVENTS_FLAG_SPEED: //Scroll Speed!!
			{
				events.speed.value1 = event->value1;
				events.speed.value2 = event->value2;
				break;
			}
			default: //nothing lol
				break;
		}

			event->event |= EVENTS_FLAG_PLAYED;
	}

	Events_Tick();
}

//Initialize some stuffs
void Events_Load(void)
{
	//Scroll Speed
	events.speed.ogspd = stage.speed;
	events.speed.value1 = FIXED_UNIT;
	events.speed.value2 = 0;
}
