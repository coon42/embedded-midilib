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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"
#include "midiutil.h"

#define DEFAULT_BPM 120; // If no tempo is define, 120 beats per minute is assumed in MIDI standard.


void HexList(BYTE *pData, int iNumBytes)
{
int i;

	for(i=0;i<iNumBytes;i++)
		printf("%.2x ", pData[i]);
}

void DumpEventList(const char *pFilename)
{
  MIDI_FILE* mf = midiFileOpen(pFilename, FALSE);
  MIDI_FILE* mfEmbedded = midiFileOpen(pFilename, TRUE);
  char str[128];
  int ev;

	if (mf) {
		MIDI_MSG msg, msgEmbedded;
		int i, iNum;
		unsigned int j;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf, mfEmbedded);

		for(i=0;i<iNum;i++) {
			printf("# Track %d\n", i);
			while(midiReadGetNextMessage(mf, mfEmbedded, i, &msg, &msgEmbedded, FALSE)) {
				printf(" %.6ld ", msgEmbedded.dwAbsPos);
				if (msgEmbedded.bImpliedMsg)
					{ ev = msgEmbedded.iImpliedMsg; }
				else
					{ ev = msgEmbedded.iType; }

				if (muGetMIDIMsgName(str, ev))	printf("%s\t", str);
				switch(ev) {
					case	msgNoteOff:
							muGetNameFromNote(str, msgEmbedded.MsgData.NoteOff.iNote);
							printf("(%.2d) %s", msgEmbedded.MsgData.NoteOff.iChannel, str);
							break;
					case	msgNoteOn:
							muGetNameFromNote(str, msgEmbedded.MsgData.NoteOn.iNote);
							printf("\t(%.2d) %s %d", msgEmbedded.MsgData.NoteOn.iChannel, str, msgEmbedded.MsgData.NoteOn.iVolume);
							break;
					case	msgNoteKeyPressure:
							muGetNameFromNote(str, msgEmbedded.MsgData.NoteKeyPressure.iNote);
							printf("(%.2d) %s %d", msgEmbedded.MsgData.NoteKeyPressure.iChannel, 
									str,
									msgEmbedded.MsgData.NoteKeyPressure.iPressure);
							break;
					case	msgSetParameter:
							muGetControlName(str, msgEmbedded.MsgData.NoteParameter.iControl);
							printf("(%.2d) %s -> %d", msgEmbedded.MsgData.NoteParameter.iChannel, 
									str, msgEmbedded.MsgData.NoteParameter.iParam);
							break;
					case	msgSetProgram:
							muGetInstrumentName(str, msgEmbedded.MsgData.ChangeProgram.iProgram);
							printf("(%.2d) %s", msgEmbedded.MsgData.ChangeProgram.iChannel, str);
							break;
					case	msgChangePressure:
							muGetControlName(str, msgEmbedded.MsgData.ChangePressure.iPressure);
							printf("(%.2d) %s", msgEmbedded.MsgData.ChangePressure.iChannel, str);
							break;
					case	msgSetPitchWheel:
							printf("(%.2d) %d", msgEmbedded.MsgData.PitchWheel.iChannel,  
									msgEmbedded.MsgData.PitchWheel.iPitch);
							break;

					case	msgMetaEvent:
							printf("---- ");
							switch(msgEmbedded.MsgData.MetaEvent.iType) {
								case	metaMIDIPort:
										printf("MIDI Port = %d", msgEmbedded.MsgData.MetaEvent.Data.iMIDIPort);
										break;

								case	metaSequenceNumber:
										printf("Sequence Number = %d",msgEmbedded.MsgData.MetaEvent.Data.iSequenceNumber);
										break;

								case	metaTextEvent:
										printf("Text = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaCopyright:
										printf("Copyright = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaTrackName:
										printf("Track name = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaInstrument:
										printf("Instrument = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaLyric:
										printf("Lyric = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaMarker:
										printf("Marker = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaCuePoint:
										printf("Cue point = '%s'",msgEmbedded.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaEndSequence:
										printf("End Sequence");
										break;
								case	metaSetTempo:
										printf("Tempo = %d",msgEmbedded.MsgData.MetaEvent.Data.Tempo.iBPM);
										break;
								case	metaSMPTEOffset:
										printf("SMPTE offset = %d:%d:%d.%d %d",
												msgEmbedded.MsgData.MetaEvent.Data.SMPTE.iHours,
												msgEmbedded.MsgData.MetaEvent.Data.SMPTE.iMins,
												msgEmbedded.MsgData.MetaEvent.Data.SMPTE.iSecs,
												msgEmbedded.MsgData.MetaEvent.Data.SMPTE.iFrames,
												msgEmbedded.MsgData.MetaEvent.Data.SMPTE.iFF
												);
										break;
								case	metaTimeSig:
										printf("Time sig = %d/%d",msgEmbedded.MsgData.MetaEvent.Data.TimeSig.iNom,
													msgEmbedded.MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
										break;
								case	metaKeySig:
										if (muGetKeySigName(str, msgEmbedded.MsgData.MetaEvent.Data.KeySig.iKey))
											printf("Key sig = %s", str);
										break;

								case	metaSequencerSpecific:
										printf("Sequencer specific = ");
										HexList(msgEmbedded.MsgData.MetaEvent.Data.Sequencer.pData, msgEmbedded.MsgData.MetaEvent.Data.Sequencer.iSize);
										break;
								}
							break;

					case	msgSysEx1:
					case	msgSysEx2:
							printf("Sysex = ");
							HexList(msgEmbedded.MsgData.SysEx.pData, msgEmbedded.MsgData.SysEx.iSize);
							break;
					}

				if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msgEmbedded.MsgData.MetaEvent.iType==metaSequencerSpecific))
					{
					/* Already done a hex dump */
					}
				else {
					printf("\t[");
					if (msgEmbedded.bImpliedMsg) printf("%.2x!", msgEmbedded.iImpliedMsg);
					for(j=0;j<msgEmbedded.iMsgSize;j++)
						printf("%.2x ", msgEmbedded.dataEmbedded[j]);
					printf("]\n");
					}
				}
			}

		midiReadFreeMessage(&msg);
		midiFileClose(mf, mfEmbedded); // obsolete?
		}

    printf("\r\nPlayback finished!\r\n\r\n");
}

int main(int argc, char* argv[])
{
int i=0;

	if (argc==1)
		{
		printf("Usage: %s <filename>\n", argv[0]);
		}
	else
		{
		for(i=1;i<argc;++i)
			DumpEventList(argv[i]);
		}

	return 0;
}
