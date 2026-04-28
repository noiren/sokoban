#ifndef EVENT_SCRIPT_H
#define EVENT_SCRIPT_H

#include <cstdint>

// Event script command types
enum class EventCmd : uint8_t {
    // Text & Dialog
    TEXT,            // Show text in dialog box
    WAIT_INPUT,      // Wait for A button press
    CLEAR_TEXT,      // Clear dialog text

    // Characters
    SHOW_LEFT,       // Show left character (arg = character id)
    SHOW_RIGHT,      // Show right character (arg = character id)
    HIDE_LEFT,       // Hide left character
    HIDE_RIGHT,      // Hide right character

    // Scene
    SET_BG,          // Set background (arg = bg id)
    CLEAR_BG,        // Clear background
    FULL_STILL,      // Show full-screen still image (arg = still id)
    CLEAR_STILL,     // Clear full-screen still

    // Flow
    SET_FLAG,        // Set a knowledge flag (arg = flag id)
    CHECK_FLAG,      // Check flag, skip next N commands if false (arg = flag id, arg2 = skip count)
    GOTO_PUZZLE,     // Transition to puzzle mode (arg = level id)
    END,             // End of script

    // Sound
    PLAY_SE,         // Play sound effect (arg = se id)
};

// A single script command
struct EventCommand {
    EventCmd cmd;
    int16_t  arg1;
    int16_t  arg2;
    const char* text;  // For TEXT command
};

// A complete event script
struct EventScript {
    const EventCommand* commands;
    int num_commands;
};

#endif // EVENT_SCRIPT_H
