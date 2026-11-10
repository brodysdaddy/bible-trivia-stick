// main.cpp - Windows/PC console front end.
// All input/output lives here. The M5StickS3 version will replace this file
// with one that draws to the screen and reads the two buttons, while reusing
// everything in src/core unchanged.
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include "../core/trivia_game.h"
#include "../core/sound_player.h"

static const char LETTERS[4] = { 'A', 'B', 'C', 'D' };

// Pause between results lines (after each line's sound finishes).
static const int REVEAL_PAUSE_MS = 500;

static void pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Reads a line and returns the first non-space character, uppercased.
// Returns 0 on end of input (e.g. Ctrl+Z).
static char readChoice() {
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    for (char c : line) {
        // Skip spaces and non-ASCII bytes (e.g. a UTF-8 BOM from piped input).
        if (c != ' ' && c != '\t' && c != '\r' && (unsigned char)c < 0x80) {
            return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
        }
    }
    return ' ';
}

static void showQuestion(const TriviaGame& game) {
    const Question& q = game.currentQuestion();
    std::cout << "\nQuestion " << (game.currentIndex() + 1) << " of "
              << game.totalQuestions()
              << (q.testament == Testament::Old ? "  [Old Testament]"
                                                : "  [New Testament]")
              << "\n" << q.text << "\n";
    for (int pos = 0; pos < 4; ++pos) {
        std::cout << "  " << LETTERS[pos] << ") "
                  << game.choice(game.currentIndex(), pos) << "\n";
    }
}

// Returns false if input ended.
static bool playTest(TriviaGame& game) {
    while (!game.isFinished()) {
        showQuestion(game);
        for (;;) {
            std::cout << "Your answer (A-D): ";
            char c = readChoice();
            if (c == 0) return false;
            if (c >= 'A' && c <= 'D') {
                // Only a neutral click here; right/wrong is revealed on the
                // results screen so the score stays hidden until the end.
                game.answer((uint8_t)(c - 'A'));
                playSound(SoundId::Click);
                break;
            }
            std::cout << "Please type A, B, C, or D.\n";
        }
    }
    return true;
}

static void showResults(const TriviaGame& game) {
    std::cout << "\n========== RESULTS ==========\n" << std::flush;
    pause(REVEAL_PAUSE_MS);

    // Reveal each question one at a time with its sound.
    for (int i = 0; i < game.totalQuestions(); ++i) {
        const Question& q = game.testQuestion(i);
        bool right = game.isCorrect(i);
        std::cout << (right ? "[ok] " : "[x]  ") << q.text << "\n";
        if (!right) {
            uint8_t c = game.correctChoice(i);
            std::cout << "      Correct answer: " << LETTERS[c] << ") "
                      << game.choice(i, c) << "\n";
        }
        std::cout << std::flush;
        playSound(right ? SoundId::Correct : SoundId::Wrong);
        pause(REVEAL_PAUSE_MS);
    }

    // Then the score, with its fanfare.
    int s = game.score();
    std::cout << "\nYou scored " << s << " out of " << game.totalQuestions()
              << "\n=============================\n" << std::flush;
    if (s == game.totalQuestions()) playSound(SoundId::Perfect);
    else if (s <= 1)                playSound(SoundId::LowScore);
    else                            playSound(SoundId::Results);
}

int main() {
    uint32_t seed = (uint32_t)std::chrono::high_resolution_clock::now()
                        .time_since_epoch().count();
    TriviaGame game(seed);

    std::cout << "Bible Trivia\n"
              << QUESTION_POOL_SIZE << " questions in the pool, "
              << game.totalQuestions() << " per test.\n";

    for (;;) {
        std::cout << "Sound on? (Y/N): ";
        char c = readChoice();
        if (c == 0) return 0;
        if (c == 'Y' || c == 'N') { setSoundEnabled(c == 'Y'); break; }
        std::cout << "Please type Y or N.\n";
    }
    playSound(SoundId::Click);

    game.newTest();
    for (;;) {
        if (!playTest(game)) break;
        showResults(game);

        std::cout << "\nR) Retake this test   N) New test   Q) Quit\n";
        char c;
        for (;;) {
            std::cout << "Choose: ";
            c = readChoice();
            if (c == 0 || c == 'R' || c == 'N' || c == 'Q') break;
            std::cout << "Please type R, N, or Q.\n";
        }
        if (c != 0) playSound(SoundId::Click);
        if (c == 'R')      game.retakeTest();
        else if (c == 'N') game.newTest();
        else               break;
    }

    std::cout << "\nThanks for playing!\n";
    return 0;
}
