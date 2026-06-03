#pragma once

#include "Move.hpp"
#include "Position.hpp"

Move find_best_move(
    const Position& pos,
    int depth
);

