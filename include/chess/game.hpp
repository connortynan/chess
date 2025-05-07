#pragma once

#include <vector>
#include <unordered_map>

#include "chess/position.hpp"

namespace chess
{
    struct Game
    {
        Position position;
        std::vector<UndoState> history;
        std::vector<Move> moves;
        std::unordered_map<u64, int> seen_positions; // for 3-fold repetition

        Game(const std::string &starting_fen = default_fen)
        {
            position.from_fen(starting_fen);
            seen_positions[position.hash()] = 1;
        }

        static Game import_pgn(const std::string &pgn);
        std::string export_pgn() const;

        std::size_t get_moves(Move *moves) const;
        void make_move(Move m);
        void undo_move();
        void reset();

        bool is_draw() const;
    };

} // namespace chess