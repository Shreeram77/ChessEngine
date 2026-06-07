#pragma once

#include "Move.hpp"
#include "Position.hpp"

#include <cstdint>
#include <string>

// ============================================================
//  Search.hpp
//  Public interface for the search engine.
//
//  SearchResult carries all data emitted on the UCI info line
//  so the caller (UCI.cpp) can forward it without re-parsing.
// ============================================================

struct SearchResult
{
    Move        best_move;          // Best move found
    int         score    = 0;       // Score in centipawns (side-to-move perspective)
    uint64_t    nodes    = 0;       // Total nodes searched across all depths
    int         depth    = 0;       // Final depth reached
    long long   time_ms  = 0;       // Total wall-clock time in milliseconds
    std::string pv;                 // Principal variation in UCI format ("e2e4 e7e5 ...")
};

// Searches the given position to the given depth using iterative
// deepening alpha-beta and returns a fully populated SearchResult.
SearchResult find_best_move(
    const Position& pos,
    int depth);