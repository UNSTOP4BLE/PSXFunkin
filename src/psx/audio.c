/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//Most of this code is written by spicyjpeg

#include "../audio.h"
#include "../io.h"             
#include "../main.h"  
#include "../timer.h"
#include <stdlib.h>
#include <hwregs_c.h>

#define SWAP_ENDIAN(x) ( \
	(((uint32_t) (x) & 0x000000ff) << 24) | \
	(((uint32_t) (x) & 0x0000ff00) <<  8) | \
	(((uint32_t) (x) & 0x00ff0000) >>  8) | \
	(((uint32_t) (x) & 0xff000000) >> 24) \
)

typedef struct {
	uint32_t magic;			// 0x69474156 ("VAGi") for interleaved files
	uint32_t version;
	uint32_t interleave;	// Little-endian, size of each channel buffer
	uint32_t size;			// Big-endian, in bytes
	uint32_t sample_rate;	// Big-endian, in Hertz
	uint16_t _reserved[5];
	uint16_t channels;		// Little-endian, if 0 the file is mono
	char     name[16];
} VAG_Header;

typedef struct {
	uint32_t *read_buffer;
	int lba, chunk_secs, time;
	int buffer_size, num_chunks, sample_rate, samples, channels;
	boolean loop;

	volatile int    next_chunk, spu_addr;
	volatile int8_t db_active, state;

	// used for timing
	volatile uint32_t last_irq;
	uint32_t last_paused, chunk_ticks;
} StreamContext;

static StreamContext str_ctx;
static int lastChannelUsed = 0;
boolean playing;

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

#define BUFFER_SIZE (13 << 11) //13 sectors
#define CHUNK_SIZE (BUFFER_SIZE)
#define CHUNK_SIZE_MAX (BUFFER_SIZE * 4) // there are never more than 4 channels
#define BUFFER_START_ADDR 0x1010
#define DUMMY_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2))
#define SPU_CHANNELS    ((volatile Audio_SPUChannel*)0x1f801c00)
#define SPU_RAM_ADDR(x) ((u16)(((u32)(x)) >> 3))
#define VAG_HEADER_SIZE 48
#define DUMMY_BLOCK_ADDR  0x1000
#define BUFFER_START_ADDR 0x1010
#define ALLOC_START_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2) + 64)

static volatile u32 audio_alloc_ptr = 0;

typedef enum {
	STATE_IDLE,
	STATE_BUFFERING,
	STATE_DATA_NEEDED,
	STATE_READING
} StreamState;

static void spu_irq_handler() {
	// Acknowledge the interrupt to ensure it can be triggered again. The only
	// way to do this is actually to disable the interrupt entirely; we'll
	// enable it again once the chunk is ready.
	SPU_CTRL &= ~(1 << 6);

	int chunk_size = str_ctx.buffer_size * str_ctx.channels;
	int chunk      = (str_ctx.next_chunk + 1) % (uint32_t) str_ctx.num_chunks;

	str_ctx.db_active ^= 1;
	str_ctx.state      = STATE_BUFFERING;
	str_ctx.next_chunk = chunk;
	str_ctx.last_irq   = Timer_GetTime();

	// Configure to SPU to trigger an IRQ once the chunk that is going to be
	// filled now starts playing (so the next buffer can be loaded) and
	// override both channels' loop addresses to make them "jump" to the new
	// buffers, rather than actually looping when they encounter the loop flag
	// at the end of the currently playing buffers.
	int addr = BUFFER_START_ADDR + (str_ctx.db_active ? chunk_size : 0);
	str_ctx.spu_addr = addr;

	if (chunk == 0)
		playing = false;

	if (chunk == 0 && str_ctx.loop == false)
	{
		Audio_StopStream();
		return;
	}
	SPU_IRQ_ADDR = getSPUAddr(addr);
	for (int i = 0; i < str_ctx.channels; i++)
		SPU_CH_LOOP_ADDR(i) = getSPUAddr(addr + str_ctx.buffer_size * i);

	// Start uploading the next chunk to the SPU.
	SpuSetTransferStartAddr(addr);
	SpuWrite(str_ctx.read_buffer, chunk_size);
}

static void spu_dma_handler(void) {
	// Note that we can't call CdRead() here as it requires interrupts to be
	// enabled. Instead, feed_stream() (called from the main loop) will check
	// if str_ctx.state is set to STATE_DATA_NEEDED and fetch the next chunk.
	str_ctx.state = STATE_DATA_NEEDED;
}

static void cd_read_handler(int event, uint8_t *payload) {
	// Attempt to read the chunk again if an error has occurred.
	if (event != CdlDataReady) {
		printf("Chunk read error, retrying...\n");

		str_ctx.state = STATE_DATA_NEEDED;
		return;
	}

	// Re-enable the SPU IRQ once the new chunk has been fully uploaded.
	SPU_CTRL |= 1 << 6;

	str_ctx.state = STATE_IDLE;
}


//Audio functions
void Audio_ResetChannels(void) {
	SpuSetKey(0, 0x00ffffff);

	for (int i = 0; i < 24; i++) {
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);
		SPU_CH_FREQ(i) = 0x1000;
	}

	SpuSetKey(1, 0x00ffffff);
}

void Audio_Init() {
	SpuInit();
	Audio_ResetChannels();
}

void Audio_FeedStream(void) {
	if (str_ctx.state != STATE_DATA_NEEDED)
		return;

	// Start reading the next chunk from the CD.
	int lba = str_ctx.lba + str_ctx.next_chunk * str_ctx.chunk_secs;

	CdlLOC pos;
	CdIntToPos(lba, &pos);
	CdControl(CdlSetloc, &pos, 0);

	CdReadCallback(&cd_read_handler);
	CdRead(str_ctx.chunk_secs, str_ctx.read_buffer, CdlModeSpeed);

	str_ctx.state = STATE_READING;
}

void Audio_LoadStream(const char *path, boolean loop) {
	CdlFILE file;
	printf("OPENING STREAM FILE\n");
	if (!CdSearchFile(&file, path))
	{
		sprintf(error_msg, "[Audio_LoadStream] failed to find stream file %s", path);
		ErrorLock();
	}

	printf("BUFFERING STREAM\n");
	
	str_ctx.loop = loop;

	EnterCriticalSection();
	InterruptCallback(IRQ_SPU, &spu_irq_handler);
	DMACallback(DMA_SPU, &spu_dma_handler);
	ExitCriticalSection();

	// Read the header. Note that in interleaved .VAG files the first sector.
	// does not hold any audio data (i.e. the header is padded to 2048 bytes).
	uint32_t header[512];
	CdControl(CdlSetloc, &file.pos, 0);

	CdReadCallback(0);
	CdRead(1, header, CdlModeSpeed);
	CdReadSync(0, 0);

	VAG_Header *vag = (VAG_Header *) header;
	int buf_size    = vag->interleave;
	int channels    = vag->channels ? vag->channels : 1;
	int chunk_secs  = ((buf_size * channels) + 2047) / 2048;

	str_ctx.read_buffer = malloc(chunk_secs * 2048);
	str_ctx.lba         = CdPosToInt(&file.pos) + 1;
	str_ctx.chunk_secs  = chunk_secs;
	str_ctx.buffer_size = buf_size;
	str_ctx.num_chunks  = (SWAP_ENDIAN(vag->size) + buf_size - 1) / buf_size;
	str_ctx.sample_rate = SWAP_ENDIAN(vag->sample_rate);
	str_ctx.channels    = channels;
	str_ctx.chunk_ticks = (buf_size / 16) * (TICKS_PER_SEC * 28);
	str_ctx.db_active  = 1;
	str_ctx.next_chunk = 0;
	str_ctx.time = (SWAP_ENDIAN(vag->size) / 16) * (TICKS_PER_SEC * 28);
	str_ctx.samples = (SWAP_ENDIAN(vag->size) / 16) * 28;

	// Preload the first chunk into main RAM, then copy it into SPU RAM by
	// invoking the IRQ handler manually and blocking until the second chunk is
	// also loaded. The first delay is required in order to let the drive
	// finish processing the previous read command.
	str_ctx.state = STATE_DATA_NEEDED;
	for (int i = 0; i < 1000; i++)
		__asm__ volatile("");

	while (str_ctx.state != STATE_IDLE)
		Audio_FeedStream();

	spu_irq_handler();
	while (str_ctx.state != STATE_IDLE)
		Audio_FeedStream();
}

void Audio_StartStream() {
	int bits = 0x00ffffff >> (24 - str_ctx.channels);

	// Disable the IRQ as we're going to call spu_irq_handler() manually (due
	// to finicky SPU timings).
	SPU_CTRL &= ~(1 << 6);

	for (int i = 0; i < str_ctx.channels; i++) {
		SPU_CH_ADDR(i)  = getSPUAddr(str_ctx.spu_addr + str_ctx.buffer_size * i);
		SPU_CH_FREQ(i)  = getSPUSampleRate(str_ctx.sample_rate);
		SPU_CH_ADSR1(i) = 0x80ff;
		SPU_CH_ADSR2(i) = 0x1fee;
	}

	// Unmute the channels and route them for stereo output. You'll want to
	// edit this if you are using more than 2 channels, and/or if you want to
	// provide an option to output mono audio instead of stereo.
	SPU_CH_VOL_L(0) = 0x3fff;
	SPU_CH_VOL_R(0) = 0x0000;
	SPU_CH_VOL_L(1) = 0x0000;
	SPU_CH_VOL_R(1) = 0x3fff;
	SPU_CH_VOL_L(2) = 0x3fff;
	SPU_CH_VOL_R(2) = 0x3fff;

	SpuSetKey(1, bits);
	spu_irq_handler();
	str_ctx.last_paused = 0;
	lastChannelUsed = str_ctx.channels;
	playing = true;
}

void Audio_StopStream(void) {
	int bits = 0x00ffffff >> (24 - str_ctx.channels);

	SpuSetKey(0, bits);
	str_ctx.last_paused = Timer_GetTime();

	for (int i = 0; i < str_ctx.channels; i++)
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);

	SpuSetKey(1, bits);
	playing = false;
}

uint64_t Audio_GetTimeMS(void) {
	uint64_t chunk_start_time = (str_ctx.next_chunk - 1) *
	str_ctx.chunk_ticks / str_ctx.sample_rate;

	uint64_t current_time = str_ctx.last_paused;
	if (!current_time)
		current_time = Timer_GetTime();

	uint64_t time = chunk_start_time + (current_time - str_ctx.last_irq);
	return (time * 1000) / TICKS_PER_SEC;
}

u32 Audio_GetInitialTime(void) {
	return str_ctx.samples / str_ctx.sample_rate;
}

boolean Audio_IsPlaying(void)
{
	return playing;
}

void Audio_SetVolume(u8 i, u16 vol_left, u16 vol_right)
{
	SPU_CHANNELS[i].vol_left = vol_left;
	SPU_CHANNELS[i].vol_right = vol_right;
}

/* .VAG file loader */

static int getFreeChannel(void) {
	int channel = lastChannelUsed + 1;
	if (channel > 23)
		channel = str_ctx.channels;

	lastChannelUsed = channel;
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
		sprintf(error_msg, "[Audio_LoadVAGData] FATAL: SPU RAM overflow! (%d bytes overflowing)\n", audio_alloc_ptr - 0x80000);
		ErrorLock();
	}

	SpuSetTransferStartAddr(addr); // set transfer starting address to malloced area
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA); // set transfer mode to DMA
	SpuWrite((uint32_t *)data + VAG_HEADER_SIZE, xfer_size); // perform actual transfer
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT); // wait for DMA to complete

	printf("Allocated new sound (addr=%08x, size=%d)\n", addr, xfer_size);
	return addr;
}

void Audio_PlaySoundOnChannel(u32 addr, u32 channel, int volume) {
	SpuSetKey(0, 1 << channel);

	SPU_CHANNELS[channel].vol_left   = volume;
	SPU_CHANNELS[channel].vol_right  = volume;
	SPU_CHANNELS[channel].addr       = SPU_RAM_ADDR(addr);
	SPU_CHANNELS[channel].loop_addr  = SPU_RAM_ADDR(DUMMY_ADDR);
	SPU_CHANNELS[channel].freq       = 0x1000; // 44100 Hz
	SPU_CHANNELS[channel].adsr_param = 0x1fc080ff;

	SpuSetKey(1, 1 << channel);
}

void Audio_PlaySound(u32 addr, int volume) {
	Audio_PlaySoundOnChannel(addr, getFreeChannel(), volume);
   // printf("Could not find free channel to play sound (addr=%08x)\n", addr);
}

u32 Audio_LoadSound(const char *path)
{
	CdlFILE file;
	u32 Sound;

	IO_FindFile(&file, path);
	u32 *data = IO_ReadFile(&file);
	Sound = Audio_LoadVAGData(data, file.size);
	free(data);

	return Sound;
}