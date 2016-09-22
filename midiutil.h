/*
 * midiutil.h - Header for auxiliary MIDI functionality. 
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *      Copyright 2010, Steven Goodwin.
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

#include <stdint.h>
#include "midiinfo.h"


#ifndef _MIDIUTIL_H
#define _MIDIUTIL_H

// Source: http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm
//
// There is a discrepancy that occurs between various models of MIDI devices and software programs, and that 
// concerns the octave numbers for note names. If your MIDI software / device considers octave 0 as being the 
// lowest octave of the MIDI note range, then middle C's note name is C5.
// The lowest note name is then C0 (note number 0), and the highest possible note name is G10 (note number 127).
// Some software / devices instead consider the third octave of the MIDI note range (2 octaves below middle C) 
// as octave 0. In that case, the first 2 octaves are referred to as -2 and -1. So, middle C's note name is C3, 
// the lowest note name is C-2, and the highest note name is G8. Use the C0_BASE parameter to define the lowest 
// tone. Valid values are: -2, -1, and 0.

#define C0_BASE -1

#if (C0_BASE != -2 && C0_BASE != -1 && C0_BASE != 0)
  #error Invalid value for C0_BASE. Valid values are: -2, -1 and 0.
#endif

// chord masks
#define CHORD_ROOT_MASK     0x000000ff
#define CHORD_TYPE_MASK     0x0000ff00
#define CHORD_BASS_MASK     0x00ff0000
#define CHORD_ADDITION_MASK 0xff000000

#define CHORD_TYPE_MAJOR    0x00000100
#define CHORD_TYPE_MINOR    0x00000200
#define CHORD_TYPE_AUG      0x00000300
#define CHORD_TYPE_DIM      0x00000400

#define CHORD_ADD_7TH       0x01000000
#define CHORD_ADD_9TH       0x02000000
#define CHORD_ADD_MAJ7TH    0x04000000

/*
** Name resolving prototypes
*/
const char* muGetInstrumentName(int8_t iInstr);
const char* muGetDrumName(int8_t iInstr);
const char* muGetMIDIMsgName(tMIDI_MSG iMsg);
const char* muGetControlName(tMIDI_CC iCC);
bool        muGetKeySigName(char *pName, tMIDI_KEYSIG iKey);
const char* muGetTextName(tMIDI_TEXT iEvent);
const char* muGetMetaName(tMIDI_META iEvent);

/*
** Conversion prototypes
*/
int8_t      muGetNoteFromName(const char *pName);
const char* muGetNameFromNote(int8_t iNote);
float       muGetFreqFromNote(int8_t iNote);
int8_t      muGetNoteFromFreq(float fFreq);
int32_t     muGuessChord(const int32_t *pNoteStatus, const int32_t channel, const int32_t lowRange, const int32_t highRange);
char*       muGetChordName(char *str, int32_t chord);

#endif  /* _MIDIUTIL_H*/
