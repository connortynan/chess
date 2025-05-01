#include "chess/position.hpp"
#include <sstream>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cassert>

#include "zobrist.hpp"

namespace chess
{
    void Position::from_fen(const std::string &fen)
    {
        std::memset(pieces, 0, sizeof(pieces));
        castling_rights = 0;
        en_passant_square = -1;
        halfmove_clock = 0;
        ply = 0;

        std::istringstream iss(fen);
        std::string board_part, turn_part, castle_part, ep_part;
        int halfmove, fullmove;

        iss >> board_part >> turn_part >> castle_part >> ep_part >> halfmove >> fullmove;

        int sq = 56;
        for (char c : board_part)
        {
            if (c == '/')
                sq -= 16;
            else if (std::isdigit(c))
                sq += c - '0';
            else
            {
                Color color = std::isupper(c) ? Color::WHITE : Color::BLACK;
                PieceType pt;
                switch (std::tolower(c))
                {
                case 'p':
                    pt = PieceType::PAWN;
                    break;
                case 'n':
                    pt = PieceType::KNIGHT;
                    break;
                case 'b':
                    pt = PieceType::BISHOP;
                    break;
                case 'r':
                    pt = PieceType::ROOK;
                    break;
                case 'q':
                    pt = PieceType::QUEEN;
                    break;
                case 'k':
                    pt = PieceType::KING;
                    break;
                default:
                    throw std::invalid_argument("Invalid FEN piece");
                }
                pieces[static_cast<u8>(color)][static_cast<u8>(pt)] |= 1ULL << sq;
                ++sq;
            }
        }

        ply = (turn_part == "w") ? 0 : 1;
        ply += (fullmove - 1) * 2;

        for (char c : castle_part)
        {
            switch (c)
            {
            case 'K':
                castling_rights |= castle_rights::WK;
                break;
            case 'Q':
                castling_rights |= castle_rights::WQ;
                break;
            case 'k':
                castling_rights |= castle_rights::BK;
                break;
            case 'q':
                castling_rights |= castle_rights::BQ;
                break;
            case '-':
                break;
            default:
                throw std::invalid_argument("Invalid FEN castling");
            }
        }

        if (ep_part != "-")
        {
            int file = ep_part[0] - 'a';
            int rank = ep_part[1] - '1';
            en_passant_square = rank * 8 + file;
        }

        halfmove_clock = halfmove;

        compute_occupancy();
        compute_attacks();
    }

    std::string Position::to_fen() const
    {
        std::ostringstream fen;

        for (int rank = 7; rank >= 0; --rank)
        {
            int empty = 0;
            for (int file = 0; file < 8; ++file)
            {
                int sq = rank * 8 + file;
                char piece = 0;
                for (int c = 0; c < 2; ++c)
                {
                    for (int pt = 0; pt < 6; ++pt)
                    {
                        if (pieces[c][pt] & (1ULL << sq))
                        {
                            piece = "PNBRQKpnbrqk"[c * 6 + pt];
                            break;
                        }
                    }
                    if (piece)
                        break;
                }

                if (piece)
                {
                    if (empty)
                    {
                        fen << empty;
                        empty = 0;
                    }
                    fen << piece;
                }
                else
                    ++empty;
            }
            if (empty)
                fen << empty;
            if (rank > 0)
                fen << '/';
        }

        fen << ' ' << (turn() == Color::WHITE ? 'w' : 'b');
        fen << ' ';
        if (castling_rights == 0)
            fen << '-';
        else
        {
            if (castling_rights & castle_rights::WK)
                fen << 'K';
            if (castling_rights & castle_rights::WQ)
                fen << 'Q';
            if (castling_rights & castle_rights::BK)
                fen << 'k';
            if (castling_rights & castle_rights::BQ)
                fen << 'q';
        }

        fen << ' ';
        if (en_passant_square == -1)
            fen << '-';
        else
            fen << static_cast<char>('a' + (en_passant_square % 8))
                << static_cast<char>('1' + (en_passant_square / 8));

        fen << ' ' << halfmove_clock;
        fen << ' ' << fullmove_number();

        return fen.str();
    }

    void Position::compute_occupancy()
    {
        all_occupancy = 0;
        for (int c = 0; c < 2; ++c)
        {
            occupancy[c] = 0;
            for (int pt = 0; pt < 6; ++pt)
                occupancy[c] |= pieces[c][pt];
            all_occupancy |= occupancy[c];
        }
    }

    u64 Position::hash() const
    {
        u64 h = 0;

        // Pieces
        for (int color = 0; color < 2; ++color)
        {
            for (int pt = 0; pt < 6; ++pt)
            {
                u64 bb = pieces[color][pt];
                while (bb)
                {
                    u8 sq = __builtin_ctzll(bb); // index of LS1B
                    h ^= zobrist::pieces[color][pt][sq];
                    bb &= bb - 1; // clear LS1B
                }
            }
        }

        // Castling rights
        h ^= zobrist::castling[castling_rights];

        // En passant (only if there's a pawn that can capture it)
        if (en_passant_square != -1)
        {
            int file = en_passant_square % 8;
            h ^= zobrist::ep[file];
        }

        // Turn
        if (turn() == Color::BLACK)
            h ^= zobrist::turn;

        return h;
    }

    void Position::make_move(const Move &m, bool recompute)
    {
        Color us = turn();              // side to move
        Color them = Color(1 ^ (u8)us); // opponent
        u8 from = move::from(m);
        u8 to = move::to(m);

        // Find the moving piece type
        PieceType moving_type = PieceType::PAWN;
        for (int pt = 0; pt < 6; ++pt)
        {
            if (pieces[(u8)us][pt] & (1ULL << from))
            {
                moving_type = (PieceType)pt;
                pieces[(u8)us][pt] ^= (1ULL << from); // Remove from 'from'
                break;
            }
        }

        // Handle castling
        if (move::is_castle_kingside(m))
        {
            pieces[(u8)us][(u8)PieceType::KING] |= (1ULL << to);
            if (us == Color::WHITE)
            {
                pieces[(u8)us][(u8)PieceType::ROOK] ^= (1ULL << 7) | (1ULL << 5);
            }
            else
            {
                pieces[(u8)us][(u8)PieceType::ROOK] ^= (1ULL << 63) | (1ULL << 61);
            }
        }
        else if (move::is_castle_queenside(m))
        {
            pieces[(u8)us][(u8)PieceType::KING] |= (1ULL << to);
            if (us == Color::WHITE)
            {
                pieces[(u8)us][(u8)PieceType::ROOK] ^= (1ULL << 0) | (1ULL << 3);
            }
            else
            {
                pieces[(u8)us][(u8)PieceType::ROOK] ^= (1ULL << 56) | (1ULL << 59);
            }
        }
        else
        {
            // Handle promotion
            if (move::is_promotion(m))
            {
                PieceType promoted_type = (PieceType)move::promo_piece_index(m);
                pieces[(u8)us][(u8)promoted_type] |= (1ULL << to);
            }
            else
            {
                // Normal move
                pieces[(u8)us][(u8)moving_type] |= (1ULL << to);
            }

            // Handle captures
            if (move::is_capture(m))
            {
                if (move::is_en_passant(m))
                {
                    int ep_square = to + ((us == Color::WHITE) ? -8 : 8);
                    for (int pt = 0; pt < 6; ++pt)
                    {
                        pieces[(u8)them][pt] &= ~(1ULL << ep_square);
                    }
                }
                else
                {
                    for (int pt = 0; pt < 6; ++pt)
                    {
                        pieces[(u8)them][pt] &= ~(1ULL << to);
                    }
                }
            }
        }

        // Update en passant square
        en_passant_square = move::is_double_push(m) ? (from + to) / 2 : -1;

        // Update castling rights
        if (moving_type == PieceType::KING)
        {
            switch (from)
            {
            case 4: // e1 king moves: lose WK and WQ
                castling_rights &= ~(castle_rights::WK | castle_rights::WQ);
                break;
            case 60: // e8 king moves: lose BK and BQ
                castling_rights &= ~(castle_rights::BK | castle_rights::BQ);
                break;
            }
        }

        else if (moving_type == PieceType::ROOK)
        {
            switch (from)
            {
            case 0: // a1 rook moves: lose WQ
                castling_rights &= ~castle_rights::WQ;
                break;
            case 7: // h1 rook moves: lose WK
                castling_rights &= ~castle_rights::WK;
                break;
            case 56: // a8 rook moves: lose BQ
                castling_rights &= ~castle_rights::BQ;
                break;
            case 63: // h8 rook moves: lose BK
                castling_rights &= ~castle_rights::BK;
                break;
            }
        }

        if (move::is_capture(m))
        {
            switch (to)
            {
            case 0:
                castling_rights &= ~castle_rights::WQ;
                break;
            case 7:
                castling_rights &= ~castle_rights::WK;
                break;
            case 56:
                castling_rights &= ~castle_rights::BQ;
                break;
            case 63:
                castling_rights &= ~castle_rights::BK;
                break;
            }
        }

        // Halfmove clock (reset if pawn move or capture, else increment)
        if (moving_type == PieceType::PAWN || move::is_capture(m))
            halfmove_clock = 0;
        else
            halfmove_clock++;

        // Next ply
        ply += 1;

        if (recompute)
        {
            compute_occupancy();
            compute_attacks();
        }
    }

    void Position::make_move(const Move &m, UndoState &undo, bool recompute)
    {
        undo.move = m;
        undo.castling_rights = castling_rights;
        undo.en_passant_square = en_passant_square;
        undo.halfmove_clock = halfmove_clock;

        // Find moved piece type
        undo.moved_type = PieceType::PAWN; // default
        for (int pt = 0; pt < 6; ++pt)
        {
            if (pieces[(u8)turn()][pt] & (1ULL << move::from(m)))
            {
                undo.moved_type = (PieceType)pt;
                break;
            }
        }

        // Find captured piece type, if any
        undo.captured_type = PieceType::PAWN; // default
        if (move::is_capture(m))
        {
            if (move::is_en_passant(m))
            {
                undo.captured_type = PieceType::PAWN;
            }
            else
            {
                int to_sq = move::to(m);
                for (int pt = 0; pt < 6; ++pt)
                {
                    if (pieces[(int)(turn()) ^ 1][pt] & (1ULL << to_sq))
                    {
                        undo.captured_type = (PieceType)pt;
                        break;
                    }
                }
            }
        }

        make_move(m, recompute);
    }

    void Position::undo_move(const UndoState &undo, bool recompute)
    {
        ply -= 1;

        Move m = undo.move;
        u8 us = (u8)turn();
        u8 them = 1 ^ (u8)us;

        halfmove_clock = undo.halfmove_clock;
        castling_rights = undo.castling_rights;
        en_passant_square = undo.en_passant_square;

        u8 from = move::from(m);
        u8 to = move::to(m);

        if (move::is_castle_kingside(m))
        {
            pieces[us][(u8)PieceType::KING] ^= (1ULL << from) | (1ULL << to);
            if ((Color)us == Color::WHITE)
                pieces[us][(u8)PieceType::ROOK] ^= (1ULL << 5) | (1ULL << 7);
            else
                pieces[us][(u8)PieceType::ROOK] ^= (1ULL << 61) | (1ULL << 63);
        }
        else if (move::is_castle_queenside(m))
        {
            pieces[us][(u8)PieceType::KING] ^= (1ULL << from) | (1ULL << to);
            if ((Color)us == Color::WHITE)
                pieces[us][(u8)PieceType::ROOK] ^= (1ULL << 3) | (1ULL << 0);
            else
                pieces[us][(u8)PieceType::ROOK] ^= (1ULL << 59) | (1ULL << 56);
        }

        else
        {
            if (move::is_promotion(m))
            {
                pieces[us][(u8)move::promo_piece_index(m)] ^= (1ULL << to);
                pieces[us][(u8)PieceType::PAWN] |= (1ULL << from);
            }
            else
            {
                pieces[us][(u8)undo.moved_type] ^= (1ULL << to);
                pieces[us][(u8)undo.moved_type] |= (1ULL << from);
            }

            if (move::is_capture(m))
            {
                if (move::is_en_passant(m))
                {
                    int dir = ((Color)us == Color::WHITE) ? -8 : 8;
                    int ep_square = to + dir;
                    pieces[them][(u8)undo.captured_type] |= (1ULL << ep_square);
                }
                else
                {
                    pieces[them][(u8)undo.captured_type] |= (1ULL << to);
                }
            }
        }

        if (recompute)
        {
            compute_occupancy();
            compute_attacks();
        }
    }

    bool Position::validate_occupancy() const
    {
        // only one king per color
        if (__builtin_popcountll(pieces[0][5]) != 1 || __builtin_popcountll(pieces[1][5]) != 1)
        {
            std::cerr << "Invalid number of kings" << std::endl;
            return false;
        }

        u64 white_occ = 0;
        u64 black_occ = 0;

        // Check piece bitboards don't overlap within same color
        for (int color = 0; color < 2; ++color)
        {
            u64 pieces_combined = 0;
            for (int pt = 0; pt < 6; ++pt)
            {
                u64 bb = pieces[color][pt];

                // Overlap within color
                if (pieces_combined & bb)
                {
                    std::cerr << "Overlap in color " << color << " for piece type " << pt << std::endl;
                    return false; // Two pieces of same color overlapping
                }

                pieces_combined |= bb;
            }

            // Match against occupancy
            if (color == 0)
                white_occ = pieces_combined;
            else
                black_occ = pieces_combined;
        }

        // White and Black should not overlap
        if (white_occ & black_occ)
        {
            std::cerr << "Overlap between colors" << std::endl;
            return false; // White and Black have overlapping squares
        }

        // Check that occupancy arrays match
        if (occupancy[0] != white_occ || occupancy[1] != black_occ)
        {
            std::cerr << "Occupancy mismatch" << std::endl;
            return false;
        }

        // Check that all_occupancy is the union
        if (all_occupancy != (white_occ | black_occ))
        {
            std::cerr << "all_occupancy mismatch" << std::endl;
            return false;
        }

        return true;
    }
}
