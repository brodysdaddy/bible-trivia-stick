// sound_player.h - The playback interface every platform implements.
//
// Only declarations here. Each platform provides one .cpp:
//   Windows console : src/console/sound_windows.cpp  (uses Beep)
//   M5StickS3       : e.g. sound_m5.cpp               (uses M5.Speaker.tone)
#pragma once
#include "sounds.h"

void setSoundEnabled(bool enabled);
bool isSoundEnabled();

// Plays the whole sound. Returns when it is finished (blocking), so the
// front end can play sounds in sequence without overlap.
void playSound(SoundId id);
