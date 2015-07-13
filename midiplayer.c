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
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "midifile.h"
#include "midiutil.h"
#include "midiplayer.h"
#include "hal/hal_misc.h"

static void dispatchMidiMsg(MIDI_PLAYER* pMidiPlayer, int32_t trackIndex) {
  MIDI_MSG* msg = &pMidiPlayer->msg[trackIndex];

  int32_t eventType = msg->bImpliedMsg ? msg->iImpliedMsg : msg->iType;
  switch (eventType) {
    case	msgNoteOff:
      if (pMidiPlayer->pOnNoteOffCb)
        pMidiPlayer->pOnNoteOffCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOff.iChannel, msg->MsgData.NoteOff.iNote);
      break;
    case	msgNoteOn:
      if (pMidiPlayer->pOnNoteOnCb)
        pMidiPlayer->pOnNoteOnCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOn.iChannel, msg->MsgData.NoteOn.iNote, msg->MsgData.NoteOn.iVolume);
      break;
    case	msgNoteKeyPressure:
      if (pMidiPlayer->pOnNoteKeyPressureCb)
        pMidiPlayer->pOnNoteKeyPressureCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteKeyPressure.iChannel, msg->MsgData.NoteKeyPressure.iNote, msg->MsgData.NoteKeyPressure.iPressure);
      break;
    case	msgControlChange:
      if (pMidiPlayer->pOnSetParameterCb)
        pMidiPlayer->pOnSetParameterCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteParameter.iChannel, msg->MsgData.NoteParameter.iControl, msg->MsgData.NoteParameter.iParam);
      break;
    case	msgSetProgram:
      if (pMidiPlayer->pOnSetProgramCb)
        pMidiPlayer->pOnSetProgramCb(trackIndex, msg->dwAbsPos, msg->MsgData.ChangeProgram.iChannel, msg->MsgData.ChangeProgram.iProgram);
      break;
    case	msgChangePressure:
      if (pMidiPlayer->pOnChangePressureCb)
        pMidiPlayer->pOnChangePressureCb(trackIndex, msg->dwAbsPos, msg->MsgData.ChangePressure.iChannel, msg->MsgData.ChangePressure.iPressure);
      break;
    case	msgSetPitchWheel:
      if (pMidiPlayer->pOnSetPitchWheelCb)
        pMidiPlayer->pOnSetPitchWheelCb(trackIndex, msg->dwAbsPos, msg->MsgData.PitchWheel.iChannel, msg->MsgData.PitchWheel.iPitch + 8192);
      break;
    case	msgMetaEvent:
      switch (msg->MsgData.MetaEvent.iType) {
      case	metaMIDIPort:
        if (pMidiPlayer->pOnMetaMIDIPortCb)
          pMidiPlayer->pOnMetaMIDIPortCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iMIDIPort);
        break;
      case	metaSequenceNumber:
        if (pMidiPlayer->pOnMetaSequenceNumberCb)
          pMidiPlayer->pOnMetaSequenceNumberCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iSequenceNumber);
        break;
      case	metaTextEvent:
        if (pMidiPlayer->pOnMetaTextEventCb)
          pMidiPlayer->pOnMetaTextEventCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCopyright:
        if (pMidiPlayer->pOnMetaCopyrightCb)
          pMidiPlayer->pOnMetaCopyrightCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaTrackName:
        if (pMidiPlayer->pOnMetaTrackNameCb)
          pMidiPlayer->pOnMetaTrackNameCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaInstrument:
        if (pMidiPlayer->pOnMetaInstrumentCb)
          pMidiPlayer->pOnMetaInstrumentCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaLyric:
        if (pMidiPlayer->pOnMetaLyricCb)
          pMidiPlayer->pOnMetaLyricCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaMarker:
        if (pMidiPlayer->pOnMetaMarkerCb)
          pMidiPlayer->pOnMetaMarkerCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCuePoint:
        if (pMidiPlayer->pOnMetaCuePointCb)
          pMidiPlayer->pOnMetaCuePointCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaEndSequence:
        if (pMidiPlayer->pOnMetaEndSequenceCb)
          pMidiPlayer->pOnMetaEndSequenceCb(trackIndex, msg->dwAbsPos);
        break;
      case	metaSetTempo:
        setPlaybackTempo(pMidiPlayer->pMidiFile, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        adjustTimeFactor(pMidiPlayer);

        if (pMidiPlayer->pOnMetaSetTempoCb)
          pMidiPlayer->pOnMetaSetTempoCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        break;
      case	metaSMPTEOffset:
        if (pMidiPlayer->pOnMetaSMPTEOffsetCb)
          pMidiPlayer->pOnMetaSMPTEOffsetCb(trackIndex, msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.SMPTE.iHours,
            msg->MsgData.MetaEvent.Data.SMPTE.iMins,
            msg->MsgData.MetaEvent.Data.SMPTE.iSecs,
            msg->MsgData.MetaEvent.Data.SMPTE.iFrames,
            msg->MsgData.MetaEvent.Data.SMPTE.iFF
          );
        break;
      case	metaTimeSig:
        // TODO: Metronome and thirtyseconds are missing!!!
        if (pMidiPlayer->pOnMetaTimeSigCb)
          pMidiPlayer->pOnMetaTimeSigCb(trackIndex,
            msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.TimeSig.iNom,
            msg->MsgData.MetaEvent.Data.TimeSig.iDenom / MIDI_NOTE_CROCHET,
            0, 0
          );
        break;
      case	metaKeySig: // TODO: scale is missing!!!
        if (pMidiPlayer->pOnMetaKeySigCb)
          pMidiPlayer->pOnMetaKeySigCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.KeySig.iKey, 0);
        break;
      case	metaSequencerSpecific:
        if (pMidiPlayer->pOnMetaSequencerSpecificCb)
          pMidiPlayer->pOnMetaSequencerSpecificCb(trackIndex, msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.Sequencer.pData, msg->MsgData.MetaEvent.Data.Sequencer.iSize
          );
        break;
      }
      break;

    case	msgSysEx1:
    case	msgSysEx2:
      if (pMidiPlayer->pOnMetaSysExCb)
        pMidiPlayer->pOnMetaSysExCb(trackIndex, msg->dwAbsPos, msg->MsgData.SysEx.pData, msg->MsgData.SysEx.iSize);
      break;
    }
}

void midiplayer_init(MIDI_PLAYER* mpl, OnNoteOffCallback_t pOnNoteOffCb, OnNoteOnCallback_t pOnNoteOnCb,
    OnNoteKeyPressureCallback_t pOnNoteKeyPressureCb, OnSetParameterCallback_t pOnSetParameterCb,
    OnSetProgramCallback_t pOnSetProgramCb, OnChangePressureCallback_t pOnChangePressureCb, 
    OnSetPitchWheelCallback_t pOnSetPitchWheelCb, OnMetaMIDIPortCallback_t pOnMetaMIDIPortCb,
    OnMetaSequenceNumberCallback_t pOnMetaSequenceNumberCb, OnMetaTextEventCallback_t pOnMetaTextEventCb,
    OnMetaCopyrightCallback_t pOnMetaCopyrightCb, OnMetaTrackNameCallback_t pOnMetaTrackNameCb,
    OnMetaInstrumentCallback_t pOnMetaInstrumentCb, OnMetaLyricCallback_t pOnMetaLyricCb,
    OnMetaMarkerCallback_t pOnMetaMarkerCb, OnMetaCuePointCallback_t pOnMetaCuePointCb,
    OnMetaEndSequenceCallback_t pOnMetaEndSequenceCb, OnMetaSetTempoCallback_t pOnMetaSetTempoCb,
    OnMetaSMPTEOffsetCallback_t pOnMetaSMPTEOffsetCb, OnMetaTimeSigCallback_t pOnMetaTimeSigCb,
    OnMetaKeySigCallback_t pOnMetaKeySigCb, OnMetaSequencerSpecificCallback_t pOnMetaSequencerSpecificCb,
    OnMetaSysExCallback_t pOnMetaSysExCb) {
  memset(mpl, 0, sizeof(MIDI_PLAYER));

  mpl->pOnNoteOffCb = pOnNoteOffCb;
  mpl->pOnNoteOnCb = pOnNoteOnCb;
  mpl->pOnNoteKeyPressureCb = pOnNoteKeyPressureCb;
  mpl->pOnSetParameterCb = pOnSetParameterCb;
  mpl->pOnSetProgramCb = pOnSetProgramCb;
  mpl->pOnChangePressureCb = pOnChangePressureCb;
  mpl->pOnSetPitchWheelCb = pOnSetPitchWheelCb;
  mpl->pOnMetaMIDIPortCb = pOnMetaMIDIPortCb;
  mpl->pOnMetaSequenceNumberCb = pOnMetaSequenceNumberCb;
  mpl->pOnMetaTextEventCb = pOnMetaTextEventCb;
  mpl->pOnMetaCopyrightCb = pOnMetaCopyrightCb;
  mpl->pOnMetaTrackNameCb = pOnMetaTrackNameCb;
  mpl->pOnMetaInstrumentCb = pOnMetaInstrumentCb;
  mpl->pOnMetaLyricCb = pOnMetaLyricCb;
  mpl->pOnMetaMarkerCb = pOnMetaMarkerCb;
  mpl->pOnMetaCuePointCb = pOnMetaCuePointCb;
  mpl->pOnMetaEndSequenceCb = pOnMetaEndSequenceCb;
  mpl->pOnMetaSetTempoCb = pOnMetaSetTempoCb;
  mpl->pOnMetaSMPTEOffsetCb = pOnMetaSMPTEOffsetCb;
  mpl->pOnMetaTimeSigCb = pOnMetaTimeSigCb;
  mpl->pOnMetaKeySigCb = pOnMetaKeySigCb;
  mpl->pOnMetaSequencerSpecificCb = pOnMetaSequencerSpecificCb;
  mpl->pOnMetaSysExCb = pOnMetaSysExCb;
}

bool midiPlayerOpenFile(MIDI_PLAYER* pMidiPlayer, const char* pFileName) {
  pMidiPlayer->pMidiFile = midiFileOpen(pFileName);
  if (!pMidiPlayer->pMidiFile)
    return false;
  
  // Load initial midi events
  for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
    midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]);
    pMidiPlayer->pMidiFile->Track[iTrack].deltaTime = pMidiPlayer->msg[iTrack].dt;
  }

  pMidiPlayer->startTime = hal_clock() * 1000;
  pMidiPlayer->currentTick = 0;
  pMidiPlayer->lastTick = 0;
  pMidiPlayer->trackIsFinished = true;
  pMidiPlayer->allTracksAreFinished = false;
  pMidiPlayer->lastUsPerTick = pMidiPlayer->pMidiFile->usPerTick;

  return true;
}

bool playMidiFile(MIDI_PLAYER* pMidiPlayer, const char *pFilename) {
  if (!midiPlayerOpenFile(pMidiPlayer, pFilename))
    return false;

  hal_printfInfo("Midi Format: %d", pMidiPlayer->pMidiFile->Header.iVersion);
  hal_printfInfo("Number of tracks: %d", midiReadGetNumTracks(pMidiPlayer->pMidiFile));
  hal_printfSuccess("Start playing...");
  return true;
}

void adjustTimeFactor(MIDI_PLAYER* pMp) {
  // On a tempo change we need to transform the old absolute time scale to the new scale by setting the current 
  // tick to the right position.

  // To avoid floating point operations, a technique called "fixed point arithmetic" is used here.
  // This is simply done by shifting the values, calculating them und shifting them back the same amount to
  // get the result. To control the precision you can adjust the PRECISION constant. Using too high values
  // may lead to an int32 overflow so a compromise between accuracy and integer size must be met.
  // A value between 5 - 8 seem to be a good choice. Anything lower than 5 leads to audio glitches.
  // The fixed point operation is equvalent to the following floating point operations:

  // float timeScaleFactor = (float)pMp->lastUsPerTick / pMp->pMidiFile->usPerTick;
  // pMp->currentTick * timeScaleFactor;

  const uint32_t PRECISION = 8;
  int32_t fracFixed = (pMp->currentTick << PRECISION) / pMp->pMidiFile->usPerTick;
  int32_t mulFixed = fracFixed * pMp->lastUsPerTick;

  if (mulFixed > INT32_MAX - INT32_MAX / 4)
    hal_printfWarning("Warning: mulFixed value is about to overflow! (%d / %d)", mulFixed, INT32_MAX);
  
  pMp->currentTick = mulFixed >> PRECISION;
  pMp->lastUsPerTick = pMp->pMidiFile->usPerTick;
}

bool isItTimeToFireThisEvent(MIDI_PLAYER* pMp, int iTrack) {
  if (pMp->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMp->trackIsFinished) {
    dispatchMidiMsg(pMp, iTrack); // shoot

    // Debug 1/2
    int32_t expectedWaitTime = pMp->pMidiFile->Track[iTrack].debugLastMsgDt * pMp->lastUsPerTick / 1000;
    int32_t realWaitTime = hal_clock() - pMp->pMidiFile->Track[iTrack].debugLastClock;
    int32_t diff = realWaitTime - expectedWaitTime;

    if (abs(diff > 10))
      hal_printfWarning("Expected: %d ms, real: %d ms, diff: %d ms", expectedWaitTime, realWaitTime, diff);
    // ---

    midiReadGetNextMessage(pMp->pMidiFile, iTrack, &pMp->msg[iTrack]); // reload
    pMp->pMidiFile->Track[iTrack].deltaTime += pMp->msg[iTrack].dt;

    // Debug 2/2
    pMp->pMidiFile->Track[iTrack].debugLastClock = hal_clock();
    pMp->pMidiFile->Track[iTrack].debugLastMsgDt = pMp->msg[iTrack].dt;
    // ---

    return true;
  }

  return false;
}

bool processTracks(MIDI_PLAYER* pMp) {
  int32_t deltaTick = pMp->currentTick - pMp->lastTick; // Must NEVER be negative!!!

  if (deltaTick < 0) {
    hal_printfWarning("Warning: deltaTick is negative! Fast forward? deltaTick=%d", deltaTick);

    // TODO: correct time here, to prevent skips on following tempo changes!
    deltaTick = 0;
  }

  bool eventsNeedToBeFetched = false;
  pMp->allTracksAreFinished = true;

  for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMp->pMidiFile); iTrack++) {
    pMp->pMidiFile->Track[iTrack].deltaTime -= deltaTick;
    pMp->trackIsFinished = pMp->pMidiFile->Track[iTrack].ptrNew == pMp->pMidiFile->Track[iTrack].pEndNew;

    if (!pMp->trackIsFinished) {
      pMp->allTracksAreFinished = false;
      eventsNeedToBeFetched = isItTimeToFireThisEvent(pMp, iTrack);
    }
  }

  pMp->lastTick = pMp->currentTick;

  return eventsNeedToBeFetched;
}

bool midiPlayerTick(MIDI_PLAYER* pMidiPlayer) {
  MIDI_PLAYER* pMp = pMidiPlayer;

  if (pMp->pMidiFile == NULL)
    return false;

  pMp->currentTick = (hal_clock() * 1000 - pMp->startTime) / pMp->pMidiFile->usPerTick;
  while (processTracks(pMidiPlayer)); // This loop keeps all tracks synchronized in case of a lag

  return !pMp->allTracksAreFinished; // TODO: close file
}
