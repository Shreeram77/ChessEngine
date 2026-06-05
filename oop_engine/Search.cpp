#include "Search.hpp"
#include <iostream>
#include <chrono>
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
    static constexpr int MAX_PLY = 64;


    // killer_moves[ply][0] = best killer
    // killer_moves[ply][1] = second best killer
    static Move killer_moves[MAX_PLY][2];

    static int history_table[64][64];

    int negamax(
        const Position& pos,
        int depth,
        int alpha,
        int beta,
        int ply);

    int move_score(
        const Move& move,
        int ply)
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

        if (move == killer_moves[ply][0])
            return 90;

        if (move == killer_moves[ply][1])
            return 80;

        return history_table[
            move.from
        ][
            move.to
        ];
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
                return move_score(a, 0) 
                        > move_score(b, 0);
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

    int negamax(
        const Position& pos,
        int depth,
        int alpha,
        int beta,
        int ply)
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
                return -MATE_SCORE - depth;
            }

            return 0;
        }

        if (depth == 0)
        {
            int score = evaluate(pos);

            if (pos.side_to_move == Color::Black)
                score = -score;

            return score;
        }

        std::sort(
            moves.begin(),
            moves.end(),
            [ply](const Move& a, const Move& b)
            {
                return move_score(a, ply)
                        > move_score(b, ply);
            });

        int best = NEG_INF;

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                -negamax(
                    next,
                    depth - 1,
                    -beta,
                    -alpha,
                    ply + 1);

            if (score > best)
                best = score;

            if (score > alpha)
                alpha = score;

            if (alpha >= beta)
            {
                if (!move.is_capture())
                {
                    killer_moves[ply][1] =
                        killer_moves[ply][0];

                    killer_moves[ply][0] =
                        move;

                    history_table[
                        move.from
                    ][
                        move.to
                    ] += depth * depth;
                }

                break;
            }
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

SearchResult find_best_move(
    const Position& pos,
    int depth)
{
    nodes_searched = 0;

    auto start =
        std::chrono::steady_clock::now();

    ChessBoard board(pos);

    auto moves = board.legal_moves();

    std::sort(
        moves.begin(),
        moves.end(),
        [](const Move& a, const Move& b)
        {
            return move_score(a, 0)
                > move_score(b, 0);
        });

    SearchResult result;

    if (moves.empty())
        return result;

    Move best_move = moves.front();

    Move last_best_move = best_move;

    int last_best_score = 0;

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

        int best_score = NEG_INF;

        for (const auto& move : moves)
        {
            Position next =
                board.apply_move(move);

            int score =
                -negamax(
                    next,
                    current_depth - 1,
                    NEG_INF,
                    POS_INF,
                    1);

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }
        }

        last_best_move = best_move;
        last_best_score = best_score;

        auto now =
            std::chrono::steady_clock::now();

        long long elapsed =
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                now - start).count();

        std::cout
            << "info depth "
            << current_depth
            << " score cp "
            << best_score
            << " nodes "
            << nodes_searched
            << " time "
            << elapsed
            << "\n"
            << std::flush;
    }

    auto end =
        std::chrono::steady_clock::now();

    result.best_move = last_best_move;
    result.score = last_best_score;
    result.depth = depth;
    result.nodes = nodes_searched;
    result.time_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
            end - start).count();

    return result;
}