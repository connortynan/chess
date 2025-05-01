#pragma once

#ifdef DEBUG
#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>

namespace chess
{
    inline const char piece_char[2][6] = {
        {'P', 'N', 'B', 'R', 'Q', 'K'},
        {'p', 'n', 'b', 'r', 'q', 'k'}};

    inline std::ostream &debug_print(std::ostream &os, const Position &pos)
    {
        for (int rank = 7; rank >= 0; --rank)
        {
            os << rank + 1 << " ";
            for (int file = 0; file < 8; ++file)
            {
                u8 sq = rank * 8 + file;
                bool found = false;

                for (u8 c = 0; c < 2 && !found; ++c)
                {
                    for (u8 pt = 0; pt < 6; ++pt)
                    {
                        if (pos.pieces[c][pt] & (1ULL << sq))
                        {
                            os << piece_char[c][pt] << " ";
                            found = true;
                            break;
                        }
                    }
                }

                if (!found)
                    os << ". ";
            }
            os << '\n';
        }

        os << "  a b c d e f g h\n";

        os << "Turn: " << (pos.turn() == Color::WHITE ? "White" : "Black") << "\n";
        os << "Castling: ";
        if (pos.castling_rights == 0)
            os << "-";
        else
        {
            if (pos.castling_rights & 0b0001)
                os << "K";
            if (pos.castling_rights & 0b0010)
                os << "Q";
            if (pos.castling_rights & 0b0100)
                os << "k";
            if (pos.castling_rights & 0b1000)
                os << "q";
        }
        os << "\n";

        os << "En passant: ";
        if (pos.en_passant_square == -1)
            os << "-";
        else
        {
            char file = 'a' + (pos.en_passant_square % 8);
            char rank = '1' + (pos.en_passant_square / 8);
            os << file << rank;
        }
        os << "\n";

        os << "Halfmove clock: " << pos.halfmove_clock << "\n";
        os << "Fullmove number: " << pos.fullmove_number() << "\n";
        os << "Hash: 0x" << std::hex << pos.hash() << "\n";
        os << "FEN: " << pos.to_fen() << "\n";

        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const Position &pos)
    {
        return debug_print(os, pos);
    }

    inline u64 perft(Position &pos, int depth)
    {
        if (depth == 0)
            return 1;

        Move moves[256];
        size_t move_count = get_moves(pos, moves);

        u64 nodes = 0;

        for (size_t i = 0; i < move_count; ++i)
        {
            UndoState undo;
            pos.make_move(moves[i], undo, /*recompute=*/false); // Fast make_move without recomputing attacks
            pos.compute_occupancy();                            // Just update occupancy (enough for legal move generation)

            nodes += perft(pos, depth - 1);

            pos.undo_move(undo, /*recompute=*/false);
            pos.compute_occupancy(); // Restore occupancy
        }

        return nodes;
    }

}
#endif
