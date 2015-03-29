#ifndef __HAL_MIDIPLAYER_WIN32_H
#define __HAL_MIDIPLAYER_WIN32_H

///////////////////////////////////////////////////////
// Hardware abstraction layer for windows enviroment //
///////////////////////////////////////////////////////

// 1. Place initialazing code in hal_init() (for example opening a MIDI device)
// 2. Free memory using in hall_free()

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include "midiplayer.h"
#pragma comment (lib, "winmm.lib")

static HMIDIOUT hMidiOut;

// Timing function
uint32_t hal_clock();

// Colored debugging print functions
static void _printColored(const char* text, uint16_t colorAttributes);
void hal_printfError(const char* format, ...);
void hal_printfWarning(char* format, ...);
void hal_printfSuccess(char* format, ...);
void hal_printfInfo(char* format, ...);
void hal_midiplayer_init(MIDI_PLAYER* mpl);
void hal_midiplayer_free(MIDI_PLAYER* mpl);

// ---- Filesystem functions ----
// Returns 1, if file was opened successfully or 0 on error.
int hal_fopen(FILE** pFile, const char* pFileName);
int hal_fclose(FILE* pFile);
int hal_fseek(FILE* pFile, int startPos);
size_t hal_fread(void* dst, size_t numBytes, FILE* pFile);
long hal_ftell(FILE* pFile);

// Routine for simplifying MIDI output
// ------------------------------------
uint32_t MidiOutMessage(int32_t iStatus, int32_t iChannel, int32_t iData1, int32_t iData2);

#endif // __HAL_MIDIPLAYER_WIN32_H