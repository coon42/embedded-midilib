#ifndef _MIDIFILE_H
#define _MIDIFILE_H

#include <stdint.h>
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
**		midiSong*		For operations that work across the song, i.e. SetTempo
**		midiTrack*		For operations on a specific track, i.e. AddNoteOn
*/

/*
** Types because we're dealing with files, and need to be careful
*/
typedef int32_t	  BOOL;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif


// Embedded Constants
#define META_EVENT_MAX_DATA_SIZE 128 // The meta event size must be at least 5 bytes long, to store: variable 4 byte length, 1 byte event id.


/*
** MIDI Constants
*/
#define MIDI_PPQN_DEFAULT		384
#define MIDI_VERSION_DEFAULT	1

/*
** MIDI Limits
*/
#define MAX_MIDI_TRACKS			256
#define MAX_TRACK_POLYPHONY		64

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

typedef struct 	{
  uint8_t *ptr; // obsolete
  uint8_t *pBase; // obsolete
  uint8_t *pEnd; // obsolete
  uint32_t ptrNew;
  uint32_t pBaseNew;
  uint32_t pEndNew;

  uint32_t pos; // TODO: which pos is meant here?
  uint32_t dt; // TODO: what is this?
  /* For Reading MIDI Files */
  uint32_t sz;						/* size of whole iTrack */
  /* For Writing MIDI Files */
  uint32_t iBlockSize;				/* max size of track */
  uint8_t iDefaultChannel;		/* use for write only */
  uint8_t last_status;				/* used for running status */

  MIDI_LAST_NOTE LastNote[MAX_TRACK_POLYPHONY];
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
  BOOL				bOpenForWriting;

  MIDI_HEADER			Header;
  uint8_t *ptr;			/* to whole data block */
  uint32_t file_sz;

  MIDI_FILE_TRACK		Track[MAX_MIDI_TRACKS];
} _MIDI_FILE;

/*
** MIDI structures, accessibly externably
*/
typedef	void 	MIDI_FILE;
typedef struct {
					tMIDI_MSG	iType;

					uint32_t		dt;		/* delta time */
					uint32_t		dwAbsPos;
					uint32_t		iMsgSize;

					BOOL		bImpliedMsg;
					tMIDI_MSG	iImpliedMsg;

					/* Raw data chunk */
					uint8_t *data;		/* dynamic data block */
          uint8_t dataEmbedded[META_EVENT_MAX_DATA_SIZE + 1]; // constant data block (+ 1 byte for nullterminator on text events)
					uint32_t data_sz; // This is the size of the dynamic allocated buffer
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
MIDI_FILE  *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists);
int32_t			midiFileSetTracksDefaultChannel(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iChannel, BOOL embedded);
int32_t			midiFileGetTracksDefaultChannel(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, BOOL embedded);
BOOL	  midiFileFlushTrack(MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, BOOL bFlushToEnd, uint32_t dwEndTimePos);
BOOL		midiFileSyncTracks(MIDI_FILE* pMF, MIDI_FILE* _pMFembedded, int32_t iTrack1, int32_t iTrack2);
int32_t			midiFileSetPPQN(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t PPQN);
int32_t			midiFileGetPPQN(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
int32_t			midiFileSetVersion(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iVersion);
int32_t			midiFileGetVersion(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
MIDI_FILE  *midiFileOpen(const char *pFilename, BOOL embedded);
BOOL		midiFileClose(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);

/*
** midiSong* Prototypes
*/
BOOL		midiSongAddSMPTEOffset(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iHours, int32_t iMins, int32_t iSecs, int32_t iFrames, int32_t iFFrames);
BOOL		midiSongAddSimpleTimeSig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom);
BOOL		midiSongAddTimeSig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom, int32_t iClockInMetroTick, int32_t iNotated32nds);
BOOL		midiSongAddKeySig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_KEYSIG iKey);
BOOL		midiSongAddTempo(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iTempo);
BOOL		midiSongAddMIDIPort(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iPort);
BOOL		midiSongAddEndSequence(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack);

/*
** midiTrack* Prototypes
*/
BOOL		midiTrackAddRaw(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iDataSize, const uint8_t *pData, BOOL bMovePtr, int32_t iDeltaTime);
BOOL		midiTrackIncTime(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iDeltaTime, BOOL bOverridePPQN);
BOOL		midiTrackAddText(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_TEXT iType, const char *pTxt);
BOOL		midiTrackAddMsg(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_MSG iMsg, int32_t iParam1, int32_t iParam2);
BOOL		midiTrackSetKeyPressure(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNote, int32_t iAftertouch);
BOOL		midiTrackAddControlChange(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_CC iCCType, int32_t iParam);
BOOL		midiTrackAddProgramChange(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iInstrPatch);
BOOL		midiTrackChangeKeyPressure(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded,  int32_t iTrack, int32_t iDeltaPressure);
BOOL		midiTrackSetPitchWheel(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iWheelPos);
BOOL		midiTrackAddNote(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNote, int32_t iLength, int32_t iVol, BOOL bAutoInc, BOOL bOverrideLength);
BOOL		midiTrackAddRest(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iLength, BOOL bOverridePPQN);
BOOL		midiTrackGetEndPos(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int32_t iTrack);

/*
** midiRead* Prototypes
*/
int32_t			midiReadGetNumTracks(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
BOOL		midiReadGetNextMessage(const MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, MIDI_MSG* pMsg, MIDI_MSG* pMsgEmbedded, BOOL embedded);
void		midiReadInitMessage(MIDI_MSG *pMsg);
void		midiReadFreeMessage(MIDI_MSG *pMsg);


#endif /* _MIDIFILE_H */

