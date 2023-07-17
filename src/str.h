#ifndef PSXF_GUARD_STR_H
#define PSXF_GUARD_STR_H

#include <psxcd.h>
#include "stage.h"

static volatile struct {
    int stage;
    char path[64];
} movies[] = {
    #include "strdef.h"
};

void STR_Init(void);

void STR_InitStream(void);
void STR_StartStream(const char* path);
void STR_StopStream(void);
void STR_Proccess(void);

#endif