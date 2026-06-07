#include "Search.hpp"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <sstream>

#include "ChessBoard.hpp"
#include "Evaluator.hpp"
#include "Zobrist.hpp"
#include "TranspositionTable.hpp"

// ============================================================
//  Search.cpp
//  Negamax alpha-beta with:
//    - Iterative deepening
//    - Quiescence search (fail-hard, pure negamax style)
//    - Transposition table (Zobrist hashing)
//    - Move ordering: promotions > MVV-LVA captures > killers > history
//    - Killer move heuristic (2 per ply, reset each search)
//    - History heuristic (aged each iteration)
//    - Triangular PV table with UCI info line emission
// ============================================================

namespace
{
    // ── Search-wide state ────────────────────────────────────

    uint64_t nodes_searched = 0;

    constexpr int NEG_INF    = -10'000'000;
    constexpr int POS_INF    =  10'000'000;
    constexpr int MATE_SCORE =     100'000;
    constexpr int MAX_PLY    =          64;

    // ── Killer moves ─────────────────────────────────────────
    // Two quiet moves per ply that caused a beta cutoff in a
    // sibling node. Reset at the start of each find_best_move call.
    Move killer_moves[MAX_PLY][2];

    // ── History table ─────────────────────────────────────────
    // history_table[from][to] accumulates depth^2 for every quiet
    // move that causes a beta cutoff. Aged (halved) each iteration
    // to prevent int overflow and decay stale signal.
    int history_table[64][64];

    // ── Triangular PV table ───────────────────────────────────
    // pv_table[ply][ply..pv_length[ply]-1] holds the best line
    // from that ply forward. pv_length[ply] is an absolute end index.
    Move pv_table[MAX_PLY][MAX_PLY];
    int  pv_length[MAX_PLY];

    // ── Forward declaration ───────────────────────────────────

    int negamax(const Position& pos, int depth, int alpha, int beta, int ply);

    // ── MVV-LVA tables ────────────────────────────────────────
    // Indexed by PieceType enum value (None=0 .. King=6).
    // MVV_VALUE:  how desirable it is to capture this piece type.
    // LVA_VALUE:  how undesirable it is to use this piece type as aggressor.
    //             Lower aggressor cost → higher final score → searched earlier.
    //
    // Score formula: 200 + MVV_VALUE[victim] - LVA_VALUE[aggressor]
    // Range: PxQ = 695 (best) .. QxP = 299 (worst)
    // All captures score above killers (90) and below promotions (1000).
    static constexpr int MVV_VALUE[7] = {
        0,    // None   (shouldn't occur as victim in legal play)
        100,  // Pawn
        200,  // Knight
        300,  // Bishop
        400,  // Rook
        500,  // Queen
        600   // King   (shouldn't occur in legal play)
    };

    static constexpr int LVA_VALUE[7] = {
        0,  // None
        5,  // Pawn   (cheapest aggressor → highest score)
        4,  // Knight
        3,  // Bishop
        2,  // Rook
        1,  // Queen  (most expensive aggressor → lowest score)
        0   // King
    };

    // ── Move scoring ──────────────────────────────────────────
    // Returns an integer used to sort moves before searching.
    // Higher = searched earlier.
    //
    // Priority order:
    //   1. Promotions              (1000)
    //   2. Captures by MVV-LVA     (299 – 695)
    //   3. Primary killer move     (90)
    //   4. Secondary killer move   (80)
    //   5. History heuristic       (0 .. varies)

    int move_score(const Move& move, int ply)
    {
        // 1. Promotions — always searched first
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

        // 2. Captures — ordered by MVV-LVA
        if (!move.captured.is_empty())
        {
            const int victim    = MVV_VALUE[static_cast<int>(move.captured.type)];
            const int aggressor = LVA_VALUE[static_cast<int>(move.moving.type)];
            return 200 + victim - aggressor;
        }

        // 3. Killer moves (quiet moves that caused cutoffs at this ply)
        if (move == killer_moves[ply][0]) return 90;
        if (move == killer_moves[ply][1]) return 80;

        // 4. History heuristic
        return history_table[move.from][move.to];
    }

    // ── Quiescence search ─────────────────────────────────────
    // Fail-hard, pure negamax style. Returns a score from the
    // perspective of the side to move (consistent with negamax).
    // Searches only captures until a quiet position is reached,
    // eliminating the horizon effect at the search frontier.

    int quiescence(const Position& pos, int alpha, int beta)
    {
        ++nodes_searched;

        // Stand-pat score: evaluate the position without making a move.
        // evaluate() is white-positive; convert to side-to-move perspective.
        int stand_pat = evaluate(pos);
        if (pos.side_to_move == Color::Black)
            stand_pat = -stand_pat;

        // Fail-hard upper-bound cutoff
        if (stand_pat >= beta)
            return beta;

        // Raise alpha floor if standing pat is better than current lower bound
        if (stand_pat > alpha)
            alpha = stand_pat;

        // Generate and filter to captures only
        ChessBoard board(pos);
        auto moves = board.legal_moves();

        moves.erase(
            std::remove_if(
                moves.begin(),
                moves.end(),
                [](const Move& m) { return !m.is_capture(); }),
            moves.end());

        // Order captures by MVV-LVA (ply 0 — killers irrelevant in QS)
        std::sort(
            moves.begin(),
            moves.end(),
            [](const Move& a, const Move& b)
            {
                return move_score(a, 0) > move_score(b, 0);
            });

        for (const auto& move : moves)
        {
            Position next = board.apply_move(move);

            // Pure negamax: negate and swap window on recursion
            const int score = -quiescence(next, -beta, -alpha);

            if (score >= beta)
                return beta;    // Fail-hard cutoff

            if (score > alpha)
                alpha = score;
        }

        return alpha;
    }

    // ── Negamax alpha-beta ────────────────────────────────────
    // Fail-hard. Score is from the perspective of the side to move.
    // Maintains the triangular PV table on the way up.

    int negamax(
        const Position& pos,
        int depth,
        int alpha,
        int beta,
        int ply)
    {
        ++nodes_searched;

        // Initialise PV length for this node (no line found yet)
        pv_length[ply] = ply;

        // ── Leaf node: drop into quiescence ──────────────────
        // Checked BEFORE move generation to avoid paying full movegen
        // cost at every leaf. Depth-0 nodes are the vast majority of
        // all nodes at typical search depths.
        if (depth == 0)
            return quiescence(pos, alpha, beta);

        // ── Transposition table probe ─────────────────────────
        int cached_score;
        const uint64_t hash = compute_hash(pos);

        if (probe_tt(hash, depth, alpha, beta, cached_score))
            return cached_score;

        const int original_alpha = alpha;
        const int original_beta  = beta;

        // ── Move generation ───────────────────────────────────
        ChessBoard board(pos);
        auto moves = board.legal_moves();

        // ── Terminal node detection ───────────────────────────
        if (moves.empty())
        {
            // Checkmate: penalise by depth so shorter mates score higher.
            if (board.is_in_check(pos.side_to_move))
                return -MATE_SCORE - depth;

            return 0; // Stalemate
        }

        // ── Move ordering ─────────────────────────────────────
        std::sort(
            moves.begin(),
            moves.end(),
            [ply](const Move& a, const Move& b)
            {
                return move_score(a, ply) > move_score(b, ply);
            });

        int best = NEG_INF;

        for (const auto& move : moves)
        {
            const Position next = board.apply_move(move);
            const int score     = -negamax(next, depth - 1, -beta, -alpha, ply + 1);

            if (score > best)
            {
                best = score;

                // ── Update PV ─────────────────────────────────
                // Store this move at [ply][ply], then copy the child's
                // continuation from pv_table[ply+1] forward.
                pv_table[ply][ply] = move;
                for (int i = ply + 1; i < pv_length[ply + 1]; ++i)
                    pv_table[ply][i] = pv_table[ply + 1][i];
                pv_length[ply] = pv_length[ply + 1];
            }

            if (score > alpha)
                alpha = score;

            if (alpha >= beta)
            {
                // Beta cutoff: update killer and history for quiet moves
                if (!move.is_capture())
                {
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;

                    history_table[move.from][move.to] += depth * depth;
                }
                break;
            }
        }

        // ── Store in transposition table ──────────────────────
        TTFlag flag;
        if      (best <= original_alpha) flag = TTFlag::UpperBound;
        else if (best >= original_beta)  flag = TTFlag::LowerBound;
        else                             flag = TTFlag::Exact;

        store_tt(hash, depth, best, flag);

        return best;
    }

    // ── Build PV string from the triangular table ─────────────
    // Assembles a space-separated UCI move string starting at root_ply.

    std::string build_pv_string(int root_ply)
    {
        std::ostringstream ss;
        for (int i = root_ply; i < pv_length[root_ply]; ++i)
        {
            if (i > root_ply) ss << ' ';
            ss << pv_table[root_ply][i].to_uci();
        }
        return ss.str();
    }

} // anonymous namespace

// ============================================================
//  find_best_move (public API)
// ============================================================

SearchResult find_best_move(
    const Position& pos,
    int depth)
{
    // ── Reset per-search state ────────────────────────────────

    nodes_searched = 0;

    // Killers are ply-indexed and position-specific. A killer from
    // a previous position is noise at the same ply in a new position.
    std::memset(killer_moves, 0, sizeof(killer_moves));

    // Age history: halving preserves relative ordering while
    // preventing int overflow over many consecutive searches.
    for (int f = 0; f < 64; ++f)
        for (int t = 0; t < 64; ++t)
            history_table[f][t] /= 2;

    std::memset(pv_table,  0, sizeof(pv_table));
    std::memset(pv_length, 0, sizeof(pv_length));

    // ── Root move generation ──────────────────────────────────

    const auto start = std::chrono::steady_clock::now();

    ChessBoard board(pos);
    auto moves = board.legal_moves();

    if (moves.empty())
        return SearchResult{};

    // Initial root ordering before depth 1 provides a best move
    std::sort(
        moves.begin(),
        moves.end(),
        [](const Move& a, const Move& b)
        {
            return move_score(a, 0) > move_score(b, 0);
        });

    SearchResult result;
    result.best_move = moves.front();

    // ── Iterative deepening loop ──────────────────────────────
    // Each completed iteration updates result and emits a UCI info line.
    // The best move from each iteration is rotated to the front of the
    // move list so it is searched first at the next depth, improving
    // alpha-beta efficiency.

    for (int current_depth = 1; current_depth <= depth; ++current_depth)
    {
        int  best_score  = NEG_INF;
        Move best_move   = moves.front();

        // Snapshot of pv_table[1] taken at the moment best_move is set.
        // Protects the winning PV from being overwritten by subsequent
        // root move searches that score lower but still write into pv_table[1].
        Move best_pv[MAX_PLY] = {};
        int  best_pv_len      = 1;  // absolute end index, same convention as pv_length

        for (const auto& move : moves)
        {
            // Reset pv_length before each subtree search so that each
            // candidate's PV is built independently. Without this, a deeper
            // pv_length value from a previous candidate bleeds into the copy
            // loop bounds of the current candidate's internal nodes.
            std::memset(pv_length, 0, sizeof(pv_length));

            const Position next = board.apply_move(move);
            const int score     = -negamax(next, current_depth - 1,
                                           NEG_INF, POS_INF, 1);

            if (score > best_score)
            {
                best_score = score;
                best_move  = move;

                // Snapshot the PV for this move immediately.
                // pv_table[1] will be overwritten by the next candidate's
                // search regardless of whether that candidate beats best_score.
                best_pv_len = pv_length[1];
                for (int i = 1; i < pv_length[1]; ++i)
                    best_pv[i] = pv_table[1][i];
            }
        }

        // Commit iteration result
        result.best_move = best_move;
        result.score     = best_score;
        result.depth     = current_depth;

        // Build PV from the snapshot, not from pv_table[1].
        // pv_table[1] belongs to whichever move was searched last;
        // best_pv belongs to best_move.
        result.pv = best_move.to_uci();
        for (int i = 1; i < best_pv_len; ++i)
        {
            result.pv += ' ';
            result.pv += best_pv[i].to_uci();
        }

        // Emit UCI info line
        const auto now = std::chrono::steady_clock::now();
        const long long elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start).count();

        std::cout
            << "info depth "  << current_depth
            << " score cp "   << best_score
            << " nodes "      << nodes_searched
            << " time "       << elapsed
            << " pv "         << result.pv
            << '\n'
            << std::flush;

        // Promote best move to front for better ordering at next depth
        auto it = std::find(moves.begin(), moves.end(), best_move);
        if (it != moves.end() && it != moves.begin())
            std::rotate(moves.begin(), it, it + 1);
    }

    // ── Populate final result fields ──────────────────────────

    const auto end = std::chrono::steady_clock::now();

    result.nodes   = nodes_searched;
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    return result;
}