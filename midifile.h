#ifndef _MIDIFILE_H
#define _MIDIFILE_H

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
typedef	unsigned char		BYTE;
typedef	unsigned short  WORD;
typedef	unsigned long		DWORD;
typedef int					BOOL;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif


// Embedded Constants
#define META_EVENT_MAX_DATA_SIZE 64


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
  BYTE note, chn;
  BYTE valid, p2;
  DWORD end_pos;
} MIDI_LAST_NOTE;

typedef struct 	{
  BYTE *ptr; // obsolete
  BYTE *pBase; // obsolete
  BYTE *pEnd; // obsolete
  DWORD ptrNew;
  DWORD pBaseNew;
  DWORD pEndNew;

  DWORD pos; // TODO: which pos is meant here?
  DWORD dt; // TODO: what is this?
  /* For Reading MIDI Files */
  DWORD sz;						/* size of whole iTrack */
  /* For Writing MIDI Files */
  DWORD iBlockSize;				/* max size of track */
  BYTE iDefaultChannel;		/* use for write only */
  BYTE last_status;				/* used for running status */

  MIDI_LAST_NOTE LastNote[MAX_TRACK_POLYPHONY];
} MIDI_FILE_TRACK;

typedef struct 	{
  DWORD	iHeaderSize;
  /**/
  WORD	iVersion;		/* 0, 1 or 2 */
  WORD	iNumTracks;		/* number of tracks... (will be 1 for MIDI type 0) */
  WORD	PPQN;			/* pulses per quarter note */
} MIDI_HEADER;

typedef struct {
  FILE				*pFile;
  BOOL				bOpenForWriting;

  MIDI_HEADER			Header;
  BYTE *ptr;			/* to whole data block */
  DWORD file_sz;

  MIDI_FILE_TRACK		Track[MAX_MIDI_TRACKS];
} _MIDI_FILE;

/*
** MIDI structures, accessibly externably
*/
typedef	void 	MIDI_FILE;
typedef struct {
					tMIDI_MSG	iType;

					DWORD		dt;		/* delta time */
					DWORD		dwAbsPos;
					DWORD		iMsgSize;

					BOOL		bImpliedMsg;
					tMIDI_MSG	iImpliedMsg;

					/* Raw data chunk */
					BYTE *data;		/* dynamic data block */
          BYTE dataEmbedded[META_EVENT_MAX_DATA_SIZE]; // constant data block
					DWORD data_sz; // This is the size of the dynamic allocated buffer
          DWORD data_sz_embedded; // This is the real size of meta text data!

					union {
						struct {
								int			iNote;
								int			iChannel;
								int			iVolume;
								} NoteOn;
						struct {
								int			iNote;
								int			iChannel;
								} NoteOff;
						struct {
								int			iNote;
								int			iChannel;
								int			iPressure;
								} NoteKeyPressure;
						struct {
								int			iChannel;
								tMIDI_CC	iControl;
								int			iParam;
								} NoteParameter;
						struct {
								int			iChannel;
								int			iProgram;
								} ChangeProgram;
						struct {
								int			iChannel;
								int			iPressure;
								} ChangePressure;
						struct {
								int			iChannel;
								int			iPitch;
								} PitchWheel;
						struct {
								tMIDI_META	iType;
								union {
									int					iMIDIPort;
									int					iSequenceNumber;
									struct {
										BYTE			*pData;
										} Text;
									struct {
										int				iBPM;
										} Tempo;
									struct {
										int				iHours, iMins;
										int				iSecs, iFrames,iFF;
										} SMPTE;
									struct {
										tMIDI_KEYSIG	iKey;
										} KeySig;
									struct {
										int				iNom, iDenom;
										} TimeSig;
									struct {
										BYTE			*pData;
										int				iSize;
										} Sequencer;
									} Data;
								} MetaEvent;
						struct {
								BYTE		*pData;
								int			iSize;
								} SysEx;
						} MsgData;

				/* State information - Please treat these as private*/
				tMIDI_MSG	iLastMsgType;
				BYTE		iLastMsgChnl;
	
				} MIDI_MSG;

/*
** midiFile* Prototypes
*/
MIDI_FILE  *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists);
int			midiFileSetTracksDefaultChannel(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iChannel, BOOL embedded);
int			midiFileGetTracksDefaultChannel(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, BOOL embedded);
BOOL	  midiFileFlushTrack(MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int iTrack, BOOL bFlushToEnd, DWORD dwEndTimePos);
BOOL		midiFileSyncTracks(MIDI_FILE* pMF, MIDI_FILE* _pMFembedded, int iTrack1, int iTrack2);
int			midiFileSetPPQN(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int PPQN);
int			midiFileGetPPQN(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
int			midiFileSetVersion(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iVersion);
int			midiFileGetVersion(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
MIDI_FILE  *midiFileOpen(const char *pFilename, BOOL embedded);
BOOL		midiFileClose(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);

/*
** midiSong* Prototypes
*/
BOOL		midiSongAddSMPTEOffset(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iHours, int iMins, int iSecs, int iFrames, int iFFrames);
BOOL		midiSongAddSimpleTimeSig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iNom, int iDenom);
BOOL		midiSongAddTimeSig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iNom, int iDenom, int iClockInMetroTick, int iNotated32nds);
BOOL		midiSongAddKeySig(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, tMIDI_KEYSIG iKey);
BOOL		midiSongAddTempo(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iTempo);
BOOL		midiSongAddMIDIPort(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iPort);
BOOL		midiSongAddEndSequence(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack);

/*
** midiTrack* Prototypes
*/
BOOL		midiTrackAddRaw(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iDataSize, const BYTE *pData, BOOL bMovePtr, int iDeltaTime);
BOOL		midiTrackIncTime(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iDeltaTime, BOOL bOverridePPQN);
BOOL		midiTrackAddText(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, tMIDI_TEXT iType, const char *pTxt);
BOOL		midiTrackAddMsg(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, tMIDI_MSG iMsg, int iParam1, int iParam2);
BOOL		midiTrackSetKeyPressure(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iNote, int iAftertouch);
BOOL		midiTrackAddControlChange(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, tMIDI_CC iCCType, int iParam);
BOOL		midiTrackAddProgramChange(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iInstrPatch);
BOOL		midiTrackChangeKeyPressure(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded,  int iTrack, int iDeltaPressure);
BOOL		midiTrackSetPitchWheel(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iWheelPos);
BOOL		midiTrackAddNote(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iNote, int iLength, int iVol, BOOL bAutoInc, BOOL bOverrideLength);
BOOL		midiTrackAddRest(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack, int iLength, BOOL bOverridePPQN);
BOOL		midiTrackGetEndPos(MIDI_FILE *pMF, MIDI_FILE* _pMFembedded, int iTrack);

/*
** midiRead* Prototypes
*/
int			midiReadGetNumTracks(const MIDI_FILE *pMF, MIDI_FILE* _pMFembedded);
BOOL		midiReadGetNextMessage(const MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int iTrack, MIDI_MSG* pMsg, MIDI_MSG* pMsgEmbedded, BOOL embedded);
void		midiReadInitMessage(MIDI_MSG *pMsg);
void		midiReadFreeMessage(MIDI_MSG *pMsg);


#endif /* _MIDIFILE_H */

