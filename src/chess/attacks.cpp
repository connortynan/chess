#include "chess/position.hpp"

#include <cassert>
#include <iostream>

namespace
{
    u64 knight_attacks[64];
    u64 king_attacks[64];
    u64 pawn_attacks[2][64];

    u64 generate_knight_attacks(int sq)
    {
        int r = sq / 8, f = sq % 8;
        u64 attacks = 0;

        auto add = [&](int dr, int df)
        {
            int nr = r + dr, nf = f + df;
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                attacks |= (1ULL << (nr * 8 + nf));
        };

        add(2, 1);
        add(1, 2);
        add(-1, 2);
        add(-2, 1);
        add(-2, -1);
        add(-1, -2);
        add(1, -2);
        add(2, -1);

        return attacks;
    }

    u64 generate_king_attacks(int sq)
    {
        int r = sq / 8, f = sq % 8;
        u64 attacks = 0;

        for (int dr = -1; dr <= 1; ++dr)
        {
            for (int df = -1; df <= 1; ++df)
            {
                if (dr == 0 && df == 0)
                    continue;
                int nr = r + dr, nf = f + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                    attacks |= (1ULL << (nr * 8 + nf));
            }
        }
        return attacks;
    }

    u64 generate_pawn_attacks(chess::Color color, int sq)
    {
        int r = sq / 8, f = sq % 8;
        u64 attacks = 0;
        int dir = (color == chess::Color::WHITE) ? 1 : -1;
        int nr = r + dir;

        if (nr >= 0 && nr < 8)
        {
            if (f > 0)
                attacks |= (1ULL << (nr * 8 + (f - 1)));
            if (f < 7)
                attacks |= (1ULL << (nr * 8 + (f + 1)));
        }

        return attacks;
    }

    u64 diag_attacks(int sq, u64 occupancy)
    {
        int r = sq / 8, f = sq % 8;
        u64 attacks = 0;

        // NE
        for (int nr = r + 1, nf = f + 1; nr < 8 && nf < 8; ++nr, ++nf)
        {
            attacks |= (1ULL << (nr * 8 + nf));
            if (occupancy & (1ULL << (nr * 8 + nf)))
                break;
        }
        // NW
        for (int nr = r + 1, nf = f - 1; nr < 8 && nf >= 0; ++nr, --nf)
        {
            attacks |= (1ULL << (nr * 8 + nf));
            if (occupancy & (1ULL << (nr * 8 + nf)))
                break;
        }
        // SE
        for (int nr = r - 1, nf = f + 1; nr >= 0 && nf < 8; --nr, ++nf)
        {
            attacks |= (1ULL << (nr * 8 + nf));
            if (occupancy & (1ULL << (nr * 8 + nf)))
                break;
        }
        // SW
        for (int nr = r - 1, nf = f - 1; nr >= 0 && nf >= 0; --nr, --nf)
        {
            attacks |= (1ULL << (nr * 8 + nf));
            if (occupancy & (1ULL << (nr * 8 + nf)))
                break;
        }

        return attacks;
    }

    u64 ortho_attacks(int sq, u64 occupancy)
    {
        int r = sq / 8, f = sq % 8;
        u64 attacks = 0;

        // North
        for (int nr = r + 1; nr < 8; ++nr)
        {
            attacks |= (1ULL << (nr * 8 + f));
            if (occupancy & (1ULL << (nr * 8 + f)))
                break;
        }
        // South
        for (int nr = r - 1; nr >= 0; --nr)
        {
            attacks |= (1ULL << (nr * 8 + f));
            if (occupancy & (1ULL << (nr * 8 + f)))
                break;
        }
        // East
        for (int nf = f + 1; nf < 8; ++nf)
        {
            attacks |= (1ULL << (r * 8 + nf));
            if (occupancy & (1ULL << (r * 8 + nf)))
                break;
        }
        // West
        for (int nf = f - 1; nf >= 0; --nf)
        {
            attacks |= (1ULL << (r * 8 + nf));
            if (occupancy & (1ULL << (r * 8 + nf)))
                break;
        }

        return attacks;
    }
}

namespace chess
{
    void init_attacks()
    {
        for (int sq = 0; sq < 64; ++sq)
        {
            knight_attacks[sq] = generate_knight_attacks(sq);
            king_attacks[sq] = generate_king_attacks(sq);
            pawn_attacks[(int)Color::WHITE][sq] = generate_pawn_attacks(Color::WHITE, sq);
            pawn_attacks[(int)Color::BLACK][sq] = generate_pawn_attacks(Color::BLACK, sq);
        }
    }

    void Position::compute_attacks()
    {
        attacks[0] = 0;
        attacks[1] = 0;

        for (int color = 0; color < 2; ++color)
        {
            for (int pt = 0; pt < 6; ++pt)
            {
                u64 bb = pieces[color][pt];
                while (bb)
                {
                    u8 sq = __builtin_ctzll(bb);
                    bb &= bb - 1;

                    switch (static_cast<PieceType>(pt))
                    {
                    case PieceType::PAWN:
                        attacks[color] |= pawn_attacks[color][sq];
                        break;
                    case PieceType::KNIGHT:
                        attacks[color] |= knight_attacks[sq];
                        break;
                    case PieceType::BISHOP:
                        attacks[color] |= diag_attacks(sq, all_occupancy);
                        break;
                    case PieceType::ROOK:
                        attacks[color] |= ortho_attacks(sq, all_occupancy);
                        break;
                    case PieceType::QUEEN:
                        attacks[color] |= diag_attacks(sq, all_occupancy);
                        attacks[color] |= ortho_attacks(sq, all_occupancy);
                        break;
                    case PieceType::KING:
                        attacks[color] |= king_attacks[sq];
                        break;
                    }
                }
            }
        }
    }

    bool Position::king_checked(Color us) const
    {
        u8 them = 1 ^ (u8)us;

        assert(__builtin_popcountll(pieces[(u8)us][(u8)PieceType::KING]) == 1);
        u8 king_sq = __builtin_ctzll(pieces[(u8)us][(u8)PieceType::KING]);

        if (pawn_attacks[(int)us][king_sq] & pieces[them][(int)PieceType::PAWN])
            return true;
        if (knight_attacks[king_sq] & pieces[them][(int)PieceType::KNIGHT])
            return true;
        if (king_attacks[king_sq] & pieces[them][(int)PieceType::KING])
            return true;

        u64 diag = diag_attacks(king_sq, all_occupancy);
        if (diag & (pieces[them][(int)PieceType::BISHOP] | pieces[them][(int)PieceType::QUEEN]))
            return true;

        u64 ortho = ortho_attacks(king_sq, all_occupancy);
        if (ortho & (pieces[them][(int)PieceType::ROOK] | pieces[them][(int)PieceType::QUEEN]))
            return true;

        return false;
    }

    static bool is_valid(Move move, const Position &pos, Color us, PieceType piece)
    {
        int them = 1 ^ (int)us;

        u64 new_occ = pos.all_occupancy;
        new_occ ^= (1ULL << move::from(move));
        new_occ |= (1ULL << move::to(move));

        if (is_en_passant(move))
        {
            int dir = (us == Color::WHITE) ? -8 : 8;
            new_occ ^= (1ULL << (move::to(move) + dir));
        }

        int king_sq = (piece == PieceType::KING) ? move::to(move) : __builtin_ctzll(pos.pieces[(u8)us][(u8)PieceType::KING]);

        u64 mask = ~0ULL; // mask out the peices that have been captured at each stetp in cases of the checking piece being captured
        if (move::is_capture(move))
            mask &= ~(1ULL << move::to(move));
        if (move::is_en_passant(move))
        {
            int dir = (us == Color::WHITE) ? -8 : 8;
            mask &= ~(1ULL << (move::to(move) + dir));
        }

        if (pawn_attacks[(int)us][king_sq] & (pos.pieces[them][(int)PieceType::PAWN] & mask))
            return false;
        if (knight_attacks[king_sq] & (pos.pieces[them][(int)PieceType::KNIGHT] & mask))
            return false;
        if (king_attacks[king_sq] & (pos.pieces[them][(int)PieceType::KING] & mask))
            return false;

        u64 diag = diag_attacks(king_sq, new_occ);
        if (diag & (pos.pieces[them][(int)PieceType::BISHOP] & mask))
            return false;
        if (diag & (pos.pieces[them][(int)PieceType::QUEEN] & mask))
            return false;

        u64 ortho = ortho_attacks(king_sq, new_occ);
        if (ortho & (pos.pieces[them][(int)PieceType::ROOK] & mask))
            return false;
        if (ortho & (pos.pieces[them][(int)PieceType::QUEEN] & mask))
            return false;

        return true;
    }

    std::size_t get_moves(const Position &pos, Move *moves)
    {
        assert(moves != nullptr);

        if (!pos.validate_occupancy())
        {
            std::cerr << "Invalid occupancy for fen: \"" << pos.to_fen() << "\"" << std::endl;
            assert(false);
        }

        Color us = pos.turn();
        Color them = (Color)(1 ^ (u8)us);
        std::size_t move_count = 0;

        // Precompute
        u64 own_occ = pos.occupancy[(u8)us];
        u64 enemy_occ = pos.occupancy[(u8)them];
        u64 empty = ~pos.all_occupancy;

        auto add = [&](Move move, PieceType piece)
        {
            if (is_valid(move, pos, us, piece))
                moves[move_count++] = move;
        };

        // --- Pawns ---
        {
            u64 pawns = pos.pieces[(u8)us][(u8)PieceType::PAWN];
            int push_dir = (us == Color::WHITE) ? 8 : -8;
            int start_rank = (us == Color::WHITE) ? 1 : 6;
            int promotion_rank = (us == Color::WHITE) ? 7 : 0;

            while (pawns)
            {
                u8 from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;

                u8 from_rank = from / 8;
                u8 from_file = from % 8;

                u8 to = from + push_dir;
                if (to < 64 && (empty & (1ULL << to)))
                {
                    // Promotion
                    if (to / 8 == promotion_rank)
                    {
                        add(move::make(from, to, move::flags::PROMO_Q), PieceType::PAWN);
                        add(move::make(from, to, move::flags::PROMO_R), PieceType::PAWN);
                        add(move::make(from, to, move::flags::PROMO_B), PieceType::PAWN);
                        add(move::make(from, to, move::flags::PROMO_N), PieceType::PAWN);
                    }
                    else
                    {
                        add(move::make(from, to), PieceType::PAWN);
                        // Double push
                        if (from_rank == start_rank)
                        {
                            u8 to2 = from + 2 * push_dir;
                            if (empty & (1ULL << to2))
                                add(move::make(from, to2, move::flags::DOUBLE_PUSH), PieceType::PAWN);
                        }
                    }
                }

                // Captures
                if (from_file > 0)
                {
                    u8 cap_left = to - 1;
                    if (enemy_occ & (1ULL << cap_left))
                    {
                        if (to / 8 == promotion_rank)
                        {
                            add(move::make(from, cap_left, move::flags::PROMO_Q | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_left, move::flags::PROMO_R | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_left, move::flags::PROMO_B | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_left, move::flags::PROMO_N | move::flags::CAPTURE), PieceType::PAWN);
                        }
                        else
                        {
                            add(move::make(from, cap_left, move::flags::CAPTURE), PieceType::PAWN);
                        }
                    }
                    else if (pos.en_passant_square == cap_left)
                    {
                        add(move::make(from, cap_left, move::flags::EN_PASSANT | move::flags::CAPTURE), PieceType::PAWN);
                    }
                }
                if (from_file < 7)
                {
                    u8 cap_right = to + 1;
                    if (enemy_occ & (1ULL << cap_right))
                    {
                        if (to / 8 == promotion_rank)
                        {
                            add(move::make(from, cap_right, move::flags::PROMO_Q | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_right, move::flags::PROMO_R | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_right, move::flags::PROMO_B | move::flags::CAPTURE), PieceType::PAWN);
                            add(move::make(from, cap_right, move::flags::PROMO_N | move::flags::CAPTURE), PieceType::PAWN);
                        }
                        else
                        {
                            add(move::make(from, cap_right, move::flags::CAPTURE), PieceType::PAWN);
                        }
                    }
                    else if (pos.en_passant_square == cap_right)
                    {
                        add(move::make(from, cap_right, move::flags::EN_PASSANT | move::flags::CAPTURE), PieceType::PAWN);
                    }
                }
            }
        }

        // --- Knights ---
        {
            u64 knights = pos.pieces[(u8)us][(u8)PieceType::KNIGHT];
            while (knights)
            {
                u8 from = __builtin_ctzll(knights);
                knights &= knights - 1;

                u64 targets = knight_attacks[from] & ~own_occ;
                while (targets)
                {
                    u8 to = __builtin_ctzll(targets);
                    targets &= targets - 1;

                    add(move::make(from, to, (enemy_occ & (1ULL << to)) ? move::flags::CAPTURE : move::flags::QUIET), PieceType::KNIGHT);
                }
            }
        }

        // --- Bishops ---
        {
            u64 bishops = pos.pieces[(u8)us][(u8)PieceType::BISHOP];
            while (bishops)
            {
                u8 from = __builtin_ctzll(bishops);
                bishops &= bishops - 1;

                u64 targets = diag_attacks(from, pos.all_occupancy) & ~own_occ;
                while (targets)
                {
                    u8 to = __builtin_ctzll(targets);
                    targets &= targets - 1;

                    add(move::make(from, to, (enemy_occ & (1ULL << to)) ? move::flags::CAPTURE : move::flags::QUIET), PieceType::BISHOP);
                }
            }
        }

        // --- Rooks ---
        {
            u64 rooks = pos.pieces[(u8)us][(u8)PieceType::ROOK];
            while (rooks)
            {
                u8 from = __builtin_ctzll(rooks);
                rooks &= rooks - 1;

                u64 targets = ortho_attacks(from, pos.all_occupancy) & ~own_occ;
                while (targets)
                {
                    u8 to = __builtin_ctzll(targets);
                    targets &= targets - 1;

                    add(move::make(from, to, (enemy_occ & (1ULL << to)) ? move::flags::CAPTURE : move::flags::QUIET), PieceType::ROOK);
                }
            }
        }

        // --- Queens ---
        {
            u64 queens = pos.pieces[(u8)us][(u8)PieceType::QUEEN];
            while (queens)
            {
                u8 from = __builtin_ctzll(queens);
                queens &= queens - 1;

                u64 targets = (diag_attacks(from, pos.all_occupancy) | ortho_attacks(from, pos.all_occupancy)) & ~own_occ;
                while (targets)
                {
                    u8 to = __builtin_ctzll(targets);
                    targets &= targets - 1;

                    add(move::make(from, to, (enemy_occ & (1ULL << to)) ? move::flags::CAPTURE : move::flags::QUIET), PieceType::QUEEN);
                }
            }
        }

        // --- King ---
        {
            u64 kings = pos.pieces[(u8)us][(u8)PieceType::KING];
            assert(__builtin_popcountll(kings) == 1);
            u8 from = __builtin_ctzll(kings);

            u64 targets = king_attacks[from] & ~own_occ;
            while (targets)
            {
                u8 to = __builtin_ctzll(targets);
                targets &= targets - 1;

                add(move::make(from, to, (enemy_occ & (1ULL << to)) ? move::flags::CAPTURE : move::flags::QUIET), PieceType::KING);
            }

            // --- Castling ---
            bool kc = (us == Color::WHITE) ? (pos.castling_rights & castle_rights::WK) : (pos.castling_rights & castle_rights::BK);
            bool qc = (us == Color::WHITE) ? (pos.castling_rights & castle_rights::WQ) : (pos.castling_rights & castle_rights::BQ);

            if (kc)
            {
                // Squares between king and rook must be empty
                if (!(pos.all_occupancy & ((1ULL << (from + 1)) | (1ULL << (from + 2))))
                    // and squares king moves across must not be attacked
                    && !(pos.attacks[(u8)them] & ((1ULL << (from)) | (1ULL << (from + 1)) | (1ULL << (from + 2)))))
                {
                    add(move::make(from, from + 2, move::flags::KING_CASTLE), PieceType::KING);
                }
            }

            if (qc)
            {
                // Squares between king and rook must be empty
                if (!(pos.all_occupancy & ((1ULL << (from - 1)) | (1ULL << (from - 2)) | (1ULL << (from - 3))))
                    // and squares king moves across must not be attacked
                    && !(pos.attacks[(u8)them] & ((1ULL << (from)) | (1ULL << (from - 1)) | (1ULL << (from - 2)))))
                {
                    add(move::make(from, from - 2, move::flags::QUEEN_CASTLE), PieceType::KING);
                }
            }
        }

        return move_count;
    }
} // namespace chess
