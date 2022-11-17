/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_SAVE_H
#define PSXF_GUARD_SAVE_H

#include "psx.h"

typedef struct {
  	u16 id; // must be 0x4353
  	u8 iconDisplayFlag;
  	u8 iconBlockNum; // always 1
  	u8 title[64]; // 16 bit shift-jis format
  	u8 reserved[28];
  	u8 iconPalette[32];
  	u8 iconImage[128];
  	u8 saveData[7936];
} SaveFile;


void defaultSettings();
boolean readSaveFile();
void writeSaveFile();

#endif
