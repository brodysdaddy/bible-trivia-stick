// sound_m5.cpp - M5StickS3 playback of the sounds in sounds.h.
#include <M5Unified.h>
#include "sound_player.h"

static bool g_soundEnabled = true;

void setSoundEnabled(bool enabled) { g_soundEnabled = enabled; }
bool isSoundEnabled() { return g_soundEnabled; }

void playSound(SoundId id) {
    if (!g_soundEnabled) return;
    const Sound& s = getSound(id);
    for (uint8_t i = 0; i < s.count; ++i) {
        const Note& n = s.notes[i];
        // tone() returns immediately, so wait out each note to keep
        // playSound() blocking, matching the Windows version.
        if (n.freqHz > 0) M5.Speaker.tone(n.freqHz, n.ms);
        delay(n.ms);
    }
}
