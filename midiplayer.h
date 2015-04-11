#ifndef __MIDIFILE_H
#define __MIDIFILE_H

#include <stdbool.h>
#include "midifile.h"

typedef struct {
  _MIDI_FILE* pMidiFile;
  MIDI_MSG msg[MAX_MIDI_TRACKS];
  int32_t startTime;
  int32_t currentTick;
  int32_t lastTick;
  int32_t deltaTick; // Must NEVER be negative!!!
  bool eventsNeedToBeFetched;
  bool trackIsFinished;
  bool allTracksAreFinished;
  float lastMsPerTick;
  float timeScaleFactor;
} MIDI_PLAYER;

bool midiPlayerTick(MIDI_PLAYER* pMidiPlayer);
bool playMidiFile(MIDI_PLAYER* pMidiPlayer, const char *pFilename);

#endif // __MIDIFILE_H
