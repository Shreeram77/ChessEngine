#pragma once

#include "ChessBoard.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "Piece.hpp"

#include <vector>
#include <string>
#include <optional>
#include <stdexcept>

// ============================================================
//  Game.hpp
//  Session manager. Sits above ChessBoard and owns everything
//  that spans multiple moves: history, draw rules, the move log,
//  and game-over state.
//
//  Design principles:
//  - Game delegates ALL rules to ChessBoard. It never checks
//    move legality itself — it asks ChessBoard.
//  - Undo is a single position restore: pop history, call
//    board_.restore(). Zero special-case logic.
//  - History stores Position objects (not moves), so undo is
//    O(sizeof(Position)) regardless of what the move did.
//  - Draw detection (repetition, 50-move, insufficient material)
//    lives here because it requires full history and game context,
//    not just the current board state.
//  - Game is completely independent of SFML, Qt, or any UI.
//    The UI reads Game::board() and calls Game::make_move().
// ============================================================

// ------------------------------------------------------------
//  GameResult
// ------------------------------------------------------------

enum class GameResult
{
    Ongoing,
    WhiteWins,     // by checkmate
    BlackWins,     // by checkmate
    DrawStalemate,
    DrawRepetition,
    DrawFiftyMoves,
    DrawInsufficientMaterial,
    DrawAgreement  // set externally (e.g. players agree)
};

[[nodiscard]] inline bool is_draw(GameResult r) noexcept
{
    return r == GameResult::DrawStalemate           ||
           r == GameResult::DrawRepetition          ||
           r == GameResult::DrawFiftyMoves          ||
           r == GameResult::DrawInsufficientMaterial||
           r == GameResult::DrawAgreement;
}

[[nodiscard]] inline bool is_over(GameResult r) noexcept
{
    return r != GameResult::Ongoing;
}

[[nodiscard]] const char* game_result_string(GameResult r) noexcept;

// ------------------------------------------------------------
//  MoveRecord
// ------------------------------------------------------------

// Stored in the move log for display and export.
// Separate from Move to allow annotation / SAN string caching.
struct MoveRecord
{
    Move        move;
    std::string uci;         // "e2e4"
    std::string san;         // "e4" or "Nf3" — filled in later by a SAN generator
    int         fullmove;    // fullmove number at time of move
    Color       side;        // which side made this move
};

// ------------------------------------------------------------
//  Game
// ------------------------------------------------------------

class Game
{
public:
    // ── Construction ────────────────────────────────────────

    // Standard starting position.
    Game();

    // Custom starting position (testing, puzzles, analysis).
    explicit Game(const Position& start);
    explicit Game(const std::string& fen);

    // ── State queries ────────────────────────────────────────

    [[nodiscard]] const ChessBoard&  board()           const noexcept { return board_; }
    [[nodiscard]] const Position&    position()        const noexcept { return board_.position(); }
    [[nodiscard]] GameResult         result()          const noexcept { return result_; }
    [[nodiscard]] bool               is_ongoing()      const noexcept { return result_ == GameResult::Ongoing; }
    [[nodiscard]] bool               is_over()         const noexcept { return ::is_over(result_); }
    [[nodiscard]] Color              side_to_move()    const noexcept { return board_.side_to_move(); }

    // Number of half-moves (plies) played so far.
    [[nodiscard]] int  ply_count()   const noexcept { return static_cast<int>(move_log_.size()); }

    // Current fullmove number (1-based, increments after Black plays).
    [[nodiscard]] int  move_number() const noexcept { return board_.position().fullmove_number; }

    [[nodiscard]] bool can_undo()    const noexcept { return !history_.empty(); }

    // All legal moves in the current position.
    [[nodiscard]] std::vector<Move> legal_moves() const { return board_.legal_moves(); }

    // Legal moves from a specific square (for click-to-move UI).
    [[nodiscard]] std::vector<Move> legal_moves_from(sq::Square s) const
    {
        return board_.legal_moves_from(s);
    }

    // ── Actions ──────────────────────────────────────────────

    // Attempts to make a move.
    // Returns false (no state change) if:
    //   - the game is already over, OR
    //   - the move is not in legal_moves().
    // Returns true and advances the game state on success.
    bool make_move(const Move& m);

    // Convenience overload: specify move by from/to squares and
    // optional promotion piece type (for UI drag-and-drop).
    // Finds the matching legal move automatically.
    // Returns false if no legal move matches.
    bool make_move(sq::Square from, sq::Square to,
                   PieceType promotion = PieceType::Queen);

    // Undoes the last move.
    // Returns false if there is nothing to undo.
    bool undo_move();

    // External draw offer accepted (e.g. both players agree).
    void declare_draw();

    // ── History and log ──────────────────────────────────────

    // The full position history: history_[0] is before move 1,
    // history_.back() is the position before the latest move.
    [[nodiscard]] const std::vector<Position>&    position_history() const noexcept { return history_; }

    // The ordered list of moves made so far.
    [[nodiscard]] const std::vector<MoveRecord>&  move_log()         const noexcept { return move_log_; }

    // The position at a specific ply (0 = starting position).
    // Throws std::out_of_range for invalid indices.
    [[nodiscard]] const Position& position_at_ply(int ply) const;

    // Exports the game as a PGN string (minimal: no headers, moves only).
    [[nodiscard]] std::string to_pgn() const;

    // ── Draw detection queries (public for AI / UI) ──────────

    // How many times has the current position appeared in history?
    // (Includes the current position as count 1.)
    [[nodiscard]] int  repetition_count() const;

    [[nodiscard]] bool is_draw_by_repetition()            const;  // 3 appearances
    [[nodiscard]] bool is_draw_by_fifty_moves()           const;  // halfmove_clock >= 100
    [[nodiscard]] bool is_draw_by_insufficient_material() const;

private:
    ChessBoard            board_;
    std::vector<Position> history_;    // position BEFORE each move; same size as move_log_
    std::vector<MoveRecord> move_log_; // all moves made so far, in order
    GameResult            result_ = GameResult::Ongoing;

    // Called after every successful make_move() to determine if
    // the game has ended by any rule.
    void update_result();
};

// ============================================================
//  Inline implementations
// ============================================================

inline Game::Game()
    : board_()
{
    update_result();
}

inline Game::Game(const Position& start)
    : board_(start)
{
    update_result();
}

inline Game::Game(const std::string& fen)
    : board_(Position::from_fen(fen))
{
    update_result();
}

inline bool Game::make_move(const Move& m)
{
    if (is_over()) return false;
    if (!board_.is_legal(m)) return false;

    // Save current position to history BEFORE applying the move.
    history_.push_back(board_.position());

    // Apply the move.
    board_.apply_move_inplace(m);

    // Record the move.
    MoveRecord rec;
    rec.move      = m;
    rec.uci       = m.to_uci();
    rec.san       = "";           // filled later by a SAN generator
    rec.fullmove  = board_.position().fullmove_number;
    rec.side      = history_.back().side_to_move; // side that just moved
    move_log_.push_back(rec);

    update_result();
    return true;
}

inline bool Game::make_move(sq::Square from, sq::Square to, PieceType promotion)
{
    for (const Move& m : board_.legal_moves_from(from))
    {
        if (m.to != to) continue;

        // For promotions, match the requested promotion piece.
        if (m.is_promotion())
        {
            if (m.promotion_piece() == promotion)
                return make_move(m);
            // Skip non-matching promotion variants.
            continue;
        }

        return make_move(m);
    }
    return false;
}

inline bool Game::undo_move()
{
    if (!can_undo()) return false;

    // Restore the saved position.
    board_.restore(history_.back());

    history_.pop_back();
    move_log_.pop_back();
    result_ = GameResult::Ongoing;
    return true;
}

inline void Game::declare_draw()
{
    result_ = GameResult::DrawAgreement;
}

inline const Position& Game::position_at_ply(int ply) const
{
    // ply 0 = starting position = history_[0] (if it exists)
    if (ply < 0 || ply > static_cast<int>(history_.size()))
        throw std::out_of_range("Game::position_at_ply: index out of range");

    if (ply == static_cast<int>(history_.size()))
        return board_.position(); // current position

    return history_[static_cast<size_t>(ply)];
}

inline void Game::update_result()
{
    // Draw checks come before checkmate/stalemate so the more
    // specific draw reason is reported.
    if (is_draw_by_fifty_moves())
    {
        result_ = GameResult::DrawFiftyMoves;
        return;
    }
    if (is_draw_by_repetition())
    {
        result_ = GameResult::DrawRepetition;
        return;
    }
    if (is_draw_by_insufficient_material())
    {
        result_ = GameResult::DrawInsufficientMaterial;
        return;
    }
    if (board_.is_checkmate())
    {
        // The side that just moved wins; that's the opponent of side_to_move.
        result_ = (board_.side_to_move() == Color::White)
                  ? GameResult::BlackWins
                  : GameResult::WhiteWins;
        return;
    }
    if (board_.is_stalemate())
    {
        result_ = GameResult::DrawStalemate;
        return;
    }

    result_ = GameResult::Ongoing;
}

inline bool Game::is_draw_by_fifty_moves() const
{
    return board_.position().halfmove_clock >= 100;
}

inline bool Game::is_draw_by_repetition() const
{
    return repetition_count() >= 3;
}

inline int Game::repetition_count() const
{
    const Position& current = board_.position();
    int count = 1; // the current position itself
    for (const Position& p : history_)
        if (p == current) ++count;
    return count;
}

inline bool Game::is_draw_by_insufficient_material() const
{
    return board_.is_insufficient_material();
}

inline std::string Game::to_pgn() const
{
    std::string pgn;
    for (size_t i = 0; i < move_log_.size(); ++i)
    {
        const MoveRecord& rec = move_log_[i];
        if (rec.side == Color::White)
        {
            pgn += std::to_string(rec.fullmove);
            pgn += ". ";
        }
        pgn += rec.san.empty() ? rec.uci : rec.san;
        pgn += ' ';
    }

    switch (result_)
    {
        case GameResult::WhiteWins:  pgn += "1-0";     break;
        case GameResult::BlackWins:  pgn += "0-1";     break;
        case GameResult::Ongoing:    pgn += "*";       break;
        default:                     pgn += "1/2-1/2"; break;
    }

    return pgn;
}

inline const char* game_result_string(GameResult r) noexcept
{
    switch (r)
    {
        case GameResult::Ongoing:                    return "Ongoing";
        case GameResult::WhiteWins:                  return "White wins by checkmate";
        case GameResult::BlackWins:                  return "Black wins by checkmate";
        case GameResult::DrawStalemate:              return "Draw by stalemate";
        case GameResult::DrawRepetition:             return "Draw by threefold repetition";
        case GameResult::DrawFiftyMoves:             return "Draw by fifty-move rule";
        case GameResult::DrawInsufficientMaterial:   return "Draw by insufficient material";
        case GameResult::DrawAgreement:              return "Draw by agreement";
        default:                                     return "Unknown";
    }
}