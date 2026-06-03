#pragma once

#include <cstdint>
#include <unordered_map>

enum class TTFlag : uint8_t
{
    Exact,
    LowerBound,
    UpperBound
};

struct TTEntry
{
    uint64_t hash = 0;
    int depth = 0;
    int score = 0;
    TTFlag flag = TTFlag::Exact;
};

bool probe_tt(
    uint64_t hash,
    int depth,
    int alpha,
    int beta,
    int& score);

void store_tt(
    uint64_t hash,
    int depth,
    int score,
    TTFlag flag);

void clear_tt();