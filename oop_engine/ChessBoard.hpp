#pragma once

#include "Position.hpp"
#include "Move.hpp"
#include "Piece.hpp"

#include <vector>
#include <optional>

// ============================================================
//  ChessBoard.hpp
//  The rules engine. Knows every chess rule; owns the current
//  Position; produces and validates moves.
//
//  Design principles:
//  - ChessBoard wraps ONE Position. Everything else is derived.
//  - Two apply flavors:
//      apply_move()         → returns a NEW Position (non-mutating).
//                             Used by minimax: copy-and-recurse,
//                             no undo needed.
//      apply_move_inplace() → mutates the internal Position.
//                             Used by Game for actual play.
//  - legal_moves() is the authoritative source of all moves the
//    current side may make. No rule logic leaks into Game or AI.
//  - is_attacked() and is_in_check() are the only two primitives
//    needed for check detection. Everything else builds on them.
//  - Move generation (pseudo-legal expansion) is declared but
//    bodies are left for a separate implementation file.
//    All PUBLIC APIs are fully defined here.
// ============================================================

class ChessBoard
{
public:
    // ── Construction ────────────────────────────────────────

    // Starts from the standard chess starting position.
    ChessBoard();

    // Starts from a given position (useful for testing and AI search).
    explicit ChessBoard(const Position& pos);
    explicit ChessBoard(Position&& pos);

    // ── Position access ──────────────────────────────────────

    [[nodiscard]] const Position& position() const noexcept { return pos_; }

    // Direct board reads (forwarded for convenience)
    [[nodiscard]] const Piece& piece_at(sq::Square s)        const noexcept { return pos_.at(s); }
    [[nodiscard]] Color        side_to_move()                 const noexcept { return pos_.side_to_move; }
    [[nodiscard]] bool         is_empty_sq(sq::Square s)      const noexcept { return pos_.is_empty(s); }

    // ── Move generation ──────────────────────────────────────

    // Returns all pseudo-legal moves for the side to move.
    // Pseudo-legal = correct piece movement, but may leave king in check.
    // You almost never need this directly; prefer legal_moves().
    [[nodiscard]] std::vector<Move> pseudo_legal_moves() const;

    // Returns only the pseudo-legal moves from a specific square.
    [[nodiscard]] std::vector<Move> pseudo_legal_moves_from(sq::Square from) const;

    // Returns all fully legal moves (pseudo-legal filtered for self-check).
    // This is the primary interface for Game and AI.
    [[nodiscard]] std::vector<Move> legal_moves() const;

    // Returns legal moves from a single square (for UI: click-to-move).
    [[nodiscard]] std::vector<Move> legal_moves_from(sq::Square from) const;

    // Returns true if the given move is in legal_moves().
    [[nodiscard]] bool is_legal(const Move& m) const;

    // ── Check and attack detection ───────────────────────────

    // Is the given color's king currently in check?
    [[nodiscard]] bool is_in_check(Color color) const;

    // Convenience: is the side-to-move's king in check?
    [[nodiscard]] bool is_in_check() const { return is_in_check(pos_.side_to_move); }

    // Is square 's' attacked by any piece of color 'by'?
    // The foundation of all check/pin detection.
    [[nodiscard]] bool is_attacked(sq::Square s, Color by) const;

    // ── Game-over detection ──────────────────────────────────

    // True if side to move has no legal moves AND is in check.
    [[nodiscard]] bool is_checkmate() const;

    // True if side to move has no legal moves AND is NOT in check.
    [[nodiscard]] bool is_stalemate() const;

    // True if neither side has sufficient material to mate.
    // Covers: K vs K, K+B vs K, K+N vs K, K+B vs K+B same color.
    [[nodiscard]] bool is_insufficient_material() const;

    // ── Move application ─────────────────────────────────────

    // NON-MUTATING: applies the move to the internal position and
    // returns the resulting Position. *this is unchanged.
    // This is what minimax uses:
    //   Position next = board.apply_move(m);
    //   ChessBoard child(next);
    //   int score = minimax(child, depth - 1, ...);
    [[nodiscard]] Position apply_move(const Move& m) const;

    // MUTATING: applies the move in-place, updating pos_.
    // Used by Game::make_move() during actual play.
    void apply_move_inplace(const Move& m);

    // Replaces the current position entirely (used by Game::undo_move).
    // Restoring a saved Position is always O(sizeof(Position)).
    void restore(const Position& saved);
    void restore(Position&& saved);

    // ── Debug / display ──────────────────────────────────────

    [[nodiscard]] std::string to_ascii() const { return pos_.to_ascii(); }
    [[nodiscard]] std::string to_fen()   const { return pos_.to_fen();   }

private:
    Position pos_;

    // ── Internal helpers ─────────────────────────────────────
    // These populate 'out' rather than returning a vector
    // to allow the caller to reserve capacity once.

    void gen_pawn_moves   (sq::Square from, std::vector<Move>& out) const;
    void gen_knight_moves (sq::Square from, std::vector<Move>& out) const;
    void gen_bishop_moves (sq::Square from, std::vector<Move>& out) const;
    void gen_rook_moves   (sq::Square from, std::vector<Move>& out) const;
    void gen_queen_moves  (sq::Square from, std::vector<Move>& out) const;
    void gen_king_moves   (sq::Square from, std::vector<Move>& out) const;

    // Shared helper for bishop/rook/queen rays.
    void gen_sliding_moves(sq::Square from, const int* dirs, int dir_count,
                           std::vector<Move>& out) const;

    // Finds the square of the king of the given color.
    // Returns sq::NONE if the king is absent (invalid position).
    [[nodiscard]] sq::Square find_king(Color color) const noexcept;

    // Core of apply_move logic, shared by both apply variants.
    [[nodiscard]] Position apply_move_to(const Position& base, const Move& m) const;
};

// ============================================================
//  Inline implementations of non-generation methods
//  (Generation stubs go in ChessBoard.cpp)
// ============================================================

inline ChessBoard::ChessBoard()
    : pos_(Position::starting())
{}

inline ChessBoard::ChessBoard(const Position& pos)
    : pos_(pos)
{}

inline ChessBoard::ChessBoard(Position&& pos)
    : pos_(std::move(pos))
{}

inline void ChessBoard::restore(const Position& saved)
{
    pos_ = saved;
}

inline void ChessBoard::restore(Position&& saved)
{
    pos_ = std::move(saved);
}

inline Position ChessBoard::apply_move(const Move& m) const
{
    return apply_move_to(pos_, m);
}

inline void ChessBoard::apply_move_inplace(const Move& m)
{
    pos_ = apply_move_to(pos_, m);
}

inline bool ChessBoard::is_in_check(Color color) const
{
    sq::Square king_sq = find_king(color);
    if (!sq::is_valid(king_sq)) return false; // malformed position
    return is_attacked(king_sq, opposite(color));
}

inline bool ChessBoard::is_checkmate() const
{
    return legal_moves().empty() && is_in_check();
}

inline bool ChessBoard::is_stalemate() const
{
    return legal_moves().empty() && !is_in_check();
}

inline bool ChessBoard::is_legal(const Move& m) const
{
    // A move is legal iff it appears in legal_moves().
    // For a production engine you would do this without generating
    // the full list, but this is correct and easy to maintain.
    for (const Move& legal : legal_moves())
        if (legal == m) return true;
    return false;
}

inline std::vector<Move> ChessBoard::legal_moves_from(sq::Square from) const
{
    std::vector<Move> result;
    for (Move& m : legal_moves())
        if (m.from == from) result.push_back(m);
    return result;
}

// legal_moves() filters pseudo-legal moves by simulating each and
// checking whether the moving side's king is left in check.
inline std::vector<Move> ChessBoard::legal_moves() const
{
    std::vector<Move> candidates = pseudo_legal_moves();
    std::vector<Move> legal;
    legal.reserve(candidates.size());

    for (const Move& m : candidates)
    {
        Position next = apply_move_to(pos_, m);
        ChessBoard child(next);
        // The move is legal only if it does NOT leave OUR king in check.
        if (!child.is_in_check(pos_.side_to_move))
            legal.push_back(m);
    }

    return legal;
}

inline sq::Square ChessBoard::find_king(Color color) const noexcept
{
    for (int s = 0; s < 64; ++s)
    {
        const Piece& p = pos_.board[static_cast<size_t>(s)];
        if (p.type == PieceType::King && p.color == color)
            return static_cast<sq::Square>(s);
    }
    return sq::NONE;
}