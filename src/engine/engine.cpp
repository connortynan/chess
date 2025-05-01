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

        static int negamax(Position &pos, int depth, int alpha, int beta)
        {
            if (depth == 0)
                return eval(pos);

            Move moves[256];
            std::size_t n_moves = get_moves(pos, moves);

            if (n_moves == 0)
            {
                if (pos.king_checked(pos.turn()))
                    return -MATE_SCORE + depth;
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

                UndoState undo;
                pos.make_move(move, undo);

                int score = -negamax(pos, depth - 1, -beta, -alpha);

                pos.undo_move(undo);

                if (score > max_eval)
                    max_eval = score;
                if (score > alpha)
                    alpha = score;
                if (alpha >= beta)
                    break; // beta cutoff
            }

            return max_eval;
        }

        Move solve(const Position &pos_in, int depth, int *eval_cp)
        {
            Position pos = pos_in; // Make a copy for mutation
            Move moves[256];
            std::size_t n_moves = get_moves(pos, moves);

            Move best_move = 0;
            int best_score = -INF;
            int alpha = -INF, beta = INF;

            for (std::size_t i = 0; i < n_moves; ++i)
            {
                UndoState undo;
                pos.make_move(moves[i], undo);

                int score = -negamax(pos, depth - 1, -beta, -alpha);

                pos.undo_move(undo);

                if (score > best_score)
                {
                    best_score = score;
                    best_move = moves[i];
                }

                if (score > alpha)
                    alpha = score;
            }

            if (eval_cp)
                *eval_cp = best_score;

            return best_move;
        }

    } // namespace engine

} // namespace chess
