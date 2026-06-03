#include "TranspositionTable.hpp"

namespace
{
    std::unordered_map<uint64_t, TTEntry>
        transposition_table;
}

bool probe_tt(
    uint64_t hash,
    int depth,
    int alpha,
    int beta,
    int& score)
{
    auto it =
        transposition_table.find(hash);

    if (it == transposition_table.end())
        return false;

    if (it->second.depth < depth)
        return false;

    const TTEntry& entry =
        it->second;

    if (entry.flag == TTFlag::Exact)
    {
        score = entry.score;
        return true;
    }

    if (entry.flag == TTFlag::LowerBound &&
        entry.score >= beta)
    {
        score = entry.score;
        return true;
    }

    if (entry.flag == TTFlag::UpperBound &&
        entry.score <= alpha)
    {
        score = entry.score;
        return true;
    }

    return false;
}

void store_tt(
    uint64_t hash,
    int depth,
    int score,
    TTFlag flag)
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
        score,
        flag
    };
}

void clear_tt()
{
    transposition_table.clear();
}