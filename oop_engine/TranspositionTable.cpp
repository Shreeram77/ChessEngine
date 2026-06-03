#include "TranspositionTable.hpp"

namespace
{
    std::unordered_map<uint64_t, TTEntry>
        transposition_table;
}

bool probe_tt(
    uint64_t hash,
    int depth,
    int& score)
{
    auto it =
        transposition_table.find(hash);

    if (it == transposition_table.end())
        return false;

    if (it->second.depth < depth)
        return false;

    score = it->second.score;
    return true;
}

void store_tt(
    uint64_t hash,
    int depth,
    int score)
{
    auto it =
        transposition_table.find(hash);

    if (it != transposition_table.end() &&
        it->second.depth > depth)
    {
        return;
    }

    transposition_table[hash] =
    {
        hash,
        depth,
        score
    };
}

void clear_tt()
{
    transposition_table.clear();
}