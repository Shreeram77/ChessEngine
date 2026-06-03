#include "Search.hpp"
#include <iostream>
#include "ChessBoard.hpp"
#include "Evaluator.hpp"

#include <limits>

namespace
{
    uint64_t nodes_searched = 0;

    int minimax(
    const Position& pos,
    int depth)
    {
        ++nodes_searched;

        if (depth == 0)
            return evaluate(pos);

        ChessBoard board(pos);

        if (board.is_checkmate())
        {
            if (pos.side_to_move == Color::White)
                return -100000;

            return 100000;
        }

        if (board.is_stalemate())
            return 0;

        auto moves = board.legal_moves();

        if (pos.side_to_move == Color::White)
        {
            int best =
                std::numeric_limits<int>::min();

            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    minimax(next, depth - 1);

                if (score > best)
                    best = score;
            }

            return best;
        }
        else
        {
            int best =
                std::numeric_limits<int>::max();

            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    minimax(next, depth - 1);

                if (score < best)
                    best = score;
            }

            return best;
        }
    }
}

Move find_best_move(
    const Position& pos,
    int depth)
{
    nodes_searched = 0;

    ChessBoard board(pos);

    auto moves = board.legal_moves();

    if (moves.empty())
        return Move{};

    Move best_move = moves.front();

    if (pos.side_to_move == Color::White)
    {
        int best_score =
            std::numeric_limits<int>::min();

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                minimax(next, depth - 1);

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }
        }
    }
    else
    {
        int best_score =
            std::numeric_limits<int>::max();

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                minimax(next, depth - 1);

            if (score < best_score)
            {
                best_score = score;
                best_move = move;
            }
        }
    }

    std::cout
    << "Nodes searched: "
    << nodes_searched
    << "\n";

    return best_move;
}