
// We are trying to generate magic numbers for the rook and bishop attacks.
// This will allow us to use the perfect hash function to generate the attacks for each square.
//   e.g., given rook_magics is a Magic[64] array, we can use the following function to get the rook attacks for a square:
// ```
//     u64 ortho_attacks(int square, u64 occupancy)
//     {
//         u64 blockers = occupancy & rook_magics[square].mask;
//         u64 index = (blockers * rook_magics[square].magic) >> rook_magics[square].shift;
//         return rook_magics[square].attack_table[index];
//     }
// ```

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <string>

using u64 = uint64_t;

struct Magic
{
    u64 mask;
    u64 magic;
    int shift;
    const char *table_name;
};

u64 random_u64()
{
    return ((u64(rand()) & 0xFFFF) << 48) |
           ((u64(rand()) & 0xFFFF) << 32) |
           ((u64(rand()) & 0xFFFF) << 16) |
           ((u64(rand()) & 0xFFFF));
}

u64 rook_mask(int sq)
{
    u64 mask = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i < 7; ++i)
        mask |= (1ULL << (i * 8 + f));
    for (int i = r - 1; i > 0; --i)
        mask |= (1ULL << (i * 8 + f));
    for (int i = f + 1; i < 7; ++i)
        mask |= (1ULL << (r * 8 + i));
    for (int i = f - 1; i > 0; --i)
        mask |= (1ULL << (r * 8 + i));
    return mask;
}

u64 bishop_mask(int sq)
{
    u64 mask = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = 1; r + i < 7 && f + i < 7; ++i)
        mask |= (1ULL << ((r + i) * 8 + (f + i)));
    for (int i = 1; r + i < 7 && f - i > 0; ++i)
        mask |= (1ULL << ((r + i) * 8 + (f - i)));
    for (int i = 1; r - i > 0 && f + i < 7; ++i)
        mask |= (1ULL << ((r - i) * 8 + (f + i)));
    for (int i = 1; r - i > 0 && f - i > 0; ++i)
        mask |= (1ULL << ((r - i) * 8 + (f - i)));
    return mask;
}

u64 rook_attacks(int sq, u64 block)
{
    u64 attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i < 8; ++i)
    {
        attacks |= (1ULL << (i * 8 + f));
        if (block & (1ULL << (i * 8 + f)))
            break;
    }
    for (int i = r - 1; i >= 0; --i)
    {
        attacks |= (1ULL << (i * 8 + f));
        if (block & (1ULL << (i * 8 + f)))
            break;
    }
    for (int i = f + 1; i < 8; ++i)
    {
        attacks |= (1ULL << (r * 8 + i));
        if (block & (1ULL << (r * 8 + i)))
            break;
    }
    for (int i = f - 1; i >= 0; --i)
    {
        attacks |= (1ULL << (r * 8 + i));
        if (block & (1ULL << (r * 8 + i)))
            break;
    }
    return attacks;
}

u64 bishop_attacks(int sq, u64 block)
{
    u64 attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = 1; r + i < 8 && f + i < 8; ++i)
    {
        attacks |= (1ULL << ((r + i) * 8 + (f + i)));
        if (block & (1ULL << ((r + i) * 8 + (f + i))))
            break;
    }
    for (int i = 1; r + i < 8 && f - i >= 0; ++i)
    {
        attacks |= (1ULL << ((r + i) * 8 + (f - i)));
        if (block & (1ULL << ((r + i) * 8 + (f - i))))
            break;
    }
    for (int i = 1; r - i >= 0 && f + i < 8; ++i)
    {
        attacks |= (1ULL << ((r - i) * 8 + (f + i)));
        if (block & (1ULL << ((r - i) * 8 + (f + i))))
            break;
    }
    for (int i = 1; r - i >= 0 && f - i >= 0; ++i)
    {
        attacks |= (1ULL << ((r - i) * 8 + (f - i)));
        if (block & (1ULL << ((r - i) * 8 + (f - i))))
            break;
    }
    return attacks;
}

void generate_blocker_boards(u64 mask, std::vector<u64> &out)
{
    int bits[64], n = 0;
    for (int i = 0; i < 64; ++i)
        if (mask & (1ULL << i))
            bits[n++] = i;

    int permutations = 1 << n;
    out.reserve(permutations);
    for (int i = 0; i < permutations; ++i)
    {
        u64 b = 0;
        for (int j = 0; j < n; ++j)
            if (i & (1 << j))
                b |= (1ULL << bits[j]);
        out.push_back(b);
    }
}

u64 find_magic(int square, int bits, const std::vector<u64> &blockers, const std::vector<u64> &attacks)
{
    int size = 1 << bits;
    std::vector<u64> table(size);
    std::vector<bool> used(size);

    while (true)
    {
        u64 magic = random_u64() & random_u64() & random_u64();
        std::fill(table.begin(), table.end(), 0);
        std::fill(used.begin(), used.end(), false);

        bool fail = false;
        for (size_t i = 0; i < blockers.size(); ++i)
        {
            u64 index = (blockers[i] * magic) >> (64 - bits);
            if (!used[index])
            {
                used[index] = true;
                table[index] = attacks[i];
            }
            else if (table[index] != attacks[i])
            {
                fail = true;
                break;
            }
        }

        if (!fail)
            return magic;
    }
}

void write_all_magics()
{
    std::ostream &out = std::cout;

    std::ostringstream rook_magics, bishop_magics;

    out << "// Auto-generated magic bitboards and tables\n\n";
    out << "#pragma once\n\n";
    out << "#include <cstdint>\n\n";
    out << "using u64 = uint64_t;\n\n";

    out << "// --- Attack tables ---\n\n";

    auto write_piece = [&](const std::string &prefix, u64 (*mask_func)(int), u64 (*attack_func)(int, u64), std::ostringstream &magics_out)
    {
        magics_out << "static Magic " << prefix << "_magics[64] = {\n";

        for (int sq = 0; sq < 64; ++sq)
        {
            u64 mask = mask_func(sq);
            int bits = __builtin_popcountll(mask);

            std::vector<u64> blockers, attacks;
            generate_blocker_boards(mask, blockers);
            for (u64 b : blockers)
                attacks.push_back(attack_func(sq, b));

            u64 magic = find_magic(sq, bits, blockers, attacks);
            int shift = 64 - bits;

            std::string table_name = prefix + "_attacks_" + std::to_string(sq);
            out << "static u64 " << table_name << "[] = {\n";
            for (size_t i = 0; i < (1ULL << bits); ++i)
            {
                if (i % 4 == 0)
                    out << "    ";
                u64 entry = 0;
                for (size_t j = 0; j < blockers.size(); ++j)
                {
                    if (((blockers[j] * magic) >> (64 - bits)) == i)
                    {
                        entry = attacks[j];
                        break;
                    }
                }
                out << "0x" << std::hex << entry << "ULL, ";
                if (i % 4 == 3)
                    out << "\n";
            }
            if ((1 << bits) % 4 != 0)
                out << "\n";
            out << "};\n\n";

            magics_out << "    { 0x" << std::hex << mask << "ULL, 0x" << magic << "ULL, "
                       << std::dec << shift << ", " << table_name << " },\n";
        }

        magics_out << "};\n\n";
    };

    write_piece("rook", rook_mask, rook_attacks, rook_magics);
    write_piece("bishop", bishop_mask, bishop_attacks, bishop_magics);

    out << "// --- Magic arrays ---\n\n";
    out << rook_magics.str();
    out << bishop_magics.str();
}

int main()
{
    srand(time(0));
    write_all_magics();
    return 0;
}