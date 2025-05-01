#include "chess/game.hpp"

#include <cassert>

namespace chess
{
    void Game::make_move(Move m)
    {
        // Save current state for undo
        UndoState undo;

        position.make_move(m, undo);

        // Push undo and move
        history.push_back(undo);
        moves.push_back(m);
    }

    std::size_t Game::get_moves(Move *moves)
    {
        position.compute_occupancy();
        position.compute_attacks();
        return ::chess::get_moves(position, moves);
    }

    void Game::undo_move()
    {
        if (history.empty())
            return;

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
    }
}
