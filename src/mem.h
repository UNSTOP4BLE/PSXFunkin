#include <stdlib.h>

#define Mem_Init(ptr, size)
#define Mem_Alloc(size) malloc(size)
#define Mem_Free(ptr) free(ptr)
#define Mem_GetStat(used, size, max)