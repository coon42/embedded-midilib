#ifndef __MIDIFILE_H
#define __MIDIFILE_H

#include "midifile.h"

typedef struct {
  _MIDI_FILE* pMidiFile;
  MIDI_MSG msg[MAX_MIDI_TRACKS];
  int32_t startTime;
  int32_t currentTick;
  int32_t lastTick;
  int32_t deltaTick; // Must NEVER be negative!!!
  BOOL eventsNeedToBeFetched;
  BOOL trackIsFinished;
  BOOL allTracksAreFinished;
  float lastMsPerTick;
  float timeScaleFactor;
} MIDI_PLAYER;

BOOL midiPlayerTick(MIDI_PLAYER* pMidiPlayer);
BOOL playMidiFile(MIDI_PLAYER* pMidiPlayer, const char *pFilename);

#endif // __MIDIFILE_H
