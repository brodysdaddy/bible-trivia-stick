// stick_app.cpp - M5StickS3 front end: screens and two-button controls.
//
// Controls (landscape, 240x135):
//   Side button  (BtnB) : move to the next choice
//   Front button (BtnA) : select / confirm
//
// Flow mirrors the console version: main menu (start / sound / power off)
// -> 5 questions (click only) -> results revealed one per screen with
// correct/wrong sound -> score + fanfare -> Retake / New test / Main menu /
// Power off.
//
// Idle: the screen dims after DIM_AFTER_MS with no button press. On battery,
// the stick powers off after OFF_AFTER_MS (never while plugged into USB).
// Press the side button briefly to turn it back on.
#include <M5Unified.h>
#include <Preferences.h>
#include <esp_random.h>
#include <string.h>
#include "trivia_game.h"
#include "sound_player.h"

namespace {

// ---- Tunable settings ----------------------------------------------------
const int      CORRECT_PAUSE_MS = 1500;      // reveal screen time, right answer
const int      WRONG_PAUSE_MS   = 2500;      // longer, so you can read the answer
const uint32_t DIM_AFTER_MS     = 30000;     // dim screen after 30 s idle
const uint32_t OFF_AFTER_MS     = 180000;    // power off after 3 min idle (battery only)
const uint8_t  DIM_BRIGHTNESS   = 16;        // 0-255

// ---- Colors --------------------------------------------------------------
const uint16_t BG     = 0x0010;          // dark navy
const uint16_t HILITE = TFT_YELLOW;
const uint16_t DIM    = TFT_LIGHTGREY;

const char LETTERS[4] = { 'A', 'B', 'C', 'D' };

enum class Btn { A, B };

M5Canvas     g(&M5.Display);   // off-screen buffer: draw, then push (no flicker)
int          W = 240, H = 135;
uint8_t      normalBrightness = 128;
TriviaGame*  game = nullptr;
Preferences  prefs;

void powerOffNow();

// True when USB is supplying power (read from the StickS3's power chip).
bool onUsbPower() {
    return M5.Power.getVBUSVoltage() > 4000;   // millivolts; -1 if unreadable
}

// ---- Input ---------------------------------------------------------------

// Waits for a button. Handles idle dimming and auto power-off; a press that
// only wakes a dimmed screen is not passed on to the game.
Btn waitButton(bool click = true) {
    uint32_t lastInput = millis();
    bool dimmed = false;
    for (;;) {
        M5.update();
        bool a = M5.BtnA.wasPressed();
        bool b = M5.BtnB.wasPressed();
        if (a || b) {
            if (dimmed) {
                M5.Display.setBrightness(normalBrightness);
                dimmed = false;
                lastInput = millis();
                continue;
            }
            if (click) playSound(SoundId::Click);
            return a ? Btn::A : Btn::B;
        }

        uint32_t idle = millis() - lastInput;
        if (!dimmed && idle > DIM_AFTER_MS) {
            M5.Display.setBrightness(DIM_BRIGHTNESS);
            dimmed = true;
            Serial.printf("Idle: screen dimmed. USB %d mV -> %s\n",
                          (int)M5.Power.getVBUSVoltage(),
                          onUsbPower() ? "plugged in, no auto power-off"
                                       : "on battery, auto power-off armed");
        }
        // Auto power-off only on battery; plugged in, it just stays dimmed.
        if (idle > OFF_AFTER_MS && !onUsbPower()) {
            powerOffNow();
            // Only reached if power-off didn't happen.
            dimmed = false;
            lastInput = millis();
        }
        delay(10);
    }
}

// Waits up to ms; returns early if the front button is pressed.
void waitOrSkip(int ms) {
    uint32_t end = millis() + ms;
    while ((int32_t)(end - millis()) > 0) {
        M5.update();
        if (M5.BtnA.wasPressed()) { playSound(SoundId::Click); return; }
        delay(5);
    }
}

// ---- Text helpers --------------------------------------------------------

// Word-wraps text into lines no wider than maxW (current font). Draws them
// when draw is true; always returns the number of lines.
int wrapText(const char* text, int x, int y, int maxW, int lineH, bool draw) {
    char line[128];
    int  len = 0, lines = 0;
    const char* p = text;

    auto flush = [&]() {
        line[len] = 0;
        if (draw) g.drawString(line, x, y + lines * lineH);
        ++lines;
        len = 0;
    };

    while (*p) {
        while (*p == ' ') ++p;
        if (!*p) break;
        const char* end = p;
        while (*end && *end != ' ') ++end;
        int wLen = (int)(end - p);
        if (wLen > 60) wLen = 60;

        if (len > 0) {
            char trial[128];
            bool fits = (len + 1 + wLen < (int)sizeof(trial));
            if (fits) {
                memcpy(trial, line, len);
                trial[len] = ' ';
                memcpy(trial + len + 1, p, wLen);
                trial[len + 1 + wLen] = 0;
                fits = g.textWidth(trial) <= maxW;
            }
            if (fits) {
                line[len] = ' ';
                memcpy(line + len + 1, p, wLen);
                len += 1 + wLen;
            } else {
                flush();
                memcpy(line, p, wLen);
                len = wLen;
            }
        } else {
            memcpy(line, p, wLen);
            len = wLen;
        }
        p = end;
    }
    if (len > 0) flush();
    return lines;
}

// Question text in the area [y, y+maxH). Uses the normal font, or a smaller
// one if a long question wouldn't fit.
void drawQuestionText(const char* text, int y, int maxH) {
    g.setTextDatum(top_left);
    g.setTextColor(TFT_WHITE);
    g.setFont(&fonts::Font2);
    int lineH = 16;
    if (wrapText(text, 4, y, W - 8, lineH, false) * lineH > maxH) {
        g.setFont(&fonts::Font0);
        lineH = 9;
    }
    wrapText(text, 4, y, W - 8, lineH, true);
}

// One line, shortened with ".." if too wide.
void drawFitted(const char* s, int x, int y, int maxW) {
    if (g.textWidth(s) <= maxW) { g.drawString(s, x, y); return; }
    char buf[96];
    int n = (int)strlen(s);
    if (n > (int)sizeof(buf) - 3) n = sizeof(buf) - 3;
    while (n > 0) {
        memcpy(buf, s, n);
        strcpy(buf + n, "..");
        if (g.textWidth(buf) <= maxW) break;
        --n;
    }
    g.drawString(buf, x, y);
}

// "B) Bethlehem" for test question i, shown position pos (shuffled order).
void choiceLabel(char* out, size_t size, int i, int pos) {
    snprintf(out, size, "%c) %s", LETTERS[pos], game->choice(i, pos));
}

// Small battery gauge in the top-right corner. "+" means charging.
void drawBattery() {
    int level = M5.Power.getBatteryLevel();
    if (level < 0 || level > 100) return;   // not readable
    const int bw = 20, bh = 9;
    const int bx = W - bw - 5, by = 3;
    uint16_t color = level > 50 ? TFT_GREEN : level > 20 ? TFT_YELLOW : TFT_RED;

    g.drawRect(bx, by, bw, bh, DIM);
    g.fillRect(bx + bw, by + 3, 2, 3, DIM);                 // terminal nub
    int fill = (bw - 4) * level / 100;
    if (fill > 0) g.fillRect(bx + 2, by + 2, fill, bh - 4, color);

    char buf[8];
    snprintf(buf, sizeof(buf), "%s%d%%", M5.Power.isCharging() ? "+" : "", level);
    g.setFont(&fonts::Font0);
    g.setTextColor(DIM);
    g.setTextDatum(middle_right);
    g.drawString(buf, bx - 3, by + bh / 2 + 1);
}

// ---- Menus ---------------------------------------------------------------

typedef void (*HeaderFn)();

// Vertical list of options under a header. Side = next, front = select.
// Returns the selected index.
int runMenu(HeaderFn header, const char* const* opts, int n, int top, int rowH, int sel) {
    for (;;) {
        g.fillScreen(BG);
        header();
        drawBattery();

        g.setFont(&fonts::Font2);
        g.setTextDatum(middle_center);
        for (int i = 0; i < n; ++i) {
            int y = top + i * rowH;
            if (i == sel) {
                g.fillRoundRect(20, y, W - 40, rowH - 3, 6, HILITE);
                g.setTextColor(TFT_BLACK);
            } else {
                g.drawRoundRect(20, y, W - 40, rowH - 3, 6, DIM);
                g.setTextColor(TFT_WHITE);
            }
            g.drawString(opts[i], W / 2, y + (rowH - 3) / 2);
        }
        g.setFont(&fonts::Font0);
        g.setTextColor(DIM);
        g.drawString("Side: next    Front: select", W / 2, H - 6);
        g.pushSprite(0, 0);

        if (waitButton() == Btn::B) sel = (sel + 1) % n;
        else return sel;
    }
}

void drawTitleHeader() {
    g.setTextDatum(middle_center);
    g.setTextColor(HILITE);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.drawString("Bible Trivia", W / 2, 24);
}

// Returns when the player chooses Start.
void mainMenu() {
    int sel = 0;
    for (;;) {
        const char* opts[3] = {
            "Start",
            isSoundEnabled() ? "Sound: ON" : "Sound: OFF",
            "Power off",
        };
        sel = runMenu(drawTitleHeader, opts, 3, 44, 26, sel);
        if (sel == 0) return;
        if (sel == 1) {
            setSoundEnabled(!isSoundEnabled());
            prefs.putBool("sound", isSoundEnabled());   // remembered after power-off
            playSound(SoundId::Click);                  // audible only when turned on
        } else {
            powerOffNow();
        }
    }
}

int g_menuScore = 0;
void drawScoreHeader() {
    g.setTextDatum(middle_center);
    g.setFont(&fonts::Font2);
    g.setTextColor(DIM);
    char buf[24];
    snprintf(buf, sizeof(buf), "Score: %d / %d", g_menuScore, TriviaGame::QUESTIONS_PER_TEST);
    g.drawString(buf, W / 2, 10);
}

// Returns 0 = retake, 1 = new test, 2 = main menu, 3 = power off.
int endMenu() {
    static const char* const OPTIONS[4] = {
        "Retake this test", "New test", "Main menu", "Power off",
    };
    g_menuScore = game->score();
    return runMenu(drawScoreHeader, OPTIONS, 4, 22, 26, 0);
}

// ---- Power ---------------------------------------------------------------

void powerOffNow() {
    M5.Display.setBrightness(normalBrightness);
    g.fillScreen(BG);
    g.setTextDatum(middle_center);
    g.setTextColor(HILITE);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.drawString("Goodbye!", W / 2, H / 2 - 10);
    g.setFont(&fonts::Font0);
    g.setTextColor(DIM);
    g.drawString("Press the side button to turn on", W / 2, H / 2 + 20);
    g.pushSprite(0, 0);
    delay(1200);

    M5.Power.powerOff();   // normally does not return

    // Fallback if it did return: wake the screen back up.
    M5.Display.wakeup();
    M5.Display.setBrightness(normalBrightness);
}

// ---- Game screens --------------------------------------------------------

void drawQuestion(int sel) {
    const Question& q = game->currentQuestion();
    const int idx = game->currentIndex();
    g.fillScreen(BG);

    char hdr[24];
    snprintf(hdr, sizeof(hdr), "Question %d of %d", idx + 1, game->totalQuestions());
    g.setFont(&fonts::Font0);
    g.setTextColor(DIM);
    g.setTextDatum(top_left);
    g.drawString(hdr, 4, 2);
    g.setTextDatum(top_right);
    g.drawString(q.testament == Testament::Old ? "Old Testament" : "New Testament",
                 W - 4, 2);

    drawQuestionText(q.text, 13, 48);

    g.setFont(&fonts::Font2);
    g.setTextDatum(top_left);
    for (int pos = 0; pos < 4; ++pos) {
        int y = 63 + pos * 18;
        if (pos == sel) {
            g.fillRoundRect(2, y, W - 4, 18, 4, HILITE);
            g.setTextColor(TFT_BLACK);
        } else {
            g.setTextColor(TFT_WHITE);
        }
        char label[64];
        choiceLabel(label, sizeof(label), idx, pos);
        drawFitted(label, 8, y + 1, W - 16);
    }
    g.pushSprite(0, 0);
}

void playTest() {
    while (!game->isFinished()) {
        int sel = 0;
        for (;;) {
            drawQuestion(sel);
            if (waitButton() == Btn::B) {
                sel = (sel + 1) % 4;
            } else {
                game->answer((uint8_t)sel);   // click only; result is revealed later
                break;
            }
        }
    }
}

void revealQuestion(int i) {
    const Question& q = game->testQuestion(i);
    bool right = game->isCorrect(i);

    g.fillScreen(BG);
    char hdr[24];
    snprintf(hdr, sizeof(hdr), "Result %d of %d", i + 1, game->totalQuestions());
    g.setFont(&fonts::Font0);
    g.setTextColor(DIM);
    g.setTextDatum(top_left);
    g.drawString(hdr, 4, 2);

    drawQuestionText(q.text, 13, 48);

    g.setTextDatum(top_left);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(right ? TFT_GREEN : TFT_RED);
    g.drawString(right ? "Correct!" : "Wrong", 4, 64);

    g.setFont(&fonts::Font2);
    char label[64], line[80];
    if (right) {
        choiceLabel(label, sizeof(label), i, game->correctChoice(i));
        g.setTextColor(TFT_GREEN);
        drawFitted(label, 4, 96, W - 8);
    } else {
        choiceLabel(label, sizeof(label), i, game->givenAnswer(i));
        snprintf(line, sizeof(line), "You: %s", label);
        g.setTextColor(DIM);
        drawFitted(line, 4, 94, W - 8);

        choiceLabel(label, sizeof(label), i, game->correctChoice(i));
        snprintf(line, sizeof(line), "Answer: %s", label);
        g.setTextColor(HILITE);
        drawFitted(line, 4, 113, W - 8);
    }
    g.pushSprite(0, 0);

    playSound(right ? SoundId::Correct : SoundId::Wrong);
    waitOrSkip(right ? CORRECT_PAUSE_MS : WRONG_PAUSE_MS);
}

void showScore() {
    int s = game->score();
    int total = game->totalQuestions();

    g.fillScreen(BG);
    g.setTextDatum(middle_center);
    g.setFont(&fonts::Font2);
    g.setTextColor(DIM);
    g.drawString("YOUR SCORE", W / 2, 12);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d / %d", s, total);
    g.setFont(&fonts::FreeSansBold24pt7b);
    g.setTextColor(s == total ? HILITE : TFT_WHITE);
    g.drawString(buf, W / 2, 50);

    // One dot per question: green right, red wrong.
    const int spacing = 28;
    int x0 = W / 2 - (total - 1) * spacing / 2;
    for (int i = 0; i < total; ++i) {
        g.fillCircle(x0 + i * spacing, 92, 8, game->isCorrect(i) ? TFT_GREEN : TFT_RED);
    }

    g.setFont(&fonts::Font2);
    g.setTextColor(TFT_WHITE);
    const char* msg = (s == total) ? "Perfect score!" : (s <= 1) ? "Keep studying!" : "Well done!";
    g.drawString(msg, W / 2, 120);
    g.pushSprite(0, 0);

    if (s == total)  playSound(SoundId::Perfect);
    else if (s <= 1) playSound(SoundId::LowScore);
    else             playSound(SoundId::Results);
    waitOrSkip(1200);
}

}  // namespace

// ---- Entry points (called from trivia_stick.ino) -------------------------

void appSetup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);       // landscape
    M5.Speaker.setVolume(160);       // 0-255
    normalBrightness = M5.Display.getBrightness();
    if (normalBrightness < 32) normalBrightness = 128;
    W = M5.Display.width();
    H = M5.Display.height();
    g.createSprite(W, H);
    Serial.begin(115200);

    prefs.begin("trivia", false);
    setSoundEnabled(prefs.getBool("sound", true));

    static TriviaGame instance(esp_random());   // hardware random seed
    game = &instance;
}

void appLoop() {
    mainMenu();
    game->newTest();
    for (;;) {
        playTest();
        for (int i = 0; i < game->totalQuestions(); ++i) revealQuestion(i);
        showScore();

        int choice = endMenu();
        if (choice == 0)      game->retakeTest();
        else if (choice == 1) game->newTest();
        else if (choice == 2) return;          // back to the main menu
        else { powerOffNow(); return; }
    }
}
