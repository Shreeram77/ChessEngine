#include "Perft.hpp"

#include "ChessBoard.hpp"
#include "Move.hpp"

#include <iostream>

uint64_t perft(const Position& pos, int depth)
{
    if(depth == 0)
        return 1ULL;

    ChessBoard board(pos);

    auto moves = board.legal_moves();

    if(depth == 1)
        return static_cast<uint64_t>(moves.size());

    uint64_t nodes = 0;

    for(const auto& move : moves)
    {
        Position next = board.apply_move(move);

        nodes += perft(next, depth - 1);
    }

    return nodes;
}

void perft_divide(const Position& pos, int depth)
{
    ChessBoard board(pos);

    auto moves = board.legal_moves();

    uint64_t total = 0;

    for(const auto& move : moves)
    {
        Position next = board.apply_move(move);

        uint64_t count =
            perft(next, depth - 1);

        total += count;

        std::cout
            << move.to_uci()
            << " : "
            << count
            << "\n";
    }

    std::cout
        << "TOTAL = "
        << total
        << "\n";
}