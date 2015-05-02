/*
 * midiutil.c - Auxiliary MIDI functionality. Requires Steevs MIDI Library
 *		        (midiinfo.h) for enumerations used in name mapping.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "midifile.h"
#include "midiutil.h"

/*
** Data Tables
*/
static const char* szPatchList[128] = {
	/*Pianos*/
	"Acoustic Grand Piano",
	"Bright Acoustic Piano",
	"Electric Grand Piano",
	"Honky-tonk Piano",
	"Electric Piano 1",
	"Electric Piano 2",
	"Harpsichord",
	"Clavinet",
	/*Chromatic Percussion*/
	"Celesta",
	"Glockenspiel",
	"Music Box",
	"Vibraphone",
	"Marimba",
	"Xylophone",
	"Tubular Bells",
	"Dulcimer",
	/*Organs*/
	"Draw Organ",
	"Percussive Organ",
	"Rock Organ",
	"Church Organ",
	"Reed Organ",
	"Accordian",
	"Harmonica",
	"Tango Accordian",
	/*Guitars*/
	"Acoustic Guitar (nylon)",
	"Acoustic Guitar (steel)",
	"Electric Guitar (jazz)",
	"Electric Guitar (clean)",
	"Electric Guitar (muted)",
	"Overdriven Guitar",
	"Distortion Guitar",
	"Guitar harmonics",
	/*Basses*/
	"Acoustic bass",
	"Electric Bass (finger)",
	"Electric Bass (picked)",
	"Fretless Bass",
	"Slap Bass 1",
	"Slap Bass 2",
	"Synth bass 1",
	"Synth bass 2",
	/*Strings*/
	"Violin",
	"Viola",
	"Cello",
	"Contrabass",
	"Tremolo strings",
	"Pizzicato strings",
	"Orchestral harp",
	"Timpani",
	/*Ensembles*/
	"String ensemble 1",
	"String ensemble 2",
	"Synth strings 1",
	"Synth strings 2",
	"Choir Ahhs",
	"Voice oohs",
	"Synth voice",
	"Orchestra hit",
	/*Brass*/
	"Trumpet",
	"Trombone",
	"Tuba",
	"Muted trumpet",
	"French horn",
	"Brass section",
	"Synth brass 1",
	"Synth brass 2",
	/*Reeds*/
	"Soprano sax",
	"Alto sax",
	"Tenor sax",
	"Baritone sax",
	"Oboe",
	"English horn",
	"Bassoon",
	"Clarinet",
	/*Pipes*/
	"Picclo",
	"Flute",
	"Recorder",
	"Pan Flute",
	"Bottle Blow",
	"Shakuhachi",
	"Whistle",
	"Ocarina",
	/*Synth Lead*/
	"Lead 5 (Square)",
	"Lead 5 (Sawtooth)",
	"Lead 5 (Calliope)",
	"Lead 5 (Chiff)",
	"Lead 5 (Charang)",
	"Lead 5 (Voice)",
	"Lead 5 (Fifths)",
	"Lead 5 (Bass+lead)",
	/*Synth Pads*/
	"Pad 1 (New age)",
	"Pad 2 (Warm)",
	"Pad 3 (Polysynth)",
	"Pad 4 (Choir)",
	"Pad 5 (Bowed)",
	"Pad 6 (Metallic)",
	"Pad 7 (Halo)",
	"Pad 8 (Sweep)",
	/*Synth FX*/
	"FX 1 (Rain)",
	"FX 2 (Soundtrack)",
	"FX 3 (Crystal)",
	"FX 4 (Atmosphere)",
	"FX 5 (Brightness)",
	"FX 6 (Goblins)",
	"FX 7 (Echoes)",
	"FX 8 (Sci-fi)",
	/*Ethnic*/
	"Sitar",
	"Banjo",
	"Shamisen",
	"Koto",
	"Kalimba",
	"Bagpipe",
	"Fiddle",
	"Shanai",
	/*Percussive*/
	"Tinkle bell",
	"Agogo",
	"Steel drums",
	"Woodblock",
	"Taiko drum",
	"Melodic tom",
	"Synth drum",
	"Reverse cymbal",
	/*FX*/
	"Guitar fret noise",
	"Breath noise",
	"Seashore",
	"Bird tweet",
	"Telephone ring",
	"Helicopter",
	"Applause",
	"Gunshot",
};

static const char* szGMDrums[128] = {
	"???",	/* C0 */
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	/* C1  */
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	/* C2 */
	"???",	
	"???",	
	"High Q",	
	"Slap",	
	"???",	
	"???",	
	"Sticks",	
	"Square click",	
	"???",	
	"???",	
	"Acoustic Kick Drum",	
	"Electric Kick Drum",		/* C3=36 */
	"Side Stick",
	"Acoustic Snare Drum",
	"Hand Clap",
	"Electric Snare Drum ",	/*(crisp, electronic)", */
	"Low Floor Tom",
	"Closed Hi Hat",
	"High Floor Tom",
	"Opening Hi Hat",	/* pedal hh */
	"Low Tom",
	"Open Hi Hat",
	"Low Mid Tom",
	"High Mid Tom",		/* c4 */
	"Crash Cymbal 1",
	"High Tom",
	"Ride Cymbal 1",
	"Chinese Boom",
	"Ride Bell",
	"Tamborine",
	"Splash cymbal",
	"Cowbell",
	"Crash Cymbal (2)",
	"Vibra Slap",
	"Ride Cymbal (2)",
	"Hi bongo",		/* c5 */
	"Lo bongo",
	"Mute High Conga",
	"Open High Conga",
	"Low Conga",
	"High Timbale",
	"Low Timbale",
	"High Agogo",
	"Low Agogo",
	"Cabasa",
	"Maracas",
	"Short Hi Whistle",
	"Long Low Whistle",	/*c6 */
	"Short Guiro",
	"Long Guiro",
	"Claves",
	"High Woodblock",
	"Low Woodblock",
	"Mute Cuica",
	"Open Cuica",
	"Mute Triangle",
	"Open Triangle",
	"Shaker",
	"Jingle Bell",
	"Tring",	/* C7 */
	"Castinets",	
	"Mute Sudro",	
	"Open Sudro",	
};

static const char* szCCList[] = {
	"Bank Select",
	"Modulation",
	"Breath Control",
	"Undefined 3",
	"Foot",
	"Portamento Time",
	"Date Entry",
	"Volume	",
	"Balance",
	"Undefined 9",
	"Pan",
	"Expression",
	"Effort Control 1",
	"Effort Control 2",
	"Undefined 14",
	"Undefined 15",
	"General Purpose 1",
	"General Purpose 2",
	"General Purpose 3",
	"General Purpose 4",
	/* 20-31 are undefined */
	"Undefined 20",
	"Undefined 21",
	"Undefined 22",
	"Undefined 23",
	"Undefined 24",
	"Undefined 25",
	"Undefined 26",
	"Undefined 27",
	"Undefined 28",
	"Undefined 29",
	"Undefined 30",
	"Undefined 31",
	/* LSB for control changes 0-31		32-63 */
	"lsb-32", "lsb-33", "lsb-34", "lsb-35", "lsb-36", "lsb-37", "lsb-38", 
	"lsb-39", "lsb-40", "lsb-41", "lsb-42", "lsb-43", "lsb-44", "lsb-45", 
	"lsb-46", "lsb-47", "lsb-48", "lsb-49", "lsb-50", "lsb-51", "lsb-52", 
	"lsb-53", "lsb-54", "lsb-55", "lsb-56", "lsb-57", "lsb-58", "lsb-59", 
	"lsb-60", "lsb-61", "lsb-62", "lsb-63",

	"Sustain Pedal",
	"Portamento",
	"Pedal Sustenuto",
	"Pedal Soft",
	"Legato Foot Switch",
	"Hold 2",
	"Sound Variation",
	"Harm Content",
	"Release Time",
	"Attack Time",
	"Brightness",
	"Reverb",
	"Delay",
	"Pitch Transpose",
	"Flange",
	"Special FX",
	"General Purpose 5",
	"General Purpose 6",
	"General Purpose 7",
	"General Purpose 8",
	"Portamento Control",
	/* 85-90 are undefined */
	"Undefined 85",
	"Undefined 86",
	"Undefined 87",
	"Undefined 88",
	"Undefined 89",
	"Undefined 90",
	"FX Depth",
	"Tremelo Depth",
	"Chorus Depth",
	"Celesta Depth",
	"Phaser Depth",
	"Data Inc",
	"Data Dec",
	"Non Reg Param LSB",
	"Non Ref Param MSB",
	"Reg Param LSB",
	"Reg Param MSB",
	/* 102-119 are undefined */
	"Undefined 102",
	"Undefined 103",
	"Undefined 104",
	"Undefined 105",
	"Undefined 106",
	"Undefined 107",
	"Undefined 108",
	"Undefined 109",
	"Undefined 110",
	"Undefined 111",
	"Undefined 112",
	"Undefined 113",
	"Undefined 114",
	"Undefined 115",
	"Undefined 116",
	"Undefined 117",
	"Undefined 118",
	"Undefined 119",
	"All Sound Off",
	"Reset All Controllers",
	"Local Control",
	"All Notes Off",
	"Omni Mode Off",
	"Omni Mode On",
	"Mono Mode On",
	"Poly Mode On",
};

static const char* szNoteName[] = {
	"C ",
	"Db",
	"D ",
	"Eb",
	"E ",
	"F ",
	"Gb",
	"G ",
	"Ab",
	"A ",
	"Bb",
	"B ",
};

static const char* szMidiNoteName[] = {
  "C -2",
  "Db-2",
  "D -2",
  "Eb-2",
  "E -2",
  "F -2",
  "Gb-2",
  "G -2",
  "Ab-2",
  "A -2",
  "Bb-2",
  "B -2",

  "C -1",
  "Db-1",
  "D -1",
  "Eb-1",
  "E -1",
  "F -1",
  "Gb-1",
  "G -1",
  "Ab-1",
  "A -1",
  "Bb-1",
  "B -1",

  "C 0",
  "Db0",
  "D 0",
  "Eb0",
  "E 0",
  "F 0",
  "Gb0",
  "G 0",
  "Ab0",
  "A 0",
  "Bb0",
  "B 0",

  "C 1",
  "Db1",
  "D 1",
  "Eb1",
  "E 1",
  "F 1",
  "Gb1",
  "G 1",
  "Ab1",
  "A 1",
  "Bb1",
  "B 1",

  "C 2",
  "Db2",
  "D 2",
  "Eb2",
  "E 2",
  "F 2",
  "Gb2",
  "G 2",
  "Ab2",
  "A 2",
  "Bb2",
  "B 2",

  "C 3",
  "Db3",
  "D 3",
  "Eb3",
  "E 3",
  "F 3",
  "Gb3",
  "G 3",
  "Ab3",
  "A 3",
  "Bb3",
  "B 3",

  "C 4",
  "Db4",
  "D 4",
  "Eb4",
  "E 4",
  "F 4",
  "Gb4",
  "G 4",
  "Ab4",
  "A 4",
  "Bb4",
  "B 4",

  "C 5",
  "Db5",
  "D 5",
  "Eb5",
  "E 5",
  "F 5",
  "Gb5",
  "G 5",
  "Ab5",
  "A 5",
  "Bb5",
  "B 5",

  "C 6",
  "Db6",
  "D 6",
  "Eb6",
  "E 6",
  "F 6",
  "Gb6",
  "G 6",
  "Ab6",
  "A 6",
  "Bb6",
  "B 6",

  "C 7",
  "Db7",
  "D 7",
  "Eb7",
  "E 7",
  "F 7",
  "Gb7",
  "G 7",
  "Ab7",
  "A 7",
  "Bb7",
  "B 7",

  "C 8",
  "Db8",
  "D 8",
  "Eb8",
  "E 8",
  "F 8",
  "Gb8",
  "G 8",
  "Ab8",
  "A 8",
  "Bb8",
  "B 8",

  "C 9",
  "Db9",
  "D 9",
  "Eb9",
  "E 9",
  "F 9",
  "Gb9",
  "G 9",
  "Ab9",
  "A 9",
  "Bb9",
  "B 9",

  "C 10",
  "Db10",
  "D 10",
  "Eb10",
  "E 10",
  "F 10",
  "Gb10",
  "G 10",
};

static const float fMidiNoteFreqList[128] = {
  8.18f,
  8.66f,
  9.18f,
  9.72f,
  10.30f,
  10.91f,
  11.56f,
  12.25f,
  12.98f,
  13.75f,
  14.57f,
  15.43f,
  16.35f,
  17.32f,
  18.35f,
  19.45f,
  20.60f,
  21.83f,
  23.12f,
  24.50f,
  25.96f,
  27.50f,
  29.14f,
  30.87f,
  32.70f,
  34.65f,
  36.71f,
  38.89f,
  41.20f,
  43.65f,
  46.25f,
  49.00f,
  51.91f,
  55.00f,
  58.27f,
  61.74f,
  65.41f,
  69.30f,
  73.42f,
  77.78f,
  82.41f,
  87.31f,
  92.50f,
  98.00f,
  103.83f,
  110.00f,
  116.54f,
  123.47f,
  130.81f,
  138.59f,
  146.83f,
  155.56f,
  164.81f,
  174.61f,
  185.00f,
  196.00f,
  207.65f,
  220.00f,
  233.08f,
  246.94f,
  261.63f,
  277.18f,
  293.66f,
  311.13f,
  329.63f,
  349.23f,
  369.99f,
  392.00f,
  415.30f,
  440.00f, // A
  466.16f,
  493.88f,
  523.25f,
  554.37f,
  587.33f,
  622.25f,
  659.26f,
  698.46f,
  739.99f,
  783.99f,
  830.61f,
  880.00f,
  932.33f,
  987.77f,
  1046.50f,
  1108.73f,
  1174.66f,
  1244.51f,
  1318.51f,
  1396.91f,
  1479.98f,
  1567.98f,
  1661.22f,
  1760.00f,
  1864.66f,
  1975.53f,
  2093.00f,
  2217.46f,
  2349.32f,
  2489.02f,
  2637.02f,
  2793.83f,
  2959.96f,
  3135.96f,
  3322.44f,
  3520.00f,
  3729.31f,
  3951.07f,
  4186.01f,
  4434.92f,
  4698.64f,
  4978.03f,
  5274.04f,
  5587.65f,
  5919.91f,
  6271.93f,
  6644.88f,
  7040.00f,
  7458.62f,
  7902.13f,
  8372.02f,
  8869.84f,
  9397.27f,
  9956.06f,
  10548.08f,
  11175.30f,
  11839.82f,
  12543.85f
};

// TODO: add frequencies of all 127 midi notes here!
static const float fFreqlist[] = {
	261.63f,
	277.18f,
	293.66f,
	311.13f,
	329.63f,
	349.23f,
	369.99f,
	392.00f,
	415.30f,
	440.00f,
	466.16f,
	493.88f,
};

/*
** Name resolving functions
*/
bool muGetInstrumentName(char *pName, int32_t iInstr) {
	if (iInstr < 0 || iInstr > 127)
		return false;

  strcpy(pName, szPatchList[iInstr]);
	return true;
}

bool muGetDrumName(char *pName, int32_t iInstr) {
	if (iInstr < 0 || iInstr > 127)
		return false;
	strcpy(pName, szGMDrums[iInstr]);
	return true;
}

void muGetMIDIMsgName(char *pName, tMIDI_MSG iMsg) {
	switch(iMsg){
		case	msgNoteOff:         strcpy(pName, "Note off");          break;
		case	msgNoteOn:          strcpy(pName, "Note on");           break;
		case	msgNoteKeyPressure: strcpy(pName, "Note key pressure"); break;
		case	msgSetParameter:    strcpy(pName, "Set parameter");     break;
		case	msgSetProgram:      strcpy(pName, "Set program");       break;
		case	msgChangePressure:  strcpy(pName, "Change pressure");   break;
		case	msgSetPitchWheel:   strcpy(pName, "Set pitch wheel");   break;
		case	msgMetaEvent:       strcpy(pName, "Meta event");        break;
		case	msgSysEx1:          strcpy(pName, "SysEx1");            break;
		case	msgSysEx2:          strcpy(pName, "SysEx2");            break;
    default:                  strcpy(pName, "Unknown");           break;
		}
}

bool muGetControlName(char *pName, tMIDI_CC iCC) {
	if (iCC < 0 || iCC > 127)
		return false;
	strcpy(pName, szCCList[iCC]);
	return true;
}

bool muGetKeySigName(char *pName, tMIDI_KEYSIG iKey) {
static char *iKeysList[2][8] = {
/*#*/{"C ", "G ", "D ", "A ", "E ", "B ", "F#", "C#", },
/*b*/{"C ", "F ", "Bb", "Eb", "Ab", "Db", "Gb", "Cb", },
};

  int32_t iRootNum = (iKey&7);
  int32_t iFlats = (iKey&keyMaskNeg);
  int32_t iMin = (iKey&keyMaskMin);

	strcpy(pName, iKeysList[iFlats?1:0][iRootNum]);
	strcat(pName, iMin ? " Min" : " Maj");
	return true;
}

bool muGetTextName(char *pName, tMIDI_TEXT iEvent) {
	if (iEvent<1 || iEvent>7)	
    return false;

	return muGetMetaName(pName, (tMIDI_META)iEvent);
}

bool muGetMetaName(char *pName, tMIDI_META iEvent) {
	switch(iEvent) {
		case	metaSequenceNumber:	    strcpy(pName, "Sequence Number");	    break;
		case	metaTextEvent:		      strcpy(pName, "Text Event");		      break;
		case	metaCopyright:		      strcpy(pName, "Copyright");			      break;
		case	metaTrackName:		      strcpy(pName, "Track Name");		      break;
		case	metaInstrument:		      strcpy(pName, "Instrument");		      break;
		case	metaLyric:			        strcpy(pName, "Lyric");				        break;
		case	metaMarker:			        strcpy(pName, "Marker");			        break;
		case	metaCuePoint:		        strcpy(pName, "Cue Point");			      break;
		case	metaMIDIPort:		        strcpy(pName, "MIDI Port");		        break;
		case	metaEndSequence:	      strcpy(pName, "End Sequence");		    break;
		case	metaSetTempo:		        strcpy(pName, "Set Tempo");			      break;
		case	metaSMPTEOffset:	      strcpy(pName, "SMPTE Offset");		    break;
		case	metaTimeSig:		        strcpy(pName, "Time Sig");			      break;
		case	metaKeySig:			        strcpy(pName, "Key Sig");			        break;
		case	metaSequencerSpecific:	strcpy(pName, "Sequencer Specific");	break;
		default:	return false;
  }
	return true;

}


/*
** Conversion Functions
*/
int32_t muGetNoteFromName(const char *pName) {
  int32_t note_map[] = {9, 11, 0, 2, 4, 5, 7};
  char *p, cpy[16];
  int32_t note = 0;

	strncpy(cpy, pName, 15);
	cpy[15] = '\0';
	p = cpy;

	while(!isalpha(*p) && *p)
		p++;
	
	if (*p) {
		note = toupper(*p)-'A';
		if (note >= 0 && note <= 7) {
			note = note_map[note];
			p++;
			if (*p == 'b')
				note--, p++;
			else if (*p == '#')
				note++, p++;
			
			note += atoi(p)*12+MIDI_NOTE_C0;
    }
  }
	
	return note;
}

const char* muGetNameFromNote(int32_t iNote) {
  if (iNote < 0 || iNote > 127)
    return "ERR";
  else
    return szMidiNoteName[iNote + 24 + C0_BASE * 12];
}

float muGetFreqFromNote(int32_t iNote) {
int32_t oct = iNote/12-5;
float freq;

	if (iNote<0 || iNote>127)	return 0;

	freq = fFreqlist[iNote%12];
	
	while(oct > 0)
		freq *= 2.0f, oct--;
	
	while(oct < 0)
		freq /= 2.0f, oct++;
	
	return freq;
}

int32_t muGetNoteFromFreq(float fFreq) {
/* This is for completeness, I'm not sure of how often it
** will get used. Therefore, the code is un-optimised :)
*/
int32_t iNote, iBestNote=0;
float fDiff=20000, f;

	for(iNote=0;iNote<127;++iNote)
		{
		f = muGetFreqFromNote(iNote);
		f -= fFreq; if (f<0) f=-f;
		if (f < fDiff)
			{
			fDiff = f; 
			iBestNote = iNote;
			}
		}
		
	return iBestNote;
}


int32_t muGuessChord(const int32_t *pNoteStatus, const int32_t channel, const int32_t lowRange, const int32_t highRange) {
	int32_t octave[24];
	int32_t i;
	int32_t lowestNote=999;
	int32_t startNote = 999;
	int32_t chordRoot = 0;
	int32_t chordType = 0;
	int32_t chordAdditions = 0;

	for(i=0;i<24;++i) {
		octave[i] = 0;
	}

	for(i=lowRange;i<=highRange;++i) {
		if (pNoteStatus[channel*128 + i]) {
			if (i<lowestNote) {
				lowestNote = i;
			}
			++octave[i%12];
			++octave[i%12+12];
			if ((i%12) < startNote) {
				startNote = i%12;
			}
		}
	}

	if (lowestNote == 999) {
		return -1;
	}

	/* Bring it into line with the 0-11 range */
	lowestNote %= 12;
	
	/* Majors */
	if (octave[startNote+3] && octave[startNote+8]) {
		chordRoot = startNote+8;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+5] && octave[startNote+9]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+4] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MAJOR;

	/* Minor */
	} else if (octave[startNote+4] && octave[startNote+9]) {
		chordRoot = startNote+9;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+5] && octave[startNote+8]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+3] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MINOR;

	/* Diminished */
	} else if (octave[startNote+3] && octave[startNote+6]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;
	} else if (octave[startNote+6] && octave[startNote+9]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;

	/* Augmented */
	} else if (octave[startNote+4] && octave[startNote+8]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_AUG;
	}

	if (octave[chordRoot + 10]) {
		chordAdditions |= CHORD_ADD_7TH;
	}
	if (octave[chordRoot + 11]) {
		chordAdditions |= CHORD_ADD_MAJ7TH;
	}
	if (octave[chordRoot + 2]) {
		chordAdditions |= CHORD_ADD_9TH;
	}

	chordRoot %= 12;

	if (chordType == 0) {
		return -1;
	}

	return chordRoot | chordType | chordAdditions | (lowestNote<<16);
}

char *muGetChordName(char *str, int32_t chord) {
	int32_t root = chord & CHORD_ROOT_MASK;
	int32_t bass = (chord & CHORD_BASS_MASK) >> 16;

	if (root < 0 || root > 11) {
		root = 0;
	}

	if (bass < 0 || bass > 11) {
		bass = 0;
	}

	strcpy(str, szNoteName[root]);

	switch(chord & CHORD_TYPE_MASK) {
		case CHORD_TYPE_MAJOR:
			break;
		case CHORD_TYPE_MINOR:
			strcat(str, "m");
			break;
		case CHORD_TYPE_AUG:
			strcat(str, " aug");
			break;
		case CHORD_TYPE_DIM:
			strcat(str, " dim");
			break;
	}

	if (chord & CHORD_ADD_7TH) {
		strcat(str, "+7");
	}
	if (chord & CHORD_ADD_9TH) {
		strcat(str, "+9");
	}
	if (chord & CHORD_ADD_MAJ7TH) {
		strcat(str, "+7M");
	}
	
	if (bass != root) {
		strcat(str, "/");
		strcat(str, szNoteName[bass]);
	}

	return str;
}

