
// We are trying to generate magic numbers for the rook and bishop attacks.
// This will allow us to use the perfect hash function to generate the attacks for each square.
//   e.g., given rook_magics is a Magic[64] array, we can use the following function to get the rook attacks for a square:
// ```
//     u64 ortho_attacks(int square, u64 occupancy)
//     {
//         u64 blockers = occupancy & rook_magics[square].mask;
//         u64 index = (blockers * rook_magics[square].magic) >> rook_magics[square].shift;
//         return attack_pool[rook_magics[square].pool_index[index]];
//     }
// ```

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

using u64 = uint64_t;

struct Magic
{
    u64 mask;
    u64 magic;
    int shift;
    const uint16_t *pool_indexes;
};

// Utility functions
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

u64 find_magic(int bits, const std::vector<u64> &blockers, const std::vector<u64> &attacks)
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

int get_or_add_attack(u64 attack, std::vector<u64> &pool, std::unordered_map<u64, int> &index_map)
{
    auto it = index_map.find(attack);
    if (it != index_map.end())
        return it->second;
    int idx = pool.size();
    pool.push_back(attack);
    index_map[attack] = idx;
    return idx;
}

void write_all_magics(std::ostream &magics_out, std::ostream &attacks_out)
{
    std::vector<u64> pool;
    std::unordered_map<u64, int> index_map;

    std::ostringstream magics_decl_stream;

    auto write_piece = [&](const std::string &prefix, u64 (*mask_func)(int), u64 (*attack_func)(int, u64))
    {
        magics_decl_stream << "static Magic " << prefix << "_magics[64] = {\n";

        for (int sq = 0; sq < 64; ++sq)
        {
            u64 mask = mask_func(sq);
            int bits = __builtin_popcountll(mask);

            std::vector<u64> blockers, attacks;
            generate_blocker_boards(mask, blockers);
            for (u64 b : blockers)
                attacks.push_back(attack_func(sq, b));

            u64 magic = find_magic(bits, blockers, attacks);
            int shift = 64 - bits;

            std::string index_name = prefix + "_pool_indexes_" + std::to_string(sq);

            // Emit pool index array directly
            magics_out << "static uint16_t " << index_name << "[] = {\n";
            for (int i = 0; i < (1 << bits); ++i)
            {
                if (i % 8 == 0)
                    magics_out << "    ";
                u64 val = 0;
                for (size_t j = 0; j < blockers.size(); ++j)
                {
                    if (((blockers[j] * magic) >> (64 - bits)) == u64(i))
                    {
                        val = attacks[j];
                        break;
                    }
                }
                int pool_idx = get_or_add_attack(val, pool, index_map);
                magics_out << pool_idx << ", ";
                if (i % 8 == 7)
                    magics_out << "\n";
            }
            if ((1 << bits) % 8 != 0)
                magics_out << "\n";
            magics_out << "};\n\n";

            // Queue the Magic struct
            magics_decl_stream << "    { 0x" << std::hex << mask << "ULL, 0x" << magic << "ULL, "
                               << std::dec << shift << ", " << index_name << " },\n";
        }

        magics_decl_stream << "};\n\n";
    };

    // Emit both rook and bishop data
    write_piece("rook", rook_mask, rook_attacks);
    write_piece("bishop", bishop_mask, bishop_attacks);

    // Write the magic arrays after all pool index arrays
    magics_out << magics_decl_stream.str();

    // Emit the attack pool
    attacks_out << "// Auto generated with extra/magic_generator.cpp\n// DO NOT EDIT THIS FILE\n";
    attacks_out << "{\n";
    for (size_t i = 0; i < pool.size(); ++i)
    {
        if (i % 4 == 0)
            attacks_out << "    ";
        attacks_out << "0x" << std::hex << pool[i] << "ULL, ";
        if (i % 4 == 3)
            attacks_out << "\n";
    }
    if (pool.size() % 4 != 0)
        attacks_out << "\n";
    attacks_out << "}\n";
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <magics.inc> <magic_attacks.inc>\n";
        return 1;
    }

    std::ofstream magics_file(argv[1]);
    std::ofstream attacks_file(argv[2]);
    if (!magics_file || !attacks_file)
    {
        std::cerr << "Failed to open output files.\n";
        return 1;
    }

    srand(time(0));
    write_all_magics(magics_file, attacks_file);
    std::cout << "Generated " << argv[1] << " and " << argv[2] << "\n";
    return 0;
}
