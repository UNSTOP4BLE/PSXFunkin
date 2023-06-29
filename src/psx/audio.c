/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../audio.h"
#include <hwregs_c.h>
#include <stdlib.h>

#include "../io.h"
#include "../main.h"

//XA state
#define XA_STATE_INIT    (1 << 0)
#define XA_STATE_PLAYING (1 << 1)
#define XA_STATE_LOOPS   (1 << 2)
#define XA_STATE_SEEKING (1 << 3)

static uint8_t xa_state, xa_resync, xa_volume, xa_channel;
static uint32_t xa_pos, xa_start;

//audio stuff
#define XA_FLAG_EOR (1 << 0)
#define XA_FLAG_EOF (1 << 7)
#define BUFFER_SIZE (13 << 11) //13 sectors
#define CHUNK_SIZE (BUFFER_SIZE)
#define CHUNK_SIZE_MAX (BUFFER_SIZE * 4) // there are never more than 4 channels

#define BUFFER_TIME FIXED_DEC(((BUFFER_SIZE * 28) / 16), 44100)

#define BUFFER_START_ADDR 0x1010
#define DUMMY_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2))
#define ALLOC_START_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2) + 64)

//SPU registers
typedef struct
{
    uint16_t vol_left;
    uint16_t vol_right;
    uint16_t freq;
    uint16_t addr;
    uint32_t adsr_param;
    uint16_t _reserved;
    uint16_t loop_addr;
} Audio_SPUChannel;

#define SPU_CHANNELS    ((volatile Audio_SPUChannel*)0x1f801c00)
#define SPU_RAM_ADDR(x) ((uint16_t)(((uint32_t)(x)) >> 3))

static volatile uint32_t audio_alloc_ptr = 0;

//XA files and tracks
static CdlFILE xa_files[XA_Max];

#include "../audio_def.h"

static void IO_SeekFile(CdlFILE *file)
{
    //Seek to file position
    CdControl(CdlSetloc, (uint8_t*)&file->pos, NULL);
    CdControlB(CdlSeekL, NULL, NULL);
}

uint32_t Audio_GetLength(XA_Track lengthtrack)
{
    return (xa_tracks[lengthtrack].length / 75) / IO_SECT_SIZE;
}

//Internal XA functions
static uint8_t XA_BCD(uint8_t x)
{
    return x - 6 * (x >> 4);
}

static uint32_t XA_TellSector(void)
{
    uint8_t result[8];
    CdControl(CdlGetlocL, NULL, result);
    if (result[6] & (XA_FLAG_EOF | XA_FLAG_EOR))
        return -1; // file ended

    return CdPosToInt((CdlLOC *) result);
}

static void XA_SetVolume(uint8_t x)
{
    //Set CD mix volume
    CdlATV cd_vol;
    xa_volume = cd_vol.val0 = cd_vol.val1 = cd_vol.val2 = cd_vol.val3 = x;
    CdMix(&cd_vol);
}

static void XA_Init(void)
{
    uint8_t param[4];
    
    //Set XA state
    if (xa_state & XA_STATE_INIT)
        return;
    xa_state = XA_STATE_INIT;
    xa_resync = 0;

    //Set volume
    SPU_CD_VOL_L = 0x6000;
    SPU_CD_VOL_R = 0x6000;

    //Set initial volume
    XA_SetVolume(0);
    
    //Prepare CD drive for XA reading
    param[0] = CdlModeRT | CdlModeSF | CdlModeSize;
    
    CdControl(CdlSetmode, param, NULL);
    CdControlB(CdlPause, NULL, NULL);
}

static void XA_Quit(void)
{
    //Set XA state
    if (!(xa_state & XA_STATE_INIT))
        return;
    xa_state = 0;
    
    //Stop playing XA
    XA_SetVolume(0);
    CdControlB(CdlPause, NULL, NULL);
}

static void XA_Play(uint32_t start)
{
    //Play at given position
    CdlLOC cd_loc;
    CdIntToPos(start, &cd_loc);
    CdControl(CdlSetloc, (uint8_t*)&cd_loc, NULL);
    CdControl(CdlReadS, NULL, NULL);
}

static void XA_WaitPlay(void) {
    CdControl(CdlNop, NULL, NULL);
    while (!(CdStatus() & CdlStatRead))
        CdControl(CdlNop, NULL, NULL);
}

static void XA_Pause(void)
{
    //Set XA state
    if (!(xa_state & XA_STATE_PLAYING))
        return;
    xa_state &= ~XA_STATE_PLAYING;
    
    //Pause playback
    CdControlB(CdlPause, NULL, NULL);
}

static void XA_SetFilter(uint8_t channel)
{
    //Change CD filter
    CdlFILTER filter;
    filter.file = 1;
    xa_channel = filter.chan = channel;
    CdControlF(CdlSetfilter, (uint8_t*)&filter);
}

//Audio functions
void Audio_Init(void)
{
    //Initialize SPU
    SpuInit();
    Audio_ClearAlloc();
    
    //Set volume (this is done by default but i just put it here cus why not)
    SPU_MASTER_VOL_L = 0x3fff;
    SPU_MASTER_VOL_R = 0x3fff;
    
    //Set XA state
    xa_state = 0;
    
    //Get file positions
    CdlFILE *filep = xa_files;
    for (const char **pathp = xa_paths; *pathp != NULL; pathp++)
        IO_FindFile(filep++, *pathp);
}

void Audio_Quit(void)
{
    
}

static void Audio_GetXAFile(CdlFILE *file, XA_Track track)
{
    const XA_TrackDef *track_def = &xa_tracks[track];
    file->pos = xa_files[track_def->file].pos;
    file->size = track_def->length;
}

static void Audio_PlayXA_File(CdlFILE *file, uint8_t volume, uint8_t channel, bool loop)
{
    //Initialize XA system and stop previous song
    XA_Init();
    XA_SetVolume(0);
    
    //Set XA state
    xa_start = xa_pos = CdPosToInt(&file->pos);
    //xa_end = xa_start + (file->size / IO_SECT_SIZE) - 1;
    xa_state = XA_STATE_INIT | XA_STATE_PLAYING | XA_STATE_SEEKING;
    xa_resync = 0;
    if (loop)
        xa_state |= XA_STATE_LOOPS;
    
    //Start seeking to XA and use parameters
    IO_SeekFile(file);
    XA_SetFilter(channel);
    XA_SetVolume(volume);
}

void Audio_PlayXA_Track(XA_Track track, uint8_t volume, uint8_t channel, bool loop)
{
    //Get track information
    CdlFILE file;
    Audio_GetXAFile(&file, track);

    //Play track
    Audio_PlayXA_File(&file, volume, channel, loop);
}

void Audio_SeekXA_Track(XA_Track track)
{
    //Get track file and seek
    CdlFILE file;
    Audio_GetXAFile(&file, track);
    IO_SeekFile(&file);
}

void Audio_PauseXA(void)
{
    //Pause playing XA file
    XA_Pause();
}

void Audio_ResumeXA(void)
{
    if (xa_state & XA_STATE_PLAYING)
        return;
    xa_state |= XA_STATE_PLAYING;

    XA_Play(xa_pos);
    XA_WaitPlay();
}

void Audio_StopXA(void)
{
    //Deinitialize XA system
    XA_Quit();
}

void Audio_ChannelXA(uint8_t channel)
{
    //Set XA filter to the given channel
    XA_SetFilter(channel);
}

int32_t Audio_TellXA_Sector(void)
{
    //Get CD position
    return (int32_t)xa_pos - (int32_t)xa_start; //Meh casting
}

int32_t Audio_TellXA_Milli(void)
{  
    int pos = XA_TellSector();
    if (pos != -1)  
        return ((int32_t)pos - (int32_t)xa_start) * 1000 / 75; //1000 / (75 * speed (1x))
}

bool Audio_PlayingXA(void)
{
    return (xa_state & XA_STATE_PLAYING) != 0;
}

void Audio_WaitPlayXA(void)
{
    while (1)
    {
        Audio_ProcessXA();
        if (Audio_PlayingXA())
            return;
        VSync(0);
    }   

    XA_WaitPlay();
}

void Audio_ProcessXA(void)
{
    //Handle playing state
    if (xa_state & XA_STATE_PLAYING)
    {
        //Retrieve CD status
        CdControl(CdlNop, NULL, NULL);
        uint8_t cd_status = CdStatus();
        
        //Handle resync timer
        if (xa_resync != 0)
        {
            //Wait for resync timer
            if (--xa_resync != 0)
                return;
            
            //Check if we're in a proper state
            if (cd_status & CdlStatShellOpen)
                return;
            
            //Attempt to get CD drive active
            while (1)
            {
                CdControl(CdlNop, NULL, NULL);
                cd_status = CdStatus();
                if (cd_status & CdlStatStandby)
                    break;
            }
            
            //Re-initialize XA system
            uint8_t prev_state = xa_state;
            XA_Init();
            xa_state = prev_state;
            
            XA_SetFilter(xa_channel);
            XA_SetVolume(xa_volume);
            
            //Get new CD status
            CdControl(CdlNop, NULL, NULL);
            cd_status = CdStatus();
        }
        
        //Check CD status for issues
        if (cd_status & CdlStatShellOpen)
        {
            //Seek just ahead of last reported valid position
            if (!(xa_state & XA_STATE_SEEKING))
            {
                xa_pos++;
                xa_state |= XA_STATE_SEEKING;
            }
            
            //Wait a moment before attempting the actual resync
            xa_resync = 60;
            return;
        }
        
        //Handle seeking state
        if (xa_state & XA_STATE_SEEKING)
        {
            //Check if CD is still seeking to the XA's beginning
            if (!(cd_status & CdlStatSeek))
            {
                //Stopped seeking
                xa_state &= ~XA_STATE_SEEKING;
                XA_Play(xa_pos);
            }
            else
            {
                //Still seeking
                return;
            }
        }
        
        //Get CD position
        uint32_t next_pos = XA_TellSector();
        if (next_pos > xa_pos)
            xa_pos = next_pos;
        
        //Check position
        if (XA_TellSector() == -1)
        {
            if (xa_state & XA_STATE_LOOPS)
            {
                //Reset XA playback
                CdlLOC cd_loc;
                CdIntToPos(xa_pos = xa_start, &cd_loc);
                CdControl(CdlSetloc, (uint8_t*)&cd_loc, NULL);
                CdControlB(CdlSeekL, NULL, NULL);
                xa_state |= XA_STATE_SEEKING;
            }
            else
            {
                //Stop XA playback
                Audio_StopXA();
            }
        }
    }
}

/* .VAG file loader */
#define VAG_HEADER_SIZE 48
static int lastChannelUsed = 0;

static int getFreeChannel(void) {
    int channel = lastChannelUsed;
    lastChannelUsed = (channel + 1) % 24;
    return channel;
}

void Audio_ClearAlloc(void) {
    audio_alloc_ptr = ALLOC_START_ADDR;
}

uint32_t Audio_LoadVAGData(uint32_t *sound, uint32_t sound_size) {
    // subtract size of .vag header (48 bytes), round to 64 bytes
    uint32_t xfer_size = ((sound_size - VAG_HEADER_SIZE) + 63) & 0xffffffc0;
    uint8_t  *data = (uint8_t *) sound;

    // modify sound data to ensure sound "loops" to dummy sample
    // https://psx-spx.consoledev.net/soundprocessingunitspu/#flag-bits-in-2nd-byte-of-adpcm-header
    data[sound_size - 15] = 1; // end + mute

    // allocate SPU memory for sound
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
    return addr;
}

void Audio_PlaySoundOnChannel(uint32_t addr, uint32_t channel, int volume) {
    SPU_KEY_OFF1 = (1 << channel);

    SPU_CHANNELS[channel].vol_left   = volume;
    SPU_CHANNELS[channel].vol_right  = volume;
    SPU_CHANNELS[channel].addr       = SPU_RAM_ADDR(addr);
    SPU_CHANNELS[channel].loop_addr  = SPU_RAM_ADDR(DUMMY_ADDR);
    SPU_CHANNELS[channel].freq       = 0x1000; // 44100 Hz
    SPU_CHANNELS[channel].adsr_param = 0x1fc080ff;

    SPU_KEY_ON1 = (1 << channel);
}

void Audio_PlaySound(uint32_t addr, int volume) {
    Audio_PlaySoundOnChannel(addr, getFreeChannel(), volume);
   // printf("Could not find free channel to play sound (addr=%08x)\n", addr);
}

uint32_t Audio_LoadSound(const char *path)
{
    CdlFILE file;
    uint32_t Sound;

    IO_FindFile(&file, path);
    uint32_t *data = IO_ReadFile(&file);
    Sound = Audio_LoadVAGData(data, file.size);
    free(data);

    return Sound;
}