#include "engine/engine.hpp"

#include <algorithm>

#include "eval.hpp"

namespace chess
{
    namespace engine
    {
        static constexpr int MATE_SCORE = 30000;
        static constexpr int DRAW_SCORE = 0;
        static constexpr int INF = 32000;

        static int score_move(const Position &pos, Move m)
        {
            int score = 0;
            const u8 from_sq = move::from(m);
            const u8 to_sq = move::to(m);

            // Capture bonus (MVV-LVA approximation)
            if (move::is_capture(m))
            {
                for (int pt = 0; pt < 6; ++pt)
                {
                    if (pos.pieces[1 ^ (u8)pos.turn()][pt] & (1ULL << to_sq))
                        score += (pt + 1) * 100; // victim
                    if (pos.pieces[(u8)pos.turn()][pt] & (1ULL << from_sq))
                        score -= (pt + 1) * 10; // attacker
                }
            }

            // Promotion bonus
            if (move::is_promotion(m))
            {
                score += 800;
            }

            // Central pawn push
            const bool is_pawn = pos.pieces[(u8)pos.turn()][0] & (1ULL << from_sq);
            if (is_pawn && (to_sq == 27 || to_sq == 28 || to_sq == 35 || to_sq == 36))
            {
                score += 20; // moving into e4/d4/d5/e5
            }

            return score;
        }

        static int negamax(Game &game, int depth, int alpha, int beta)
        {
            Position &pos = game.position;

            if (depth == 0)
                return (pos.turn() == Color::WHITE) ? eval(pos) : -eval(pos);
            if (game.is_draw())
                return DRAW_SCORE;

            Move moves[256];
            std::size_t n_moves = get_moves(pos, moves);

            if (n_moves == 0)
            {
                if (pos.king_checked(pos.turn()))
                    return -MATE_SCORE - depth;
                else
                    return DRAW_SCORE;
            }

            // Score and sort moves
            std::vector<std::pair<Move, int>> sorted_moves;
            sorted_moves.reserve(n_moves);
            for (std::size_t i = 0; i < n_moves; ++i)
            {
                int score = score_move(pos, moves[i]);
                sorted_moves.emplace_back(moves[i], score);
            }

            std::sort(sorted_moves.begin(), sorted_moves.end(),
                      [](const auto &a, const auto &b)
                      { return a.second > b.second; });

            int max_eval = -INF;

            for (const auto &[move, _] : sorted_moves)
            {
                game.make_move(move);
                int score = -negamax(game, depth - 1, -beta, -alpha);
                game.undo_move();

                if (score > max_eval)
                    max_eval = score;
                if (score > alpha)
                    alpha = score;
                if (alpha >= beta)
                    break; // beta cutoff
            }

            return max_eval;
        }

        Move solve(Game &game, int depth, int *eval_centipawns)
        {
            Move moves[256];
            std::size_t n_moves = get_moves(game.position, moves);

            Move best_move = 0;
            int best_score = -INF;
            int alpha = -INF, beta = INF;

            for (std::size_t i = 0; i < n_moves; ++i)
            {
                game.make_move(moves[i]);
                int score = -negamax(game, depth - 1, -beta, -alpha);
                game.undo_move();

                if (score > best_score)
                {
                    best_score = score;
                    best_move = moves[i];
                }

                if (score > alpha)
                    alpha = score;
            }

            if (eval_centipawns)
                *eval_centipawns = best_score;

            return best_move;
        }

    } // namespace engine

} // namespace chess
