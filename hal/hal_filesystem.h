#ifndef __HAL_FILESYSTEM_H
#define __HAL_FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  char fileName[256];
} FO_FIND_DATA;

// finding functions
bool hal_findInit(char* path, FO_FIND_DATA* findData);
bool hal_findNext(FO_FIND_DATA* findData);
void hal_findFree();

// ---- basic file system functions ----
// Returns 1, if file was opened successfully or 0 on error.
int32_t hal_fopen(FILE** pFile, const char* pFileName);
int32_t hal_fclose(FILE* pFile);
int32_t hal_fseek(FILE* pFile, int startPos);
size_t hal_fread(FILE* pFile, void* dst, size_t numBytes);
int32_t hal_ftell(FILE* pFile);

#endif