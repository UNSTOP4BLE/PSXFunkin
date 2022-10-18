 /*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
  The bulk of this code was written by spicyjpeg
  (C) 2021 spicyjpeg
*/

#include "../audio.h"
#include "../main.h"
#include <hwregs_c.h>

#include "../timer.h"
#include "../io.h"

//Audio constants
#define SAMPLE_RATE 0x1000 //44100 Hz

#define BUFFER_SIZE (13 << 11) //13 sectors
#define CHUNK_SIZE (BUFFER_SIZE * audio_streamcontext.header.s.channels)

#define BUFFER_TIME FIXED_DEC(((BUFFER_SIZE * 28) / 16), 44100)

//SPU registers
typedef struct
{
	u16 vol_left;
	u16 vol_right;
	u16 freq;
	u16 addr;
	u32 adsr_param;
	u16 _reserved;
	u16 loop_addr;
} Audio_SPUChannel;

#define SPU_CHANNELS    ((volatile Audio_SPUChannel*)0x1f801c00)
#define SPU_RAM_ADDR(x) ((u16)(((u32)(x)) >> 3))

#define DUMMY_ADDR 0x1000
#define BUFFER_START_ADDR 0x1040

#define CHUNK_SIZE_MAX (BUFFER_SIZE * 4) // there are never more than 4 channels
#define ALLOC_START_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2))
//Audio streaming
typedef struct
{
	//CD state
	enum
	{
		Audio_StreamState_Stopped,
		Audio_StreamState_Ini,
		Audio_StreamState_Play,
		Audio_StreamState_Playing,
	} state;
	boolean loops;
	
	u32 cd_lba;
	u32 cd_length;
	u32 cd_pos;
	
	//SPU state
	u32 spu_addr;
	u32 spu_pos;
	u32 spu_swap;
	
	//Timing state
	u32 timing_chunk;
	fixed_t timing_pos, timing_start;
	
	//Header
	union
	{
		struct
		{
			u8 channels;
		} s;
		u8 d[2048];
	} header;
} Audio_StreamContext;

static volatile Audio_StreamContext audio_streamcontext;
static volatile u32 audio_alloc_ptr = ALLOC_START_ADDR;

void Audio_StreamCallback(u32 *sector) {
	
	//Don't run if stopped
	if (audio_streamcontext.state == Audio_StreamState_Stopped)
		return;

	audio_streamcontext.cd_pos++;
	
	//DMA to SPU
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(audio_streamcontext.spu_addr + audio_streamcontext.spu_pos);
	SpuWrite(sector, 2048);
	audio_streamcontext.spu_pos += 2048;

	//Start SPU IRQ if finished reading
	if (audio_streamcontext.spu_pos >= CHUNK_SIZE)
	{
		switch (audio_streamcontext.state)
		{
			case Audio_StreamState_Ini:
			{
				//Update addresses
				audio_streamcontext.spu_addr = BUFFER_START_ADDR + CHUNK_SIZE;
				audio_streamcontext.spu_swap = 1;
				audio_streamcontext.spu_pos = 0;
				
				//Set state
				audio_streamcontext.state = Audio_StreamState_Play;
				break;
			}
			case Audio_StreamState_Play:
			{
				//Stop and turn on SPU IRQ
				SPU_IRQ_ADDR = SPU_RAM_ADDR(audio_streamcontext.spu_addr);
				SPU_CTRL |= 0x0040;
				
				//Set state
				audio_streamcontext.state = Audio_StreamState_Playing;
				break;
			}
			case Audio_StreamState_Playing:
			{
				//Stop and turn on SPU IRQ
				SPU_IRQ_ADDR = SPU_RAM_ADDR(audio_streamcontext.spu_addr);
				SPU_CTRL |= 0x0040;
				break;
			}
			default:
				break;
		}
	}
}

void Audio_StreamIRQ_SPU(void)
{
	//Disable SPU IRQ until we've finished streaming more data
	SPU_CTRL &= 0xffbf;

	//Don't run if stopped
	if (audio_streamcontext.state == Audio_StreamState_Stopped)
		return;
	
	//Update timing state
	audio_streamcontext.timing_chunk++;
	audio_streamcontext.timing_pos = (audio_streamcontext.timing_chunk << FIXED_SHIFT) * (BUFFER_SIZE / 16) / 1575;
	audio_streamcontext.timing_start = timer_sec;
	
	//Update addresses
	if ((audio_streamcontext.spu_swap ^= 1) != 0)
		audio_streamcontext.spu_addr = BUFFER_START_ADDR + CHUNK_SIZE;
	else
		audio_streamcontext.spu_addr = BUFFER_START_ADDR;
	audio_streamcontext.spu_pos = 0;
	
	for (int i = 0; i < audio_streamcontext.header.s.channels; i++)
		SPU_CHANNELS[i].loop_addr = SPU_RAM_ADDR(audio_streamcontext.spu_addr + BUFFER_SIZE * i);
	
	//Check for loop
	if (audio_streamcontext.loops)
	{
		//Return to beginning of mus
		if (audio_streamcontext.cd_pos >= audio_streamcontext.cd_length)
		{
			audio_streamcontext.timing_pos = 0;
			audio_streamcontext.cd_pos = 0;
		}
	}
	else
	{
		//Stop playing
		if (audio_streamcontext.cd_pos == audio_streamcontext.cd_length)
		{
			//Continue streaming from CD (wrap to prevent unintended errors)
			IO_ReadAudioChunk(audio_streamcontext.cd_lba, CHUNK_SIZE / 2048, &Audio_StreamCallback);
			return;
		}
		else if (audio_streamcontext.cd_pos > audio_streamcontext.cd_length)
		{
			//Stop playing
			Audio_StopMus();
			return;
		}
	}

	//Continue streaming from CD
	IO_ReadAudioChunk(
		audio_streamcontext.cd_lba + audio_streamcontext.cd_pos,
		CHUNK_SIZE / 2048, &Audio_StreamCallback
	);
}

//Audio interface
void Audio_Init(void)
{
	//Initialize SPU
	SpuInit();
	
	//Set volume (this is done by default but i just put it here cus why not)
	SPU_MASTER_VOL_L = 0x3fff;
	SPU_MASTER_VOL_R = 0x3fff;
	
	//Reset context
	audio_streamcontext.timing_start = -1;
}

void Audio_Quit(void)
{
	
}

void Audio_Reset(void)
{
	//Reset SPU
	//InterruptCallback(9, NULL);
	SPU_CTRL &= 0xffbf;
	
	//Upload dummy block (not needed, psn00b already does this)
	/*static u8 dummy[64] = {0, 5};
	
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(DUMMY_ADDR);
	SpuWrite((const uint32_t *)dummy, sizeof(dummy));
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);*/
	
	//Reset keys
	SPU_KEY_OFF = 0x00FFFFFF;

	for (int i = 0; i < 24; i++)
	{
		SPU_CHANNELS[i].vol_left   = 0x0000;
		SPU_CHANNELS[i].vol_right  = 0x0000;
		SPU_CHANNELS[i].addr       = SPU_RAM_ADDR(DUMMY_ADDR);
		SPU_CHANNELS[i].freq       = 0x1000;
		SPU_CHANNELS[i].adsr_param = 0x1fc080ff;
	}

	SPU_KEY_ON = 0x00FFFFFF;
}

void Audio_LoadMusFile(CdlFILE *file)
{
	//Stop playing mus
	Audio_StopMus();

	//Read header
	IO_ReadDataChunk(CdPosToInt(&file->pos), 1, (uint32_t *)audio_streamcontext.header.d);
	IO_WaitRead();
	printf(
		"[Audio_LoadMusFile] Loaded %s (%d ch)\n",
		file->name,
		audio_streamcontext.header.s.channels
	);
	
	//Reset context
	audio_streamcontext.state = Audio_StreamState_Ini;
	
	audio_streamcontext.timing_chunk = 0;
	audio_streamcontext.timing_pos = 0;
	audio_streamcontext.timing_start = -1;
	
	audio_streamcontext.spu_addr = BUFFER_START_ADDR;
	audio_streamcontext.spu_swap = 0;
	audio_streamcontext.spu_pos = 0;
	
	//Use mus file
	audio_streamcontext.cd_lba = CdPosToInt(&file->pos) + 1;
	audio_streamcontext.cd_length = ((file->size + 2047) >> 11) - 1;
	audio_streamcontext.cd_pos = 0;

	//Setup SPU
	Audio_Reset();

	EnterCriticalSection();
	InterruptCallback(9, &Audio_StreamIRQ_SPU);
	IO_ReadAudioChunk(audio_streamcontext.cd_lba, CHUNK_SIZE / 2048 * 2, &Audio_StreamCallback);
	ExitCriticalSection();
}

int Audio_GetLength()
{
	return (audio_streamcontext.cd_length / audio_streamcontext.header.s.channels) * 0xe00 * 1 / 44100;
}

void Audio_LoadMus(const char *path)
{
	//Find requested file
	CdlFILE file;
	IO_FindFile(&file, path);
	
	//Load found file
	Audio_LoadMusFile(&file);
}

void Audio_PlayMus(boolean loops)
{
	//Wait for play state
	while (audio_streamcontext.state != Audio_StreamState_Playing)
		__asm__ volatile("");

	//Start timing
	audio_streamcontext.timing_chunk = 0;
	audio_streamcontext.timing_pos = 0;
	audio_streamcontext.timing_start = timer_sec;
	
	//Play keys
	audio_streamcontext.loops = loops;
	u32 key_or = 0x00ffffff >> (24 - audio_streamcontext.header.s.channels);

	SPU_KEY_OFF = key_or;

	for (int i = 0; i < audio_streamcontext.header.s.channels; i++)
	{
		SPU_CHANNELS[i].addr       = SPU_RAM_ADDR(BUFFER_START_ADDR + BUFFER_SIZE * i);
		SPU_CHANNELS[i].loop_addr  = SPU_CHANNELS[i].addr + SPU_RAM_ADDR(CHUNK_SIZE);
		SPU_CHANNELS[i].freq       = SAMPLE_RATE;
		SPU_CHANNELS[i].adsr_param = 0x1fc080ff;
	}

	SPU_KEY_ON = key_or;
	//Audio_StreamIRQ_SPU();
}

void Audio_StopMus(void)
{
	//Reset context
	audio_streamcontext.state = Audio_StreamState_Stopped;

	Audio_Reset();

	//Reset CD
	IO_AbortAudioRead();
}

void Audio_SetVolume(u8 i, u16 vol_left, u16 vol_right)
{
	SPU_CHANNELS[i].vol_left = vol_left;
	SPU_CHANNELS[i].vol_right = vol_right;
}

fixed_t Audio_GetTime(void)
{
	if (audio_streamcontext.timing_pos < 0 || audio_streamcontext.timing_start < 0)
		return 0;
	fixed_t dt = timer_sec - audio_streamcontext.timing_start;
	if (dt > BUFFER_TIME)
		return audio_streamcontext.timing_pos + BUFFER_TIME;
	return audio_streamcontext.timing_pos + dt;
}

boolean Audio_IsPlaying(void)
{
	return audio_streamcontext.state != Audio_StreamState_Stopped;
}

/* .VAG file loader */
#define VAG_HEADER_SIZE 48
static int lastChannelUsed = 23;

static int getFreeChannel(void) {
    int channel = lastChannelUsed;
    lastChannelUsed--;
	if (lastChannelUsed < audio_streamcontext.header.s.channels)
		lastChannelUsed = 23;
    return channel;
}

void Audio_ClearAlloc(void) {
	audio_alloc_ptr = ALLOC_START_ADDR;
}

u32 Audio_LoadVAGData(u32 *sound, u32 sound_size) {
	// subtract size of .vag header (48 bytes), round to 64 bytes
	u32 xfer_size = ((sound_size - VAG_HEADER_SIZE) + 63) & 0xffffffc0;
	u8  *data = (u8 *) sound;

	// modify sound data to ensure sound "loops" to dummy sample
	// https://psx-spx.consoledev.net/soundprocessingunitspu/#flag-bits-in-2nd-byte-of-adpcm-header
	data[sound_size - 15] = 1; // end + mute

	// allocate SPU memory for sound
	u32 addr = audio_alloc_ptr;
	audio_alloc_ptr += xfer_size;

	if (audio_alloc_ptr > 0x80000) {
		sprintf(error_msg, "[Audio_LoadVAGData] SPU RAM overflow");
		ErrorLock();
	}

	SpuSetTransferStartAddr(addr); // set transfer starting address to malloced area
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA); // set transfer mode to DMA
	SpuWrite((uint32_t *)data + VAG_HEADER_SIZE, xfer_size); // perform actual transfer
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT); // wait for DMA to complete

	printf("[Audio_LoadVAGData] Allocated new sound (addr=%08x size=%d)\n", addr, xfer_size);
	return addr;
}

void Audio_PlaySoundOnChannel(u32 addr, u32 channel, int volume) {
	SPU_KEY_OFF = (1 << channel);

	SPU_CHANNELS[channel].vol_left   = volume;
	SPU_CHANNELS[channel].vol_right  = volume;
	SPU_CHANNELS[channel].addr       = SPU_RAM_ADDR(addr);
	SPU_CHANNELS[channel].loop_addr  = SPU_RAM_ADDR(DUMMY_ADDR);
	SPU_CHANNELS[channel].freq       = SAMPLE_RATE; // 44100 Hz
	SPU_CHANNELS[channel].adsr_param = 0x1fc080ff;

	SPU_KEY_ON = (1 << channel);
}

void Audio_PlaySound(u32 addr, int volume) {
    Audio_PlaySoundOnChannel(addr, getFreeChannel(), volume);
}

