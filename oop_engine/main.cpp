#include <iostream>

#include "Position.hpp"
#include "Perft.hpp"

int main()
{
    Position pos = Position::from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );

    const uint64_t expected[] =
    {
        0,
        20,
        400,
        8902,
        197281,
        4865609
    };

    for(int depth = 1; depth <= 5; depth++)
    {
        uint64_t result =
            perft(pos, depth);

        std::cout
            << "Depth "
            << depth
            << " : "
            << result;

        if(result == expected[depth])
            std::cout << " PASS";
        else
            std::cout << " FAIL";

        std::cout << "\n";
    }

    return 0;
}