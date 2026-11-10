// sound_windows.cpp - Windows playback of the sounds in sounds.h, via Beep().
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../core/sound_player.h"

static bool g_soundEnabled = true;

void setSoundEnabled(bool enabled) { g_soundEnabled = enabled; }
bool isSoundEnabled() { return g_soundEnabled; }

void playSound(SoundId id) {
    if (!g_soundEnabled) return;
    const Sound& s = getSound(id);
    for (uint8_t i = 0; i < s.count; ++i) {
        const Note& n = s.notes[i];
        // Beep() accepts 37-32767 Hz; anything outside is treated as a rest.
        if (n.freqHz >= 37) Beep(n.freqHz, n.ms);
        else                Sleep(n.ms);
    }
}
