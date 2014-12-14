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

#pragma comment (lib, "winmm.lib")

void HexList(BYTE *pData, int iNumBytes)
{
  int i;

  for (i = 0; i<iNumBytes; i++)
    printf("%.2x ", pData[i]);
}

// Routines for simplifying MIDI output
// ------------------------------------

DWORD MidiOutMessage(HMIDIOUT hMidi, int iStatus, int iChannel, int iData1, int iData2) {
  DWORD dwMessage = iStatus | iChannel - 1 | (iData1 << 8) | (iData2 << 16);
  return midiOutShortMsg(hMidi, dwMessage);
}

// Midi Event handlers
char noteName[64];

void onNoteOff(HMIDIOUT hMidiOut, int channel, int note) {
  muGetNameFromNote(noteName, note);
  MidiOutMessage(hMidiOut, msgNoteOff, channel, note, 0);
  printf("(%d) %s", channel, noteName);
}

void onNoteOn(HMIDIOUT hMidiOut, int channel, int note, int velocity) {
  muGetNameFromNote(noteName, note);
  MidiOutMessage(hMidiOut, msgNoteOn, channel, note, velocity);
  printf("(%d) %s [%d] %d", channel, noteName, note, velocity);
}

void onNoteKeyPressure(HMIDIOUT hMidiOut, int channel, int note, int pressure) {
  muGetNameFromNote(noteName, note, pressure);
  MidiOutMessage(hMidiOut, msgNoteKeyPressure, channel, note, pressure);
  printf("(%d) %s %d", channel, noteName, pressure);
}

void onSetParameter(HMIDIOUT hMidiOut, int channel, int control, int parameter) {
  muGetControlName(noteName, control);
  MidiOutMessage(hMidiOut, msgSetParameter, channel, control, parameter);
  printf("(%d) %s -> %d", channel, noteName, parameter);
}

void onSetProgram(HMIDIOUT hMidiOut, int channel, int program) {
  muGetInstrumentName(noteName, program);
  MidiOutMessage(hMidiOut, msgSetProgram, channel, program, 0);
  printf("(%d) %s", channel, noteName);
}

void onChangePressure(HMIDIOUT hMidiOut, int channel, int pressure) {
  muGetControlName(noteName, pressure);
  MidiOutMessage(hMidiOut, msgChangePressure, channel, pressure, 0);
  printf("(%d) %s", channel, noteName);
}

void onSetPitchWheel(HMIDIOUT hMidiOut, int channel, short pitch) {
  MidiOutMessage(hMidiOut, msgSetPitchWheel, channel, pitch << 1, pitch >> 7);
  printf("(%d) %d", channel, pitch);
}

void onMetaEvent() {
  // TODO
}

void playMidiFile(const char *pFilename) {
  _MIDI_FILE* pMF;
  _MIDI_FILE* pMFembedded;
  BOOL open_success;
  char str[128];
  int ev;

  HMIDIOUT hMidiOut;
  int i = 0;
  clock_t t2;

  unsigned int result = midiOutOpen(&hMidiOut, MIDI_MAPPER, 0, 0, 0);
  if (result != MMSYSERR_NOERROR)
    printf("Midi device Geht nicht!");

  printf("Opening file: '%s' ...", pFilename);
  pMF = midiFileOpen(pFilename, FALSE);
  pMFembedded = midiFileOpen(pFilename, TRUE);

  open_success = pMF != NULL;
  if (open_success) {
    printf(" success!\r\n");
    
    static MIDI_MSG msg[MAX_MIDI_TRACKS];
    static MIDI_MSG msgEmbedded[MAX_MIDI_TRACKS];
    int i, iNumTracks;
    int any_track_had_data = 1;
    DWORD current_midi_tick = 0;
    DWORD bpm = 192;
    float ms_per_tick;
    DWORD ticks_to_wait = 0;

    iNumTracks = midiReadGetNumTracks(pMF, pMFembedded);

    for (i = 0; i< iNumTracks; i++) {
      pMFembedded->Track[i].iDefaultChannel = 0;
      midiReadInitMessage(&msgEmbedded[i]);
      midiReadGetNextMessage(pMF, pMFembedded, i, &msg[i], &msgEmbedded[i], TRUE);
    }

    printf("Midi Format: %d\r\n", pMF->Header.iVersion);
    printf("Number of tracks: %d\r\n", iNumTracks);
    printf("start playing...\r\n");

    while (any_track_had_data) {
      any_track_had_data = 0;
      ticks_to_wait = -1;

      for (i = 0; i < iNumTracks; i++) {
        while (current_midi_tick == pMFembedded->Track[i].pos && pMFembedded->Track[i].ptrNew < pMFembedded->Track[i].pEndNew) {
          printf("[Track: %d]", i);
          ev = msgEmbedded[i].bImpliedMsg ? msgEmbedded[i].iImpliedMsg : msgEmbedded[i].iType;

          printf(" %06d ", msgEmbedded[i].dwAbsPos);
          if (muGetMIDIMsgName(str, ev))
            printf("%s  ", str);

          switch (ev) {
            case	msgNoteOff:
              onNoteOff(hMidiOut, msgEmbedded[i].MsgData.NoteOff.iChannel, msgEmbedded[i].MsgData.NoteOff.iNote);
              break;
            case	msgNoteOn:
              onNoteOn(hMidiOut, msgEmbedded[i].MsgData.NoteOn.iChannel, msgEmbedded[i].MsgData.NoteOn.iNote, msgEmbedded[i].MsgData.NoteOn.iVolume);
              break;
            case	msgNoteKeyPressure:
              onNoteKeyPressure(hMidiOut, msgEmbedded[i].MsgData.NoteKeyPressure.iChannel, msgEmbedded[i].MsgData.NoteKeyPressure.iNote, msgEmbedded[i].MsgData.NoteKeyPressure.iPressure);
              break;
            case	msgSetParameter:
              onSetParameter(hMidiOut, msgEmbedded[i].MsgData.NoteParameter.iChannel, msgEmbedded[i].MsgData.NoteParameter.iControl, msgEmbedded[i].MsgData.NoteParameter.iParam);
              break;
            case	msgSetProgram:
              onSetProgram(hMidiOut, msgEmbedded[i].MsgData.ChangeProgram.iChannel, msgEmbedded[i].MsgData.ChangeProgram.iProgram);
              break;
            case	msgChangePressure:
              onChangePressure(hMidiOut, msgEmbedded[i].MsgData.ChangePressure.iChannel, msgEmbedded[i].MsgData.ChangePressure.iPressure);
              break;
            case	msgSetPitchWheel:
              onSetPitchWheel(hMidiOut, msgEmbedded[i].MsgData.PitchWheel.iChannel, msgEmbedded[i].MsgData.PitchWheel.iPitch + 8192);
              break;
            case	msgMetaEvent:
              printf("---- ");
              switch (msgEmbedded[i].MsgData.MetaEvent.iType) {
              case	metaMIDIPort:
                printf("MIDI Port = %d", msgEmbedded[i].MsgData.MetaEvent.Data.iMIDIPort);
                break;
              case	metaSequenceNumber:
                printf("Sequence Number = %d", msgEmbedded[i].MsgData.MetaEvent.Data.iSequenceNumber);
                break;
              case	metaTextEvent:
                printf("Text = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaCopyright:
                printf("Copyright = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaTrackName:
                printf("Track name = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaInstrument:
                printf("Instrument = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaLyric:
                printf("Lyric = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaMarker:
                printf("Marker = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaCuePoint:
                printf("Cue point = '%s'", msgEmbedded[i].MsgData.MetaEvent.Data.Text.pData);
                break;
              case	metaEndSequence:
                printf("End Sequence");
                break;
              case	metaSetTempo:
                bpm = msgEmbedded[i].MsgData.MetaEvent.Data.Tempo.iBPM;
                ms_per_tick = 60000.0f / (bpm * pMFembedded->Header.PPQN);
                printf("Tempo = %d", msgEmbedded[i].MsgData.MetaEvent.Data.Tempo.iBPM);
                break;
              case	metaSMPTEOffset:
                printf("SMPTE offset = %d:%d:%d.%d %d",
                  msgEmbedded[i].MsgData.MetaEvent.Data.SMPTE.iHours,
                  msgEmbedded[i].MsgData.MetaEvent.Data.SMPTE.iMins,
                  msgEmbedded[i].MsgData.MetaEvent.Data.SMPTE.iSecs,
                  msgEmbedded[i].MsgData.MetaEvent.Data.SMPTE.iFrames,
                  msgEmbedded[i].MsgData.MetaEvent.Data.SMPTE.iFF
                  );
                break;
              case	metaTimeSig:
                printf("Time sig = %d/%d", msgEmbedded[i].MsgData.MetaEvent.Data.TimeSig.iNom,
                  msgEmbedded[i].MsgData.MetaEvent.Data.TimeSig.iDenom / MIDI_NOTE_CROCHET);
                break;
              case	metaKeySig:
                if (muGetKeySigName(str, msgEmbedded[i].MsgData.MetaEvent.Data.KeySig.iKey))
                  printf("Key sig = %s", str);
                break;

              case	metaSequencerSpecific:
                printf("Sequencer specific = ");
                HexList(msgEmbedded[i].MsgData.MetaEvent.Data.Sequencer.pData, msgEmbedded[i].MsgData.MetaEvent.Data.Sequencer.iSize); // ok
                printf("\r\n");
                break;
              }
              break;

            case	msgSysEx1:
            case	msgSysEx2:
              HexList(msgEmbedded[i].MsgData.SysEx.pData, msgEmbedded[i].MsgData.SysEx.iSize); // ok
              printf("\r\n");
              break;
            }

          if (ev == msgSysEx1 || ev == msgSysEx1 || (ev == msgMetaEvent && msgEmbedded[i].MsgData.MetaEvent.iType == metaSequencerSpecific)) {
            // Already done a hex dump
          }
          else {
            /*
            printf("  [");
            if (msg[i].bImpliedMsg) printf("%X!", msg[i].iImpliedMsg);
            for (unsigned int j = 0; j < msg[i].iMsgSize; j++)
              printf("%X ", msg[i].data[j]);
            printf["]");
            */
            printf("\r\n");            
          }

          if (midiReadGetNextMessage(pMF, pMFembedded, i, &msg[i], &msgEmbedded[i], TRUE)) {
            any_track_had_data = 1; // 0 ???
          }
        }

        ticks_to_wait = (pMFembedded->Track[i].pos - current_midi_tick > 0 && ticks_to_wait > pMFembedded->Track[i].pos - current_midi_tick) ? pMFembedded->Track[i].pos - current_midi_tick : ticks_to_wait;
      }

      if (ticks_to_wait == -1)
        ticks_to_wait = 0;

      // wait microseconds per tick here
      t2 = clock() + ticks_to_wait * ms_per_tick;

      time_t t1 = clock();

      while (t1 < t2) {
        t1 = clock();
        t1 = t1;
        // just wait here...
      }
      current_midi_tick += ticks_to_wait;
    }

    midiReadFreeMessage(msg);
    midiFileClose(pMF, pMFembedded);

    printf("done.\r\n");
  }
  else
    printf("Open Failed!\r\n");

  midiOutClose(hMidiOut);
}

int main(int argc, char* argv[]) {
  if (argc == 1)
    printf("Usage: %s <filename>\n", argv[0]);
  else {
    for (int i = 1; i<argc; ++i)
      playMidiFile(argv[i]);
  }

  return 0;
}
