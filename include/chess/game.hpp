#pragma once

#include <vector>

#include "chess/position.hpp"

namespace chess
{
    struct Game
    {
        Position position;
        std::vector<UndoState> history;
        std::vector<Move> moves;

        Game(const std::string &starting_fen = default_fen) { position.from_fen(starting_fen); }

        std::size_t get_moves(Move *moves);
        void make_move(Move m);
        void undo_move();
        void reset();
    };

} // namespace chess