#include "eval.hpp"
#include <bit>

#include "pst.hpp"

namespace chess
{
    namespace engine
    {
        static constexpr int piece_values[6] = {
            100, 320, 330, 500, 900, 0};

        // Midgame phase weights (used for tapered eval)
        static constexpr int phase_weights[6] = {
            0, 1, 1, 2, 4, 0 // pawn, knight, bishop, rook, queen, king
        };

        static int total_phase(const Position &pos)
        {
            int phase = 0;
            for (int c = 0; c < 2; ++c)
            {
                for (int pt = 0; pt < 6; ++pt)
                {
                    phase += __builtin_popcountll(pos.pieces[c][pt]) * phase_weights[pt];
                }
            }
            return phase;
        }

        static bool is_passed_pawn(const Position &pos, Color us, u8 square)
        {
            u64 forward_mask = (us == Color::WHITE) ? 0xFFFFFFFFFFFFFFFFULL << (square + 8)
                                                    : 0xFFFFFFFFFFFFFFFFULL >> (63 - square + 8);

            u64 file_mask = 0x0101010101010101ULL << (square % 8);
            u64 adjacent = file_mask;
            if (square % 8 > 0)
                adjacent |= file_mask >> 1;
            if (square % 8 < 7)
                adjacent |= file_mask << 1;

            Color them = us == Color::WHITE ? Color::BLACK : Color::WHITE;
            u64 their_pawns = pos.pieces[(int)them][(int)PieceType::PAWN];

            return (their_pawns & adjacent & forward_mask) == 0;
        }

        int eval(const Position &pos)
        {
            int mg_score = 0;
            int eg_score = 0;
            int phase = total_phase(pos);
            constexpr int max_phase = 24; // 2*(1+1+2)+4=12, max phase for both sides

            for (int c = 0; c < 2; ++c)
            {
                Color color = static_cast<Color>(c);
                int sign = (color == Color::WHITE) ? 1 : -1;

                for (int pt = 0; pt < 6; ++pt)
                {
                    PieceType piece = static_cast<PieceType>(pt);
                    u64 bb = pos.pieces[c][pt];

                    while (bb)
                    {
                        u8 square = __builtin_ctzll(bb);
                        bb &= bb - 1;

                        mg_score += sign * piece_values[pt];
                        mg_score += sign * PST[MIDGAME][pt][mirror_square(color, square)];

                        eg_score += sign * piece_values[pt];
                        eg_score += sign * PST[ENDGAME][pt][mirror_square(color, square)];

                        if (piece == PieceType::PAWN && is_passed_pawn(pos, color, square))
                        {
                            mg_score += sign * 20;
                            eg_score += sign * 40;
                        }
                    }
                }
            }

            // Tapered eval: blend midgame/endgame score based on remaining material
            int score = (mg_score * phase + eg_score * (max_phase - phase)) / max_phase;

            return score;
        }
    } // namespace engine
} // namespace chess
