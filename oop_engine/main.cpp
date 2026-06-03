#include <iostream>

#include "Position.hpp"
#include "Search.hpp"

int main()
{
    Position pos = Position::from_fen(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
);
    Move best = find_best_move(pos, 4);

    std::cout
        << "Best move: "
        << best.to_uci()
        << "\n";

    // std::cout
    //     << "Minimax score: "
    //     << debug_minimax_score(pos, 4)
    //     << "\n";

    // std::cout
    //     << "Negamax score: "
    //     << debug_negamax_score(pos, 4)
    //     << "\n";
}