#include "UCI.hpp"
#include "Position.hpp"
#include <iostream>
#include <string>
#include "ChessBoard.hpp"
#include "Search.hpp"
#include <sstream>
#include <vector>

Move find_uci_move(
    const Position& pos,
    const std::string& move_text)
{
    ChessBoard board(pos);

    auto moves =
        board.legal_moves();

    for (const auto& move : moves)
    {
        if (move.to_uci() == move_text)
            return move;
    }

    return Move{};
}

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
        else if (command.rfind(
            "position startpos",
            0) == 0)
        {
            current_position =
                Position::starting();

            size_t moves_pos =
                command.find(" moves ");

            if (moves_pos != std::string::npos)
            {
                std::stringstream ss(
                    command.substr(
                        moves_pos + 7));

                std::string move_text;

                while (ss >> move_text)
                {
                    Move move =
                        find_uci_move(
                            current_position,
                            move_text);

                    ChessBoard board(
                        current_position);

                    current_position =
                        board.apply_move(
                            move);
                }
            }
        }
        else if (command.rfind(
            "go",
            0) == 0)
        {
            int depth = 5;

            if (command.rfind(
                "go depth ",
                0) == 0)
            {
                depth =
                    std::stoi(
                        command.substr(9));
            }

            SearchResult result =
                find_best_move(
                    current_position,
                    depth);

            std::cout
                << "bestmove "
                << result.best_move.to_uci()
                << "\n"
                << std::flush;
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