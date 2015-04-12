#ifndef __HAL_MISC_H
#define __HAL_MISC_H

#include <stdint.h>

// Timing function
uint32_t hal_clock();

// Colored debugging print functions
void hal_printfError(const char* format, ...);
void hal_printfWarning(char* format, ...);
void hal_printfSuccess(char* format, ...);
void hal_printfInfo(char* format, ...);

#endif // __HAL_MISC_H