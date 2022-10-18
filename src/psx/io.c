/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../io.h"

#include <stdlib.h>                  
#include "../audio.h"
#include "../main.h"

static u32 audio_sector_buf[512];

static volatile void (*data_callback)(void);
static volatile void (*audio_callback)(u32 *sector);
static volatile u32 *data_ptr = 0;
static volatile int data_lba = 0, data_sectors_pending = 0;
static volatile int audio_lba = 0, audio_sectors_pending = 0;

static void dataHandler(int event, u8 *payload);

static void audioHandler(int event, u8 *payload) {
	if (event != CdlDataReady)
		return;

	// handle this audio sector
	CdGetSector(audio_sector_buf, 512);
	if (audio_callback)
		audio_callback(audio_sector_buf);
	audio_lba++;
	audio_sectors_pending--;

	if (!audio_sectors_pending) {
		CdReadyCallback(&dataHandler);

		// if some data still has to be read, continue reading it after all audio data has been read
		if (data_sectors_pending) {
			CdlLOC pos;
			CdIntToPos(data_lba, &pos);
			CdControlF(CdlReadN, &pos);
		} else {
			// otherwise just stop reading
			CdControlF(CdlPause, 0);
		}
	}
}

static void dataHandler(int event, u8 *payload) {
	if (event != CdlDataReady)
		return;

	// handle this data sector
	CdGetSector((void *)data_ptr, 512);
	data_ptr += 512;
	data_lba++;
	data_sectors_pending--;

	// pause reading data and start reading audio chunk if necessary
	// (audio has higher priority than data)
	if (audio_sectors_pending) {
		CdReadyCallback(&audioHandler);

		CdlLOC pos;
		CdIntToPos(audio_lba, &pos);
		CdControlF(CdlReadN, &pos);
	} else {
		// stop reading data sectors if there's no more data to read
		if (!data_sectors_pending) {
			CdControlF(CdlPause, 0);
			if (data_callback)
				data_callback();
		}
	}
}

//IO functions
void IO_Init(void)
{
	//Initialize CD IO
	CdInit();

	u8 param[4];
	param[0] = CdlModeSpeed;
	CdControl(CdlSetmode, param, 0);

	EnterCriticalSection();
	CdReadyCallback(&dataHandler);
	ExitCriticalSection();
}

void IO_Quit(void)
{
	//IO_AbortAudioRead();
}

void IO_FindFile(CdlFILE *file, const char *path)
{
	if (data_sectors_pending || audio_sectors_pending) {
		sprintf(
			error_msg,
			"[IO_FindFile] Can't find files while reading (data=%d audio=%d)",
			data_sectors_pending, audio_sectors_pending
		);
		ErrorLock();
	}

	// TODO: make sure it's not called during audio playback
	printf("[IO_FindFile] Searching for %s\n", path);
	
	//Search for file
	if (!CdSearchFile(file, (char*)path))
	{
		sprintf(error_msg, "[IO_FindFile] %s not found", path);
		ErrorLock();
	}
}

IO_Data IO_AsyncReadFile(CdlFILE *file)
{	
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
	int lba = CdPosToInt(&(file->pos));
	IO_ReadDataChunk(lba, sects, (uint32_t *)buffer);
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

IO_Data IO_ReadFile(CdlFILE *file)
{
	//Read file then sync
	IO_Data buffer = IO_AsyncReadFile(file);
	if (!IO_WaitRead()) {
		sprintf(
			error_msg,
			"[IO_ReadFile] Timeout while loading %s (data=%d audio=%d)",
			file->name, data_sectors_pending, audio_sectors_pending
		);
		ErrorLock();
		return NULL;
	}
	return buffer;
}

IO_Data IO_Read(const char *path)
{
	printf("[IO_Read] Reading file %s\n", path);
	
	//Search for file
	CdlFILE file;
	IO_FindFile(&file, path);

	return IO_ReadFile(&file);
}

boolean IO_IsReading(void) {
	return data_sectors_pending ? true : false;
}

boolean IO_WaitRead(void) {
	int start = VSync(-1);

	while ((VSync(-1) - start) < 300) { // about 5 seconds
		if (!data_sectors_pending)
			return true;
	}

	printf("[IO_WaitRead] Timeout\n");
	return false;
}

// must be called from critical section
void IO_ReadDataChunk(int lba, int sectors, u32 *buf) {
	if (data_sectors_pending) {
		printf("[IO_ReadDataChunk] Data is already being read\n");
		return;
	}

	data_lba = lba;
	data_ptr = buf;
	data_callback = 0;
	data_sectors_pending = sectors;

	if (!audio_sectors_pending) {
		CdReadyCallback(&dataHandler);

		CdlLOC pos;
		CdIntToPos(lba, &pos);
		CdControlF(CdlReadN, &pos);
	}
}

// must be called from critical section
void IO_ReadAudioChunk(int lba, int sectors, void (*callback)(u32 *sector)) {
	if (audio_sectors_pending) {
		printf("[IO_ReadAudioChunk] Audio data is already being read\n");
		return;
	}

	audio_lba = lba;
	audio_callback = callback;
	audio_sectors_pending = sectors;

	if (!data_sectors_pending) {
		CdReadyCallback(&audioHandler);

		CdlLOC pos;
		CdIntToPos(lba, &pos);
		CdControlF(CdlReadN, &pos);
	}
}

// must be called from critical section
void IO_AbortAudioRead(void) {
	CdReadyCallback(&dataHandler);

	audio_sectors_pending = 0;
	if (data_sectors_pending) {
		CdlLOC pos;
		CdIntToPos(data_lba, &pos);
		CdControlF(CdlReadN, &pos);
	} else {
		CdControlF(CdlPause, 0);
	}
}
