/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_SAVE_H
#define PSXF_GUARD_SAVE_H

#include "psx.h"

typedef struct {
    uint16_t id; // must be 0x4353
    uint8_t iconDisplayFlag;
    uint8_t iconBlockNum; // always 1
    uint8_t title[64]; // 16 bit shift-jis format
    uint8_t reserved[28];
    uint8_t iconPalette[32];
    uint8_t iconImage[128];
    uint8_t saveData[7936];
} SaveFile;

void defaultSettings();
bool readSaveFile();
void writeSaveFile();

#endif
