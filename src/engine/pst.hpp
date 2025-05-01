#pragma once

#include <cstdint>
#include "chess/position.hpp"

namespace chess::engine
{
    enum Phase
    {
        MIDGAME = 0,
        ENDGAME = 1,
    };

    // Mirror square for black pieces
    inline constexpr u8 mirror_square(Color color, u8 sq)
    {
        // (56 ^ (sq & 56)) | (sq & 7) is the mirror square for white pieces
        // (56 = 0b00111000, which isolates rank bits)
        return color == Color::WHITE ? sq : (56 ^ (sq & 56)) | (sq & 7);
    }

#define ENGINE_PST_PAWN_MID               \
    {                                     \
        0, 0, 0, 0, 0, 0, 0, 0,           \
        10, 10, 10, -10, -10, 10, 10, 10, \
        5, 5, 10, 15, 15, 10, 5, 5,       \
        2, 2, 5, 10, 10, 5, 2, 2,         \
        1, 1, 2, 5, 5, 2, 1, 1,           \
        0, 0, 0, 0, 0, 0, 0, 0,           \
        0, 0, 0, -10, -10, 0, 0, 0,       \
        0, 0, 0, 0, 0, 0, 0, 0}

#define ENGINE_PST_PAWN_END             \
    {                                   \
        0, 0, 0, 0, 0, 0, 0, 0,         \
        10, 10, 10, 15, 15, 10, 10, 10, \
        10, 10, 15, 20, 20, 15, 10, 10, \
        15, 15, 20, 30, 30, 20, 15, 15, \
        20, 20, 30, 40, 40, 30, 20, 20, \
        30, 30, 40, 50, 50, 40, 30, 30, \
        50, 50, 60, 70, 70, 60, 50, 50, \
        0, 0, 0, 0, 0, 0, 0, 0}

    // knight and bishop mid vs endgame don't differ much
#define ENGINE_PST_KNIGHT                       \
    {                                           \
        -50, -40, -30, -30, -30, -30, -40, -50, \
        -40, -20, 0, 0, 0, 0, -20, -40,         \
        -30, 0, 10, 15, 15, 10, 0, -30,         \
        -30, 5, 15, 20, 20, 15, 5, -30,         \
        -30, 0, 15, 20, 20, 15, 0, -30,         \
        -30, 5, 10, 15, 15, 10, 5, -30,         \
        -40, -20, 0, 5, 5, 0, -20, -40,         \
        -50, -40, -30, -30, -30, -30, -40, -50}

#define ENGINE_PST_BISHOP                       \
    {                                           \
        -20, -10, -10, -10, -10, -10, -10, -20, \
        -10, 0, 0, 0, 0, 0, 0, -10,             \
        -10, 0, 5, 10, 10, 5, 0, -10,           \
        -10, 5, 5, 10, 10, 5, 5, -10,           \
        -10, 0, 10, 10, 10, 10, 0, -10,         \
        -10, 10, 10, 10, 10, 10, 10, -10,       \
        -10, 5, 0, 0, 0, 0, 5, -10,             \
        -20, -10, -10, -10, -10, -10, -10, -20}

#define ENGINE_PST_ROOK_MID           \
    {                                 \
        0, 0, 5, 10, 10, 5, 0, 0,     \
        -5, 0, 0, 0, 0, 0, 0, -5,     \
        -5, 0, 0, 0, 0, 0, 0, -5,     \
        -5, 0, 0, 0, 0, 0, 0, -5,     \
        -5, 0, 0, 0, 0, 0, 0, -5,     \
        -5, 0, 0, 0, 0, 0, 0, -5,     \
        5, 10, 10, 10, 10, 10, 10, 5, \
        0, 0, 0, 0, 0, 0, 0, 0}

#define ENGINE_PST_ROOK_END         \
    {                               \
        0, 0, 0, 5, 5, 0, 0, 0,     \
        0, 0, 0, 10, 10, 0, 0, 0,   \
        0, 0, 0, 15, 15, 0, 0, 0,   \
        5, 5, 10, 20, 20, 10, 5, 5, \
        5, 5, 10, 20, 20, 10, 5, 5, \
        0, 5, 10, 15, 15, 10, 5, 0, \
        0, 0, 5, 10, 10, 5, 0, 0,   \
        0, 0, 5, 5, 5, 5, 0, 0}

#define ENGINE_PST_QUEEN_MID                  \
    {                                         \
        -20, -10, -10, -5, -5, -10, -10, -20, \
        -10, 0, 0, 0, 0, 0, 0, -10,           \
        -10, 0, 5, 5, 5, 5, 0, -10,           \
        -5, 0, 5, 5, 5, 5, 0, -5,             \
        0, 0, 5, 5, 5, 5, 0, -5,              \
        -10, 5, 5, 5, 5, 5, 0, -10,           \
        -10, 0, 5, 0, 0, 0, 0, -10,           \
        -20, -10, -10, -5, -5, -10, -10, -20}

#define ENGINE_PST_QUEEN_END              \
    {                                     \
        -10, -5, -5, -5, -5, -5, -5, -10, \
        -5, 0, 0, 0, 0, 0, 0, -5,         \
        -5, 0, 5, 5, 5, 5, 0, -5,         \
        -5, 0, 5, 10, 10, 5, 0, -5,       \
        -5, 0, 5, 10, 10, 5, 0, -5,       \
        -5, 0, 5, 5, 5, 5, 0, -5,         \
        -5, 0, 0, 0, 0, 0, 0, -5,         \
        -10, -5, -5, -5, -5, -5, -5, -10}

#define ENGINE_PST_KING_MID                     \
    {                                           \
        -30, -40, -40, -50, -50, -40, -40, -30, \
        -30, -40, -40, -50, -50, -40, -40, -30, \
        -30, -40, -40, -50, -50, -40, -40, -30, \
        -30, -40, -40, -50, -50, -40, -40, -30, \
        -20, -30, -30, -40, -40, -30, -30, -20, \
        -10, -20, -20, -20, -20, -20, -20, -10, \
        20, 20, 0, 0, 0, 0, 20, 20,             \
        20, 30, 10, 0, 0, 10, 30, 20}

#define ENGINE_PST_KING_END                     \
    {                                           \
        -50, -40, -30, -20, -20, -30, -40, -50, \
        -30, -20, -10, 0, 0, -10, -20, -30,     \
        -30, -10, 20, 30, 30, 20, -10, -30,     \
        -30, -10, 30, 40, 40, 30, -10, -30,     \
        -30, -10, 30, 40, 40, 30, -10, -30,     \
        -30, -10, 20, 30, 30, 20, -10, -30,     \
        -30, -30, 0, 0, 0, 0, -30, -30,         \
        -50, -30, -30, -30, -30, -30, -30, -50}

    // PST lookup: pst[phase][piece_type][square]
    inline constexpr int PST[2][6][64] = {
        {
            ENGINE_PST_PAWN_MID,
            ENGINE_PST_KNIGHT,
            ENGINE_PST_BISHOP,
            ENGINE_PST_ROOK_MID,
            ENGINE_PST_QUEEN_MID,
            ENGINE_PST_KING_MID,
        },
        {
            ENGINE_PST_PAWN_END,
            ENGINE_PST_KNIGHT,
            ENGINE_PST_BISHOP,
            ENGINE_PST_ROOK_END,
            ENGINE_PST_QUEEN_END,
            ENGINE_PST_KING_END,
        }};

#undef ENGINE_PST_PAWN_MID
#undef ENGINE_PST_PAWN_END
#undef ENGINE_PST_KNIGHT
#undef ENGINE_PST_BISHOP
#undef ENGINE_PST_ROOK_MID
#undef ENGINE_PST_ROOK_END
#undef ENGINE_PST_QUEEN_MID
#undef ENGINE_PST_QUEEN_END
#undef ENGINE_PST_KING_MID
#undef ENGINE_PST_KING_END

}
