# Bible Trivia Stick

A Bible trivia game written in C++ that runs in two places from one shared codebase:

- a **Windows console app**, and
- a **handheld M5StickS3** (ESP32-S3, 135×240 screen, two buttons, built-in speaker and battery).

Each test draws **5 random multiple-choice questions** from a pool of **500** (250 Old Testament, 250 New Testament). Answer choices are shuffled every time, the score stays hidden until the end, and then each answer is revealed one by one with a sound before the final score and a fanfare. Afterwards you can retake the same test, start a new one, or quit.

---

## Architecture

```
src/
  core/        Portable game logic: no I/O, no platform code
    question.h / questions.cpp   The 500-question pool (plain data)
    trivia_game.h / .cpp         Test selection, shuffling, answers, scoring
    sounds.h                     Every sound effect as data (Hz + ms)
    sound_player.h               3-function playback interface
  console/     Windows front end
    main.cpp                     Text UI (stdin/stdout)
    sound_windows.cpp            playSound() via Win32 Beep()
stick/         M5StickS3 front end (Arduino + M5Unified)
  trivia_stick/
    stick_app.cpp                Screen drawing, 2-button menus, power management
    sound_m5.cpp                 playSound() via M5.Speaker.tone()
    trivia_stick.ino             Thin Arduino entry point
  hello_stick/                   Hardware bring-up sketch
  upload.bat                     Compile + flash helper
build.bat      Windows build script
```

### Why the logic is separated

The game was built on Windows first and ported to the stick later. Everything about *what the game is* lives in `src/core`, which:

- uses **no heap allocation**, no `<random>`, no `time()`, and no I/O, so it compiles unchanged for both a PC and a microcontroller;
- has its **own small PRNG** (xorshift32 with rejection sampling). Each front end supplies its seed: a high-resolution clock on Windows, the hardware RNG (`esp_random()`) on the stick;
- exposes a small API (`newTest()`, `retakeTest()`, `answer(pos)`, `choice(i, pos)`, `isCorrect(i)`, `score()`, …) so a front end only handles drawing and input.

Porting to the stick meant writing a new front end only. The core went in untouched as an Arduino library (`--library src/core`). The same design would make a web, desktop GUI, or voice front end straightforward.

### Game logic details

- **Question picking:** 5 distinct questions per test (duplicates are re-rolled).
- **Choice shuffling:** a Fisher–Yates shuffle per question, stored as a position→original-index map. This keeps the question data simple: the correct answer can be listed first in the source, and players still see it in a random spot. Tested over 10,000 games: the correct answer lands on A/B/C/D evenly.
- **Retake:** same questions in the same order, with the choices reshuffled so you can't just memorize letters.

---

## Sound effects

Sounds are designed **as data, not code**. `src/core/sounds.h` defines each effect as an array of `{frequencyHz, durationMs}` notes:

| Sound | When it plays |
|---|---|
| Click | Every answer / menu selection |
| Correct | Ascending chime during the reveal |
| Wrong | Low buzz during the reveal |
| Results | Fanfare for 2–4 correct |
| Perfect | Bigger fanfare for 5/5 |
| Low score | "Womp womp" for 0–1 |

Each platform implements one function, `playSound(SoundId)`: `Beep()` on Windows, `M5.Speaker.tone()` on the stick. A `static_assert` caps every sound at 1.5 seconds at compile time.

**Design decision:** playing Correct/Wrong right after each answer would give away the score early. So during the test only a neutral click plays, and right/wrong sounds come on the **results reveal**, one question at a time, followed by the fanfare.

To tweak a sound, edit its note array in `sounds.h` (a frequency of `0` is a rest) and rebuild. Both platforms pick up the change.

---

## Building for Windows

Requires either **Visual Studio / Build Tools** (MSVC) or **MinGW/MSYS2 g++**.

```bat
:: From a "Developer Command Prompt for VS" (or any terminal with g++ on PATH):
build.bat
trivia.exe
```

Or compile manually:

```bat
g++ -std=c++17 -O2 src\core\questions.cpp src\core\trivia_game.cpp src\console\main.cpp src\console\sound_windows.cpp -o trivia.exe
```

At startup it asks whether sound should be on, then you answer with `A`–`D`.

---

## Building for the M5StickS3

Uses [arduino-cli](https://arduino.github.io/arduino-cli/) with the M5Stack ESP32 board package and the M5Unified library.

**One-time setup:**

```bat
arduino-cli config add board_manager.additional_urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core update-index
arduino-cli core install m5stack:esp32
arduino-cli lib install M5Unified
```

**Compile and upload** (the stick connects as a USB serial port; check Device Manager for the COM number):

```bat
stick\upload.bat                       :: trivia game on COM4
stick\upload.bat stick\trivia_stick COM5
```

If the upload can't connect, hold the side button about 2 seconds until the green LED blinks (download mode) and run it again.

**Controls on the stick:**

- **Side button:** move to the next choice.
- **Front button:** select.

**Stick features:**

- Main menu with Start, Sound on/off (saved to flash) and Power off.
- Word-wrapped questions with an automatic smaller font for long text.
- Battery gauge with a charging indicator.
- Flicker-free drawing to an off-screen sprite.
- **Power management:** dims after 30 seconds idle. On battery it powers off after 3 minutes idle. On USB it never auto-powers off.

The whole firmware is about 700 KB, around 20% of flash. Each question costs about 94 bytes, so the pool could grow to tens of thousands of questions.

---

## Adding questions

Questions live in `src/core/questions.cpp`, one per line:

```cpp
{ Testament::Old, "Who built the ark?", { "Noah", "Moses", "Abraham", "David" }, 0 },
```

The last number is the index of the correct choice. Since choices are shuffled at runtime, listing the correct answer first is fine. Avoid "All of the above" style answers, since order isn't fixed. Keep questions under about 95 characters and choices under about 28 so they fit the stick's screen.

---

## Future ideas

- **Voice answers:** say "A" or the answer text aloud, using the M5StickS3's built-in microphone or speech recognition on the PC version.
- **More questions:** grow the pool further, and add a generator that builds `questions.cpp` from a spreadsheet so non-programmers can contribute.
- **Old/New Testament mix:** let the player choose OT only, NT only, or a balanced mix per test.
- **No repeats:** avoid repeating questions until the whole pool has been seen.
- **Difficulty levels and categories:** for example people, places, miracles or prophecy.
- **High scores and streaks:** saved to flash on the stick.
