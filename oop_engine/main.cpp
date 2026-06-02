#include <iostream>

#include "Position.hpp"
#include "Evaluator.hpp"

int main()
{
 Position pos = Position::from_fen(
    "rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
);

    std::cout << evaluate(pos) << "\n";
}