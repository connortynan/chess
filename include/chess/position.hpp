#pragma once

#include <vector>
#include <string>

#include "inttypes.hpp"
#include "move.hpp"

namespace chess
{
    constexpr const char *default_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    using namespace move;

    namespace castle_rights
    {
        constexpr u8 WK = 0b0001;
        constexpr u8 WQ = 0b0010;
        constexpr u8 BK = 0b0100;
        constexpr u8 BQ = 0b1000;
    } // namespace castle_rights

    enum class Color : u8
    {
        WHITE = 0,
        BLACK = 1,
    };

    enum class PieceType : u8
    {
        PAWN = 0,
        KNIGHT = 1,
        BISHOP = 2,
        ROOK = 3,
        QUEEN = 4,
        KING = 5,
    };

    struct UndoState
    {
        Move move;               // The move that was made (plus flags)
        PieceType moved_type;    // The piece that was moved
        PieceType captured_type; // what piece was captured (if any)

        u8 castling_rights;   // old castling rights
        i8 en_passant_square; // old en passant target (-1 if none)
        u32 halfmove_clock;   // old halfmove clock
    };

    struct Position
    {
        u64 pieces[2][6];
        u64 occupancy[2];
        u64 all_occupancy;

        u8 castling_rights;
        i8 en_passant_square = -1;
        u32 halfmove_clock = 0;
        u32 ply = 0;

        void make_move(const Move &move);
        void make_move(const Move &move, UndoState &undo); // creates undo to save state
        void undo_move(const UndoState &undo);
        u64 hash() const;
        std::string to_fen() const;
        void from_fen(const std::string &fen = default_fen);

        inline constexpr u32 fullmove_number() const { return (ply / 2) + 1; }
        inline constexpr Color turn() const { return static_cast<Color>(ply & 1); }
        std::string algebraic_notation(const Move &move) const;

        void compute_occupancy();
        bool validate_occupancy() const;
        bool square_attacked(Color us, u8 square) const;
        u64 attacked_squares(Color us, u64 bb = ~(0ULL)) const;
        bool king_checked(Color us) const;

        inline u64 get_piece_bb(Color c, PieceType pt) const { return pieces[static_cast<u8>(c)][static_cast<u8>(pt)]; }
        inline bool is_occupied(u8 square) const { return all_occupancy & (1ULL << square); }
        inline bool is_occupied(Color c, u8 square) const { return occupancy[static_cast<u8>(c)] & (1ULL << square); }
        inline bool is_occupied(Color c, PieceType pt, u8 square) const { return get_piece_bb(c, pt) & (1ULL << square); }
    };

    std::size_t get_moves(const Position &pos, Move *moves);
} // namespace chess
