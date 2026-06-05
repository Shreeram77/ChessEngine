#pragma once

#include "Move.hpp"
#include "Position.hpp"

struct SearchResult
{
    Move best_move;
    int score = 0;
    uint64_t nodes = 0;
    int depth = 0;
    long long time_ms = 0;
};

SearchResult find_best_move(
    const Position& pos,
    int depth);

