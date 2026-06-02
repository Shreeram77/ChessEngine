#include "Game.hpp"
#include <iostream>

int main()
{
    Game game(
        Position::from_fen(
            "4k3/8/8/8/8/8/8/4K3 w - - 100 1"
        )
    );

    std::cout
        << game_result_string(game.result())
        << "\n";
}