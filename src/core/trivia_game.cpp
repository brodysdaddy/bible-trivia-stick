// trivia_game.cpp - Game logic implementation (portable, no I/O).
#include "trivia_game.h"

// Requires QUESTION_POOL_SIZE >= QUESTIONS_PER_TEST (5).

TriviaGame::TriviaGame(uint32_t seed)
    : rngState_(seed ? seed : 0x12345678u), current_(0) {
    for (int i = 0; i < QUESTIONS_PER_TEST; ++i) {
        picked_[i] = i;
        answers_[i] = 0;
        for (uint8_t p = 0; p < 4; ++p) order_[i][p] = p;
    }
}

uint32_t TriviaGame::nextRandom() {
    // xorshift32: tiny, fast, identical results on PC and ESP32.
    uint32_t x = rngState_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState_ = x;
    return x;
}

uint32_t TriviaGame::randomBelow(uint32_t n) {
    // Rejection sampling avoids modulo bias.
    uint32_t limit = 0xFFFFFFFFu - (0xFFFFFFFFu % n);
    uint32_t r;
    do { r = nextRandom(); } while (r >= limit);
    return r % n;
}

void TriviaGame::newTest() {
    // Pick 5 distinct random questions (re-roll on duplicates). Needs no
    // pool-sized buffer, so it scales to big pools on the ESP32.
    const int poolSize = QUESTION_POOL_SIZE;

    for (int i = 0; i < QUESTIONS_PER_TEST; ++i) {
        int candidate;
        bool duplicate;
        do {
            candidate = (int)randomBelow((uint32_t)poolSize);
            duplicate = false;
            for (int j = 0; j < i; ++j) {
                if (picked_[j] == candidate) { duplicate = true; break; }
            }
        } while (duplicate);
        picked_[i] = candidate;
    }
    retakeTest();
}

void TriviaGame::shuffleChoices() {
    // Fisher-Yates shuffle of the 4 choices for each question.
    for (int i = 0; i < QUESTIONS_PER_TEST; ++i) {
        for (uint8_t p = 0; p < 4; ++p) order_[i][p] = p;
        for (int p = 3; p > 0; --p) {
            int j = (int)randomBelow((uint32_t)(p + 1));
            uint8_t t = order_[i][p];
            order_[i][p] = order_[i][j];
            order_[i][j] = t;
        }
    }
}

void TriviaGame::retakeTest() {
    // Same questions, but new choice order so a retake isn't just
    // remembering letters.
    shuffleChoices();
    for (int i = 0; i < QUESTIONS_PER_TEST; ++i) answers_[i] = 0xFF;  // unanswered
    current_ = 0;
}

bool TriviaGame::isFinished() const {
    return current_ >= QUESTIONS_PER_TEST;
}

int TriviaGame::currentIndex() const {
    return current_;
}

const Question& TriviaGame::currentQuestion() const {
    int i = current_ < QUESTIONS_PER_TEST ? current_ : QUESTIONS_PER_TEST - 1;
    return QUESTION_POOL[picked_[i]];
}

void TriviaGame::answer(uint8_t choice) {
    if (isFinished() || choice > 3) return;
    answers_[current_] = choice;
    ++current_;
}

int TriviaGame::score() const {
    int correct = 0;
    for (int i = 0; i < QUESTIONS_PER_TEST; ++i) {
        if (isCorrect(i)) ++correct;
    }
    return correct;
}

const Question& TriviaGame::testQuestion(int i) const {
    return QUESTION_POOL[picked_[i]];
}

const char* TriviaGame::choice(int i, int pos) const {
    return testQuestion(i).choices[order_[i][pos & 3]];
}

uint8_t TriviaGame::correctChoice(int i) const {
    const uint8_t original = testQuestion(i).correct;
    for (uint8_t p = 0; p < 4; ++p) {
        if (order_[i][p] == original) return p;
    }
    return 0;  // unreachable: order_ is always a permutation
}

bool TriviaGame::isCorrect(int i) const {
    return answers_[i] == correctChoice(i);
}

uint8_t TriviaGame::givenAnswer(int i) const {
    return answers_[i];
}
