#pragma once

#include "inttypes.hpp"

namespace chess
{
    namespace move
    {
        using Move = u16;

        namespace flags
        {
            using flag_t = u8;

            constexpr flag_t QUIET = 0b0000;
            constexpr flag_t CAPTURE = 0b0001;
            constexpr flag_t DOUBLE_PUSH = 0b0010;
            constexpr flag_t EN_PASSANT = 0b0011;
            constexpr flag_t KING_CASTLE = 0b0100;
            constexpr flag_t QUEEN_CASTLE = 0b0101;

            constexpr flag_t PROMO_MASK = 0b1000;
            constexpr flag_t PROMO_N = 0b1000;
            constexpr flag_t PROMO_B = 0b1010;
            constexpr flag_t PROMO_R = 0b1100;
            constexpr flag_t PROMO_Q = 0b1110;

            inline constexpr flag_t get(move::Move m, flag_t mask = 0xF)
            {
                return (m >> 12) & mask;
            }

            inline constexpr move::Move set(move::Move m, flag_t val, flag_t mask = 0xF)
            {
                move::Move cleared = m & ~(static_cast<move::Move>(mask) << 12);
                move::Move inserted = (static_cast<move::Move>(val & mask) << 12);
                return cleared | inserted;
            }

            inline constexpr void set(move::Move &m, flag_t val, flag_t mask = 0xF)
            {
                m &= ~(static_cast<move::Move>(mask) << 12);
                m |= (static_cast<move::Move>(val & mask) << 12);
            }
        }

        inline constexpr Move make(Move from, Move to, flags::flag_t flags = flags::QUIET)
        {
            return (from << 6) | (to & 0b111111) | (flags << 12);
        }
        inline constexpr u8 to(Move m) { return m & 0b111111; }
        inline constexpr u8 from(Move m) { return (m >> 6) & 0b111111; }

        inline const std::string to_string(Move m)
        {
            u8 from = move::from(m);
            u8 to = move::to(m);
            char ff = 'a' + (from % 8);
            char fr = '1' + (from / 8);
            char tf = 'a' + (to % 8);
            char tr = '1' + (to / 8);
            std::string str = std::string(1, ff) + std::string(1, fr) + std::string(1, tf) + std::string(1, tr);
            return str;
        }

        inline constexpr bool is_capture(move::Move m) { return flags::get(m, flags::CAPTURE); }
        inline constexpr bool is_promotion(move::Move m) { return flags::get(m, flags::PROMO_MASK); }
        inline constexpr bool is_castle_kingside(move::Move m) { return flags::get(m) == flags::KING_CASTLE; }
        inline constexpr bool is_castle_queenside(move::Move m) { return flags::get(m) == flags::QUEEN_CASTLE; }
        inline constexpr bool is_en_passant(move::Move m) { return flags::get(m) == flags::EN_PASSANT; }
        inline constexpr bool is_double_push(move::Move m) { return flags::get(m) == flags::DOUBLE_PUSH; }

        inline constexpr u8 promo_piece_index(Move m) { return 1 + ((flags::get(m) >> 1) & 0b11); }
    }
}
