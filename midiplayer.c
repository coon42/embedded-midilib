/*
* mididump.c - A complete textual dump of a MIDI file.
*				Requires Steevs MIDI Library & Utilities
*				as it demonstrates the text name resolution code.
* Version 1.4
*
*  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
*			Copyright 2010, Steven Goodwin.
*
*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License as
*  published by the Free Software Foundation; either version 2 of
*  the License,or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*
* TODO: rename midi events to standard names
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include "midifile.h"
#include "midiutil.h"
#include <inttypes.h>

#pragma comment (lib, "winmm.lib")
static HMIDIOUT g_hMidiOut;

void HexList(uint8_t *pData, int32_t iNumBytes) {
  for (int32_t i = 0; i < iNumBytes; i++)
    printf("%.2x ", pData[i]);
}

// Routine for simplifying MIDI output
// ------------------------------------
uint32_t MidiOutMessage(int32_t iStatus, int32_t iChannel, int32_t iData1, int32_t iData2) {
  uint32_t dwMessage = iStatus | iChannel - 1 | (iData1 << 8) | (iData2 << 16);
  return midiOutShortMsg(g_hMidiOut, dwMessage);
}

void printTrackPrefix(uint32_t track, uint32_t tick, char* pEventName)  {
  printf("[Track: %d] %06d %s ", track, tick, pEventName);
}

// Midi Event handlers
char noteName[64]; // TOOD: refactor to const string array

void onNoteOff(int32_t track, int32_t tick, int32_t channel, int32_t note) {
  muGetNameFromNote(noteName, note);
  MidiOutMessage(msgNoteOff, channel, note, 0);
  printTrackPrefix(track, tick, "Note Off");
  printf("(%d) %s", channel, noteName);
  printf("\r\n");
}

void onNoteOn(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t velocity) {
  muGetNameFromNote(noteName, note);
  MidiOutMessage(msgNoteOn, channel, note, velocity);
  printTrackPrefix(track, tick, "Note On");
  printf("(%d) %s [%d] %d", channel, noteName, note, velocity);
  printf("\r\n");
}

void onNoteKeyPressure(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t pressure) {
  muGetNameFromNote(noteName, note);
  MidiOutMessage(msgNoteKeyPressure, channel, note, pressure);
  printTrackPrefix(track, tick, "Note Key Pressure");
  printf("(%d) %s %d", channel, noteName, pressure);
  printf("\r\n");
}

void onSetParameter(int32_t track, int32_t tick, int32_t channel, int32_t control, int32_t parameter) {
  muGetControlName(noteName, control);
  printTrackPrefix(track, tick, "Set Parameter");
  MidiOutMessage(msgSetParameter, channel, control, parameter);
  printf("(%d) %s -> %d", channel, noteName, parameter);
  printf("\r\n");
}

void onSetProgram(int32_t track, int32_t tick, int32_t channel, int32_t program) {
  muGetInstrumentName(noteName, program);
  MidiOutMessage(msgSetProgram, channel, program, 0);
  printTrackPrefix(track, tick, "Set Program");
  printf("(%d) %s", channel, noteName);
  printf("\r\n");
}

void onChangePressure(int32_t track, int32_t tick, int32_t channel, int32_t pressure) {
  muGetControlName(noteName, pressure);
  MidiOutMessage(msgChangePressure, channel, pressure, 0);
  printTrackPrefix(track, tick, "Change Pressure");
  printf("(%d) %s", channel, noteName);
  printf("\r\n");
}

void onSetPitchWheel(int32_t track, int32_t tick, int32_t channel, int16_t pitch) {
  MidiOutMessage(msgSetPitchWheel, channel, pitch << 1, pitch >> 7);
  printTrackPrefix(track, tick, "Set Pitch Wheel");
  printf("(%d) %d", channel, pitch);
  printf("\r\n");
}

void onMetaMIDIPort(int32_t track, int32_t tick, int32_t midiPort) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("MIDI Port = %d", midiPort);
  printf("\r\n");
}

void onMetaSequenceNumber(int32_t track, int32_t tick, int32_t sequenceNumber) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Sequence Number = %d", sequenceNumber);
  printf("\r\n");
}

void onMetaTextEvent(int32_t track, int32_t tick, char* pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Text = '%s'", pText);
  printf("\r\n");
}

void onMetaCopyright(int32_t track, int32_t tick, char* pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Copyright = '%s'", pText);
  printf("\r\n");
}

void onMetaTrackName(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Track name = '%s'", pText);
  printf("\r\n");
}

void onMetaInstrument(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Instrument = '%s'", pText);
  printf("\r\n");
}

void onMetaLyric(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Lyric = '%s'", pText);
  printf("\r\n");
}

void onMetaMarker(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Marker = '%s'", pText);
  printf("\r\n");
}

void onMetaCuePoint(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Cue point = '%s'", pText);
  printf("\r\n");
}

void onMetaEndSequence(int32_t track, int32_t tick) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("End Sequence");
  printf("\r\n");
}

void onMetaSetTempo(int32_t track, int32_t tick, int32_t bpm) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Tempo = %d", bpm);
  printf("\r\n");
}

void onMetaSMPTEOffset(int32_t track, int32_t tick, uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frames, uint32_t subframes) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("SMPTE offset = %d:%d:%d.%d %d", hours, minutes, seconds, frames, subframes);
  printf("\r\n");
}

void onMetaTimeSig(int32_t track, int32_t tick, int32_t nom, int32_t denom, int32_t metronome, int32_t thirtyseconds) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Time sig = %d/%d", nom, denom);
  printf("\r\n");
}

void onMetaKeySig(int32_t track, int32_t tick, uint32_t key, uint32_t scale) {
  printTrackPrefix(track, tick, "Meta event ----");
  if (muGetKeySigName(noteName, key)) {
    printf("Key sig = %s", noteName);
    printf("\r\n");
  }
}

void onMetaSequencerSpecific(int32_t track, int32_t tick, void* pData, uint32_t size) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Sequencer specific = ");
  HexList(pData, size);
  printf("\r\n");
}

void onMetaSysEx(int32_t track, int32_t tick, void* pData, uint32_t size) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("SysEx = ");
  HexList(pData, size);
  printf("\r\n");
}

// TODO: Hide the following functions from user
void dispatchMidiMsg(_MIDI_FILE* midiFile, int32_t trackIndex, MIDI_MSG* msg) {
  int32_t eventType = msg->bImpliedMsg ? msg->iImpliedMsg : msg->iType;
  switch (eventType) {
    case	msgNoteOff:
      onNoteOff(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOff.iChannel, msg->MsgData.NoteOff.iNote);
      break;
    case	msgNoteOn:
      onNoteOn(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOn.iChannel, msg->MsgData.NoteOn.iNote, msg->MsgData.NoteOn.iVolume);
      break;
    case	msgNoteKeyPressure:
      onNoteKeyPressure(trackIndex, msg->dwAbsPos, msg->MsgData.NoteKeyPressure.iChannel, msg->MsgData.NoteKeyPressure.iNote, msg->MsgData.NoteKeyPressure.iPressure);
      break;
    case	msgSetParameter:
      onSetParameter(trackIndex, msg->dwAbsPos, msg->MsgData.NoteParameter.iChannel, msg->MsgData.NoteParameter.iControl, msg->MsgData.NoteParameter.iParam);
      break;
    case	msgSetProgram:
      onSetProgram(trackIndex, msg->dwAbsPos, msg->MsgData.ChangeProgram.iChannel, msg->MsgData.ChangeProgram.iProgram);
      break;
    case	msgChangePressure:
      onChangePressure(trackIndex, msg->dwAbsPos, msg->MsgData.ChangePressure.iChannel, msg->MsgData.ChangePressure.iPressure);
      break;
    case	msgSetPitchWheel:
      onSetPitchWheel(trackIndex, msg->dwAbsPos, msg->MsgData.PitchWheel.iChannel, msg->MsgData.PitchWheel.iPitch + 8192);
      break;
    case	msgMetaEvent:
      switch (msg->MsgData.MetaEvent.iType) {
      case	metaMIDIPort:
        onMetaMIDIPort(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iMIDIPort);
        break;
      case	metaSequenceNumber:
        onMetaSequenceNumber(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iSequenceNumber);
        break;
      case	metaTextEvent:
        onMetaTextEvent(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCopyright:
        onMetaCopyright(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaTrackName:
        onMetaTrackName(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaInstrument:
        onMetaInstrument(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaLyric:
        onMetaLyric(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaMarker:
        onMetaMarker(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCuePoint:
        onMetaCuePoint(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaEndSequence:
        onMetaEndSequence(trackIndex, msg->dwAbsPos);
        break;
      case	metaSetTempo:
        setPlaybackTempo(midiFile, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        onMetaSetTempo(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        break;
      case	metaSMPTEOffset:
        onMetaSMPTEOffset(trackIndex, msg->dwAbsPos,
          msg->MsgData.MetaEvent.Data.SMPTE.iHours,
          msg->MsgData.MetaEvent.Data.SMPTE.iMins,
          msg->MsgData.MetaEvent.Data.SMPTE.iSecs,
          msg->MsgData.MetaEvent.Data.SMPTE.iFrames,
          msg->MsgData.MetaEvent.Data.SMPTE.iFF
          );
        break;
      case	metaTimeSig:
        // TODO: Metronome and thirtyseconds are missing!!!
        onMetaTimeSig(trackIndex,
          msg->dwAbsPos,
          msg->MsgData.MetaEvent.Data.TimeSig.iNom,
          msg->MsgData.MetaEvent.Data.TimeSig.iDenom / MIDI_NOTE_CROCHET,
          0, 0
          );
        break;
      case	metaKeySig: // TODO: scale is missing!!!
        onMetaKeySig(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.KeySig.iKey, 0);
        break;
      case	metaSequencerSpecific:
        onMetaSequencerSpecific(trackIndex, msg->dwAbsPos,
        msg->MsgData.MetaEvent.Data.Sequencer.pData, msg->MsgData.MetaEvent.Data.Sequencer.iSize);
        break;
      }
      break;

    case	msgSysEx1:
    case	msgSysEx2:
      onMetaSysEx(trackIndex, msg->dwAbsPos, msg->MsgData.SysEx.pData, msg->MsgData.SysEx.iSize);
      break;
    }
}

/*
BOOL playMidiFile(const char *pFilename) {
  _MIDI_FILE* pMFembedded;
  int32_t timeToWait = 0;
   
  pMFembedded = midiFileOpen(pFilename);
  if (!pMFembedded) {
    return FALSE;
  }

  static MIDI_MSG msgEmbedded[MAX_MIDI_TRACKS];
  BOOL any_track_had_data = TRUE;
  int32_t current_midi_tick = 0;
  int32_t ticks_to_wait = 0;
  int32_t iNumTracks = midiReadGetNumTracks(pMFembedded);

  printf("Midi Format: %d\r\n", pMFembedded->Header.iVersion);
  printf("Number of tracks: %d\r\n", iNumTracks);
  printf("Start playing...\r\n");

  // Read initial midi events
  for (int32_t iTrack = 0; iTrack < iNumTracks; iTrack++)
    midiReadGetNextMessage(pMFembedded, iTrack, &msgEmbedded[iTrack]);

  while (any_track_had_data) {
    any_track_had_data = FALSE;
    ticks_to_wait = -1;

    // Cycle through all tracks
    for (int32_t iTrack = 0; iTrack < iNumTracks; iTrack++) {
      BOOL eventHasToBePlayedNow = current_midi_tick == pMFembedded->Track[iTrack].pos;
      BOOL trackIsFinished = pMFembedded->Track[iTrack].ptrNew >= pMFembedded->Track[iTrack].pEndNew;

      // Process all events of current track for the current tick
      while (eventHasToBePlayedNow && !trackIsFinished) {
        dispatchMidiMsg(pMFembedded, iTrack, &msgEmbedded[iTrack]);
        if (midiReadGetNextMessage(pMFembedded, iTrack, &msgEmbedded[iTrack]))
          any_track_had_data = TRUE;

        eventHasToBePlayedNow = current_midi_tick == pMFembedded->Track[iTrack].pos;
        trackIsFinished = pMFembedded->Track[iTrack].ptrNew >= pMFembedded->Track[iTrack].pEndNew;
      }

      // Get wait time for next event
      // TODO: make this line of hell readable!
      BOOL isNextEventInFuture = (int32_t)(pMFembedded->Track[iTrack].pos - current_midi_tick) > 0;
      BOOL isThisEventEarlierThanTheLastEvent = ticks_to_wait > pMFembedded->Track[iTrack].pos - current_midi_tick;

      if (isNextEventInFuture && isThisEventEarlierThanTheLastEvent)
        ticks_to_wait = pMFembedded->Track[iTrack].pos - current_midi_tick;
    }

    if (ticks_to_wait == -1)
      ticks_to_wait = 0;

    // wait microseconds per tick here
    timeToWait = clock() + ticks_to_wait * pMFembedded->msPerTick;
    while (clock() < timeToWait); // just wait here...
    current_midi_tick += ticks_to_wait;
  }
  midiFileClose(pMFembedded);

  return TRUE;
}
*/

BOOL playMidiFile2(const char *pFilename) {
  _MIDI_FILE* pMFembedded;
  static MIDI_MSG msgEmbedded[MAX_MIDI_TRACKS];

  pMFembedded = midiFileOpen(pFilename);
  if (!pMFembedded) {
    return FALSE;
  }

  int32_t iNumTracks = midiReadGetNumTracks(pMFembedded);

  // Load initial midi events
  for (int iTrack = 0; iTrack < iNumTracks; iTrack++) {
    midiReadGetNextMessage(pMFembedded, iTrack, &msgEmbedded[iTrack]);
    pMFembedded->Track[iTrack].deltaTime = msgEmbedded[iTrack].dt;
  }
  
  int32_t currentTick = 0;
  int32_t lastTick = clock();
  int32_t deltaTick; // set to unsigned later! singed only for debugging.
  BOOL eventsNeedToBeFetched = FALSE;
  BOOL trackIsFinished;

  while (TRUE) {
    currentTick += (clock() / pMFembedded->msPerTick - lastTick);
    eventsNeedToBeFetched = TRUE;
    while (eventsNeedToBeFetched) { // This loop keeps all tracks synchronized in case of a lag
      eventsNeedToBeFetched = FALSE;

      deltaTick = currentTick - lastTick;                             
      if (deltaTick < 0) printf("DEBUG: bug in delta tick: deltaTick=%d\r\n", deltaTick);

      for (int iTrack = 0; iTrack < iNumTracks; iTrack++) {
        pMFembedded->Track[iTrack].deltaTime -= deltaTick;
        trackIsFinished = pMFembedded->Track[iTrack].ptrNew == pMFembedded->Track[iTrack].pEndNew;

        if (pMFembedded->Track[iTrack].deltaTime <= 0 && !trackIsFinished) { // Is it time to play this event?
          dispatchMidiMsg(pMFembedded, iTrack, &msgEmbedded[iTrack]); // shoot
          midiReadGetNextMessage(pMFembedded, iTrack, &msgEmbedded[iTrack]); // reload
          pMFembedded->Track[iTrack].deltaTime += msgEmbedded[iTrack].dt;
        }

        if (pMFembedded->Track[iTrack].deltaTime <= 0 && !trackIsFinished)
          eventsNeedToBeFetched = TRUE;

        lastTick = currentTick;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  uint32_t result = midiOutOpen(&g_hMidiOut, MIDI_MAPPER, 0, 0, 0);
  if (result != MMSYSERR_NOERROR)
    printf("MIDI device geht nicht!");

  if (argc == 1)
    printf("Usage: %s <filename>\n", argv[0]);
  else {
    for (int i = 1; i < argc; ++i) {
      char* midiFileName = argv[i];
      printf("Playing file: '%s'\r\n", midiFileName);
      if (playMidiFile2(midiFileName))
        printf("Playback finished.\r\n");
      else
        printf("Playback failed!\r\n");
    }
  }
  
  midiOutClose(g_hMidiOut);
  return 0;
}
