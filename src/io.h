/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_IO_H
#define PSXF_GUARD_IO_H

#include "psx.h"

typedef u32* IO_Data;

//IO constants
#define IO_SECT_SIZE 2048

//IO functions
void IO_Init(void);
void IO_Quit(void);
void IO_FindFile(CdlFILE *file, const char *path);
IO_Data IO_ReadFile(CdlFILE *file);
IO_Data IO_AsyncReadFile(CdlFILE *file);
IO_Data IO_Read(const char *path);
IO_Data IO_AsyncRead(const char *path);
boolean IO_IsReading(void);
boolean IO_WaitRead(void);

void IO_ReadDataChunk(int lba, int sectors, u32 *buf);
void IO_ReadAudioChunk(int lba, int sectors, void (*callback)(u32 *sector));
void IO_AbortAudioRead(void);

#endif
