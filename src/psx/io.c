/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../io.h"

#include <stdlib.h>                  
#include "../audio.h"
#include "../main.h"

//IO functions
void IO_Init(void)
{
    //Initialize CD IO
    CdInit();
}

void IO_Quit(void)
{
    
}

void IO_FindFile(CdlFILE *file, const char *path)
{
    printf("[IO_FindFile] Searching for %s\n", path);
    
    //Stop XA playback
    Audio_StopStream();
    
    //Search for file
    if (!CdSearchFile(file, (char*)path))
    {
        sprintf(error_msg, "[IO_FindFile] %s not found", path);
        ErrorLock();
    }
}

IO_Data IO_ReadFile(CdlFILE *file)
{
    //Read file then sync
    IO_Data buffer = IO_AsyncReadFile(file);
    CdReadSync(0, NULL);
    return buffer;
}

IO_Data IO_AsyncReadFile(CdlFILE *file)
{
    //Stop XA playback
    Audio_StopStream();
    
    //Get number of sectors for the file
    size_t sects = (file->size + IO_SECT_SIZE - 1) / IO_SECT_SIZE;
    
    //Allocate a buffer for the file
    size_t size;
    IO_Data buffer = (IO_Data)malloc(size = (IO_SECT_SIZE * sects));
    if (buffer == NULL)
    {
        sprintf(error_msg, "[IO_AsyncReadFile] Malloc (size %X) fail", size);
        ErrorLock();
        return NULL;
    }
    
    //Read file
    CdControl(CdlSetloc, (uint8_t*)&file->pos, NULL);
    CdControlB(CdlSeekL, NULL, NULL);
    CdRead(sects, buffer, CdlModeSpeed);
    return buffer;
}

IO_Data IO_Read(const char *path)
{
    printf("[IO_Read] Reading file %s\n", path);
    
    //Search for file
    CdlFILE file;
    IO_FindFile(&file, path);
    
    //Read file then sync
    IO_Data buffer = IO_AsyncReadFile(&file);
    CdReadSync(0, NULL);
    return buffer;
}

IO_Data IO_AsyncRead(const char *path)
{
    printf("[IO_ReadAsync] Reading file %s\n", path);
    
    //Search for file
    CdlFILE file;
    IO_FindFile(&file, path);
    
    //Read file
    return IO_AsyncReadFile(&file);
}

bool IO_IsSeeking(void)
{
    CdControl(CdlNop, NULL, NULL);
    return (CdStatus() & (CdlStatSeek)) != 0;
}

bool IO_IsReading(void)
{
    CdControl(CdlNop, NULL, NULL);
    return (CdStatus() & (CdlStatSeek | CdlStatRead)) != 0;
}