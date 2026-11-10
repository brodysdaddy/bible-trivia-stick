// sounds.h - Every sound effect in the game, defined as plain data.
// Portable: no platform code. Each platform's player (see sound_player.h)
// just walks the note list and plays each tone for its duration.
//
// TO TWEAK A SOUND: edit its note list below and rebuild.
//   { frequency_in_Hz, duration_in_ms }    use REST (0 Hz) for silence.
// Higher Hz = higher pitch. A sound longer than 1.5 s won't compile
// (see the static_asserts at the bottom).
#pragma once
#include <stddef.h>
#include <stdint.h>

struct Note {
    uint16_t freqHz;   // 0 = rest (silence)
    uint16_t ms;
};

struct Sound {
    const Note* notes;
    uint8_t     count;
};

enum class SoundId : uint8_t {
    Click,      // answer / menu choice entered
    Correct,    // answer was right
    Wrong,      // answer was wrong
    Results,    // score appears (2-4 correct)
    Perfect,    // score appears, 5/5
    LowScore,   // score appears, 0-1 correct
    COUNT
};

// Musical note frequencies (Hz), so tunes are easier to read and edit.
enum : uint16_t {
    REST = 0,
    NOTE_A2 = 110, NOTE_C3 = 131,
    NOTE_E4 = 330, NOTE_F4 = 349, NOTE_FS4 = 370, NOTE_G4 = 392,
    NOTE_A4 = 440, NOTE_B4 = 494,
    NOTE_C5 = 523, NOTE_D5 = 587, NOTE_E5 = 659, NOTE_F5 = 698,
    NOTE_G5 = 784, NOTE_A5 = 880, NOTE_B5 = 988,
    NOTE_C6 = 1047, NOTE_E6 = 1319,
};

// ---------------------------------------------------------------- sounds

// 1. Click: very short, subtle tick.
static constexpr Note SND_CLICK[] = {
    { 1800, 15 },
};

// 2. Correct: quick rising three-note chime (C major arpeggio).
static constexpr Note SND_CORRECT[] = {
    { NOTE_E5, 80 }, { NOTE_G5, 80 }, { NOTE_C6, 160 },
};

// 3. Wrong: short low buzz, two pulses.
static constexpr Note SND_WRONG[] = {
    { NOTE_C3, 140 }, { REST, 40 }, { NOTE_A2, 220 },
};

// 4. Results fanfare (2-4 correct): bright rising arpeggio.
static constexpr Note SND_RESULTS[] = {
    { NOTE_C5, 110 }, { NOTE_E5, 110 }, { NOTE_G5, 110 }, { NOTE_C6, 320 },
};

// 5. Perfect score: longer, more triumphant fanfare ending high.
static constexpr Note SND_PERFECT[] = {
    { NOTE_G4, 90 }, { NOTE_C5, 90 }, { NOTE_E5, 90 }, { NOTE_G5, 180 },
    { REST, 40 },
    { NOTE_E5, 90 }, { NOTE_G5, 90 }, { NOTE_C6, 120 }, { NOTE_E6, 400 },
};

// 6. Low score (0-1): "womp womp" sliding down by half steps.
static constexpr Note SND_LOW_SCORE[] = {
    { NOTE_G4, 260 }, { REST, 40 },
    { NOTE_FS4, 260 }, { REST, 40 },
    { NOTE_F4, 260 }, { REST, 40 },
    { NOTE_E4, 450 },
};

// ------------------------------------------------------------- lookup

#define SOUND_ENTRY(arr) { arr, (uint8_t)(sizeof(arr) / sizeof(arr[0])) }

// Order must match the SoundId enum.
static constexpr Sound SOUND_TABLE[] = {
    SOUND_ENTRY(SND_CLICK),
    SOUND_ENTRY(SND_CORRECT),
    SOUND_ENTRY(SND_WRONG),
    SOUND_ENTRY(SND_RESULTS),
    SOUND_ENTRY(SND_PERFECT),
    SOUND_ENTRY(SND_LOW_SCORE),
};
static_assert(sizeof(SOUND_TABLE) / sizeof(SOUND_TABLE[0]) == (size_t)SoundId::COUNT,
              "SOUND_TABLE must have one entry per SoundId, in the same order");

inline const Sound& getSound(SoundId id) {
    return SOUND_TABLE[(size_t)id];
}

// ------------------------------------------------- length check (<=1.5 s)

template <size_t N>
constexpr uint32_t soundLengthMs(const Note (&notes)[N], size_t i = 0) {
    return i >= N ? 0 : notes[i].ms + soundLengthMs(notes, i + 1);
}

static_assert(soundLengthMs(SND_CLICK)     <= 1500, "SND_CLICK is over 1.5 s");
static_assert(soundLengthMs(SND_CORRECT)   <= 1500, "SND_CORRECT is over 1.5 s");
static_assert(soundLengthMs(SND_WRONG)     <= 1500, "SND_WRONG is over 1.5 s");
static_assert(soundLengthMs(SND_RESULTS)   <= 1500, "SND_RESULTS is over 1.5 s");
static_assert(soundLengthMs(SND_PERFECT)   <= 1500, "SND_PERFECT is over 1.5 s");
static_assert(soundLengthMs(SND_LOW_SCORE) <= 1500, "SND_LOW_SCORE is over 1.5 s");
