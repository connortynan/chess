#include "chess/game.hpp"

#include <cassert>

namespace chess
{
    void Game::make_move(Move m)
    {
        UndoState undo;
        position.make_move(m, undo);
        history.push_back(undo);
        moves.push_back(m);
        seen_positions[position.hash()]++;
    }

    std::size_t Game::get_moves(Move *moves)
    {
        position.compute_occupancy();
        return ::chess::get_moves(position, moves);
    }

    void Game::undo_move()
    {
        if (history.empty())
            return;
        seen_positions[position.hash()]--;
        UndoState undo = history.back();
        history.pop_back();
        moves.pop_back();
        position.undo_move(undo);
    }

    void Game::reset()
    {
        // Reset to the starting position
        position.from_fen();
        history.clear();
        moves.clear();
        seen_positions.clear();
        seen_positions[position.hash()] = 1;
    }

    bool Game::is_draw() const
    {
        // 50-move rule
        if (position.halfmove_clock >= 100)
            return true;

        // Threefold repetition
        auto it = seen_positions.find(position.hash());
        if (it != seen_positions.end() && it->second >= 3)
            return true;

        return false;
    }

}
