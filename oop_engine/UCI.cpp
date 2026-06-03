#include "UCI.hpp"
#include "Position.hpp"
#include <iostream>
#include <string>


void run_uci()
{
    Position current_position =
    Position::starting();

    std::string command;

    while (std::getline(std::cin, command))
    {
        if (command == "uci")
        {
            std::cout
                << "id name ShreeramEngine\n"
                << "id author Shreeram\n"
                << "uciok\n";
        }
        else if (command == "isready")
        {
            std::cout
                << "readyok\n";
        }
        else if (command == "position startpos")
        {
            current_position =
                Position::starting();
        }
        else if (command.rfind(
            "position fen ",
            0) == 0)
        {
            std::string fen =
                command.substr(13);

            current_position =
                Position::from_fen(fen);
        }
        else if (command == "quit")
        {
            break;
        }
    }
}