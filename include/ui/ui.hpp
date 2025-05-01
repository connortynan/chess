#pragma once

#include "chess/game.hpp"

namespace ui
{
    enum class HighlightTint : int
    {
        NORMAL,
        RED,
        GREEN,
        YELLOW,
    };

    // Initializes ui mode, sets up the screen,
    // can pass in game to have access, or it will make it's own if nullptr
    void init(chess::Game *game = nullptr);

    void cleanup();

    // Update the UI state to the position
    void update();

    // Selects a square on the board to tint
    void highlight_square(int row, int col, HighlightTint tint);

    // Updates the board and sidebar window with the current game state
    void display();

    // Waits for a single key and handles it
    void wait_for_input();

    bool is_running();
}
