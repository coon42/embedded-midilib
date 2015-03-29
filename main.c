#include "hal_midiplayer_windows.h"
#include "midiplayer.h"
#include "main.h"

static MIDI_PLAYER mpl;
int main(int argc, char* argv[]) {
  hal_midiplayer_init();

  if (argc == 1)
    printf("Usage: %s <filename>\n", argv[0]);
  else {
    for (int i = 1; i < argc; ++i) {
      char* midiFileName = argv[i];
      printf("Playing file: '%s'\r\n", midiFileName);
      if (!playMidiFile(&mpl, midiFileName)) {
        hal_printfError("Playback failed!");
        return 1;
      }

      while (midiPlayerTick(&mpl));
      hal_printfSuccess("Playback finished!");
    }
  }

  hal_midiplayer_free();
  return 0;
}