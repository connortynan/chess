#include "chess/game.hpp"

#include <cassert>
#include <string>
#include <sstream>
#include <iomanip> // for std::setfill, std::setw
#include <regex>
#include <iostream>

namespace chess
{
    Game Game::import_pgn(const std::string &pgn)
    {
        Game game;

        // Extract movetext (remove headers)
        std::istringstream iss(pgn);
        std::string line;
        std::string moves_str;

        while (std::getline(iss, line))
        {
            if (!line.empty() && line[0] == '[')
                continue; // skip tags
            moves_str += line + " ";
        }

        // Remove comments, numeric annotation glyphs (NAGs), and results
        moves_str = std::regex_replace(moves_str, std::regex("\\{[^}]*\\}"), " "); // remove {...}
        moves_str = std::regex_replace(moves_str, std::regex("\\$\\d+"), " ");     // remove $n
        moves_str = std::regex_replace(moves_str, std::regex("1-0|0-1|1/2-1/2|\\*"), " ");
        moves_str = std::regex_replace(moves_str, std::regex("\\d+\\."), " "); // remove move numbers
        moves_str = std::regex_replace(moves_str, std::regex("\\s+"), " ");    // collapse spaces

        std::istringstream tokens(moves_str);
        std::string move_str;

        while (tokens >> move_str)
        {
            Move legal_moves[256];
            std::size_t count = game.get_moves(legal_moves);

            bool matched = false;
            for (std::size_t i = 0; i < count; ++i)
            {
                const Move m = legal_moves[i];
                if (game.position.algebraic_notation(m) == move_str)
                {
                    game.make_move(m);
                    matched = true;
                    break;
                }
            }

            if (!matched)
            {
                std::cerr << "Failed to parse move: \"" << move_str << "\" at ply " << game.moves.size() << "\n";
                break;
            }
        }

        return game;
    }

    std::string chess::Game::export_pgn() const
    {
        std::ostringstream ss;

        // Metadata with placeholder values
        ss << "[Event \"?\"]\n";
        ss << "[Site \"?, ? ???\"]\n";
        ss << "[Date \"????.??.??\"]\n";
        ss << "[Round \"?\"]\n";
        ss << "[White \"?\"]\n";
        ss << "[Black \"?\"]\n";

        // Determine game result
        std::string result = "*";

        if (is_draw())
        {
            result = "1/2-1/2";
        }
        else
        {
            chess::Move valid_moves[256];
            std::size_t num_moves = get_moves(valid_moves);
            if (num_moves == 0)
            {
                if (position.king_checked(position.turn()))
                    result = (position.turn() == Color::WHITE) ? "0-1" : "1-0";
                else
                    result = "1/2-1/2";
            }
            // else game is still ongoing
        }

        ss << "[Result \"" << result << "\"]\n\n";

        // Movetext section
        chess::Position replay_pos = position;
        replay_pos.from_fen(); // reset to starting position

        int ply = 0;
        int line_length = 0;
        const int max_line_length = 80;

        UndoState undo;
        for (const Move &m : moves)
        {
            std::string move_str;

            if (replay_pos.turn() == Color::WHITE)
            {
                move_str = std::to_string(ply / 2 + 1) + ". ";
            }

            move_str += replay_pos.algebraic_notation(m);

            // Add space after move
            move_str += " ";

            // If current line exceeds max length, break
            if (line_length + move_str.size() > max_line_length)
            {
                ss << "\n";
                line_length = 0;
            }

            ss << move_str;
            line_length += move_str.size();

            // Apply move to position
            replay_pos.make_move(m, undo);
            ply++;
        }

        // Append final result
        if (line_length + result.length() + 1 > max_line_length)
            ss << "\n";
        ss << result << "\n";

        return ss.str();
    }

    void Game::make_move(Move m)
    {
        UndoState undo;
        position.make_move(m, undo);
        history.push_back(undo);
        moves.push_back(m);
        seen_positions[position.hash()]++;
    }

    std::size_t Game::get_moves(Move *moves) const
    {
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
