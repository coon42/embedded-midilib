//////////////////////////////////////////////////////
// Hardware abstraction layer for desktop Platforms //
//////////////////////////////////////////////////////

#include <stdio.h>


// ---- Filesystem functions ----

// Returns 1, if file was opened successfully or 0 on error.
int hal_fopen(FILE** pFile, const char* pFileName) {
  return fopen_s(pFile, pFileName, "rb") == 0;
}

int hal_fclose(FILE* pFile) {
  return fclose(pFile);  
}

int hal_fseek(FILE* pFile, int startPos) {
  return fseek(pFile, startPos, SEEK_SET);
}

size_t hal_fread(void* dst, size_t numBytes, FILE* pFile) {
  return fread_s(dst, numBytes, 1, numBytes, pFile); // TODO: word access? Does it increase performance?
}

long hal_ftell(FILE* pFile) {
  return ftell(pFile);
}
