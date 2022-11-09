/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//Most of this code is written by spicyjpeg

#include "../audio.h"             
#include "../main.h"
#include <hwregs_c.h>

#define SWAP_ENDIAN(x) ( \
	(((uint32_t) (x) & 0x000000ff) << 24) | \
	(((uint32_t) (x) & 0x0000ff00) <<  8) | \
	(((uint32_t) (x) & 0x00ff0000) >>  8) | \
	(((uint32_t) (x) & 0xff000000) >> 24) \
)

typedef struct {
	uint32_t *read_buffer;
	int lba, chunk_secs;
	int buffer_size, num_chunks, sample_rate;

	volatile int    next_chunk, spu_addr;
	volatile int8_t db_active, state;

	// used for timing
	volatile uint32_t last_irq;
	uint32_t last_paused, chunk_ticks;
} StreamContext;

static StreamContext str_ctx;

static void spu_irq_handler(void) {
	// Acknowledge the interrupt to ensure it can be triggered again. The only
	// way to do this is actually to disable the interrupt entirely; we'll
	// enable it again once the chunk is ready.
	SPU_CTRL &= 0xffbf;

	int chunk_size = str_ctx.buffer_size * NUM_CHANNELS;
	int chunk      = (str_ctx.next_chunk + 1) % (uint32_t) str_ctx.num_chunks;

	str_ctx.db_active ^= 1;
-	str_ctx.state      = STATE_BUFFERING;
	str_ctx.next_chunk = chunk;
	str_ctx.last_irq   = get_time();

	// Configure to SPU to trigger an IRQ once the chunk that is going to be
	// filled now starts playing (so the next buffer can be loaded) and
	// override both channels' loop addresses to make them "jump" to the new
	// buffers, rather than actually looping when they encounter the loop flag
	// at the end of the currently playing buffers.
	int addr = BUFFER_START_ADDR + (str_ctx.db_active ? chunk_size : 0);
	str_ctx.spu_addr = addr;

	SPU_IRQ_ADDR = getSPUAddr(addr);
	for (int i = 0; i < NUM_CHANNELS; i++)
		SPU_CH_LOOP_ADDR(i) = getSPUAddr(addr + str_ctx.buffer_size * i);

	// Start uploading the next chunk to the SPU.
	SpuSetTransferStartAddr(addr);
	SpuWrite(str_ctx.read_buffer, chunk_size);
}

static u32 get_time(void) {
	return
		(timer_irq_count << TIMER_SHIFT) |
		(TIMER_VALUE(2)  >> (16 - TIMER_SHIFT));
}

static u32 get_time_ms(void) {
	return (get_time() * 1000) / TICKS_PER_SEC;
}

static void reset_spu_channels(void) {
	SpuSetKey(0, 0x00ffffff);

	for (int i = 0; i < 24; i++) {
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);
		SPU_CH_FREQ(i) = 0x1000;
	}

	SpuSetKey(1, 0x00ffffff);
}


//Audio functions


void Audio_Init() {
	SpuInit();
	reset_spu_channels();
}

void Audio_LoadStream(const char *path) {
	CdlFILE file;
	printf("OPENING STREAM FILE\n");
	if (!CdSearchFile(&file, path))
	{
		sprintf(error_msg, "[Audio_LoadStream] failed to find stream file");
		ErrorLock();
	}

	printf("BUFFERING STREAM\n");

	EnterCriticalSection();
	InterruptCallback(IRQ_SPU, &spu_irq_handler);
	DMACallback(DMA_SPU, &spu_dma_handler);
	ExitCriticalSection();

	// Read the header. Note that in interleaved .VAG files the first sector.
	// does not hold any audio data (i.e. the header is padded to 2048 bytes).
	uint32_t header[512];
	CdControl(CdlSetloc, file.pos, 0);

	CdReadCallback(0);
	CdRead(1, header, CdlModeSpeed);
	CdReadSync(0, 0);

	VAG_Header *vag = (VAG_Header *) header;
	int buf_size    = vag->interleave;
	int chunk_secs  = ((buf_size * NUM_CHANNELS) + 2047) / 2048;

	str_ctx.read_buffer = malloc(chunk_secs * 2048);
	str_ctx.lba         = CdPosToInt(file.pos) + 1;
	str_ctx.chunk_secs  = chunk_secs;
	str_ctx.buffer_size = buf_size;
	str_ctx.num_chunks  = (SWAP_ENDIAN(vag->size) + buf_size - 1) / buf_size;
	str_ctx.sample_rate = SWAP_ENDIAN(vag->sample_rate);
	str_ctx.chunk_ticks = (buf_size / 16) * (TICKS_PER_SEC * 28);
	str_ctx.db_active  = 1;
	str_ctx.next_chunk = 0;

	// Ensure at least one chunk is in SPU RAM (and one in main RAM) by
	// invoking the IRQ handler manually and blocking until the chunk has
	// loaded.
	str_ctx.state = STATE_DATA_NEEDED;
	while (str_ctx.state != STATE_IDLE)
		feed_stream();

	spu_irq_handler();
	while (str_ctx.state != STATE_IDLE)
		feed_stream();
}

void Audio_StartStream(void) {
	int bits = 0x00ffffff >> (24 - NUM_CHANNELS);

	for (int i = 0; i < NUM_CHANNELS; i++) {
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

	spu_irq_handler();
	SpuSetKey(1, bits);
	str_ctx.last_paused = 0;
}

void Audio_StopStream(void) {
	int bits = 0x00ffffff >> (24 - NUM_CHANNELS);

	SpuSetKey(0, bits);
	str_ctx.last_paused = get_time();

	for (int i = 0; i < NUM_CHANNELS; i++)
		SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);

	SpuSetKey(1, bits);
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

u32 Audio_GetTime(void) {
	u32 chunk_start_time = (str_ctx.next_chunk - 2) *
		str_ctx.chunk_ticks / str_ctx.sample_rate;

	u32 current_time = str_ctx.last_paused;
	if (!current_time)
		current_time = get_time();

	return chunk_start_time + (current_time - str_ctx.last_irq);
}

u32 Audio_GetTimeMS(void) {
	return (Audio_GetTime() * 1000) / TICKS_PER_SEC;
}