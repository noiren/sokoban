#ifndef STORY_DATA_H
#define STORY_DATA_H

#include "event_script.h"

// ========================================
// Chapter 1: Introduction
// ========================================
static const EventCommand chapter1_commands[] = {
    // Scene: intro
    { EventCmd::SHOW_LEFT,   0, 0, nullptr },         // Show protagonist
    { EventCmd::TEXT,         0, 0, "..." },
    { EventCmd::TEXT,         0, 0, "Here again..." },
    { EventCmd::TEXT,         0, 0, "The warehouse." },
    { EventCmd::CLEAR_TEXT,   0, 0, nullptr },

    { EventCmd::SHOW_RIGHT,  1, 0, nullptr },          // Show partner
    { EventCmd::TEXT,         0, 0, "Hey! You made it!" },
    { EventCmd::TEXT,         0, 0, "We have boxes to sort." },
    { EventCmd::TEXT,         0, 0, "Ready to work?" },
    { EventCmd::CLEAR_TEXT,   0, 0, nullptr },

    { EventCmd::HIDE_RIGHT,  0, 0, nullptr },
    { EventCmd::TEXT,         0, 0, "Let's do this." },
    { EventCmd::CLEAR_TEXT,   0, 0, nullptr },
    { EventCmd::HIDE_LEFT,   0, 0, nullptr },

    { EventCmd::SET_FLAG,    0, 0, nullptr },           // Flag 0 = saw intro

    // Transition to puzzle
    { EventCmd::GOTO_PUZZLE, 0, 0, nullptr },           // Level 0
};

static const EventScript chapter1_script = {
    chapter1_commands,
    sizeof(chapter1_commands) / sizeof(chapter1_commands[0])
};

// ========================================
// Chapter 1: After Clear
// ========================================
static const EventCommand chapter1_clear_commands[] = {
    { EventCmd::SHOW_LEFT,    0, 0, nullptr },
    { EventCmd::SHOW_RIGHT,   1, 0, nullptr },
    { EventCmd::TEXT,          0, 0, "Nice work!" },
    { EventCmd::TEXT,          0, 0, "That was a good start." },
    { EventCmd::TEXT,          0, 0, "More boxes tomorrow." },
    { EventCmd::CLEAR_TEXT,    0, 0, nullptr },
    { EventCmd::HIDE_LEFT,    0, 0, nullptr },
    { EventCmd::HIDE_RIGHT,   0, 0, nullptr },
    { EventCmd::SET_FLAG,     1, 0, nullptr },           // Flag 1 = cleared ch1
    { EventCmd::END,          0, 0, nullptr },
};

static const EventScript chapter1_clear_script = {
    chapter1_clear_commands,
    sizeof(chapter1_clear_commands) / sizeof(chapter1_clear_commands[0])
};

// Script table
static const EventScript* const story_scripts[] = {
    &chapter1_script,
    &chapter1_clear_script,
};

static const int NUM_STORY_SCRIPTS = sizeof(story_scripts) / sizeof(story_scripts[0]);

#endif // STORY_DATA_H
