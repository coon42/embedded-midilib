#ifndef _MIDIFILE_H
#define _MIDIFILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "midiinfo.h"		/* enumerations and constants for GM */

/*
 * midiFile.c -  Header file for Steevs MIDI Library
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 1998-2010, Steven Goodwin.
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
 */

/* 
** All functions start with one of the following prefixes:
**		midiFile*		For non-GM features that relate to the file, and have
**						no use once the file has been created, i.e. CreateFile
**						or SetTrack (those data is embedded into the file, but
**						not explicitly stored)
**		midiSong*   For operations that work across the song, i.e. SetTempo
**		midiTrack*  For operations on a specific track, i.e. AddNoteOn
*/

/*
** Types because we're dealing with files, and need to be careful
*/

// Cache
#define PLAYBACK_CACHE_SIZE 10 * 1024 // 10KB cache

// Embedded Constants
#define META_EVENT_MAX_DATA_SIZE 128 // The meta event size must be at least 5 bytes long, to store: variable 4 byte length, 1 byte event id.

/*
** MIDI Constants
*/
#define MIDI_BPM_DEFAULT    120
#define MIDI_PPQN_DEFAULT		384
#define MIDI_VERSION_DEFAULT	1

/*
** MIDI Limits
*/

// This parameter should be set as small as possible. Each track will need 60 Bytes of memory.
// Using 32 Tracks will need about 1KB of RAM.
#define MAX_MIDI_TRACKS			32  // [default: 32] - Maximum supported tracks. Can be set to 1 on MIDI type 0 tracks, should be at least 1 on MIDI type 1 files.

// Don't change this!
#define MICROSECONDS_PER_MINUTE 60000000L

// temporary, move back to c file later, to make this private!
/*
** Internal Data Structures
*/
typedef struct 	{
  uint8_t note, chn;
  uint8_t valid, p2;
  uint32_t end_pos;
} MIDI_LAST_NOTE;

typedef struct {
  int32_t	iIdx;
  int32_t	iEndPos;
} MIDI_END_POINT;

typedef struct 	{
  uint32_t ptrNew;
  uint32_t pBaseNew;
  uint32_t pEndNew;

  uint32_t pos; // position of file pointer
  int32_t deltaTime; // relative offset, when this event occurs. May be negative, if current event is delayed
  /* For Reading MIDI Files */
  uint32_t sz;						/* size of whole iTrack */
  /* For Writing MIDI Files */
  uint32_t iBlockSize;				/* max size of track */
  uint8_t iDefaultChannel;		/* use for write only */
  uint8_t last_status;				/* used for running status */

  uint32_t debugLastClock;
  uint32_t debugLastMsgDt;

} MIDI_FILE_TRACK;

typedef struct 	{
  uint32_t	iHeaderSize;
  /**/
  uint16_t	iVersion;		/* 0, 1 or 2 */
  uint16_t	iNumTracks;		/* number of tracks... (will be 1 for MIDI type 0) */
  uint16_t	PPQN;			/* pulses per quarter note */
} MIDI_HEADER;

typedef struct {
  FILE				*pFile;
  bool				bOpenForWriting;

  MIDI_HEADER			Header;
  uint32_t file_sz;
  int32_t usPerTick; // microseconds per tick

  MIDI_FILE_TRACK		Track[MAX_MIDI_TRACKS];
} _MIDI_FILE;

/*
** MIDI structures, accessibly externably
*/
typedef	void 	MIDI_FILE;
typedef struct {
          tMIDI_MSG	iType;

          int32_t		dt;		/* delta time */
          uint32_t		dwAbsPos;
          uint32_t		iMsgSize;

          bool		bImpliedMsg;
          tMIDI_MSG	iImpliedMsg;

          /* Raw data chunk */
          uint8_t dataEmbedded[META_EVENT_MAX_DATA_SIZE + 1]; // constant data block (+ 1 byte for nullterminator on text events)
          uint32_t data_sz_embedded; // This is the real size of meta text data!

          union {
            struct {
                int32_t			iNote;
                int32_t			iChannel;
                int32_t			iVolume;
                } NoteOn;
            struct {
                int32_t			iNote;
                int32_t			iChannel;
                } NoteOff;
            struct {
                int32_t			iNote;
                int32_t			iChannel;
                int32_t			iPressure;
                } NoteKeyPressure;
            struct {
                int32_t			iChannel;
                tMIDI_CC	iControl;
                int32_t			iParam;
                } NoteParameter;
            struct {
                int32_t			iChannel;
                int32_t			iProgram;
                } ChangeProgram;
            struct {
                int32_t			iChannel;
                int32_t			iPressure;
                } ChangePressure;
            struct {
                int32_t			iChannel;
                int32_t			iPitch;
                } PitchWheel;
            struct {
                tMIDI_META	iType;
                uint8_t iSize;
                union {
                  int32_t					iMIDIPort;
                  int32_t					iSequenceNumber;
                  struct {
                    int32_t strLen;
                    uint8_t			*pData;
                    } Text;
                  struct {
                    int32_t				iBPM;
                    } Tempo;
                  struct {
                    int32_t				iHours, iMins;
                    int32_t				iSecs, iFrames,iFF;
                    } SMPTE;
                  struct {
                    tMIDI_KEYSIG	iKey;
                    } KeySig;
                  struct {
                    int32_t				iNom, iDenom;
                    } TimeSig;
                  struct {
                    uint8_t			*pData;
                    int32_t				iSize;
                    } Sequencer;
                  } Data;
                } MetaEvent;
            struct {
                uint8_t		*pData;
                int32_t			iSize;
                } SysEx;
            } MsgData;

        /* State information - Please treat these as private*/
        tMIDI_MSG	iLastMsgType;
        uint8_t		iLastMsgChnl;
  
        } MIDI_MSG;

/*
** midiFile* Prototypes
*/
int32_t readChunkFromFile(FILE* pFile, void* dst, int32_t startPos, size_t num);
int32_t readByteFromFile(FILE* pFile, uint8_t* dst, int32_t startPos);
int32_t readWordFromFile(FILE* pFile, uint16_t* dst, int32_t startPos);
int32_t readDwordFromFile(FILE* pFile, uint32_t* dst, int32_t startPos);
void setPlaybackTempo(_MIDI_FILE* pMidiFile, int32_t bpm);

MIDI_FILE  *midiFileCreate(const char *pFilename, bool bOverwriteIfExists);
int32_t			midiFileSetTracksDefaultChannel(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iChannel);
int32_t			midiFileGetTracksDefaultChannel(MIDI_FILE* _pMFembedded, int32_t iTrack);
bool	  midiFileFlushTrack(MIDI_FILE* _pMFembedded, int32_t iTrack, bool bFlushToEnd, uint32_t dwEndTimePos);
bool		midiFileSyncTracks(MIDI_FILE* _pMFembedded, int32_t iTrack1, int32_t iTrack2);
int32_t			midiFileSetPPQN(MIDI_FILE* _pMFembedded, int32_t PPQN);
int32_t			midiFileGetPPQN(MIDI_FILE* _pMFembedded);
int32_t			midiFileSetVersion(MIDI_FILE* _pMFembedded, int32_t iVersion);
int32_t			midiFileGetVersion(MIDI_FILE* _pMFembedded);
MIDI_FILE  *midiFileOpen(const char *pFilename);
bool		midiFileClose(MIDI_FILE* _pMFembedded);

/*
** midiSong* Prototypes
*/
bool		midiSongAddSMPTEOffset(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iHours, int32_t iMins, int32_t iSecs, int32_t iFrames, int32_t iFFrames);
bool		midiSongAddSimpleTimeSig(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom);
bool		midiSongAddTimeSig(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom, int32_t iClockInMetroTick, int32_t iNotated32nds);
bool		midiSongAddKeySig(MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_KEYSIG iKey);
bool		midiSongAddTempo(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iTempo);
bool		midiSongAddMIDIPort(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iPort);
bool		midiSongAddEndSequence(MIDI_FILE* _pMFembedded, int32_t iTrack);

/*
** midiTrack* Prototypes
*/
bool		midiTrackAddRaw(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iDataSize, const uint8_t *pData, bool bMovePtr, int32_t iDeltaTime);
bool		midiTrackIncTime(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iDeltaTime, bool bOverridePPQN);
bool		midiTrackAddText(MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_TEXT iType, const char *pTxt);
bool		midiTrackAddMsg(MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_MSG iMsg, int32_t iParam1, int32_t iParam2);
bool		midiTrackSetKeyPressure(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNote, int32_t iAftertouch);
bool		midiTrackAddControlChange(MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_CC iCCType, int32_t iParam);
bool		midiTrackAddProgramChange(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iInstrPatch);
bool		midiTrackChangeKeyPressure(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded,  int32_t iTrack, int32_t iDeltaPressure);
bool		midiTrackSetPitchWheel(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iWheelPos);
bool		midiTrackAddNote(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNote, int32_t iLength, int32_t iVol, bool bAutoInc, bool bOverrideLength);
bool		midiTrackAddRest(MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iLength, bool bOverridePPQN);
bool		midiTrackGetEndPos(MIDI_FILE* _pMFembedded, int32_t iTrack);

/*
** midiRead* Prototypes
*/
int32_t midiReadGetNumTracks(const MIDI_FILE* _pMFembedded);
bool		midiReadGetNextMessage(const MIDI_FILE* _pMFembedded, int32_t iTrack, MIDI_MSG* pMsgEmbedded);
void midiReadInitMessage(MIDI_MSG *pMsg);


#endif /* _MIDIFILE_H */

