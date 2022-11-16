#ifdef DISP_24BPP
#define BLOCK_SIZE 24
#else
#define BLOCK_SIZE 16
#define DRAW_OVERLAY
#endif

#define VRAM_X_COORD(x) ((x) * BLOCK_SIZE / 16)

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
	uint8_t file, channel;
	uint8_t submode, coding_info;
} XA_Header;

typedef struct {
	CdlLOC     pos;
	XA_Header  xa_header[2];
	STR_Header str_header;
	uint8_t    data[2016];
	uint32_t   edc;
	uint8_t    ecc[276];
} STR_Sector;

typedef struct {
	uint16_t width, height;
	uint32_t bs_data[0x2000];	// Bitstream data read from the disc
	uint32_t mdec_data[0x8000];	// Decompressed data to be fed to the MDEC
} StreamBuffer;

typedef struct {
	StreamBuffer frames[2];
	uint32_t     slices[2][BLOCK_SIZE * SCREEN_YRES / 2];

	int  frame_id, sector_count;
	int  dropped_frames;
	RECT slice_pos;
	int  frame_width;

	volatile int8_t sector_pending, frame_ready;
	volatile int8_t cur_frame, cur_slice;
} StreamContext;

StreamContext str_ctx;
STR_Sector sector_buffer;

static void cd_sector_handler(void) {
	// Fetch the .STR header of the sector that has been read and check if the
	// end-of-file bit is set in the XA header.
	CdGetSector(&sector_buffer, sizeof(STR_Sector) / 4);

	if (
		(sector_buffer.xa_header[0].submode & (1 << 7)) ||
		(sector_buffer.xa_header[1].submode & (1 << 7))
	) {
		CdControlF(CdlPause, 0);
		str_ctx.frame_ready = -1;
		return;
	}

	STR_Header   *header = &sector_buffer.str_header;
	StreamBuffer *frame  = &str_ctx.frames[str_ctx.cur_frame];

	// Ignore any non-MDEC sectors that might be present in the stream.
	if (header->type != 0x8001)
		return;

	// If this sector is actually part of a new frame, validate the sectors
	// that have been read so far and flip the bitstream data buffers.
	if (header->frame_id != str_ctx.frame_id) {
		// Do not set the ready flag if any sector has been missed.
		if (str_ctx.sector_count)
			str_ctx.dropped_frames++;
		else
			str_ctx.frame_ready = 1;

		str_ctx.frame_id     = header->frame_id;
		str_ctx.sector_count = header->sector_count;
		str_ctx.cur_frame   ^= 1;

		frame = &str_ctx.frames[str_ctx.cur_frame];

		// Initialize the next frame. Dimensions must be rounded up to the
		// nearest multiple of 16 as the MDEC operates on 16x16 pixel blocks.
		frame->width  = (header->width  + 15) & 0xfff0;
		frame->height = (header->height + 15) & 0xfff0;
	}

	// Append the payload contained in this sector to the current buffer.
	memcpy(
		&(frame->bs_data[2016 / 4 * header->sector_id]),
		sector_buffer.data,
		2016
	);
	str_ctx.sector_count--;
}

static void mdec_dma_handler(void) {
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

static void cd_event_handler(int event, uint8_t *payload) {
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

/* Stream helpers */
int decode_errors = 0;

void STR_Init(void)
{
	InitGeom(); // Required for PSn00bSDK's DecDCTvlc()
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
	DecDCTvlcCopyTable((DECDCTTAB *) 0x1f800000);

	str_ctx.dropped_frames = 0;
	str_ctx.cur_frame      = 0;
	str_ctx.cur_slice      = 0;
}

void STR_StartStream(const char* path) {

	CdlFILE file;
	if (!CdSearchFile(&file, path))
		SHOW_ERROR("FAILED TO FIND VIDEO.STR\n");

	str_ctx.frame_id       = -1;
	str_ctx.sector_pending =  0;
	str_ctx.frame_ready    =  0;

	CdSync(0, 0);

	// Configure the CD drive to read 2340-byte sectors at 2x speed and to
	// play any XA-ADPCM sectors that might be interleaved with the video data.
	uint8_t mode = CdlModeSize | CdlModeRT | CdlModeSpeed;
	CdControl(CdlSetmode, (const uint8_t *) &mode, 0);

	// Start reading in real-time mode (i.e. without retrying in case of read
	// errors) and wait for the first frame to be buffered.
	CdControl(CdlReadS, &(file.pos), 0);

	get_next_frame();
}

void STR_Proccess(void)
{
		// Wait for a full frame to be read from the disc and decompress the
		// bitstream into the format expected by the MDEC. If the video has
		// ended, restart playback from the beginning.
		StreamBuffer *frame = get_next_frame();
		if (!frame) {
			start_stream(&file);
			continue;
		}

		if (DecDCTvlc(frame->bs_data, frame->mdec_data)) {
			decode_errors++;
			continue;
		}


		// Wait for the MDEC to finish decoding the previous frame, then flip
		// the framebuffers to display it and prepare the buffer for the next
		// frame.
		// NOTE: you should *not* call VSync(0) during playback, as the refresh
		// rate of the GPU is not synced to the video's frame rate. If you want
		// to minimize screen tearing, consider triple buffering instead (i.e.
		// always keep 2 fully decoded frames in VRAM and use VSyncCallback()
		// to register a function that displays the next decoded frame whenever
		// vblank occurs).
		DecDCTinSync(0);
		DecDCToutSync(0);

#ifdef DRAW_OVERLAY
		FntPrint(-1, "FRAME:%5d    READ ERRORS:  %5d\n", str_ctx.frame_id, str_ctx.dropped_frames);
		FntPrint(-1, "CPU:  %5d%%   DECODE ERRORS:%5d\n", cpu_usage, decode_errors);
		FntFlush(-1);
#endif
}

static StreamBuffer *get_next_frame(void) {
	while (!str_ctx.frame_ready)
		__asm__ volatile("");

	if (str_ctx.frame_ready < 0)
		return 0;

	str_ctx.frame_ready = 0;
	return &str_ctx.frames[str_ctx.cur_frame ^ 1];
}

/* Main */

static RenderContext ctx;

#define SHOW_STATUS(...) { FntPrint(-1, __VA_ARGS__); FntFlush(-1); display(&ctx, 1); }
#define SHOW_ERROR(...)  { SHOW_STATUS(__VA_ARGS__); while (1) __asm__("nop"); }

int main(int argc, const char* argv[]) {


	SpuInit();
	CdInit();
	


	init_stream();
	start_stream(&file);

	// Disable framebuffer clearing to get rid of flickering during playback.
	display(&ctx, 1);
	ctx.db[0].draw.isbg = 0;
	ctx.db[1].draw.isbg = 0;
#ifdef DISP_24BPP
	ctx.db[0].disp.isrgb24 = 1;
	ctx.db[1].disp.isrgb24 = 1;
#endif


	while (1) {
		display(&ctx, 0);

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
		RECT *fb_clip = &(ctx.db[ctx.db_active].draw.clip);
		int  x_offset = (fb_clip->w - frame->width)  / 2;
		int  y_offset = (fb_clip->h - frame->height) / 2;

		str_ctx.slice_pos.x = VRAM_X_COORD(fb_clip->x + x_offset);
		str_ctx.slice_pos.y = fb_clip->y + y_offset;
		str_ctx.slice_pos.w = BLOCK_SIZE;
		str_ctx.slice_pos.h = frame->height;
		str_ctx.frame_width = VRAM_X_COORD(frame->width);

		DecDCTout(
			str_ctx.slices[str_ctx.cur_slice],
			BLOCK_SIZE * str_ctx.slice_pos.h / 2
		);
	}

	return 0;
}
