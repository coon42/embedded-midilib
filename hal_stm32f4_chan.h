//////////////////////////////////////////////////////////////
// Hardware abstraction layer for embedded microcontrollers //
// Based on the file library by Chan.                       //
//////////////////////////////////////////////////////////////

#include "ff.h"

// ---- Filesystem functions ----

// Returns 1, if file was opened successfully or 0 on error.
int hal_fopen(FIL* pFile, const char* pFileName) {
  return f_open(pFile, pFileName, FA_READ);
}

int hal_fclose(FIL* pFile) {
  return f_close(pFile);
}

int hal_fseek(FIL* pFile, int startPos) {
  return f_lseek(pFile, startPos);
}

size_t hal_fread(void* dst, size_t numBytes, FIL* pFile) {
  UINT bytesRead;
  f_read(pFile, dst, numBytes, &bytesRead); // TODO: word access? Does it increase performance?
  return bytesRead;
}

long hal_ftell(FIL* pFile) {
  return f_tell(pFile);
}

char* strcpy_s(char* pDst, int szDst, const char* pSrc) {
  return strcpy(pDst, pSrc); // not secure, but works for now. :)
}

char* strcat_s(char* pDst, int32_t szDst, char* pSrc) {
  return strcat(pDst,pSrc); // not secure, but works for now. :)
}

int32_t sprintf_s(char* buffer, int32_t sizeOfBuffer, const char *format, ...) {
  sprintf(buffer, "TODO: implement sprintf()!"); // not secure, but works for now. :)
}

