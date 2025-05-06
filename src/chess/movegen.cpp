#include "chess/position.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

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

    struct Magic
    {
        u64 mask;
        u64 magic;
        int shift;
        u16 *pool_indexes;
    };

    // Magic rook_magics[64];
    // Magic bishop_magics[64];
#include "magic.inc"

    u64 attack_pool[] = // holds all the unique attacks for the rook and bishop magics
#include "magic_attacks.inc"
        ;

    u64 ortho_attacks(int sq, u64 occupancy)
    {
        u64 blockers = occupancy & rook_magics[sq].mask;
        u64 index = (blockers * rook_magics[sq].magic) >> rook_magics[sq].shift;
        return attack_pool[rook_magics[sq].pool_indexes[index]];
    }

    u64 diag_attacks(int sq, u64 occupancy)
    {
        u64 blockers = occupancy & bishop_magics[sq].mask;
        u64 index = (blockers * bishop_magics[sq].magic) >> bishop_magics[sq].shift;
        return attack_pool[bishop_magics[sq].pool_indexes[index]];
    }
}

namespace chess
{
    static void init_attacks()
    {
        for (int sq = 0; sq < 64; ++sq)
        {
            knight_attacks[sq] = generate_knight_attacks(sq);
            king_attacks[sq] = generate_king_attacks(sq);
            pawn_attacks[(int)Color::WHITE][sq] = generate_pawn_attacks(Color::WHITE, sq);
            pawn_attacks[(int)Color::BLACK][sq] = generate_pawn_attacks(Color::BLACK, sq);
        }
    }

    static struct AttackInit
    {
        AttackInit() { init_attacks(); }
    } _attack_init; // to ensure attacks are initialized

    u64 Position::attacked_squares(Color us, u64 mask) const
    {
        u64 result = mask;

        u8 them = 1 ^ (u8)us;
        while (mask)
        {
            u8 sq = __builtin_ctzll(mask);
            mask &= mask - 1;

            if (pawn_attacks[(int)us][sq] & pieces[them][(int)PieceType::PAWN])
                continue;
            if (knight_attacks[sq] & pieces[them][(int)PieceType::KNIGHT])
                continue;
            if (king_attacks[sq] & pieces[them][(int)PieceType::KING])
                continue;
            u64 diag = diag_attacks(sq, all_occupancy);
            if (diag & (pieces[them][(int)PieceType::BISHOP] | pieces[them][(int)PieceType::QUEEN]))
                continue;
            u64 ortho = ortho_attacks(sq, all_occupancy);
            if (ortho & (pieces[them][(int)PieceType::ROOK] | pieces[them][(int)PieceType::QUEEN]))
                continue;

            // if we reach here, the square is not attacked by any of the opponent's pieces
            result &= ~(1ULL << sq);
        }
        return result;
    }

    bool Position::square_attacked(Color us, u8 sq) const
    {
        u8 them = 1 ^ (u8)us;

        if (pawn_attacks[(int)us][sq] & pieces[them][(int)PieceType::PAWN])
            return true;
        if (knight_attacks[sq] & pieces[them][(int)PieceType::KNIGHT])
            return true;
        if (king_attacks[sq] & pieces[them][(int)PieceType::KING])
            return true;

        u64 diag = diag_attacks(sq, all_occupancy);
        if (diag & (pieces[them][(int)PieceType::BISHOP] | pieces[them][(int)PieceType::QUEEN]))
            return true;

        u64 ortho = ortho_attacks(sq, all_occupancy);
        if (ortho & (pieces[them][(int)PieceType::ROOK] | pieces[them][(int)PieceType::QUEEN]))
            return true;

        return false;
    }

    bool Position::king_checked(Color us) const
    {
        assert(__builtin_popcountll(pieces[(u8)us][(u8)PieceType::KING]) == 1);
        u8 king_sq = __builtin_ctzll(pieces[(u8)us][(u8)PieceType::KING]);

        return square_attacked(us, king_sq);
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

        u64 mask = ~0ULL; // mask out the peices that have been captured at each step in cases of the checking piece being captured
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
                    && !(pos.attacked_squares(them, (1ULL << (from)) | (1ULL << (from + 1)) | (1ULL << (from + 2)))))
                {
                    add(move::make(from, from + 2, move::flags::KING_CASTLE), PieceType::KING);
                }
            }

            if (qc)
            {
                // Squares between king and rook must be empty
                if (!(pos.all_occupancy & ((1ULL << (from - 1)) | (1ULL << (from - 2)) | (1ULL << (from - 3))))
                    // and squares king moves across must not be attacked
                    && !(pos.attacked_squares(them, (1ULL << (from)) | (1ULL << (from - 1)) | (1ULL << (from - 2)))))
                {
                    add(move::make(from, from - 2, move::flags::QUEEN_CASTLE), PieceType::KING);
                }
            }
        }

        return move_count;
    }

    std::string Position::algebraic_notation(const Move &m) const
    {
        if (move::is_castle_kingside(m))
            return "O-O";
        if (move::is_castle_queenside(m))
            return "O-O-O";

        const Color us = turn();
        const u8 from_sq = move::from(m);
        const u8 to_sq = move::to(m);
        const char files[] = "abcdefgh";
        const char ranks[] = "12345678";

        std::ostringstream ss;

        // Identify piece being moved
        PieceType pt = PieceType::PAWN;
        for (int i = 0; i < 6; ++i)
        {
            if (pieces[(u8)us][i] & (1ULL << from_sq))
            {
                pt = (PieceType)i;
                break;
            }
        }

        if (pt != PieceType::PAWN)
            ss << "PNBRQK"[(int)pt];

        u64 attackers = pieces[(u8)us][(u8)pt] & ~(1ULL << from_sq); // all other pieces of the same type
        switch (pt)
        {
        case PieceType::PAWN:
            attackers = 0; // pawns are not ambiguous
            break;
        case PieceType::KNIGHT:
            attackers &= knight_attacks[to_sq];
            break;
        case PieceType::BISHOP:
            attackers &= diag_attacks(to_sq, all_occupancy);
            break;
        case PieceType::ROOK:
            attackers &= ortho_attacks(to_sq, all_occupancy);
            break;
        case PieceType::QUEEN:
            attackers &= diag_attacks(to_sq, all_occupancy) | ortho_attacks(to_sq, all_occupancy);
            break;
        case PieceType::KING:
            attackers &= king_attacks[to_sq];
            break;
        default:
            assert(false);
            return "";
        }

        // If there are no ambiguous moves, we can skip the disambiguation
        if (attackers)
        {
            bool same_file = false;
            bool same_rank = false;
            while (attackers)
            {
                u8 sq = __builtin_ctzll(attackers);
                attackers &= attackers - 1;
                if (sq % 8 == from_sq % 8)
                    same_file = true;
                if (sq / 8 == from_sq / 8)
                    same_rank = true;
                if (same_file && same_rank)
                    break;
            }

            if (!same_file && !same_rank)
                ss << files[from_sq % 8];
            else if (same_file && !same_rank)
                ss << ranks[from_sq / 8];
            else
                ss << files[from_sq % 8] << ranks[from_sq / 8];
        }

        // Captures
        if (move::is_capture(m))
        {
            if (pt == PieceType::PAWN)
                ss << files[from_sq % 8]; // pawn file
            ss << 'x';
        }

        // Destination square
        ss << files[to_sq % 8];
        ss << ranks[to_sq / 8];

        // Promotions
        if (move::is_promotion(m))
            ss << '=' << "PNBRQ"[move::promo_piece_index(m)];

        return ss.str();
    }
} // namespace chess
