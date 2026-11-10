// trivia_stick.ino - Bible Trivia for the M5StickS3.
//
// The game logic, questions and sound data come from src/core (shared with
// the Windows console version). Build with stick\upload.bat, which passes
// src/core to arduino-cli as a library.
//
// The app itself lives in stick_app.cpp; sound playback in sound_m5.cpp.

void appSetup();
void appLoop();

void setup() { appSetup(); }
void loop()  { appLoop(); }
