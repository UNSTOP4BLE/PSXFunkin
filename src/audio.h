/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_AUDIO_H
#define PSXF_GUARD_AUDIO_H

#include "psx.h"

//Audio functions
void Audio_ResetChannel(int channel);
void Audio_ResetChannels(void);
void Audio_Init();
void Audio_FeedStream(void);
void Audio_LoadStream(const char *path, boolean loop);
void Audio_StartStream(void);
void Audio_StopStream(void);
u32 Audio_GetTimeMS(void);
u32 Audio_GetInitialTime(void);
boolean Audio_IsPlaying(void);
void Audio_SetVolume(u8 i, u16 vol_left, u16 vol_right);

void Audio_ClearAlloc(void);
u32 Audio_LoadVAGData(u32 *sound, u32 sound_size);
void Audio_PlaySoundOnChannel(u32 addr, u32 channel, int volume);
void Audio_PlaySound(u32 addr, int volume);
u32 Audio_LoadSound(const char *path);

#endif
