/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"
void PausedState()
{

				stage.font_bold.draw(&stage.font_bold,
					"hi",
					FIXED_DEC(10,1), 
					(screen.SCREEN_HEIGHT2 - 22) << FIXED_SHIFT,
					FontAlign_Left
					);

}