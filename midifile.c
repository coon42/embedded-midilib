/*
 * midiFile.c - A general purpose midi file handling library. This code
 *				can read and write MIDI files in formats 0 and 1.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *          Copyright 1998-2010, Steven Goodwin
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
 * TODO: - support of 'running status' interruption by realtime messages
 *       - eliminate strcpy_s() for better portability?
 *       - abstract FILE type / FILE type needs an instance on microcontroller
 *       - change size_t to int?
 *       - change iBPM to imBPM to increase accuracy of tempo?
 *       - disable cache on non MIDI0 files?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_filesystem.h"
#include "hal/hal_misc.h"
#include "midifile.h"

// -----------------------------------
// Global variables and new functions
// -----------------------------------
_MIDI_FILE _midiFile; // TODO: let the user define and pass the instance, so it is possible to open multiple MIDI files at once?

// cache (Only for midi 0 files!)
static uint8_t g_cache[PLAYBACK_CACHE_SIZE];
static int32_t g_cacheStartPos = 0;
static int32_t g_cacheEndPos = 0;
static bool cacheInitialized = false;

// TODO: lay out to external callback handler
void onCacheMiss(uint32_t reqStartPos, uint32_t reqNumBytes, uint32_t cachePosOnReq, uint32_t cacheSize) {
  hal_printfWarning("Cache Miss: requested: %d bytes from %d, cache was at %d with a size of %d!",
    reqNumBytes, reqStartPos, cachePosOnReq, cacheSize);
}

bool requestedChunkStartIsInCache(int32_t startPos, int32_t reqSize, int32_t cacheStartPos, int32_t cacheSize) {
  return startPos >= cacheStartPos && startPos < cacheStartPos + cacheSize;
}

uint32_t readDataToCache(FILE* pFile, uint8_t* cache, int32_t startPos, int32_t num) {
  g_cacheStartPos = startPos;
  cacheInitialized = true;
  hal_fseek(pFile, startPos); 
  return hal_fread(pFile, cache, num);
}

uint32_t readChunkFromCache(void* dst, uint8_t* cache, uint32_t cacheStartPos, int32_t startPos, int32_t num) {
  // This functions reads data from cache and returns the number of bytes read.
  // If the requested chunk is not in cache, 0 will be returned.
  int32_t startPosInCache = startPos - cacheStartPos;
  int32_t bytesToRead = num <= PLAYBACK_CACHE_SIZE ? num : PLAYBACK_CACHE_SIZE;

  if (!cacheInitialized || !requestedChunkStartIsInCache(startPos, num, cacheStartPos, PLAYBACK_CACHE_SIZE))
    return 0;

  memcpy(dst, &cache[startPos - cacheStartPos], bytesToRead); // requested data is in cache
  return bytesToRead;
}

int32_t readChunkFromFile(FILE* pFile, void* dst, int32_t startPos, size_t num) {
  uint32_t bytesReadTotal = 0;
  uint32_t bytesRead = 0;
  uint8_t* dstBytePtr = dst;

  while (num) {
    bytesRead = readChunkFromCache(dstBytePtr, g_cache, g_cacheStartPos, startPos, num);
    bytesReadTotal += bytesRead;
    startPos += bytesRead;
    dstBytePtr += bytesRead;
    num -= bytesRead;

    if (num) {
      // For an unknown reason, sometimes after caching, a few bytes earlier are requested, which will result
      // into another cache miss. To prevent this unnecessary cache miss, a few bytes earlier, from the 
      // requested starting position will be cached.
      // TODO: Find out, which access causes this!
      onCacheMiss(startPos, num, g_cacheStartPos, PLAYBACK_CACHE_SIZE);
      bytesRead = readDataToCache(pFile, g_cache, startPos > 8 ? startPos - 8 : startPos, PLAYBACK_CACHE_SIZE);

      if (bytesRead == 0) // end of file?
        hal_printfWarning("Warning, tried to read over end of file!\r\n");
    }
  }

  return bytesReadTotal;
}

int32_t readByteFromFile(FILE* pFile, uint8_t* dst, int32_t startPos) {
  return readChunkFromFile(pFile, dst, startPos, sizeof(uint8_t));
}

int32_t readWordFromFile(FILE* pFile, uint16_t* dst, int32_t startPos) {
  return readChunkFromFile(pFile, dst, startPos, sizeof(uint16_t));
}

int32_t readDwordFromFile(FILE* pFile, uint32_t* dst, int32_t startPos) {
  return readChunkFromFile(pFile, dst, startPos, sizeof(uint32_t));
}

void setPlaybackTempo(_MIDI_FILE* midiFile, int32_t bpm) {
  midiFile->msPerTick = 60000.0f / (bpm * midiFile->Header.PPQN);
}


/*
** Internal Functions
*/
#define DT_DEF				32			/* assume maximum delta-time + msg is no more than 32 bytes */
#define SWAP_WORD(w)		(uint16_t)(((w)>>8)|((w)<<8))
#define SWAP_DWORD(d)		(uint32_t)((d)>>24)|(((d)>>8)&0xff00)|(((d)<<8)&0xff0000)|(((d)<<24))

// WTF? What is the reason for this _VAR_CAST macro? Hiding content of _MIDI_FILE from user by casting from MIDI_FILE?
#define _VAR_CAST				_MIDI_FILE *pMFembedded = (_MIDI_FILE *)_pMFembedded; 
#define IsFilePtrValid(pMF)		(pMF)
#define IsTrackValid(_x)		  (_midiValidateTrack(pMFembedded, _x))
#define IsChannelValid(_x)		((_x)>=1 && (_x)<=16)
#define IsNoteValid(_x)			  ((_x)>=0 && (_x)<128)
#define IsMessageValid(_x)		((_x)>=msgNoteOff && (_x)<=msgMetaEvent)


// looks ok!
static bool _midiValidateTrack(const _MIDI_FILE *pMFembedded, int32_t iTrack) {
  // normal version
  if (!IsFilePtrValid(pMFembedded))	return false;

  // embedded version
  if (!IsFilePtrValid(pMFembedded))	return false;

  if (pMFembedded->bOpenForWriting) {
    if (iTrack < 0 || iTrack >= MAX_MIDI_TRACKS)
      return false;
  }
  else {	// open for reading
    if (iTrack < 0 || iTrack >= pMFembedded->Header.iNumTracks)
      return false;
  }

  return true;
}

// looks ok!
MIDI_FILE  *midiFileOpen(const char *pFilename) {
  FILE *pFileNew = NULL;
  uint32_t ptrNew;
  bool bValidFile = false;
  cacheInitialized = false; // invalidate cache

  hal_fopen(&pFileNew, pFilename);
  if (pFileNew) {
    /* Is this a valid MIDI file ? */
    ptrNew = 0;
    char magic[5];
    readChunkFromFile(pFileNew, magic, ptrNew, 4);
    magic[4] = '\0';

    if (strcmp(magic, "MThd") == 0) {
      uint32_t dwDataNew;
      uint16_t wDataNew;

      readDwordFromFile(pFileNew, &dwDataNew, 4);
      _midiFile.Header.iHeaderSize = SWAP_DWORD(dwDataNew);

      readWordFromFile(pFileNew, &wDataNew, 8);
      _midiFile.Header.iVersion = (uint16_t)SWAP_WORD(wDataNew);
          
      readWordFromFile(pFileNew, &wDataNew, 10);
      _midiFile.Header.iNumTracks = (uint16_t)SWAP_WORD(wDataNew);

      readWordFromFile(pFileNew, &wDataNew, 12);
      _midiFile.Header.PPQN = (uint16_t)SWAP_WORD(wDataNew);
          
      ptrNew += _midiFile.Header.iHeaderSize + 8;
      /*
      **	 Get all tracks
      */

      // Init
      for (int iTrack = 0; iTrack < MAX_MIDI_TRACKS; ++iTrack) {
        _midiFile.Track[iTrack].pos = 0;
        _midiFile.Track[iTrack].last_status = 0;
      }
          
      for (int iTrack = 0; iTrack < _midiFile.Header.iNumTracks && iTrack < MAX_MIDI_TRACKS; ++iTrack) {
        _midiFile.Track[iTrack].pBaseNew = ptrNew;

        readDwordFromFile(pFileNew, &dwDataNew, ptrNew + 4);
        _midiFile.Track[iTrack].sz = SWAP_DWORD(dwDataNew);
        _midiFile.Track[iTrack].ptrNew = ptrNew + 8;
        _midiFile.Track[iTrack].pEndNew = ptrNew + _midiFile.Track[iTrack].sz + 8;
        ptrNew += _midiFile.Track[iTrack].sz + 8;
      }

      _midiFile.bOpenForWriting = false;
      bValidFile = true;
    }
  }
  
  if (!bValidFile)
    return NULL;
 
  _midiFile.pFile = pFileNew;
   
  //cacheInitialized = false; // invalidate cache (somethings screws up timing, when invalidating cache here!)

  setPlaybackTempo(&_midiFile, MIDI_BPM_DEFAULT);
  setPlaybackTempo(&_midiFile, MIDI_BPM_DEFAULT);

  return (MIDI_FILE *)&_midiFile;  
}

/*
** midiRead* Functions
*/

// ok!
static uint32_t _midiReadVarLen(_MIDI_FILE* pMFembedded, uint32_t* ptrNew, uint32_t* numEmbedded) {
  uint32_t valueEmbedded; // TODO: uint8_t instead of uint32_t ???
  uint8_t c;

  // Variable-length values use the lower 7 bits of a byte for data and the top bit to signal a following data byte. 
  // If the top bit is set to 1 (0x80), then another value byte follows.
  // A variable - length value may use a maximum of 4 bytes. This means the maximum value that can be represented is 
  // 0x0FFFFFFF (represented as 0xFF, 0xFF, 0xFF, 0x7F).

  // TODO: always preload 4 bytes?
  valueEmbedded = 0;
  *ptrNew += readChunkFromFile(pMFembedded->pFile, &valueEmbedded, *ptrNew, 1);
  if (valueEmbedded & 0x80) {
    valueEmbedded &= 0x7f; // Remove the first bit to extract payload
    do {
      *ptrNew += readChunkFromFile(pMFembedded->pFile, &c, *ptrNew, 1);
      valueEmbedded = (valueEmbedded << 7) + (c & 0x7f);
    } while (c & 0x80);
  }

  *numEmbedded = valueEmbedded;
  return(*ptrNew);
}

// ok!
static bool _midiReadTrackCopyData(_MIDI_FILE* pMFembedded, MIDI_MSG* pMsgEmbedded, uint32_t ptrEmbedded, size_t* szEmbedded, bool bCopyPtrData) {
  if (*szEmbedded > META_EVENT_MAX_DATA_SIZE) {
    printf("\r\n_midiReadTrackCopyData; Warning: Meta data is greater than maximum size! (%d of %d)\r\n", szEmbedded, META_EVENT_MAX_DATA_SIZE);
    *szEmbedded = META_EVENT_MAX_DATA_SIZE; // truncate meta data, since we don't have enough space
  }

  if (bCopyPtrData) {
    readChunkFromFile(pMFembedded->pFile, pMsgEmbedded->dataEmbedded, ptrEmbedded, *szEmbedded);
    pMsgEmbedded->data_sz_embedded = *szEmbedded;
  }

  return true;
}

// ok!
int32_t midiReadGetNumTracks(const MIDI_FILE *_pMFembedded) {
  _VAR_CAST;
  return pMFembedded->Header.iNumTracks <= MAX_MIDI_TRACKS ? pMFembedded->Header.iNumTracks : MAX_MIDI_TRACKS;
}

// looks ok! (TODO: running status interruption by realtime messages?)
bool midiReadGetNextMessage(const MIDI_FILE* _pMFembedded, int32_t iTrack, MIDI_MSG* pMsgEmbedded) {
  MIDI_FILE_TRACK *pTrackNew;
  uint32_t bptrEmbedded, pMsgDataPtrEmbedded;
  size_t szEmbedded;

  _VAR_CAST;
  if (!IsTrackValid(iTrack))			return false;
  
  pTrackNew = &pMFembedded->Track[iTrack];
  /* FIXME: Check if there is data on this track first!!!	*/
  if(pTrackNew->ptrNew >= pTrackNew->pEndNew)
    return false;
      
  // Read Delta Time
  _midiReadVarLen(pMFembedded, &pTrackNew->ptrNew, &pMsgEmbedded->dt);
  pTrackNew->pos += pMsgEmbedded->dt;
  pMsgEmbedded->dwAbsPos = pTrackNew->pos;

  bool bRunningStatus = false;
  uint8_t eventType;
  readByteFromFile(pMFembedded->pFile, &eventType, pTrackNew->ptrNew);

  if (eventType & 0x80) {	/* Is this a sys message */
    pMsgEmbedded->iType = (tMIDI_MSG)(eventType & 0xF0);
    pMsgDataPtrEmbedded = pTrackNew->ptrNew + 1;

    /* SysEx & Meta events don't carry channel info, but something
    ** important in their lower bits that we must keep */
    if (pMsgEmbedded->iType == 0xF0)
      pMsgEmbedded->iType = (tMIDI_MSG)(eventType);
  }
  else {  /* just data - so use the last msg type */
    pMsgEmbedded->iType = pMsgEmbedded->iLastMsgType;
    pMsgDataPtrEmbedded = pTrackNew->ptrNew;
    bRunningStatus = true;
  }
  pMsgEmbedded->iLastMsgType = (tMIDI_MSG)pMsgEmbedded->iType;

  if (!bRunningStatus)
    pMsgEmbedded->iLastMsgChnl = (uint8_t)(eventType & 0x0f) + 1;
  
  switch (pMsgEmbedded->iType) {
    // -------------------------
    // -    Channel Events     -
    // -------------------------
    case	msgNoteOff: { // 0x08 'Note Off'
      uint8_t tmpNote = 0;
      pMsgEmbedded->MsgData.NoteOff.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpNote, pMsgDataPtrEmbedded);
      pMsgEmbedded->MsgData.NoteOff.iNote = tmpNote;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }

    case	msgNoteOn: { // 0x09 'Note On'
      uint8_t tmpNote = 0;
      uint8_t tmpVolume = 0;
      pMsgEmbedded->MsgData.NoteOn.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpNote, pMsgDataPtrEmbedded);
      readByteFromFile(pMFembedded->pFile, &tmpVolume, pMsgDataPtrEmbedded + 1);
      pMsgEmbedded->MsgData.NoteOn.iNote = tmpNote;
      pMsgEmbedded->MsgData.NoteOn.iVolume = tmpVolume;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }
      
    case	msgNoteKeyPressure: { // 0x0A 'Note Aftertouch'
      uint8_t tmpNote = 0;
      uint8_t tmpPressure = 0;
      pMsgEmbedded->MsgData.NoteKeyPressure.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpNote, pMsgDataPtrEmbedded);
      readByteFromFile(pMFembedded->pFile, &tmpPressure, pMsgDataPtrEmbedded + 1);
      pMsgEmbedded->MsgData.NoteKeyPressure.iNote = tmpNote;
      pMsgEmbedded->MsgData.NoteKeyPressure.iPressure = tmpPressure;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }

    case	msgSetParameter: { // // 0x0B 'Controller'
      uint8_t tmpControl = 0;
      uint8_t tmpParam = 0;
      pMsgEmbedded->MsgData.NoteParameter.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpControl, pMsgDataPtrEmbedded);
      readByteFromFile(pMFembedded->pFile, &tmpParam, pMsgDataPtrEmbedded + 1);
      pMsgEmbedded->MsgData.NoteParameter.iControl = tmpControl;
      pMsgEmbedded->MsgData.NoteParameter.iParam = tmpParam;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }

    case	msgSetProgram: { // 0x0C 'Program Change'
      uint8_t tmpProgram = 0;
      pMsgEmbedded->MsgData.ChangeProgram.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpProgram, pMsgDataPtrEmbedded);
      pMsgEmbedded->MsgData.ChangeProgram.iProgram = tmpProgram;
      pMsgEmbedded->iMsgSize = 2;
      break;
    }

    case	msgChangePressure: { // 0x0D 'Channel Aftertouch'
      uint8_t tmpPressure = 0;
      pMsgEmbedded->MsgData.ChangePressure.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpPressure, pMsgDataPtrEmbedded);
      pMsgEmbedded->iMsgSize = 2;
      break;
    }

    case	msgSetPitchWheel: { // 0x0F 'Pitch Bend'
      pMsgEmbedded->MsgData.PitchWheel.iChannel = pMsgEmbedded->iLastMsgChnl;
      uint8_t tmpPitchLow = 0;
      uint8_t tmpPitchHigh = 0;
      readByteFromFile(pMFembedded->pFile, &tmpPitchLow, pMsgDataPtrEmbedded);
      readByteFromFile(pMFembedded->pFile, &tmpPitchHigh, pMsgDataPtrEmbedded + 1);
      pMsgEmbedded->MsgData.PitchWheel.iPitch = tmpPitchLow | (tmpPitchHigh << 7);
      pMsgEmbedded->MsgData.PitchWheel.iPitch -= MIDI_WHEEL_CENTRE;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }

    // -------------------------
    // -    Meta Events     -
    // -------------------------
    case	msgMetaEvent:
      /* We can use 'pTrack->ptr' from now on, since meta events
      ** always have bit 7 set */

      // Get Meta Event Type
      bptrEmbedded = pTrackNew->ptrNew;
      uint8_t tmpType = 0;
      readByteFromFile(pMFembedded->pFile, &tmpType, pTrackNew->ptrNew + 1);
      pMsgEmbedded->MsgData.MetaEvent.iType = tmpType;

      // Get Meta Event Length (TODO: find a 'live' method instead of using a constant sized buffer?)
      pTrackNew->ptrNew += 2;
      _midiReadVarLen(pMFembedded, &pTrackNew->ptrNew, &pMsgEmbedded->iMsgSize);
      szEmbedded = pTrackNew->ptrNew - bptrEmbedded + pMsgEmbedded->iMsgSize;

      if (_midiReadTrackCopyData(pMFembedded, pMsgEmbedded, pTrackNew->ptrNew, &szEmbedded, false) == false)
        return false;

      /* Now copy the data...*/
      readChunkFromFile(pMFembedded->pFile, pMsgEmbedded->dataEmbedded, bptrEmbedded, szEmbedded);

      /* Place the META data it in a neat structure also for embedded! */
      switch(pMsgEmbedded->MsgData.MetaEvent.iType) {
        case	metaSequenceNumber: {
              uint8_t tmpSequenceNumber;
              readByteFromFile(pMFembedded->pFile, &tmpSequenceNumber, pTrackNew->ptrNew + 0);
              pMsgEmbedded->MsgData.MetaEvent.Data.iSequenceNumber = tmpSequenceNumber;
              break;
            }
       
        case	metaTextEvent:
        case	metaCopyright:
        case	metaTrackName:
        case	metaInstrument:
        case	metaLyric:
        case	metaMarker:
        case	metaCuePoint:
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.strLen = szEmbedded - 3;
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.pData = pMsgEmbedded->dataEmbedded + 3;
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.pData[pMsgEmbedded->MsgData.MetaEvent.Data.Text.strLen] = '\0'; // Add Null terminator
            break;

        case	metaMIDIPort: {
          uint8_t tmpMIDIPort;
          readByteFromFile(pMFembedded->pFile, &tmpMIDIPort, pTrackNew->ptrNew + 0);
          pMsgEmbedded->MsgData.MetaEvent.Data.iMIDIPort = tmpMIDIPort;
          break;
        }
        case	metaEndSequence:
            /* NO DATA */
            break;
        case	metaSetTempo: { // looks ok!
              uint8_t mpqn[3];
              readChunkFromFile(pMFembedded->pFile, mpqn, pTrackNew->ptrNew, 3);
              int32_t iMPQN = (mpqn[0] << 16) | (mpqn[1] << 8) | mpqn[2];
              pMsgEmbedded->MsgData.MetaEvent.Data.Tempo.iBPM = MICROSECONDS_PER_MINUTE / iMPQN;
            }
            break;
        case	metaSMPTEOffset: {
            // embedded
            uint8_t tmpSMPTE[5];
            readChunkFromFile(pMFembedded->pFile, tmpSMPTE, pTrackNew->ptrNew, 5);
            pMsgEmbedded->MsgData.MetaEvent.Data.SMPTE.iHours = tmpSMPTE[0];
            pMsgEmbedded->MsgData.MetaEvent.Data.SMPTE.iMins = tmpSMPTE[1];
            pMsgEmbedded->MsgData.MetaEvent.Data.SMPTE.iSecs = tmpSMPTE[2];
            pMsgEmbedded->MsgData.MetaEvent.Data.SMPTE.iFrames = tmpSMPTE[3];
            pMsgEmbedded->MsgData.MetaEvent.Data.SMPTE.iFF = tmpSMPTE[4];
            break;
        }
        case	metaTimeSig: {
            /* TODO: Variations without 24 & 8 */
            uint8_t tmpTimeSig[2];
            readChunkFromFile(pMFembedded->pFile, tmpTimeSig, pTrackNew->ptrNew, 2);
            pMsgEmbedded->MsgData.MetaEvent.Data.TimeSig.iNom = tmpTimeSig[0];
            pMsgEmbedded->MsgData.MetaEvent.Data.TimeSig.iDenom = tmpTimeSig[1] * MIDI_NOTE_MINIM;
        }
            break;
        case	metaKeySig: { // TODO: check!
            uint8_t tmp;
            readByteFromFile(pMFembedded->pFile, &tmp, pTrackNew->ptrNew);

            if (tmp & 0x80) {
              /* Do some trendy sign extending in reverse :) */
              readByteFromFile(pMFembedded->pFile, &tmp, pTrackNew->ptrNew);
              pMsgEmbedded->MsgData.MetaEvent.Data.KeySig.iKey = (256 - tmp) & keyMaskKey;
              pMsgEmbedded->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskNeg;
            }
            else {
              readByteFromFile(pMFembedded->pFile, &tmp, pTrackNew->ptrNew);
              pMsgEmbedded->MsgData.MetaEvent.Data.KeySig.iKey = (tMIDI_KEYSIG)(tmp & keyMaskKey);
            }

            readByteFromFile(pMFembedded->pFile, &tmp, pTrackNew->ptrNew + 1);
            if (tmp)
              pMsgEmbedded->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskMin; // TODO: check!
          }
          break;
        case	metaSequencerSpecific:
          pMsgEmbedded->MsgData.MetaEvent.Data.Sequencer.iSize = pMsgEmbedded->iMsgSize;
          pMsgEmbedded->MsgData.MetaEvent.Data.Sequencer.pData = pMsgEmbedded->dataEmbedded + 3;
          break;
      }

      pTrackNew->ptrNew += pMsgEmbedded->iMsgSize;
      pMsgEmbedded->iMsgSize = szEmbedded;
      break;

    // ----------------------------------
    // -    System Exclusive Events     -
    // ----------------------------------
    case	msgSysEx1:
    case	msgSysEx2:
      bptrEmbedded = pTrackNew->ptrNew;
      pTrackNew->ptrNew += 1; 
      _midiReadVarLen(pMFembedded, &pTrackNew->ptrNew, &pMsgEmbedded->iMsgSize);
      szEmbedded = (pTrackNew->ptrNew - bptrEmbedded) + pMsgEmbedded->iMsgSize;

      if (_midiReadTrackCopyData(pMFembedded, pMsgEmbedded, pTrackNew->ptrNew, &szEmbedded, false) == false)
        return false;
          
      /* Embedded: Now copy the data */
      readChunkFromFile(pMFembedded->pFile, pMsgEmbedded->dataEmbedded, bptrEmbedded, szEmbedded);
      pTrackNew->ptrNew += pMsgEmbedded->iMsgSize;
      pMsgEmbedded->iMsgSize = szEmbedded;
      pMsgEmbedded->MsgData.SysEx.pData = pMsgEmbedded->dataEmbedded;
      pMsgEmbedded->MsgData.SysEx.iSize = szEmbedded;
      break;
  }
  /*
  ** Standard MIDI messages use a common copy routine
  */

  // embedded (needs to be checked!)
  pMsgEmbedded->bImpliedMsg = false;
  if ((pMsgEmbedded->iType & 0xf0) != 0xf0) {
    uint8_t tmpVal = 0;
    readByteFromFile(pMFembedded->pFile, &tmpVal, pTrackNew->ptrNew);
    if (tmpVal & 0x80) {
    }
    else {
      pMsgEmbedded->bImpliedMsg = true;
      pMsgEmbedded->iImpliedMsg = pMsgEmbedded->iLastMsgType;
      pMsgEmbedded->iMsgSize--;
    }

    _midiReadTrackCopyData(pMFembedded, pMsgEmbedded, pTrackNew->ptrNew, &pMsgEmbedded->iMsgSize, true);
    pTrackNew->ptrNew += pMsgEmbedded->iMsgSize;
  }

  return true;
}
 // ok!
void midiReadInitMessage(MIDI_MSG *pMsg) {
  pMsg->data_sz_embedded = 0;
  pMsg->bImpliedMsg = false;
}

// TODO: 'open for write' implementation!
bool	midiFileClose(MIDI_FILE* _pMFembedded) {
  _VAR_CAST;
  if (!IsFilePtrValid(pMFembedded))			return false;

  // TODO: open for writing implementation here!
  if (pMFembedded->pFile)
    return hal_fclose(pMFembedded->pFile);
  
  return true;
}