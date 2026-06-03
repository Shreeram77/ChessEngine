#include "Search.hpp"
#include <iostream>
#include <limits>
#include <algorithm>
#include "ChessBoard.hpp"
#include "Evaluator.hpp"

#include <limits>

namespace
{
    uint64_t nodes_searched = 0;

    constexpr int NEG_INF = -10000000;
    constexpr int POS_INF =  10000000;
    constexpr int MATE_SCORE = 100000;

    int move_score(const Move& move)
    {
        switch (move.flag)
        {
            case MoveFlag::PromoteQueen:
            case MoveFlag::PromoteRook:
            case MoveFlag::PromoteBishop:
            case MoveFlag::PromoteKnight:
                return 1000;

            default:
                break;
        }

        if (!move.captured.is_empty())
            return 100;

        return 0;
    }
    
    int minimax(
        const Position& pos,
        int depth,
        int alpha,
        int beta)
    {
        ++nodes_searched;

        ChessBoard board(pos);

        auto moves = board.legal_moves();

        if (moves.empty())
        {
            if (board.is_in_check(pos.side_to_move))
            {
                if (pos.side_to_move == Color::White)
                    return -MATE_SCORE - depth;

                return MATE_SCORE + depth;
            }

            return 0;
        }

        if (depth == 0)
            return evaluate(pos);

        std::sort(
            moves.begin(),
            moves.end(),
            [](const Move& a, const Move& b)
            {
                return move_score(a) > move_score(b);
            });

        if (pos.side_to_move == Color::White)
        {
            int best = NEG_INF;

            for (const auto& move : moves)
            {
                Position next = board.apply_move(move);

                int score =
                    minimax(next,
                            depth - 1,
                            alpha,
                            beta);

                if (score > best)
                    best = score;

                if (best > alpha)
                    alpha = best;

                if (alpha >= beta)
                    break;
            }

            return best;
        }
        else
        {
            int best = POS_INF;

            for (const auto& move : moves)
            {
                Position next = board.apply_move(move);

                int score =
                    minimax(next,
                            depth - 1,
                            alpha,
                            beta);

                if (score < best)
                    best = score;

                if (best < beta)
                    beta = best;

                if (alpha >= beta)
                    break;
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

    std::sort(
        moves.begin(),
        moves.end(),
        [](const Move& a, const Move& b)
        {
            return move_score(a) > move_score(b);
        });

    if (moves.empty())
        return Move{};

    Move best_move = moves.front();

    if (pos.side_to_move == Color::White)
    {
        int best_score =
            NEG_INF;

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                minimax(
                    next,
                    depth - 1,
                    NEG_INF,
                    POS_INF);

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
            POS_INF;

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                minimax(
                    next,
                    depth - 1,
                    NEG_INF,
                    POS_INF);

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