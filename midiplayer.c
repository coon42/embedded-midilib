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
* TODO: - rename midi events to standard names
*       - avoid floating point operations on playback
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "midifile.h"
#include "midiutil.h"
#include <stdint.h>
#include <math.h>

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

void HexList(uint8_t *pData, int32_t iNumBytes) {
  for (int32_t i = 0; i < iNumBytes; i++)
    printf("%.2x ", pData[i]);
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

void _onTextEvents(int32_t track, int32_t tick, const char* textType, const char* pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  hal_printfInfo("%s = %s", textType, pText);
}

void onMetaTextEvent(int32_t track, int32_t tick, char* pText) {
  _onTextEvents(track, tick, "Text", pText);
}

void onMetaCopyright(int32_t track, int32_t tick, char* pText) {
  _onTextEvents(track, tick, "Copyright ", pText);
}

void onMetaTrackName(int32_t track, int32_t tick, char *pText) {
  _onTextEvents(track, tick, "Track name", pText);
}

void onMetaInstrument(int32_t track, int32_t tick, char *pText) {
  _onTextEvents(track, tick, "Instrument", pText);
}

void onMetaLyric(int32_t track, int32_t tick, char *pText) {
  _onTextEvents(track, tick, "Lyric", pText);
}

void onMetaMarker(int32_t track, int32_t tick, char *pText) {
  _onTextEvents(track, tick, "Marker", pText);
}

void onMetaCuePoint(int32_t track, int32_t tick, char *pText) {
  _onTextEvents(track, tick, "Cue point", pText);
}

void onMetaEndSequence(int32_t track, int32_t tick) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("End Sequence");
  printf("\r\n");
}

void onMetaSetTempo(int32_t track, int32_t tick, int32_t bpm) {
  printTrackPrefix(track, tick, "Meta event ----");
  hal_printfWarning("Tempo = %d bpm", bpm);
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

BOOL midiPlayerOpenFile(MIDI_PLAYER* pMidiPlayer, const char* pFileName) {
  pMidiPlayer->pMidiFile = midiFileOpen(pFileName);
  if (!pMidiPlayer->pMidiFile)
    return FALSE;
  
  // Load initial midi events
  for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
    midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]);
    pMidiPlayer->pMidiFile->Track[iTrack].deltaTime = pMidiPlayer->msg[iTrack].dt;
  }

  pMidiPlayer->startTime = hal_clock();
  pMidiPlayer->currentTick = 0;
  pMidiPlayer->lastTick = 0;
  pMidiPlayer->deltaTick; // Must NEVER be negative!!!
  pMidiPlayer->eventsNeedToBeFetched = FALSE;
  pMidiPlayer->trackIsFinished;
  pMidiPlayer->allTracksAreFinished = FALSE;
  pMidiPlayer->lastMsPerTick = pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->timeScaleFactor = 1.0f;

  return TRUE;
}

BOOL midiPlayerTick(MIDI_PLAYER* pMidiPlayer) {
  if (pMidiPlayer->pMidiFile == NULL)
    return FALSE;

  if (fabs(pMidiPlayer->lastMsPerTick - pMidiPlayer->pMidiFile->msPerTick) > 0.001f) { // TODO: avoid floating point operation here!
    // On a tempo change we need to transform the old absolute time scale to the new scale.
    pMidiPlayer->timeScaleFactor = pMidiPlayer->lastMsPerTick / pMidiPlayer->pMidiFile->msPerTick;
    pMidiPlayer->lastTick *= pMidiPlayer->timeScaleFactor;
  }

  pMidiPlayer->lastMsPerTick = pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->currentTick = (hal_clock() - pMidiPlayer->startTime) / pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->eventsNeedToBeFetched = TRUE;
  while (pMidiPlayer->eventsNeedToBeFetched) { // This loop keeps all tracks synchronized in case of a lag
    pMidiPlayer->eventsNeedToBeFetched = FALSE;
    pMidiPlayer->allTracksAreFinished = TRUE;
    pMidiPlayer->deltaTick = pMidiPlayer->currentTick - pMidiPlayer->lastTick;
    if (pMidiPlayer->deltaTick < 0) {
      hal_printfWarning("Warning: deltaTick is negative! Fast forward? deltaTick=%d", pMidiPlayer->deltaTick);
      // TODO: correct time here!
      pMidiPlayer->deltaTick = 0;
    }

    for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
      pMidiPlayer->pMidiFile->Track[iTrack].deltaTime -= pMidiPlayer->deltaTick;
      pMidiPlayer->trackIsFinished = pMidiPlayer->pMidiFile->Track[iTrack].ptrNew == pMidiPlayer->pMidiFile->Track[iTrack].pEndNew;

      if (!pMidiPlayer->trackIsFinished) {
        if (pMidiPlayer->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMidiPlayer->trackIsFinished) { // Is it time to play this event?
          dispatchMidiMsg(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]); // shoot
          midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]); // reload
          pMidiPlayer->pMidiFile->Track[iTrack].deltaTime += pMidiPlayer->msg[iTrack].dt;
        }

        if (pMidiPlayer->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMidiPlayer->trackIsFinished)
          pMidiPlayer->eventsNeedToBeFetched = TRUE;

        pMidiPlayer->allTracksAreFinished = FALSE;
      }
      pMidiPlayer->lastTick = pMidiPlayer->currentTick;
    }
  }

  return !pMidiPlayer->allTracksAreFinished; // TODO: close file
}

static MIDI_PLAYER mpl;
BOOL playMidiFile(const char *pFilename) {
  if (!midiPlayerOpenFile(&mpl, pFilename))
    return FALSE;
  
  hal_printfInfo("Midi Format: %d", mpl.pMidiFile->Header.iVersion);
  hal_printfInfo("Number of tracks: %d", midiReadGetNumTracks(mpl.pMidiFile));
  hal_printfSuccess("Start playing...");
  return TRUE;
}

enum {
  REG_1 = 0x01,
  REG_2 = 0x02,
  REG_3 = 0x02
} test;


int main(int argc, char* argv[]) {
  hal_init();

  if (argc == 1)
    printf("Usage: %s <filename>\n", argv[0]);
  else {
    for (int i = 1; i < argc; ++i) {
      char* midiFileName = argv[i];
      printf("Playing file: '%s'\r\n", midiFileName);
      if (!playMidiFile(midiFileName)) {
        hal_printfError("Playback failed!");
        return 1;
      }

      while (midiPlayerTick(&mpl));
      hal_printfSuccess("Playback finished!");
    }
  }
 
  

  hal_free();
  return 0;
}
