#pragma once

#include <cstdint>
#include <unordered_map>

struct TTEntry
{
    uint64_t hash = 0;
    int depth = 0;
    int score = 0;
};

bool probe_tt(
    uint64_t hash,
    int depth,
    int& score);

void store_tt(
    uint64_t hash,
    int depth,
    int score);

void clear_tt();