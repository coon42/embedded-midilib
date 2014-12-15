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
 * TODO: support of running status!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"


// -----------------------------------
// Global variables and new functions
// -----------------------------------
_MIDI_FILE _midiFile;

int32_t readChunkFromFile(FILE* pFile, void* dst, int32_t startPos, size_t num) {
  fseek(pFile, startPos, SEEK_SET);
  return fread_s(dst, num, 1, num, pFile); // TODO: word access? Does it increase performance?
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


/*
** Internal Functions
*/
#define DT_DEF				32			/* assume maximum delta-time + msg is no more than 32 bytes */
#define SWAP_WORD(w)		(uint16_t)(((w)>>8)|((w)<<8))
#define SWAP_DWORD(d)		(uint32_t)((d)>>24)|(((d)>>8)&0xff00)|(((d)<<8)&0xff0000)|(((d)<<24))

#define _VAR_CAST				_MIDI_FILE *pMF = (_MIDI_FILE *)_pMF; _MIDI_FILE *pMFembedded = (_MIDI_FILE *)_pMFembedded; // WTF? What is the reason for this macro???
#define IsFilePtrValid(pMF)		(pMF)
#define IsTrackValid(_x)		  (_midiValidateTrack(pMF, pMFembedded, _x))
#define IsChannelValid(_x)		((_x)>=1 && (_x)<=16)
#define IsNoteValid(_x)			  ((_x)>=0 && (_x)<128)
#define IsMessageValid(_x)		((_x)>=msgNoteOff && (_x)<=msgMetaEvent)


// looks ok!
static BOOL _midiValidateTrack(const _MIDI_FILE *pMF, const _MIDI_FILE *pMFembedded, int32_t iTrack) {
  // normal version
	if (!IsFilePtrValid(pMF))	return FALSE;

	if (pMF->bOpenForWriting) {
		if (iTrack < 0 || iTrack >= MAX_MIDI_TRACKS)
			return FALSE;
  }
	else {	/* open for reading */
		if (!pMF->ptr)
			return FALSE;
		
		if (iTrack < 0 || iTrack>=pMF->Header.iNumTracks)
			return FALSE;
  }

  // embedded version
  if (!IsFilePtrValid(pMFembedded))	return FALSE;

  if (pMF->bOpenForWriting) {
    if (iTrack < 0 || iTrack >= MAX_MIDI_TRACKS)
      return FALSE;
  }
  else {	// open for reading
    if (iTrack < 0 || iTrack >= pMF->Header.iNumTracks)
      return FALSE;
  }

	return TRUE;
}

// TODO!
static uint8_t *_midiWriteVarLen(uint8_t *ptr, int32_t n) {
  register long buffer;
  register long value=n;

	buffer = value & 0x7f;
	while ((value >>= 7) > 0) {
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7f);
  }

	while (TRUE) {
		*ptr++ = (uint8_t)buffer;
		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
  }
	
	return(ptr);
}

/* Return a ptr to valid block of memory to store a message
** of up to sz_reqd bytes 
*/

// TODO!
static uint8_t *_midiGetPtr(_MIDI_FILE *pMF, int32_t iTrack, int32_t sz_reqd) {
  const uint32_t mem_sz_inc = 8092;	/* arbitary */
  uint8_t *ptr;
  int32_t curr_offset;
  MIDI_FILE_TRACK *pTrack = &pMF->Track[iTrack];

	ptr = pTrack->ptr;
	if (ptr == NULL || ptr+sz_reqd > pTrack->pEnd) {	/* need more RAM! */
		curr_offset = ptr-pTrack->pBase;
		if ((ptr = (uint8_t *)realloc(pTrack->pBase, mem_sz_inc+pTrack->iBlockSize))) {
			pTrack->pBase = ptr;
			pTrack->iBlockSize += mem_sz_inc;
			pTrack->pEnd = ptr+pTrack->iBlockSize;
			/* Move new ptr to continue data entry: */
			pTrack->ptr = ptr+curr_offset;
			ptr += curr_offset;
    }
		else {
			/* NO MEMORY LEFT */
			return NULL;
    }
  }

	return ptr;
}

// TODO!
static int32_t _midiGetLength(int32_t ppqn, int32_t iNoteLen, BOOL bOverride) {
  int32_t length = ppqn;
	
	if (bOverride) {
		length = iNoteLen;
  }
	else {
    switch(iNoteLen) {
			case	MIDI_NOTE_DOTTED_MINIM:
						length *= 3;
						break;

			case	MIDI_NOTE_DOTTED_CROCHET:
						length *= 3;
						length /= 2;
						break;

			case	MIDI_NOTE_DOTTED_QUAVER:
						length *= 3;
						length /= 4;
						break;

			case	MIDI_NOTE_DOTTED_SEMIQUAVER:
						length *= 3;
						length /= 8;
						break;

			case	MIDI_NOTE_DOTTED_SEMIDEMIQUAVER:
						length *= 3;
						length /= 16;
						break;

			case	MIDI_NOTE_BREVE:
						length *= 4;
						break;

			case	MIDI_NOTE_MINIM:
						length *= 2;
						break;

			case	MIDI_NOTE_QUAVER:
						length /= 2;
						break;

			case	MIDI_NOTE_SEMIQUAVER:
						length /= 4;
						break;

			case	MIDI_NOTE_SEMIDEMIQUAVER:
						length /= 8;
						break;
			
			case	MIDI_NOTE_TRIPLE_CROCHET:
						length *= 2;
						length /= 3;
						break;			
    }
  }
	
	return length;
}

/*
** midiFile* Functions
*/

// TODO!
MIDI_FILE  *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists) {
  _MIDI_FILE *pMF = (_MIDI_FILE *)malloc(sizeof(_MIDI_FILE)); // TODO: remove
  int32_t i;

	if (!pMF)							return NULL;
	
	if (!bOverwriteIfExists) {
    if (fopen_s(&pMF->pFile, pFilename, "r") == 0) {
			fclose(pMF->pFile);
			free(pMF);
			return NULL;
    }
  }
	
  if (fopen_s(&pMF->pFile, pFilename, "wb+") == 0)
		{/*empty*/}
	else {
		free((void *)pMF);
		return NULL;
  }
	
	pMF->bOpenForWriting = TRUE;
	pMF->Header.PPQN = MIDI_PPQN_DEFAULT;
	pMF->Header.iVersion = MIDI_VERSION_DEFAULT;
	
	for(i=0;i<MAX_MIDI_TRACKS;++i) {
		pMF->Track[i].pos = 0;
		pMF->Track[i].ptr = NULL;
		pMF->Track[i].pBase = NULL;
		pMF->Track[i].pEnd = NULL;
		pMF->Track[i].iBlockSize = 0;
		pMF->Track[i].dt = 0;
		pMF->Track[i].iDefaultChannel = (uint8_t)(i & 0xf);
		
		memset(pMF->Track[i].LastNote, '\0', sizeof(pMF->Track[i].LastNote));
  }
	
	return (MIDI_FILE *)pMF;
}

// TODO!
int32_t		midiFileSetTracksDefaultChannel(MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iChannel, BOOL embedded) {
  int32_t prev;

	_VAR_CAST;
	if (!embedded && !IsFilePtrValid(pMF))	return 0;
	if (!IsTrackValid(iTrack))				      return 0;
	if (!IsChannelValid(iChannel))			    return 0;

	/* For programmer each, iChannel is between 1 & 16 - but MIDI uses
	** 0-15. Thus, the fudge factor of 1 :)
	*/
	prev = pMF->Track[iTrack].iDefaultChannel + 1;
	pMF->Track[iTrack].iDefaultChannel = (uint8_t)(iChannel-1);
	return prev;
}

// TODO!
int32_t		midiFileGetTracksDefaultChannel(const MIDI_FILE *_pMF, MIDI_FILE *_pMFembedded, int32_t iTrack, BOOL embedded) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return 0;
	if (!IsTrackValid(iTrack))			return 0;

	return pMF->Track[iTrack].iDefaultChannel+1;
}

// TODO!
int32_t		midiFileSetPPQN(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t PPQN) {
  int32_t prev;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_PPQN_DEFAULT;
	prev = pMF->Header.PPQN;
	pMF->Header.PPQN = (uint16_t)PPQN;
	return prev;
}

// TODO!
int32_t		midiFileGetPPQN(const MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_PPQN_DEFAULT;
	return (int32_t)pMF->Header.PPQN;
}

// TODO!
int32_t		midiFileSetVersion(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iVersion) {
  int32_t prev;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_VERSION_DEFAULT;
	if (iVersion<0 || iVersion>2)			return MIDI_VERSION_DEFAULT;
	prev = pMF->Header.iVersion;
	pMF->Header.iVersion = (uint16_t)iVersion;
	return prev;
}

// TODO!
int32_t			midiFileGetVersion(const MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return MIDI_VERSION_DEFAULT;
	return pMF->Header.iVersion;
}

// looks ok!
MIDI_FILE  *midiFileOpen(const char *pFilename, BOOL embedded) {
  FILE *fp = NULL; // obsolete
  FILE *fpNew = NULL;
  _MIDI_FILE *pMF = NULL;  // TODO: remove
  uint8_t *ptr; // TODO: remove
  uint8_t *ptrBase; // TODO: remove (custom)
  uint32_t ptrNew;
  BOOL bValidFile=FALSE;
  long size; // obsolete
  long sizeNew;

  fopen_s(&fp, pFilename, "rb"); // obsolete
  fopen_s(&fpNew, pFilename, "rb");

	if (fp) {
		if ((pMF = (_MIDI_FILE *)malloc(sizeof(_MIDI_FILE)))) {
			fseek(fp, 0L, SEEK_END); // obsolete
			fseek(fpNew, 0L, SEEK_END);
			size = ftell(fp); // obsolete
			sizeNew = ftell(fpNew);
			if ((pMF->ptr = (uint8_t *)malloc(size))) {
				fseek(fp, 0L, SEEK_SET); // obsolete
				fseek(fpNew, 0L, SEEK_SET);
				fread(pMF->ptr, sizeof(uint8_t), size, fp); // obsolete (read whole file into RAM)
				/* Is this a valid MIDI file ? */
				ptrBase = ptr = pMF->ptr;
        ptrNew = 0;
				if (*(ptr+0) == 'M' && *(ptr+1) == 'T' && *(ptr+2) == 'h' && *(ptr+3) == 'd') {
					uint32_t dwData; // obsolete
          uint32_t dwDataNew;
					uint16_t wData; // obsolete
          uint16_t wDataNew;
					int32_t i;

					dwData = *((uint32_t *)(ptr+4)); // obsolete
          readDwordFromFile(fpNew, &dwDataNew, 4);
					pMF->Header.iHeaderSize = SWAP_DWORD(dwData); // obsolete
          _midiFile.Header.iHeaderSize = SWAP_DWORD(dwDataNew);
					
					wData = *((uint16_t *)(ptr+8)); // obsolete
          readWordFromFile(fpNew, &wDataNew, 8);
					pMF->Header.iVersion = (uint16_t)SWAP_WORD(wData); // obsolete
          _midiFile.Header.iVersion = (uint16_t)SWAP_WORD(wDataNew);
					
					wData = *((uint16_t *)(ptr+10)); // obsolete
          readWordFromFile(fpNew, &wDataNew, 10);
					pMF->Header.iNumTracks = (uint16_t)SWAP_WORD(wData); // obsolete
          _midiFile.Header.iNumTracks = (uint16_t)SWAP_WORD(wDataNew);

					wData = *((uint16_t *)(ptr+12)); // obsolete
          readWordFromFile(fpNew, &wDataNew, 12);
					pMF->Header.PPQN = (uint16_t)SWAP_WORD(wData); // obsolete
          _midiFile.Header.PPQN = (uint16_t)SWAP_WORD(wDataNew);
					
					ptr += pMF->Header.iHeaderSize + 8; // obsolete
          ptrNew += _midiFile.Header.iHeaderSize + 8;
					/*
					**	 Get all tracks
					*/

          // Init
					for(i = 0; i < MAX_MIDI_TRACKS; ++i) {
						pMF->Track[i].pos = 0;
						pMF->Track[i].last_status = 0;
					}
					
					for(i = 0; i < pMF->Header.iNumTracks; ++i) {
						pMF->Track[i].pBase = ptr; // obsolete
            _midiFile.Track[i].pBaseNew = ptrNew;

            dwData = *((uint32_t *)(ptr + 4)); // obsolete
            readDwordFromFile(fpNew, &dwDataNew, ptrNew + 4);
            pMF->Track[i].sz = SWAP_DWORD(dwData); // obsolete
            _midiFile.Track[i].sz = SWAP_DWORD(dwDataNew);

						pMF->Track[i].ptr = ptr + 8; // obsolete
            _midiFile.Track[i].ptrNew = ptrNew + 8;
						pMF->Track[i].pEnd = ptr + pMF->Track[i].sz + 8; // obsolete

            _midiFile.Track[i].pEndNew = ptrNew + _midiFile.Track[i].sz + 8;
						ptr += pMF->Track[i].sz + 8; // obsolete
            ptrNew += _midiFile.Track[i].sz + 8;
          }

					pMF->bOpenForWriting = FALSE; // obsolete
          _midiFile.bOpenForWriting = FALSE;
					pMF->pFile = NULL; // obsolete
					bValidFile = TRUE;
				}
			}
		}

    fclose(fp); // obsolete
	}
	
	if (!bValidFile) {
		if (pMF)		free((void *)pMF); // obsolete
		return NULL;
  }
	
  _midiFile.pFile = fpNew;

  if (embedded)
    return (MIDI_FILE *)&_midiFile; // useless here, but will be used in future
  else
	  return (MIDI_FILE *)pMF; // obsolete
  
}

typedef struct {
		int32_t	iIdx;
		int32_t	iEndPos;
		} MIDI_END_POINT;


// TODO!
static int32_t qs_cmp_pEndPoints(const void *e1, const void *e2) {
  MIDI_END_POINT *p1 = (MIDI_END_POINT *)e1;
  MIDI_END_POINT *p2 = (MIDI_END_POINT *)e2;

	return p1->iEndPos-p2->iEndPos;
}

// TODO!
BOOL	midiFileFlushTrack(MIDI_FILE *_pMF, MIDI_FILE *_pMFembedded, int32_t iTrack, BOOL bFlushToEnd, uint32_t dwEndTimePos) {
  int32_t sz;
  uint8_t *ptr;
  MIDI_END_POINT *pEndPoints;
  int32_t num, i, mx_pts;
  BOOL bNoChanges = TRUE;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!_midiValidateTrack(pMF, pMFembedded, iTrack))	return FALSE;
	sz = sizeof(pMF->Track[0].LastNote)/sizeof(pMF->Track[0].LastNote[0]);

	/*
	** Flush all 
	*/
	pEndPoints = (MIDI_END_POINT *)malloc(sz * sizeof(MIDI_END_POINT));
	mx_pts = 0;
	for(i=0;i<sz;++i)
		if (pMF->Track[iTrack].LastNote[i].valid) {
			pEndPoints[mx_pts].iIdx = i;
			pEndPoints[mx_pts].iEndPos = pMF->Track[iTrack].LastNote[i].end_pos;
			mx_pts++;
    }
	
	if (bFlushToEnd) {
		if (mx_pts)
			dwEndTimePos = pEndPoints[mx_pts-1].iEndPos;
		else
			dwEndTimePos = pMF->Track[iTrack].pos;
  }
	
	if (mx_pts) {
		/* Sort, smallest first, and add the note off msgs */
		qsort(pEndPoints, mx_pts, sizeof(MIDI_END_POINT), qs_cmp_pEndPoints);
		
		i = 0;
		while ((dwEndTimePos >= (uint32_t)pEndPoints[i].iEndPos || bFlushToEnd) && i<mx_pts) {
			ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
			if (!ptr)
				return FALSE;
			
			num = pEndPoints[i].iIdx;		/* get 'LastNote' index */
			
			ptr = _midiWriteVarLen(ptr, pMF->Track[iTrack].LastNote[num].end_pos - pMF->Track[iTrack].pos);
			/* msgNoteOn  msgNoteOff */
			*ptr++ = (uint8_t)(msgNoteOff | pMF->Track[iTrack].LastNote[num].chn);
			*ptr++ = pMF->Track[iTrack].LastNote[num].note;
			*ptr++ = 0;
			
			pMF->Track[iTrack].LastNote[num].valid = FALSE;
			pMF->Track[iTrack].pos = pMF->Track[iTrack].LastNote[num].end_pos;
			
			pMF->Track[iTrack].ptr = ptr;
			
			++i;
			bNoChanges = FALSE;
		}
	}
	
	free((void *)pEndPoints);
	/*
	** Re-calc current position
	*/
	pMF->Track[iTrack].dt = dwEndTimePos - pMF->Track[iTrack].pos;
	
	return TRUE;
}

// TODO!
BOOL	midiFileSyncTracks(MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int32_t iTrack1, int32_t iTrack2) {
  int32_t p1, p2;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))			return FALSE;
	if (!IsTrackValid(iTrack1))			return FALSE;
	if (!IsTrackValid(iTrack2))			return FALSE;

	p1 = pMF->Track[iTrack1].pos + pMF->Track[iTrack1].dt;
	p2 = pMF->Track[iTrack2].pos + pMF->Track[iTrack2].dt;
	
  if (p1 < p2)		midiTrackIncTime(pMF, pMFembedded, iTrack1, p2 - p1, TRUE);
  else if (p2 < p1)	midiTrackIncTime(pMF, pMFembedded, iTrack2, p1 - p2, TRUE);
	
	return TRUE;
}

// TODO?
BOOL	midiFileClose(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))			return FALSE;
	
	if (pMF->bOpenForWriting)	{
		uint16_t iNumTracks = 0;
		uint16_t wTest = 256;
		BOOL bSwap = FALSE;
		int32_t i;

		/* Intel processor style-endians need byte swap :( */
		if (*((uint8_t *)&wTest) == 0)
			bSwap = TRUE;

		/* Flush our buffers  */
		for(i=0;i<MAX_MIDI_TRACKS;++i) {
			if (pMF->Track[i].ptr) {
        midiSongAddEndSequence(pMF, pMFembedded, i);
				midiFileFlushTrack(pMF, pMFembedded, i, TRUE, 0);
				iNumTracks++;
      }
    }
		/* 
		** Header 
		*/
		{
		  const uint8_t mthd[4] = {'M', 'T', 'h', 'd'};
		  uint32_t dwData;
		  uint16_t wData;
		  uint16_t version, PPQN;

			fwrite(mthd, sizeof(uint8_t), 4, pMF->pFile);
			dwData = 6;
			if (bSwap)	dwData = SWAP_DWORD(dwData);
			fwrite(&dwData, sizeof(uint32_t), 1, pMF->pFile);

			wData = (uint16_t)(iNumTracks==1?pMF->Header.iVersion:1);
			if (bSwap)	version = SWAP_WORD(wData); else version = (uint16_t)wData;
			if (bSwap)	iNumTracks = SWAP_WORD(iNumTracks);
			wData = pMF->Header.PPQN;
			if (bSwap)	PPQN = SWAP_WORD(wData); else PPQN = wData;
			fwrite(&version, sizeof(uint16_t), 1, pMF->pFile);
			fwrite(&iNumTracks, sizeof(uint16_t), 1, pMF->pFile);
			fwrite(&PPQN, sizeof(uint16_t), 1, pMF->pFile);
		}
		/*
		** Track data
		*/
		for(i=0;i<MAX_MIDI_TRACKS;++i)
			if (pMF->Track[i].ptr) {
				const uint8_t mtrk[4] = {'M', 'T', 'r', 'k'};
				uint32_t sz, dwData;

				/* Write track header */
				fwrite(&mtrk, sizeof(uint8_t), 4, pMF->pFile);

				/* Write data size */
				sz = dwData = (int32_t)(pMF->Track[i].ptr - pMF->Track[i].pBase);
				if (bSwap)	sz = SWAP_DWORD(sz);
				fwrite(&sz, sizeof(uint32_t), 1, pMF->pFile);

				/* Write data */
				fwrite(pMF->Track[i].pBase, sizeof(uint8_t), dwData, pMF->pFile);
				
				/* Free memory */
				free((void *)pMF->Track[i].pBase);
      }
  }

	if (pMF->pFile)
		return fclose(pMF->pFile)?FALSE:TRUE;
	free((void *)pMF);
	return TRUE;
}


/*
** midiSong* Functions
*/

// TODO!
BOOL	midiSongAddSMPTEOffset(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iHours, int32_t iMins, int32_t iSecs, int32_t iFrames, int32_t iFFrames) {
  static uint8_t tmp[] = {msgMetaEvent, metaSMPTEOffset, 0x05, 0,0,0,0,0};

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;

	if (iMins<0 || iMins>59)		iMins=0;
	if (iSecs<0 || iSecs>59)		iSecs=0;
	if (iFrames<0 || iFrames>24)	iFrames=0;

	tmp[3] = (uint8_t)iHours;
	tmp[4] = (uint8_t)iMins;
	tmp[5] = (uint8_t)iSecs;
	tmp[6] = (uint8_t)iFrames;
	tmp[7] = (uint8_t)iFFrames;
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

// TODO!
BOOL	midiSongAddSimpleTimeSig(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom) {
  return midiSongAddTimeSig(_pMF, _pMFembedded, iTrack, iNom, iDenom, 24, 8);
}

// TODO!
BOOL	midiSongAddTimeSig(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNom, int32_t iDenom, int32_t iClockInMetroTick, int32_t iNotated32nds) {
  static uint8_t tmp[] = {msgMetaEvent, metaTimeSig, 0x04, 0,0,0,0};

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;

	tmp[3] = (uint8_t)iNom;
	tmp[4] = (uint8_t)(MIDI_NOTE_MINIM/iDenom);
	tmp[5] = (uint8_t)iClockInMetroTick;
	tmp[6] = (uint8_t)iNotated32nds;
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

// TODO!
BOOL	midiSongAddKeySig(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_KEYSIG iKey) {
  static uint8_t tmp[] = {msgMetaEvent, metaKeySig, 0x02, 0, 0};

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;

	tmp[3] = (uint8_t)((iKey&keyMaskKey)*((iKey&keyMaskNeg)?-1:1));
	tmp[4] = (uint8_t)((iKey&keyMaskMin)?1:0);
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

// TODO!
BOOL	midiSongAddTempo(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iTempo) {
  static uint8_t tmp[] = {msgMetaEvent, metaSetTempo, 0x03, 0,0,0};
  int32_t us;	/* micro-seconds per qn */

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;

  us = MICROSECONDS_PER_MINUTE / iTempo;
	tmp[3] = (uint8_t)((us>>16)&0xff);
	tmp[4] = (uint8_t)((us>>8)&0xff);
	tmp[5] = (uint8_t)((us>>0)&0xff);
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

// TODO!
BOOL	midiSongAddMIDIPort(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iPort) {
static uint8_t tmp[] = {msgMetaEvent, metaMIDIPort, 1, 0};

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;
	tmp[3] = (uint8_t)iPort;
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

// TODO!
BOOL	midiSongAddEndSequence(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack) {
  static uint8_t tmp[] = {msgMetaEvent, metaEndSequence, 0};

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;

  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sizeof(tmp), tmp, FALSE, 0);
}


/*
** midiTrack* Functions
*/

// TODO!
BOOL	midiTrackAddRaw(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t data_sz, const uint8_t *pData, BOOL bMovePtr, int32_t dt) {
  MIDI_FILE_TRACK *pTrk;
  uint8_t *ptr;
  int32_t dtime;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))			return FALSE;
	if (!IsTrackValid(iTrack))			return FALSE;
	
	pTrk = &pMF->Track[iTrack];
	ptr = _midiGetPtr(pMF, iTrack, data_sz + DT_DEF);
	if (!ptr)
		return FALSE;
	
	dtime = pTrk->dt;
	if (bMovePtr)
		dtime += dt;
	
	ptr = _midiWriteVarLen(ptr, dtime);
	memcpy(ptr, pData, data_sz);
	
	pTrk->pos += dtime;
	pTrk->dt = 0;
	pTrk->ptr = ptr + data_sz;
	
	return TRUE;
}

// TODO!
BOOL	midiTrackIncTime(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iDeltaTime, BOOL bOverridePPQN) {
  uint32_t will_end_at;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;
	
	will_end_at = _midiGetLength(pMF->Header.PPQN, iDeltaTime, bOverridePPQN);
	will_end_at += pMF->Track[iTrack].pos + pMF->Track[iTrack].dt;
	
  midiFileFlushTrack(pMF, pMFembedded, iTrack, FALSE, will_end_at);
	
	return TRUE;
}


// TODO!
BOOL	midiTrackAddText(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_TEXT iType, const char *pTxt) {
  uint8_t *ptr;
  int32_t sz;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))			return FALSE;

	sz = strlen(pTxt);
	if ((ptr = _midiGetPtr(pMF, iTrack, sz+DT_DEF))) {
		*ptr++ = 0;		/* delta-time=0 */
		*ptr++ = msgMetaEvent;
		*ptr++ = (uint8_t)iType;
		ptr = _midiWriteVarLen((uint8_t *)ptr, sz);
		strcpy_s((char *)ptr, sz, pTxt);
		pMF->Track[iTrack].ptr = ptr+sz;
		return TRUE;
  }
  else
    return FALSE;
}

// ok!
BOOL	midiTrackSetKeyPressure(MIDI_FILE *pMF, MIDI_FILE* pMFembedded, int32_t iTrack, int32_t iNote, int32_t iAftertouch) {
  return midiTrackAddMsg(pMF, pMFembedded, iTrack, msgNoteKeyPressure, iNote, iAftertouch);
}

// ok!
BOOL	midiTrackAddControlChange(MIDI_FILE *pMF, MIDI_FILE* pMFembedded, int32_t iTrack, tMIDI_CC iCCType, int32_t iParam) {
  return midiTrackAddMsg(pMF, pMFembedded, iTrack, msgControlChange, iCCType, iParam);
}

// ok!
BOOL	midiTrackAddProgramChange(MIDI_FILE *pMF, MIDI_FILE* pMFembedded, int32_t iTrack, int32_t iInstrPatch) {
  return midiTrackAddMsg(pMF, pMFembedded, iTrack, msgSetProgram, iInstrPatch, 0);
}

// ok!
BOOL	midiTrackChangeKeyPressure(MIDI_FILE *pMF, MIDI_FILE* pMFembedded, int32_t iTrack, int32_t iDeltaPressure) {
  return midiTrackAddMsg(pMF, pMFembedded, iTrack, msgChangePressure, iDeltaPressure & 0x7f, 0);
}

// ok!
BOOL	midiTrackSetPitchWheel(MIDI_FILE *pMF, MIDI_FILE* pMFembedded, int32_t iTrack, int32_t iWheelPos) {
  uint16_t wheel = (uint16_t)iWheelPos;

	/* bitshift 7 instead of eight because we're dealing with 7 bit numbers */
	wheel += MIDI_WHEEL_CENTRE;
  return midiTrackAddMsg(pMF, pMFembedded, iTrack, msgSetPitchWheel, wheel & 0x7f, (wheel >> 7) & 0x7f);
}

// TODO!
BOOL	midiTrackAddMsg(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, tMIDI_MSG iMsg, int32_t iParam1, int32_t iParam2) {
  uint8_t *ptr;
  uint8_t data[3];
  int32_t sz;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))			return FALSE;
	if (!IsMessageValid(iMsg))			return FALSE;

	ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
	if (!ptr)
		return FALSE;
	
	data[0] = (uint8_t)(iMsg | pMF->Track[iTrack].iDefaultChannel);
	data[1] = (uint8_t)(iParam1 & 0x7f); 
	data[2] = (uint8_t)(iParam2 & 0x7f); 
	/*
	** Is this msg a single, or double uint8_t, prm?
	*/
 	switch(iMsg) {
		case	msgSetProgram:			/* only one byte required for these msgs */
		case	msgChangePressure:
      sz = 2;
      break;

		default:						/* double byte messages */
      sz = 3;
      break;
  }
	
  return midiTrackAddRaw(pMF, pMFembedded, iTrack, sz, data, FALSE, 0);
}

// TODO!
BOOL	midiTrackAddNote(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iNote, int32_t iLength, int32_t iVol, BOOL bAutoInc, BOOL bOverrideLength) {
  MIDI_FILE_TRACK *pTrk;
  uint8_t *ptr;
  BOOL bSuccess = FALSE;
  int32_t i, chn;

	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))				return FALSE;
	if (!IsNoteValid(iNote))				return FALSE;
	
	pTrk = &pMF->Track[iTrack];
	ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
	if (!ptr)
		return FALSE;
	
	chn = pTrk->iDefaultChannel;
	iLength = _midiGetLength(pMF->Header.PPQN, iLength, bOverrideLength);
	
	for(i=0;i<sizeof(pTrk->LastNote)/sizeof(pTrk->LastNote[0]);++i)
		if (pTrk->LastNote[i].valid == FALSE) {
			pTrk->LastNote[i].note = (uint8_t)iNote;
			pTrk->LastNote[i].chn = (uint8_t)chn;
			pTrk->LastNote[i].end_pos = pTrk->pos+pTrk->dt+iLength;
			pTrk->LastNote[i].valid = TRUE;
			bSuccess = TRUE;
			
			ptr = _midiWriteVarLen(ptr, pTrk->dt);		/* delta-time */
			*ptr++ = (uint8_t)(msgNoteOn | chn);
			*ptr++ = (uint8_t)iNote;
			*ptr++ = (uint8_t)iVol;
			break;
    }
	
	if (!bSuccess)
		return FALSE;
	
	pTrk->ptr = ptr;
	pTrk->pos += pTrk->dt;
	pTrk->dt = 0;
	
	if (bAutoInc)
    return midiTrackIncTime(pMF, pMFembedded, iTrack, iLength, bOverrideLength);
	
	return TRUE;
}

// TODO!
BOOL	midiTrackAddRest(MIDI_FILE *_pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, int32_t iLength, BOOL bOverridePPQN) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))			return FALSE;

	iLength = _midiGetLength(pMF->Header.PPQN, iLength, bOverridePPQN);
  return midiTrackIncTime(pMF, pMFembedded, iTrack, iLength, bOverridePPQN);
}

// TODO!
int32_t		midiTrackGetEndPos(MIDI_FILE *_pMF, MIDI_FILE *_pMFembedded, int32_t iTrack) {
	_VAR_CAST;
	if (!IsFilePtrValid(pMF))				return FALSE;
	if (!IsTrackValid(iTrack))			return FALSE;

	return pMF->Track[iTrack].pos;
}

/*
** midiRead* Functions
*/

// ok!
static uint8_t* _midiReadVarLen(uint8_t* ptr, _MIDI_FILE* pMFembedded, uint32_t* ptrNew, uint32_t* num, uint32_t* numEmbedded) {
  uint32_t value, valueEmbedded;
  uint8_t c;
  uint8_t* debugPtrStart = ptr;
  uint8_t ptrDiff = 0;

  // Variable-length values use the lower 7 bits of a byte for data and the top bit to signal a following data byte. 
  // If the top bit is set to 1 (0x80), then another value byte follows.
  // A variable - length value may use a maximum of 4 bytes. This means the maximum value that can be represented is 
  // 0x0FFFFFFF (represented as 0xFF, 0xFF, 0xFF, 0x7F).

  // TODO: always preload 4 bytes?
  // standard version
  if ((value = *ptr++) & 0x80) {
	  value &= 0x7f; // Remove the first bit to extract payload
	  do {
		  value = (value << 7) + ((c = *ptr++) & 0x7f);
    } while (c & 0x80);
	}

  // embedded version
  valueEmbedded = 0;
  *ptrNew += readChunkFromFile(pMFembedded->pFile, &valueEmbedded, *ptrNew, 1);
  if (valueEmbedded & 0x80) {
    valueEmbedded &= 0x7f; // Remove the first bit to extract payload
    do {
      *ptrNew += readChunkFromFile(pMFembedded->pFile, &c, *ptrNew, 1);
      valueEmbedded = (valueEmbedded << 7) + (c & 0x7f);
    } while (c & 0x80);
  }

  *num = value;
  *numEmbedded = valueEmbedded;
  return(ptr);
}

// bug in embedded part!
static BOOL _midiReadTrackCopyData(MIDI_MSG* pMsg, _MIDI_FILE* pMFembedded, MIDI_MSG* pMsgEmbedded, uint8_t* ptr, uint32_t ptrEmbedded, uint32_t sz, size_t* szEmbedded, BOOL bCopyPtrData) {
  // standard version
	if (sz > pMsg->data_sz) {
		pMsg->data = (uint8_t *)realloc(pMsg->data, sz + 1); // + 1 for nullterminator on text events
		pMsg->data_sz = sz;
  }
  if (!pMsg->data)
    return FALSE;

  // embedded
  if (*szEmbedded > META_EVENT_MAX_DATA_SIZE) {
    *szEmbedded = META_EVENT_MAX_DATA_SIZE;
    printf("\r\n_midiReadTrackCopyData; Warning: Meta data is greater than maximum size! (%d of %d)\r\n", sz, META_EVENT_MAX_DATA_SIZE);
  }
  // ---

  if (bCopyPtrData && ptr) {
		memcpy(pMsg->data, ptr, sz);

    // embedded part
    readChunkFromFile(pMFembedded->pFile, pMsgEmbedded->dataEmbedded, ptrEmbedded, *szEmbedded);
    pMsgEmbedded->data_sz_embedded = *szEmbedded;
  }

	return TRUE;
}

// TODO!
int32_t midiReadGetNumTracks(const MIDI_FILE *_pMF, MIDI_FILE *_pMFembedded) {
	_VAR_CAST;
	return pMF->Header.iNumTracks;
}

// looks ok!
BOOL midiReadGetNextMessage(const MIDI_FILE* _pMF, MIDI_FILE* _pMFembedded, int32_t iTrack, MIDI_MSG* pMsg, MIDI_MSG* pMsgEmbedded, BOOL embedded) {
  MIDI_FILE_TRACK *pTrack;
  MIDI_FILE_TRACK *pTrackNew;
  uint8_t *bptr, *pMsgDataPtr;
  uint32_t bptrEmbedded, pMsgDataPtrEmbedded;
  int32_t sz;
  size_t szEmbedded;

	_VAR_CAST;
	if (!IsTrackValid(iTrack))			return FALSE;
	
	pTrack = &pMF->Track[iTrack]; // obsolete
  pTrackNew = &pMFembedded->Track[iTrack];
	/* FIXME: Check if there is data on this track first!!!	*/
	if (pTrack->ptr >= pTrack->pEnd) // obsolete
		return FALSE; // obsolete

  if(pTrackNew->ptrNew >= pTrackNew->pEndNew)
    return FALSE;
    	
  // Read Delta Time
	pTrack->ptr = _midiReadVarLen(pTrack->ptr, pMFembedded, &pTrackNew->ptrNew, &pMsg->dt, &pMsgEmbedded->dt);
	pTrack->pos += pMsg->dt; // obsolete
  pTrackNew->pos += pMsg->dt;

	pMsg->dwAbsPos = pTrack->pos; // obsolete
  pMsgEmbedded->dwAbsPos = pTrackNew->pos;

  BOOL bRunningStatus = FALSE;
  // Standard version (TODO: check, if this is correct)
  if (*pTrack->ptr >= 0x80) {	/* If this is sys message */
	//if (*pTrack->ptr & 0x80) {	/* If this is sys message */
		pMsg->iType = (tMIDI_MSG)((*pTrack->ptr) & 0xf0);
		pMsgDataPtr = pTrack->ptr+1;

		/* SysEx & Meta events don't carry channel info, but something
		** important in their lower bits that we must keep */
		if (pMsg->iType == 0xf0)
			pMsg->iType = (tMIDI_MSG)(*pTrack->ptr); 
  }
	else {  /* just data - so use the last msg type */
		pMsg->iType = pMsg->iLastMsgType;
		pMsgDataPtr = pTrack->ptr;
    bRunningStatus = TRUE;
  }
  // ---

  // Embedded version
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
  }
  // ---

	pMsg->iLastMsgType = (tMIDI_MSG)pMsg->iType; // obsolete
  pMsgEmbedded->iLastMsgType = (tMIDI_MSG)pMsgEmbedded->iType;

  if (!bRunningStatus) {
    pMsg->iLastMsgChnl = (uint8_t)((*pTrack->ptr) & 0x0f) + 1; // obsolete
    pMsgEmbedded->iLastMsgChnl = (uint8_t)(eventType & 0x0f) + 1;
  }
  

  tMIDI_MSG type = embedded ? pMsgEmbedded->iType : pMsg->iType;
  switch(type) {  
    // -------------------------
    // -    Channel Events     -
    // -------------------------
    case	msgNoteOff: { // 0x08 'Note Off'
      // standard
      pMsg->MsgData.NoteOff.iChannel = pMsg->iLastMsgChnl;
      pMsg->MsgData.NoteOff.iNote = *(pMsgDataPtr);
      pMsg->iMsgSize = 3;

      // embedded
      uint8_t tmpNote = 0;
      pMsgEmbedded->MsgData.NoteOff.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpNote, pMsgDataPtrEmbedded);
      pMsgEmbedded->MsgData.NoteOff.iNote = tmpNote;
      pMsgEmbedded->iMsgSize = 3;
      break;
    }

		case	msgNoteOn: { // 0x09 'Note On'
      pMsg->MsgData.NoteOn.iChannel = pMsg->iLastMsgChnl;
      pMsg->MsgData.NoteOn.iNote = *(pMsgDataPtr);
      pMsg->MsgData.NoteOn.iVolume = *(pMsgDataPtr+1);
      pMsg->iMsgSize = 3;

      // embedded
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
			pMsg->MsgData.NoteKeyPressure.iChannel = pMsg->iLastMsgChnl;
			pMsg->MsgData.NoteKeyPressure.iNote = *(pMsgDataPtr);
			pMsg->MsgData.NoteKeyPressure.iPressure = *(pMsgDataPtr+1);
			pMsg->iMsgSize = 3;

      // embedded
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
			pMsg->MsgData.NoteParameter.iChannel = pMsg->iLastMsgChnl;
			pMsg->MsgData.NoteParameter.iControl = (tMIDI_CC)*(pMsgDataPtr);
			pMsg->MsgData.NoteParameter.iParam = *(pMsgDataPtr+1);
			pMsg->iMsgSize = 3;

      // embedded
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
			pMsg->MsgData.ChangeProgram.iChannel = pMsg->iLastMsgChnl;
			pMsg->MsgData.ChangeProgram.iProgram = *(pMsgDataPtr);
			pMsg->iMsgSize = 2;

      // embedded
      uint8_t tmpProgram = 0;
      pMsgEmbedded->MsgData.ChangeProgram.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpProgram, pMsgDataPtrEmbedded);
      pMsgEmbedded->MsgData.ChangeProgram.iProgram = tmpProgram;
      pMsgEmbedded->iMsgSize = 2;
			break;
    }

		case	msgChangePressure: { // 0x0D 'Channel Aftertouch'
			pMsg->MsgData.ChangePressure.iChannel = pMsg->iLastMsgChnl;
			pMsg->MsgData.ChangePressure.iPressure = *(pMsgDataPtr);
			pMsg->iMsgSize = 2;

      // embedded
      uint8_t tmpPressure = 0;
      pMsgEmbedded->MsgData.ChangePressure.iChannel = pMsgEmbedded->iLastMsgChnl;
      readByteFromFile(pMFembedded->pFile, &tmpPressure, pMsgDataPtrEmbedded);
      pMsgEmbedded->iMsgSize = 2;
			break;
    }

		case	msgSetPitchWheel: { // 0x0F 'Pitch Bend'
			pMsg->MsgData.PitchWheel.iChannel = pMsg->iLastMsgChnl;
			pMsg->MsgData.PitchWheel.iPitch = *(pMsgDataPtr) | (*(pMsgDataPtr + 1) << 7);
			pMsg->MsgData.PitchWheel.iPitch -= MIDI_WHEEL_CENTRE;
			pMsg->iMsgSize = 3;

      // embedded (needs to be checked!)
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
		  bptr = pTrack->ptr; // obsolete
      bptrEmbedded = pTrackNew->ptrNew;
		  pMsg->MsgData.MetaEvent.iType = (tMIDI_META)*(pTrack->ptr + 1); // obsolete
      uint8_t tmpType = 0;
      readByteFromFile(pMFembedded->pFile, &tmpType, pTrackNew->ptrNew + 1);
      pMsgEmbedded->MsgData.MetaEvent.iType = tmpType;

      // Get Meta Event Length (TODO: find a 'live' method instead of using a constant sized buffer?)
      pTrackNew->ptrNew += 2;
		  pTrack->ptr = _midiReadVarLen(pTrack->ptr + 2, pMFembedded, &pTrackNew->ptrNew, &pMsg->iMsgSize, &pMsgEmbedded->iMsgSize);
		  sz = (pTrack->ptr - bptr) + pMsg->iMsgSize;
      szEmbedded = pTrackNew->ptrNew - bptrEmbedded + pMsgEmbedded->iMsgSize;

		  if (_midiReadTrackCopyData(pMsg, pMFembedded, pMsgEmbedded, pTrack->ptr, pTrackNew->ptrNew, sz, &szEmbedded, FALSE) == FALSE)
			  return FALSE;

		  /* Now copy the data...*/
		  memcpy(pMsg->data, bptr, sz); // obsolete
      readChunkFromFile(pMFembedded->pFile, pMsgEmbedded->dataEmbedded, bptrEmbedded, szEmbedded);

		  /* TODO: Place the META data it in a neat structure also for embedded! */
		  switch(pMsg->MsgData.MetaEvent.iType) {
        case	metaSequenceNumber: {
					    pMsg->MsgData.MetaEvent.Data.iSequenceNumber = *(pTrack->ptr + 0);
              uint8_t tmpSequenceNumber;
              readByteFromFile(pMFembedded->pFile, &tmpSequenceNumber, pTrackNew->ptrNew + 0);
              pMsg->MsgData.MetaEvent.Data.iSequenceNumber = tmpSequenceNumber;
					    break;
            }
			 
			  case	metaTextEvent:
			  case	metaCopyright:
			  case	metaTrackName:
			  case	metaInstrument:
			  case	metaLyric:
			  case	metaMarker:
			  case	metaCuePoint:
            pMsg->MsgData.MetaEvent.Data.Text.pData = pMsg->data + 3;
            pMsg->MsgData.MetaEvent.Data.Text.strLen = sz - 3;
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.strLen = szEmbedded - 3;
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.pData = pMsgEmbedded->dataEmbedded + 3;

            // Add Null terminator
            pMsg->MsgData.MetaEvent.Data.Text.pData[pMsg->MsgData.MetaEvent.Data.Text.strLen] = '\0';
            pMsgEmbedded->MsgData.MetaEvent.Data.Text.pData[pMsgEmbedded->MsgData.MetaEvent.Data.Text.strLen] = '\0';
					  break;

        case	metaMIDIPort:
          pMsg->MsgData.MetaEvent.Data.iMIDIPort = *(pTrack->ptr + 0);

          // embedded
          uint8_t tmpMIDIPort;
          readByteFromFile(pMFembedded->pFile, &tmpMIDIPort, pTrackNew->ptrNew + 0);
          pMsgEmbedded->MsgData.MetaEvent.Data.iMIDIPort = tmpMIDIPort;
          break;
			  case	metaEndSequence:
					  /* NO DATA */
					  break;
			  case	metaSetTempo: {
					    uint32_t us = ((*(pTrack->ptr+0))<<16)|((*(pTrack->ptr+1))<<8)|(*(pTrack->ptr+2));
              pMsg->MsgData.MetaEvent.Data.Tempo.iBPM = MICROSECONDS_PER_MINUTE / us; // obsolete

              // embedded
              pMsgEmbedded->MsgData.MetaEvent.Data.Tempo.iBPM = MICROSECONDS_PER_MINUTE / us;
					  }
					  break;
			  case	metaSMPTEOffset:
					  pMsg->MsgData.MetaEvent.Data.SMPTE.iHours = *(pTrack->ptr+0);
					  pMsg->MsgData.MetaEvent.Data.SMPTE.iMins= *(pTrack->ptr+1);
					  pMsg->MsgData.MetaEvent.Data.SMPTE.iSecs = *(pTrack->ptr+2);
					  pMsg->MsgData.MetaEvent.Data.SMPTE.iFrames = *(pTrack->ptr+3);
					  pMsg->MsgData.MetaEvent.Data.SMPTE.iFF = *(pTrack->ptr+4);
					  break;
			  case	metaTimeSig:
					  pMsg->MsgData.MetaEvent.Data.TimeSig.iNom = *(pTrack->ptr+0);
					  pMsg->MsgData.MetaEvent.Data.TimeSig.iDenom = *(pTrack->ptr+1) * MIDI_NOTE_MINIM;
					  /* TODO: Variations without 24 & 8 */
					  break;
			  case	metaKeySig:
					  if (*pTrack->ptr & 0x80) {
						  /* Do some trendy sign extending in reverse :) */
						  pMsg->MsgData.MetaEvent.Data.KeySig.iKey = ((256-*pTrack->ptr)&keyMaskKey);
						  pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskNeg;
						  }
					  else {
						  pMsg->MsgData.MetaEvent.Data.KeySig.iKey = (tMIDI_KEYSIG)(*pTrack->ptr&keyMaskKey);
            }
					  if (*(pTrack->ptr+1)) 
						  pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskMin;
					  break;
			  case	metaSequencerSpecific:
					  pMsg->MsgData.MetaEvent.Data.Sequencer.iSize = pMsg->iMsgSize;
					  pMsg->MsgData.MetaEvent.Data.Sequencer.pData = pTrack->ptr;
					  break;
      }

		  pTrack->ptr += pMsg->iMsgSize; // obsolete
      pTrackNew->ptrNew += pMsgEmbedded->iMsgSize;
		  pMsg->iMsgSize = sz; // obsolete
      pMsgEmbedded->iMsgSize = szEmbedded;

		  break;

    // ----------------------------------
    // -    System Exclusive Events     -
    // ----------------------------------
		case	msgSysEx1:
		case	msgSysEx2:
		  bptr = pTrack->ptr; // obsolete
      bptrEmbedded = pTrackNew->ptrNew;
      pTrackNew->ptrNew += 1; 
		  pTrack->ptr = _midiReadVarLen(pTrack->ptr + 1, pMFembedded, &pTrackNew->ptrNew, &pMsg->iMsgSize, &pMsgEmbedded->iMsgSize);
		  sz = (pTrack->ptr - bptr) + pMsg->iMsgSize; // obsolete
      szEmbedded = (pTrackNew->ptrNew - bptrEmbedded) + pMsgEmbedded->iMsgSize;

		  if (_midiReadTrackCopyData(pMsg, pMFembedded, pMsgEmbedded, pTrack->ptr, pTrackNew->ptrNew, sz, &szEmbedded, FALSE) == FALSE)
			  return FALSE;

		  /* Now copy the data... */
		  memcpy(pMsg->data, bptr, sz);
		  pTrack->ptr += pMsg->iMsgSize;
		  pMsg->iMsgSize = sz;
		  pMsg->MsgData.SysEx.pData = pMsg->data;
		  pMsg->MsgData.SysEx.iSize = sz;

      
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

  // standard version
	pMsg->bImpliedMsg = FALSE; // obsolete
	if ((pMsg->iType & 0xf0) != 0xf0) {
		if (*pTrack->ptr & 0x80) {
    }
		else {
			pMsg->bImpliedMsg = TRUE;
			pMsg->iImpliedMsg = pMsg->iLastMsgType;
			pMsg->iMsgSize--;      
    }

    // The next two lines are done in embedded part!
		//_midiReadTrackCopyData(pMsg, pMFembedded, pMsgEmbedded, pTrack->ptr, pTrackNew->ptrNew, pMsg->iMsgSize, &pMsgEmbedded->iMsgSize, TRUE);
		//pTrack->ptr += pMsg->iMsgSize; // obsolete
  }

  // embedded (needs to be checked!)
  pMsgEmbedded->bImpliedMsg = FALSE;
  if ((pMsgEmbedded->iType & 0xf0) != 0xf0) {
    uint8_t tmpVal = 0;
    readByteFromFile(pMFembedded->pFile, &tmpVal, pTrackNew->ptrNew);
    if (tmpVal & 0x80) {
    }
    else {
      pMsgEmbedded->bImpliedMsg = TRUE;
      pMsgEmbedded->iImpliedMsg = pMsg->iLastMsgType;
      pMsgEmbedded->iMsgSize--;
    }

    _midiReadTrackCopyData(pMsg, pMFembedded, pMsgEmbedded, pTrack->ptr, pTrackNew->ptrNew, pMsg->iMsgSize, &pMsgEmbedded->iMsgSize, TRUE);
    pTrackNew->ptrNew += pMsgEmbedded->iMsgSize;
    pTrack->ptr += pMsg->iMsgSize; // obsolete
  }

  return TRUE;
}

// ok!
void midiReadInitMessage(MIDI_MSG *pMsg) {
	pMsg->data = NULL;
	pMsg->data_sz = 0;
	pMsg->bImpliedMsg = FALSE;
}

// not needed.
void midiReadFreeMessage(MIDI_MSG *pMsg) {
	if (pMsg->data)	free((void *)pMsg->data);
	pMsg->data = NULL;
}
