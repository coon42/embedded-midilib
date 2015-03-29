#include "midiplayer.h"
#include "hal_midiplayer_win32.h"

uint32_t hal_clock() {
  return clock();
}

static void _printColored(const char* text, uint16_t colorAttributes) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  WORD saved_attributes;

  GetConsoleScreenBufferInfo(hConsole, &consoleInfo); // Save current font color
  saved_attributes = consoleInfo.wAttributes;
  SetConsoleTextAttribute(hConsole, colorAttributes); // Change font color
  printf("%s\n\r", text);
  SetConsoleTextAttribute(hConsole, saved_attributes); // Restore original font color
}

void hal_printfError(const char* format, ...) {
  char formattedText[512];

  va_list args;
  va_start(args, format);
  vsnprintf_s(formattedText, sizeof(formattedText), sizeof(formattedText), format, args);
  _printColored(formattedText, FOREGROUND_RED | FOREGROUND_INTENSITY);
  va_end(args);
}

void hal_printfWarning(char* format, ...) {
  char formattedText[512];

  va_list args;
  va_start(args, format);
  vsnprintf_s(formattedText, sizeof(formattedText), sizeof(formattedText), format, args);
  _printColored(formattedText, BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_INTENSITY); // Yellow
  va_end(args);
}

void hal_printfSuccess(char* format, ...) {
  char formattedText[512];

  va_list args;
  va_start(args, format);
  vsnprintf_s(formattedText, sizeof(formattedText), sizeof(formattedText), format, args);
  _printColored(formattedText, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
  va_end(args);
}

void hal_printfInfo(char* format, ...) {
  char formattedText[512];

  va_list args;
  va_start(args, format);
  vsnprintf_s(formattedText, sizeof(formattedText), sizeof(formattedText), format, args);
  _printColored(formattedText, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
  va_end(args);
}

void hal_midiplayer_init(MIDI_PLAYER* mpl) {
  memset(mpl, 0, sizeof(MIDI_PLAYER));

  uint32_t result = midiOutOpen(&hMidiOut, MIDI_MAPPER, 0, 0, 0);
  if (result != MMSYSERR_NOERROR)
    hal_printfError("MIDI device geht nicht!");
}

void hal_midiplayer_free(MIDI_PLAYER* mpl) {
  midiOutClose(hMidiOut);
}

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

uint32_t MidiOutMessage(int32_t iStatus, int32_t iChannel, int32_t iData1, int32_t iData2) {
  uint32_t dwMessage = iStatus | iChannel - 1 | (iData1 << 8) | (iData2 << 16);
  return midiOutShortMsg(hMidiOut, dwMessage);
}