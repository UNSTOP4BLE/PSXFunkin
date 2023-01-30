/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//Most of this code is written by spicyjpeg

#include "../str.h"
#include <stdint.h>
#include <string.h>
#include <psxetc.h>
#include <psxapi.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxcd.h>
#include <psxpress.h>
#include <hwregs_c.h>
#include "../stage.h"
#include "../main.h"
#include "../gfx.h"

// Uncomment to display the video in 24bpp mode. Note that the GPU does not
// support 24bpp rendering, so the text overlay is only enabled in 16bpp mode.
//#define DISP_24BPP

/* CD and MDEC interrupt handlers */

#ifdef DISP_24BPP
#define BLOCK_SIZE 24
#else
#define BLOCK_SIZE 16
#define DRAW_OVERLAY
#endif

#define VRAM_X_COORD(x) ((x) * BLOCK_SIZE / 16)

// All non-audio sectors in .STR files begin with this 32-byte header, which
// contains metadata about the sector and is followed by a chunk of frame
// bitstream data.
// https://problemkaputt.de/psx-spx.htm#cdromfilevideostrstreamingandbspicturecompressionsony
typedef struct {
	uint16_t magic;			// Always 0x0160
	uint16_t type;			// 0x8001 for MDEC
	uint16_t sector_id;		// Chunk number (0 = first chunk of this frame)
	uint16_t sector_count;	// Total number of chunks for this frame
	uint32_t frame_id;		// Frame number
	uint32_t bs_length;		// Total length of this frame in bytes

	uint16_t width, height;
	uint8_t  bs_header[8];
	uint32_t _reserved;
} STR_Header;

typedef struct {
	uint16_t width, height;
	uint32_t bs_data[0x2000];	// Bitstream data read from the disc
	uint32_t mdec_data[0x8000];	// Decompressed data to be fed to the MDEC
} StreamBuffer;

typedef struct {
	StreamBuffer frames[2];
	uint32_t     slices[2][BLOCK_SIZE * 240 / 2];

	int  frame_id, sector_count;
	int  dropped_frames;
	RECT slice_pos;
	int  frame_width;

	volatile int8_t sector_pending, frame_ready;
	volatile int8_t cur_frame, cur_slice;
} StreamContext;

static StreamContext str_ctx;

// This buffer is used by cd_sector_handler() as a temporary area for sectors
// read from the CD. Due to DMA limitations it can't be allocated on the stack
// (especially not in the interrupt callbacks' stack, whose size is very
// limited).
static STR_Header sector_header;

void cd_sector_handler(void) {
	StreamBuffer *frame = &str_ctx.frames[str_ctx.cur_frame];

	// Fetch the .STR header of the sector that has been read and make sure it
	// is valid. If not, assume the file has ended and set frame_ready as a
	// signal for the main loop to stop playback.
	CdGetSector(&sector_header, sizeof(STR_Header) / 4);

	if (sector_header.magic != 0x0160) {
		str_ctx.frame_ready = -1;
		return;
	}

	// Ignore any non-MDEC sectors that might be present in the stream.
	if (sector_header.type != 0x8001)
		return;

	// If this sector is actually part of a new frame, validate the sectors
	// that have been read so far and flip the bitstream data buffers.
	if (sector_header.frame_id != str_ctx.frame_id) {
		// Do not set the ready flag if any sector has been missed.
		if (str_ctx.sector_count)
			str_ctx.dropped_frames++;
		else
			str_ctx.frame_ready = 1;

		str_ctx.frame_id     = sector_header.frame_id;
		str_ctx.sector_count = sector_header.sector_count;
		str_ctx.cur_frame   ^= 1;

		frame = &str_ctx.frames[str_ctx.cur_frame];

		// Initialize the next frame. Dimensions must be rounded up to the
		// nearest multiple of 16 as the MDEC operates on 16x16 pixel blocks.
		frame->width  = (sector_header.width  + 15) & 0xfff0;
		frame->height = (sector_header.height + 15) & 0xfff0;
	}

	// Append the payload contained in this sector to the current buffer.
	str_ctx.sector_count--;
	CdGetSector(
		&(frame->bs_data[2016 / 4 * sector_header.sector_id]),
		2016 / 4
	);
}

void mdec_dma_handler(void) {
	// Handle any sectors that were not processed by cd_event_handler() (see
	// below) while a DMA transfer from the MDEC was in progress. As the MDEC
	// has just finished decoding a slice, they can be safely handled now.
	if (str_ctx.sector_pending) {
		cd_sector_handler();
		str_ctx.sector_pending = 0;
	}

	// Upload the decoded slice to VRAM and start decoding the next slice (into
	// another buffer) if any.
	LoadImage(&str_ctx.slice_pos, str_ctx.slices[str_ctx.cur_slice]);

	str_ctx.cur_slice   ^= 1;
	str_ctx.slice_pos.x += BLOCK_SIZE;

	if (str_ctx.slice_pos.x < str_ctx.frame_width)
		DecDCTout(
			str_ctx.slices[str_ctx.cur_slice],
			BLOCK_SIZE * str_ctx.slice_pos.h / 2
		);
}

void cd_event_handler(CdlIntrResult event, uint8_t *payload) {
	// Ignore all events other than a sector being ready.
	if (event != CdlDataReady)
		return;

	// Only handle sectors immediately if the MDEC is not decoding a frame,
	// otherwise defer handling to mdec_dma_handler(). This is a workaround for
	// a hardware conflict between the DMA channels used for the CD drive and
	// MDEC output, which shall not run simultaneously.
	if (DecDCTinSync(1))
		str_ctx.sector_pending = 1;
	else
		cd_sector_handler();
}

/* Stream functions */

StreamBuffer *get_next_frame(void) {
	while (!str_ctx.frame_ready)
		__asm__ volatile("");

	if (str_ctx.frame_ready < 0)
		return 0;

	str_ctx.frame_ready = 0;
	return &str_ctx.frames[str_ctx.cur_frame ^ 1];
}

void STR_Init(void)
{
	InitGeom(); // GTE initialization required by the VLC decompressor
	DecDCTReset(0);
}

void STR_InitStream(void) {	
	EnterCriticalSection();
	DMACallback(1, &mdec_dma_handler);
	CdReadyCallback(&cd_event_handler);
	ExitCriticalSection();

	// Set the maximum amount of data DecDCTvlc() can output and copy the
	// lookup table used for decompression to the scratchpad area. This is
	// optional but makes the decompressor slightly faster. See the libpsxpress
	// documentation for more details.
	DecDCTvlcSize(0x8000);
	DecDCTvlcCopyTableV3((VLC_TableV3 *) 0x1f800000);

	str_ctx.cur_frame = 0;
	str_ctx.cur_slice = 0;
}

void STR_StartStream(const char* path) {
	STR_InitStream();
	CdlFILE file;

	if (!CdSearchFile(&file, path))
	{
		sprintf(error_msg, "[STR_StartStream] %s not found", path);
		ErrorLock();
	}
	stage.str_playing = true;

	str_ctx.frame_id       = -1;
	str_ctx.dropped_frames =  0;
	str_ctx.sector_pending =  0;
	str_ctx.frame_ready    =  0;

	CdSync(0, 0);

	// Configure the CD drive to read at 2x speed and to play any XA-ADPCM
	// sectors that might be interleaved with the video data.
	uint8_t mode = CdlModeRT | CdlModeSpeed;
	CdControl(CdlSetmode, (const uint8_t *) &mode, 0);

	// Start reading in real-time mode (i.e. without retrying in case of read
	// errors) and wait for the first frame to be buffered.
	CdControl(CdlReadS, &(file.pos), 0);

	get_next_frame();
}


void STR_StopStream(void)
{
	EnterCriticalSection();
	CdReadyCallback(NULL);
	DMACallback(DMA_MDEC_OUT, NULL);
	ExitCriticalSection();
	stage.str_playing = false;
}

void STR_Proccess(void)
{
	// Wait for a full frame to be read from the disc and decompress the
	// bitstream into the format expected by the MDEC. If the video has
	// ended, restart playback from the beginning.
	StreamBuffer *frame = get_next_frame();

	VLC_Context vlc_ctx;
	DecDCTvlcStart(&vlc_ctx, frame->mdec_data, sizeof(frame->mdec_data) / 4, frame->bs_data);

	// Wait for the MDEC to finish decoding the previous frame, then flip
	// the framebuffers to display it and prepare the buffer for the next
	// frame.
	// NOTE: as the refresh rate of the GPU is not synced to the video's
	// frame rate, this VSync(0) call may potentially end up waiting too
	// long and desynchronizing playback. A better solution would be to
	// implement triple buffering (i.e. always keep 2 fully decoded frames
	// in VRAM and use VSyncCallback() to register a function that displays
	// the next decoded frame if available whenever vblank occurs).
	VSync(0);
	DecDCTinSync(0);
	DecDCToutSync(0);

	FntFlush(-1);
	db ^= 1;

	DrawSync(0);
	//VSync(0);

	PutDrawEnv(&(stage.draw[db]));
	PutDispEnv(&(stage.disp[db]));
	SetDispMask(1);

	// Feed the newly decompressed frame to the MDEC. The MDEC will not
	// actually start decoding it until an output buffer is also configured
	// by calling DecDCTout() (see below).
#ifdef DISP_24BPP
	DecDCTin(frame->mdec_data, DECDCT_MODE_24BPP);
#else
	DecDCTin(frame->mdec_data, DECDCT_MODE_16BPP);
#endif

	// Place the frame at the center of the currently active framebuffer
	// and start decoding the first slice. Decoded slices will be uploaded
	// to VRAM in the background by mdec_dma_handler().
	RECT *fb_clip = &(stage.draw[db].clip);
	int  x_offset = (fb_clip->w - frame->width)  / 2;
	int  y_offset = (fb_clip->h - frame->height) / 2;
	str_ctx.slice_pos.x = fb_clip->x + VRAM_X_COORD(x_offset);
	str_ctx.slice_pos.y = fb_clip->y + y_offset;
	str_ctx.slice_pos.w = BLOCK_SIZE;
	str_ctx.slice_pos.h = frame->height;
	str_ctx.frame_width = VRAM_X_COORD(frame->width);
	DecDCTout(
		str_ctx.slices[str_ctx.cur_slice],
		BLOCK_SIZE * str_ctx.slice_pos.h / 2
	);
}