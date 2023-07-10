/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_AUDIO_H
#define PSXF_GUARD_AUDIO_H

#include "psx.h"
#include "psx/stream.h"

//Audio functions
void Audio_ResetChannels(void);
void Audio_Init(void);
bool Audio_FeedStream(void);
void Audio_LoadStream(const char *path, bool loop);
void Audio_StartStream(bool resume);
void Audio_StopStream(void);
void Audio_DestroyStream(void);
uint64_t Audio_GetTime(int unit);
uint32_t Audio_GetInitialTime(void);
bool Audio_IsPlaying(void);
void Audio_SetVolume(uint8_t i, uint16_t vol_left, uint16_t vol_right);

void Audio_ClearAlloc(void);
uint32_t Audio_LoadVAGData(uint32_t *sound, uint32_t sound_size);
void Audio_PlaySoundOnChannel(uint32_t addr, uint32_t channel, int volume);
void Audio_PlaySound(uint32_t addr, int volume);
uint32_t Audio_LoadSound(const char *path);

#endif
