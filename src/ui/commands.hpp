#pragma once

#include <ncurses.h>

namespace ui
{
    enum Command
    {
        CMD_NONE,
        CMD_QUIT, // 'Q'
        CMD_HELP, // '?'

        // cursor movement
        CMD_LEFT,  // left arrow, 'h', 'a'
        CMD_RIGHT, // right arrow, 'l', 'd'
        CMD_UP,    // up arrow, 'k', 'w'
        CMD_DOWN,  // down arrow, 'j', 's'

        // selection
        CMD_SELECT,   // space, enter
        CMD_DESELECT, // backspace, delete
        CMD_YES,      // 'y', 'Y'
        CMD_NO,       // 'n', 'N'

        // game commands
        CMD_UNDERPROMOTION, // ctrl+space: ch=0

        // ui commands
        CMD_FLIP_BOARD,  // 'F'
        CMD_UNDO,        // 'u'
        CMD_REDO,        // 'r'
        CMD_TOGGLE_EVAL, // 'E'
    };

    Command get_command(int ch)
    {
        switch (ch)
        {
        case 'Q':
            return CMD_QUIT;
        case '?':
            return CMD_HELP;

        // cursor movement
        case 'h':
        case 'a':
        case KEY_LEFT:
            return CMD_LEFT;
        case 'l':
        case 'd':
        case KEY_RIGHT:
            return CMD_RIGHT;
        case 'k':
        case 'w':
        case KEY_UP:
            return CMD_UP;
        case 'j':
        case 's':
        case KEY_DOWN:
            return CMD_DOWN;

        // selection / confirmation
        case ' ':
        case '\n':
        case '\r':
            return CMD_SELECT;
        case KEY_BACKSPACE:
        case 127:    // ASCII backspace
        case KEY_DC: // Delete key
            return CMD_DESELECT;

        // yes/no prompts
        case 'y':
        case 'Y':
            return CMD_YES;
        case 'n':
        case 'N':
            return CMD_NO;

        // game-specific
        case 0: // ctrl+space (often interpreted as NUL, may be nonportable)
            return CMD_UNDERPROMOTION;

        // ui commands
        case 'F':
            return CMD_FLIP_BOARD;
        case 'u':
            return CMD_UNDO;
        case 'r':
            return CMD_REDO;
        case 'E':
            return CMD_TOGGLE_EVAL;

        default:
            return CMD_NONE;
        }
    }

} // namespace ui
