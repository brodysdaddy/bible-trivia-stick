// question.h - Data type for one trivia question.
// Portable: no I/O, no heap allocation. Safe to use on ESP32.
#pragma once
#include <stdint.h>

enum class Testament : uint8_t { Old, New };

struct Question {
    Testament   testament;
    const char* text;
    const char* choices[4];
    uint8_t     correct;   // index 0-3 into choices
};

// Defined in questions.cpp
extern const Question QUESTION_POOL[];
extern const int      QUESTION_POOL_SIZE;
