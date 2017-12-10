#ifndef __MIDIFILE_H
#define __MIDIFILE_H

#include <stdbool.h>
#include "midifile.h"

// Callback function pointer typedefs for MIDI events
typedef void(*OnNoteOffCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t note);
typedef void(*OnNoteOnCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t velocity);
typedef void(*OnNoteKeyPressureCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t pressure);
typedef void(*OnSetParameterCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t control, int32_t parameter);
typedef void(*OnSetProgramCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t program);
typedef void(*OnChangePressureCallback_t)(int32_t track, int32_t tick, int32_t channel, int32_t pressure);
typedef void(*OnSetPitchWheelCallback_t)(int32_t track, int32_t tick, int32_t channel, int16_t pitch);
typedef void(*OnMetaMIDIPortCallback_t)(int32_t track, int32_t tick, int32_t midiPort);
typedef void(*OnMetaSequenceNumberCallback_t)(int32_t track, int32_t tick, int32_t sequenceNumber);
typedef void(*OnMetaTextEventCallback_t)(int32_t track, int32_t tick, char* pText);
typedef void(*OnMetaCopyrightCallback_t)(int32_t track, int32_t tick, char* pText);
typedef void(*OnMetaTrackNameCallback_t)(int32_t track, int32_t tick, char *pText);
typedef void(*OnMetaInstrumentCallback_t)(int32_t track, int32_t tick, char *pText);
typedef void(*OnMetaLyricCallback_t)(int32_t track, int32_t tick, char *pText);
typedef void(*OnMetaMarkerCallback_t)(int32_t track, int32_t tick, char *pText);
typedef void(*OnMetaCuePointCallback_t)(int32_t track, int32_t tick, char *pText);
typedef void(*OnMetaEndSequenceCallback_t)(int32_t track, int32_t tick);
typedef void(*OnMetaSetTempoCallback_t)(int32_t track, int32_t tick, int32_t bpm);
typedef void(*OnMetaSMPTEOffsetCallback_t)(int32_t track, int32_t tick, uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frames, uint32_t subframes);
typedef void(*OnMetaTimeSigCallback_t)(int32_t track, int32_t tick, int32_t nom, int32_t denom, int32_t metronome, int32_t thirtyseconds);
typedef void(*OnMetaKeySigCallback_t)(int32_t track, int32_t tick, uint32_t key, uint32_t scale);
typedef void(*OnMetaSequencerSpecificCallback_t)(int32_t track, int32_t tick, void* pData, uint32_t size);
typedef void(*OnMetaSysExCallback_t)(int32_t track, int32_t tick, void* pData, uint32_t size);

// Custom callbacks
// TODO: onCacheMiss()

typedef struct {
  _MIDI_FILE* pMidiFile;
  MIDI_MSG msg[MAX_MIDI_TRACKS];
  int32_t startTime;
  int32_t currentTick;
  int32_t lastTick;
  bool trackIsFinished;
  bool allTracksAreFinished;
  int32_t lastUsPerTick;

  // Callback function pointers
  OnNoteOffCallback_t pOnNoteOffCb;
  OnNoteOnCallback_t pOnNoteOnCb;
  OnNoteKeyPressureCallback_t pOnNoteKeyPressureCb;
  OnSetParameterCallback_t pOnSetParameterCb;
  OnSetProgramCallback_t pOnSetProgramCb;
  OnChangePressureCallback_t pOnChangePressureCb;
  OnSetPitchWheelCallback_t pOnSetPitchWheelCb;
  OnMetaMIDIPortCallback_t pOnMetaMIDIPortCb;
  OnMetaSequenceNumberCallback_t pOnMetaSequenceNumberCb;
  OnMetaTextEventCallback_t pOnMetaTextEventCb;
  OnMetaCopyrightCallback_t pOnMetaCopyrightCb;
  OnMetaTrackNameCallback_t pOnMetaTrackNameCb;
  OnMetaInstrumentCallback_t pOnMetaInstrumentCb;
  OnMetaLyricCallback_t pOnMetaLyricCb;
  OnMetaMarkerCallback_t pOnMetaMarkerCb;
  OnMetaCuePointCallback_t pOnMetaCuePointCb;
  OnMetaEndSequenceCallback_t pOnMetaEndSequenceCb;
  OnMetaSetTempoCallback_t pOnMetaSetTempoCb;
  OnMetaSMPTEOffsetCallback_t pOnMetaSMPTEOffsetCb;
  OnMetaTimeSigCallback_t pOnMetaTimeSigCb;
  OnMetaKeySigCallback_t pOnMetaKeySigCb;
  OnMetaSequencerSpecificCallback_t pOnMetaSequencerSpecificCb;
  OnMetaSysExCallback_t pOnMetaSysExCb;

} MIDI_PLAYER;

void midiplayer_init(MIDI_PLAYER* mpl,
  OnNoteOffCallback_t pOnNoteOffCb,
  OnNoteOnCallback_t pOnNoteOnCb,
  OnNoteKeyPressureCallback_t pOnNoteKeyPressureCb,
  OnSetParameterCallback_t pOnSetParameterCb,
  OnSetProgramCallback_t pOnSetProgramCb,
  OnChangePressureCallback_t pOnChangePressureCb,
  OnSetPitchWheelCallback_t pOnSetPitchWheelCb,
  OnMetaMIDIPortCallback_t pOnMetaMIDIPortCb,
  OnMetaSequenceNumberCallback_t pOnMetaSequenceNumberCb,
  OnMetaTextEventCallback_t pOnMetaTextEventCb,
  OnMetaCopyrightCallback_t pOnMetaCopyrightCb,
  OnMetaTrackNameCallback_t pOnMetaTrackNameCb,
  OnMetaInstrumentCallback_t pOnMetaInstrumentCb,
  OnMetaLyricCallback_t pOnMetaLyricCb,
  OnMetaMarkerCallback_t pOnMetaMarkerCb,
  OnMetaCuePointCallback_t pOnMetaCuePointCb,
  OnMetaEndSequenceCallback_t pOnMetaEndSequenceCb,
  OnMetaSetTempoCallback_t pOnMetaSetTempoCb,
  OnMetaSMPTEOffsetCallback_t pOnMetaSMPTEOffsetCb,
  OnMetaTimeSigCallback_t pOnMetaTimeSigCb,
  OnMetaKeySigCallback_t pOnMetaKeySigCb,
  OnMetaSequencerSpecificCallback_t pOnMetaSequencerSpecificCb,
  OnMetaSysExCallback_t pOnMetaSysExCb
);

bool midiPlayerTick(MIDI_PLAYER* pMidiPlayer);
bool playMidiFile(MIDI_PLAYER* pMidiPlayer, const char *pFilename);
void adjustTimeFactor(MIDI_PLAYER* pMp);

#endif // __MIDIFILE_H
