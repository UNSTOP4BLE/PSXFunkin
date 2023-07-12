/*
 * PSn00bSDK SPU CD-ROM streaming example
 * (C) 2022-2023 spicyjpeg - MPL licensed
 *
 * This is an extended version of the sound/spustream example demonstrating
 * playback of a large multi-channel audio file from the CD-ROM using the SPU,
 * without having to rely on the CD drive's own ability to play CD-DA or XA
 * tracks.
 *
 * A ring buffer takes the place of the stream_data array from the spustream
 * example. This buffer is filled from the CD-ROM by the main thread and drained
 * by the SPU IRQ handler, which pulls a single chunk at a time out of it and
 * transfers it to SPU RAM for playback. The feed_stream() function handles
 * fetching chunks, which are read once again from an interleaved .VAG file laid
 * out on the disc as follows:
 *
 *  +--Sector--+--Sector--+--Sector--+--Sector--+--Sector--+--Sector--+----
 *  |          | +--------------------+---------------------+         |
 *  |   .VAG   | | Left channel data  | Right channel data  | Padding | ...
 *  |  header  | +--------------------+---------------------+         |
 *  +----------+----------+----------+----------+----------+----------+----
 *               \__________________Chunk___________________/
 *
 * Note that the ring buffer must be large enough to give the drive enough time
 * to seek from one chunk to another. A larger buffer will take up more main RAM
 * but will not influence SPU RAM usage, which depends only on the chunk size
 * (interleave) and channel count of the .VAG file. Generally, interleave values
 * in the 2048-4096 byte range work well (the interleaving script in the
 * spustream directory uses 4096 bytes by default).
 *
 * Implementing SPU streaming might seem pointless, but it actually has a number
 * of advantages over CD-DA or XA:
 *
 * - Any sample rate up to 44.1 kHz can be used. The sample rate can also be
 *   changed on-the-fly to play the stream at different speeds and pitches (as
 *   long as the CD drive can keep up), or even interpolated for effects like
 *   tape stops.
 * - Manual streaming is not limited to mono or stereo but can be expanded to as
 *   many channels as needed, only limited by the amount of SPU RAM required for
 *   chunks and CD bandwidth. Having more than 2 channels can be useful for e.g.
 *   smoothly crossfading between tracks (not possible with XA) or controlling
 *   volume and panning of each instrument separately.
 * - XA playback tends to skip on consoles with a worn out drive, as XA sectors
 *   cannot have any error correction data. SPU streaming is not subject to this
 *   limitation since sectors are read and processed in software.
 * - Depending on how streaming/interleaving is implemented it is possible to
 *   have 500-1000ms idle periods during which the CD drive isn't buffering the
 *   stream, that can be used to read small amounts of other data without ever
 *   interrupting playback. This is different from XA-style interleaving as the
 *   drive is free to seek to *any* region of the disc during these periods (it
 *   must seek back to the stream's next chunk afterwards though).
 * - It is also possible to seek back to the beginning of the stream and load
 *   the first chunk before the end is reached, allowing for seamless looping
 *   without having to resort to tricks like separate filler samples.
 * - Finally, SPU streaming can be used on some PS1-based arcade boards that use
 *   IDE/SCSI drives or flash memory for storage and thus lack support for XA or
 *   CD-DA playback.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <stdio.h> 
#include <psxpad.h>
#include <psxspu.h>
#include <psxcd.h>
#include <hwregs_c.h>

#include "stream.h"
#include "../io.h"

#include "../timer.h"
#include "../main.h"

// Size of the ring buffer in main RAM in bytes.
#define RAM_BUFFER_SIZE 0x18000
int buffers_size = 0;
// Minimum number of sectors that will be read from the CD-ROM at once. Higher
// values will improve efficiency at the cost of requiring a larger buffer in
// order to prevent underruns and glitches in the audio output.
#define REFILL_THRESHOLD 24

/* .VAG header structure */

typedef struct {
    uint32_t magic;         // 0x69474156 ("VAGi") for interleaved files
    uint32_t version;
    uint32_t interleave;    // Little-endian, size of each channel buffer
    uint32_t size;          // Big-endian, in bytes
    uint32_t sample_rate;   // Big-endian, in Hertz
    uint16_t _reserved[5];
    uint16_t channels;      // Little-endian, channel count (stereo if 0)
    char     name[16];
} VAG_Header;

#define SWAP_ENDIAN(x) ( \
    (((uint32_t) (x) & 0x000000ff) << 24) | \
    (((uint32_t) (x) & 0x0000ff00) <<  8) | \
    (((uint32_t) (x) & 0x00ff0000) >>  8) | \
    (((uint32_t) (x) & 0xff000000) >> 24) \
)

/* Interrupt callbacks */

#define DUMMY_BLOCK_ADDR   0x1000
#define STREAM_BUFFER_ADDR 0x1010

typedef struct {
    int start_lba, stream_length, sample_rate;
    bool loop;

    volatile int    next_sector;
    volatile size_t refill_length;
} StreamReadContext;

static Stream_Context    stream_ctx;
static StreamReadContext read_ctx;

void cd_read_handler(CdlIntrResult event, uint8_t *payload) {
    // Mark the data as valid.
    if (event != CdlDiskError)
        Stream_Feed(&stream_ctx, read_ctx.refill_length * 2048);
}

/* Helper functions */

// This isn't actually required for this example, however it is necessary if the
// stream buffers are going to be allocated into a region of SPU RAM that was
// previously used (to make sure the IRQ is not going to be triggered by any
// inactive channels).
#define ALLOC_START_ADDR 0x1010
static volatile uint32_t audio_alloc_ptr = 0;
void Audio_ClearAlloc(void) {
    audio_alloc_ptr = ALLOC_START_ADDR;
}

void Audio_ResetChannels(void) {
    SpuSetKey(0, 0x00ffffff);

    for (int i = 0; i < 24; i++) {
        SPU_CH_ADDR(i) = getSPUAddr(DUMMY_BLOCK_ADDR);
        SPU_CH_FREQ(i) = 0x1000;
    }

    SpuSetKey(1, 0x00ffffff);
}

void Audio_Init(void) {
    SpuInit();
    Audio_ResetChannels();
    Audio_ClearAlloc();
}

bool Audio_FeedStream(void) {
    // Do nothing if the drive is already busy reading a chunk.
    if (CdReadSync(1, 0) > 0)
        return true;

    // To improve efficiency, do not start refilling immediately but wait until
    // there is enough space in the buffer (see REFILL_THRESHOLD).
    if (Stream_GetRefillLength(&stream_ctx) < (REFILL_THRESHOLD * 2048))
        return false;

    uint8_t *ptr;
    size_t  refill_length = Stream_GetFeedPtr(&stream_ctx, &ptr) / 2048;

    // Figure out how much data can be read in one shot. If the end of the file
    // would be reached before the buffer is full, split the read into two
    // separate reads.
    int next_sector = read_ctx.next_sector;
    int max_length  = read_ctx.stream_length - next_sector;
    
    if (read_ctx.loop) {
        while (max_length <= 0) {
            next_sector -= read_ctx.stream_length;
            max_length  += read_ctx.stream_length;
        }
    } else {
        if (max_length <= 0)
            return false;
    }

    if (refill_length > max_length)
        refill_length = max_length;

    // Start reading the next chunk from the CD-ROM into the buffer.
    CdlLOC pos;

    CdIntToPos(read_ctx.start_lba + next_sector, &pos);
    CdControl(CdlSetloc, &pos, 0);
    CdReadCallback(&cd_read_handler);
    CdRead(refill_length, (uint32_t *) ptr, CdlModeSpeed);

    read_ctx.next_sector   = next_sector + refill_length;
    read_ctx.refill_length = refill_length;

    return true;
}

void Audio_LoadStream(const char *path, bool loop) {
    Audio_ClearAlloc();
    CdlFILE file;
    if (!CdSearchFile(&file, path))
    {
        sprintf(error_msg, "[Audio_LoadStream] cant find %s", path);
        ErrorLock();
    }

    uint32_t header[512];
    CdControl(CdlSetloc, &file.pos, 0);

    CdReadCallback(0);
    CdRead(1, header, CdlModeSpeed);
    CdReadSync(0, 0);

    VAG_Header    *vag = (VAG_Header *) header;
    Stream_Config config;

    int num_channels = vag->channels ? vag->channels : 2;
    int num_chunks   =
        (SWAP_ENDIAN(vag->size) + vag->interleave - 1) / vag->interleave;

    __builtin_memset(&config, 0, sizeof(Stream_Config));

    config.spu_address = STREAM_BUFFER_ADDR;
    config.interleave  = vag->interleave;
    config.buffer_size = RAM_BUFFER_SIZE;
    config.sample_rate = SWAP_ENDIAN(vag->sample_rate);
    config.timer_function    = &Timer_GetTimeint32;
    config.timer_rate        = TICKS_PER_SEC;

    // Use the first N channels of the SPU and pan them left/right in pairs
    // (this assumes the stream contains one or more stereo tracks).
    for (int ch = 0; ch < num_channels; ch++) {
        config.channel_mask = (config.channel_mask << 1) | 1;

        SPU_CH_VOL_L(ch) = (ch % 2) ? 0x0000 : 0x3fff;
        SPU_CH_VOL_R(ch) = (ch % 2) ? 0x3fff : 0x0000;
    }

    Stream_Init(&stream_ctx, &config);
    buffers_size = stream_ctx.chunk_size * 2;
    printf("buf size%d,\n", buffers_size);

    read_ctx.start_lba     = CdPosToInt(&file.pos) + 1;
    read_ctx.stream_length =
        (num_channels * num_chunks * vag->interleave + 2047) / 2048;
    read_ctx.sample_rate   = config.sample_rate;
    read_ctx.next_sector   = 0;
    read_ctx.refill_length = 0;
    stream_ctx.samples       = (SWAP_ENDIAN(vag->size) / 16) * 28;
    stream_ctx.sample_rate   = read_ctx.sample_rate;
    read_ctx.loop = loop;

    // Ensure the buffer is full before starting playback.
    while (Audio_FeedStream())
        __asm__ volatile("");
}

void Audio_StartStream(bool resume) {
    Stream_Start(&stream_ctx, resume);
}

void Audio_StopStream(void) {
    Stream_Stop();
}

void Audio_DestroyStream(void) {
    Stream_Destroy(&stream_ctx);
}

bool Audio_IsPlaying(void) {
    return Stream_IsActive(&stream_ctx);
}

uint64_t Audio_GetTime(int unit) {
    if (stream_ctx.sample_rate != 0)
        return ((uint64_t) Stream_GetSamplesPlayed(&stream_ctx)) * ((uint64_t) unit) / ((uint64_t) stream_ctx.sample_rate);

    return 0;
}

uint32_t Audio_GetInitialTime(void) {
    if (stream_ctx.sample_rate != 0)
        return stream_ctx.samples / stream_ctx.sample_rate;
    else return 0;
}

void Audio_SetVolume(uint8_t i, uint16_t vol_left, uint16_t vol_right) {
    SPU_CH_VOL_L(i) = vol_left;
    SPU_CH_VOL_R(i) = vol_right;
}

//vag sillies
#define VAG_HEADER_SIZE 48

static uint8_t lastChannelUsed = 0;

static uint8_t getFreeChannel(void) {
    uint8_t channel = lastChannelUsed;
    lastChannelUsed = (channel + 1) % 20;
    return channel + 4;
}

void Audio_PlaySound(uint32_t addr, int volume) {
    uint8_t channel = getFreeChannel();
   
    // Make sure the channel is stopped.
    SpuSetKey(0, 1 << channel);

    // Set the channel's sample rate and start address. Note that the SPU
    // expects the sample rate to be in 4.12 fixed point format (with
    // 1.0 = 44100 Hz) and the address in 8-byte units; psxspu.h provides the
    // getSPUSampleRate() and getSPUAddr() macros to convert values to these
    // units.
    SPU_CH_FREQ(channel) = 0x1000;//getSPUSampleRate();
    SPU_CH_ADDR(channel) = getSPUAddr(addr);

    // Set the channel's volume and ADSR parameters (0x80ff and 0x1fee are
    // dummy values that disable the ADSR envelope entirely).
    SPU_CH_VOL_L(channel) = volume;
    SPU_CH_VOL_R(channel) = volume;
    SPU_CH_ADSR1(channel) = 0x00ff;
    SPU_CH_ADSR2(channel) = 0x0000;

    // Start the channel.
    SpuSetKey(1, 1 << channel);
}

uint32_t Audio_LoadSound(const char *path) {    
    //Load Sound File
    CdlFILE sfx_file;
    IO_FindFile(&sfx_file, path);
    IO_Data sfx_data = IO_ReadFile(&sfx_file);

    // subtract size of .vag header (48 bytes), round to 64 bytes
    uint32_t xfer_size = ((sfx_file.size - VAG_HEADER_SIZE) + 63) & 0xffffffc0;

    uint8_t  *data = (uint8_t *) sfx_data;

    // modify sound data to ensure sound "loops" to dummy sample
    // https://psx-spx.consoledev.net/soundprocessingunitspu/#flag-bits-in-2nd-byte-of-adpcm-header
    data[sfx_file.size - 15] = 1; // end + mute

    // allocate SPU memory for sound
    audio_alloc_ptr  += buffers_size;
    uint32_t addr = audio_alloc_ptr;
    audio_alloc_ptr += xfer_size;

    if (audio_alloc_ptr > 0x80000) {
        // TODO: add proper error handling code
        printf("FATAL: SPU RAM overflow! (%d bytes overflowing)\n", audio_alloc_ptr - 0x80000);
        while (1);
    }

    SpuSetTransferStartAddr(addr); // set transfer starting address to malloced area
    SpuSetTransferMode(SPU_TRANSFER_BY_DMA); // set transfer mode to DMA
    SpuWrite((uint32_t *)data + VAG_HEADER_SIZE, xfer_size); // perform actual transfer
    SpuIsTransferCompleted(SPU_TRANSFER_WAIT); // wait for DMA to complete

    printf("Allocated new sound (addr=%08x, size=%d)\n", addr, xfer_size);

    free(sfx_data);
    return addr;
}
