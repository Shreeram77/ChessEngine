#include <iostream>

#include "Position.hpp"
#include "Search.hpp"

int main()
{
    Position pos = Position::from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );

    Move best = find_best_move(pos, 3);

    std::cout
        << "Best move: "
        << best.to_uci()
        << "\n";
}