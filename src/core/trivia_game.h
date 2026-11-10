// trivia_game.h - Game logic: picking questions, shuffling choices,
// recording answers, scoring.
//
// Portable: no printing, no keyboard/button reading, no heap allocation,
// no <random> or time(). The front end (console or M5StickS3) supplies a
// random seed and drives the game through this small API:
//
//   game.newTest();                      // pick 5 random questions
//   while (!game.isFinished()) {
//       int i = game.currentIndex();
//       ... show game.currentQuestion().text and game.choice(i, 0..3) ...
//       game.answer(choice);             // 0-3 = position shown on screen
//   }
//   game.score();                        // shown only at the end
//   game.retakeTest();  or  game.newTest();
//
// Answer choices are shuffled per test. Always display them with
// choice(i, pos) and check them with correctChoice(i), never with the raw
// Question::choices / Question::correct (those are the unshuffled order).
#pragma once
#include <stdint.h>
#include "question.h"

class TriviaGame {
public:
    static const int QUESTIONS_PER_TEST = 5;

    explicit TriviaGame(uint32_t seed);

    void newTest();      // choose 5 new random questions, shuffle choices
    void retakeTest();   // same 5 questions in the same order, choices reshuffled

    bool            isFinished() const;
    int             currentIndex() const;   // 0-based position in the test
    const Question& currentQuestion() const;
    void            answer(uint8_t choice); // records shown position 0-3, advances

    int score() const;                       // correct answers so far
    int totalQuestions() const { return QUESTIONS_PER_TEST; }

    // Per test question i (0-4):
    const Question& testQuestion(int i) const;
    const char*     choice(int i, int pos) const;  // choice text at shown position
    uint8_t         correctChoice(int i) const;    // shown position of the right answer
    uint8_t         givenAnswer(int i) const;      // shown position the player picked
    bool            isCorrect(int i) const;

private:
    uint32_t nextRandom();             // xorshift32 PRNG
    uint32_t randomBelow(uint32_t n);  // 0 .. n-1
    void     shuffleChoices();

    uint32_t rngState_;
    int      picked_[QUESTIONS_PER_TEST];      // indexes into QUESTION_POOL
    uint8_t  order_[QUESTIONS_PER_TEST][4];    // shown position -> original choice
    uint8_t  answers_[QUESTIONS_PER_TEST];
    int      current_;
};
