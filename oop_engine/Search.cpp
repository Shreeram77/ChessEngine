#include "Search.hpp"
#include <iostream>
#include <limits>
#include <algorithm>
#include "ChessBoard.hpp"
#include "Evaluator.hpp"
#include "Zobrist.hpp"
#include "TranspositionTable.hpp"

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

    int quiescence(
        const Position& pos,
        int alpha,
        int beta)
    {
        ++nodes_searched;

        int stand_pat = evaluate(pos);

        if (pos.side_to_move == Color::White)
        {
            if (stand_pat >= beta)
                return beta;

            if (stand_pat > alpha)
                alpha = stand_pat;
        }
        else
        {
            if (stand_pat <= alpha)
                return alpha;

            if (stand_pat < beta)
                beta = stand_pat;
        }

        ChessBoard board(pos);

        auto moves = board.legal_moves();

        moves.erase(
            std::remove_if(
                moves.begin(),
                moves.end(),
                [](const Move& move)
                {
                    return !move.is_capture();
                }),
            moves.end());

        std::sort(
            moves.begin(),
            moves.end(),
            [](const Move& a, const Move& b)
            {
                return move_score(a) > move_score(b);
            });

        if (pos.side_to_move == Color::White)
        {
            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    quiescence(
                        next,
                        alpha,
                        beta);

                if (score > alpha)
                    alpha = score;

                if (alpha >= beta)
                    break;
            }

            return alpha;
        }
        else
        {
            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    quiescence(
                        next,
                        alpha,
                        beta);

                if (score < beta)
                    beta = score;

                if (alpha >= beta)
                    break;
            }

            return beta;
        }
    }
    
    int minimax(
        const Position& pos,
        int depth,
        int alpha,
        int beta)
    {
        ++nodes_searched;

        int cached_score;

        uint64_t hash =
            compute_hash(pos);

        if (probe_tt(
            hash,
            depth,
            alpha,
            beta,
            cached_score))
        {
            return cached_score;
        }

        int original_alpha = alpha;
        int original_beta  = beta;

        ChessBoard board(pos);

        auto moves = board.legal_moves();

        if (moves.empty())
        {
            if (board.is_in_check(pos.side_to_move))
            {
                int score;

                if (pos.side_to_move == Color::White)
                    score = -MATE_SCORE - depth;
                else
                    score = MATE_SCORE + depth;

                store_tt(
                    hash,
                    depth,
                    score,
                    TTFlag::Exact);

                return score;
            }

            store_tt(
                hash,
                depth,
                0,
                TTFlag::Exact);

            return 0;
        }

        if (depth == 0)
        return quiescence(
            pos,
            alpha,
            beta);

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
            TTFlag flag;

            if (best <= original_alpha)
                flag = TTFlag::UpperBound;
            else if (best >= original_beta)
                flag = TTFlag::LowerBound;
            else
                flag = TTFlag::Exact;

            store_tt(
                hash,
                depth,
                best,
                flag);

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
            TTFlag flag;

            if (best <= original_alpha)
                flag = TTFlag::UpperBound;
            else if (best >= original_beta)
                flag = TTFlag::LowerBound;
            else
                flag = TTFlag::Exact;

            store_tt(
                hash,
                depth,
                best,
                flag);

            return best;
        }
    }
}

Move find_best_move(
    const Position& pos,
    int depth)
{
    nodes_searched = 0;

    // clear_tt();

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

    Move last_best_move = best_move;

    auto it =
        std::find(
            moves.begin(),
            moves.end(),
            best_move);

    if (it != moves.end())
    {
        std::rotate(
            moves.begin(),
            it,
            it + 1);
    }

    for (int current_depth = 1;
        current_depth <= depth;
        ++current_depth)
    {
        best_move = moves.front();

        if (pos.side_to_move == Color::White)
        {
            int best_score = NEG_INF;

            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    minimax(
                        next,
                        current_depth - 1,
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
            int best_score = POS_INF;

            for (const auto& move : moves)
            {
                Position next =
                    board.apply_move(move);

                int score =
                    minimax(
                        next,
                        current_depth - 1,
                        NEG_INF,
                        POS_INF);

                if (score < best_score)
                {
                    best_score = score;
                    best_move = move;
                }
            }
        }

        last_best_move = best_move;
    }

    std::cout
        << "Nodes searched: "
        << nodes_searched
        << "\n";

    return last_best_move;
}