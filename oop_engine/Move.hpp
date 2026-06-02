#pragma once

#include "Piece.hpp"

#include <cstdint>
#include <string>
#include <cassert>

// ============================================================
//  Move.hpp
//  Two concerns live here:
//    1. Square  — a lightweight board coordinate (integer 0-63).
//    2. Move    — a complete, self-describing move record.
//
//  Design notes:
//  - Square is a plain integer alias, NOT a class. No overhead,
//    no heap, works naturally as an array index.
//  - Move is a plain struct. It stores everything needed to
//    apply or display the move. ChessBoard reads it; Game logs it.
//  - Storing captured piece + special flags means apply_move()
//    needs no extra lookups, and undo needs no reverse-engineering.
//  - MoveFlag encodes every special case as a single enum value.
//    Promotion variants are explicit (not a separate field) to
//    keep the struct small and the switch statements exhaustive.
// ============================================================

// ============================================================
//  Square
// ============================================================

namespace sq
{
    // A square is a plain integer 0-63.
    // Layout: index = rank * 8 + file
    //   rank 0 = rank 1 (white's back rank)
    //   file 0 = file a
    //
    // Example: e4 → rank=3, file=4 → index=28
    using Square = int;

    constexpr Square NONE = -1;      // Sentinel: no square (e.g. no en-passant target)
    constexpr int    SIZE = 64;

    // ── Coordinate conversions ───────────────────────────────

    [[nodiscard]] constexpr Square from_rf(int rank, int file) noexcept
    {
        return rank * 8 + file;
    }

    [[nodiscard]] constexpr int rank_of(Square s) noexcept { return s / 8; }
    [[nodiscard]] constexpr int file_of(Square s) noexcept { return s % 8; }

    [[nodiscard]] constexpr bool is_valid(Square s) noexcept
    {
        return s >= 0 && s < SIZE;
    }

    // ── Named squares (algebraic notation, rank-1 = index 0) ─

    // Rank 1 (White's back rank)
    constexpr Square a1=0,  b1=1,  c1=2,  d1=3,
                     e1=4,  f1=5,  g1=6,  h1=7;
    // Rank 2
    constexpr Square a2=8,  b2=9,  c2=10, d2=11,
                     e2=12, f2=13, g2=14, h2=15;
    // Rank 7
    constexpr Square a7=48, b7=49, c7=50, d7=51,
                     e7=52, f7=53, g7=54, h7=55;
    // Rank 8 (Black's back rank)
    constexpr Square a8=56, b8=57, c8=58, d8=59,
                     e8=60, f8=61, g8=62, h8=63;

    // ── Direction offsets (add to a square index) ────────────

    constexpr int NORTH      =  8;
    constexpr int SOUTH      = -8;
    constexpr int EAST       =  1;
    constexpr int WEST       = -1;
    constexpr int NORTH_EAST =  9;
    constexpr int NORTH_WEST =  7;
    constexpr int SOUTH_EAST = -7;
    constexpr int SOUTH_WEST = -9;

    // ── Algebraic string conversion ──────────────────────────

    // Returns "a1".."h8", or "--" for NONE.
    [[nodiscard]] std::string to_string(Square s);

    // Parses "a1".."h8". Returns NONE on bad input.
    [[nodiscard]] Square from_string(const std::string& s) noexcept;

} // namespace sq

// ============================================================
//  MoveFlag
// ============================================================

// Every special move case is its own flag value.
// One enum field covers all of: normal, double push, en passant,
// both castling directions, and all four promotion choices.
enum class MoveFlag : uint8_t
{
    Normal          = 0,   // Quiet move or ordinary capture
    DoublePawnPush  = 1,   // Pawn advances two squares (sets en-passant target)
    EnPassant       = 2,   // Pawn captures en passant (target square is empty)
    CastleKingside  = 3,   // King castles kingside (rook move is implicit)
    CastleQueenside = 4,   // King castles queenside (rook move is implicit)
    PromoteQueen    = 5,
    PromoteRook     = 6,
    PromoteBishop   = 7,
    PromoteKnight   = 8
};

// ============================================================
//  Move
// ============================================================

struct Move
{
    sq::Square from     = sq::NONE;
    sq::Square to       = sq::NONE;

    // The piece that is physically moving.
    // Stored explicitly so apply_move() never has to re-read the board.
    Piece moving = Piece::empty();

    // The piece on 'to' before the move (empty if quiet / en passant).
    // En-passant captures store the captured pawn in 'captured',
    // even though the captured square differs from 'to'.
    Piece captured = Piece::empty();

    MoveFlag flag = MoveFlag::Normal;

    // ── Factories ───────────────────────────────────────────

    // Quiet (non-capture) move
    [[nodiscard]] static Move quiet(sq::Square from, sq::Square to,
                                    Piece moving,
                                    MoveFlag flag = MoveFlag::Normal) noexcept
    {
        return Move{from, to, moving, Piece::empty(), flag};
    }

    // Capture move
    [[nodiscard]] static Move capture(sq::Square from, sq::Square to,
                                      Piece moving, Piece captured,
                                      MoveFlag flag = MoveFlag::Normal) noexcept
    {
        return Move{from, to, moving, captured, flag};
    }

    // En-passant capture
    [[nodiscard]] static Move en_passant(sq::Square from, sq::Square to,
                                         Piece pawn, Piece captured_pawn) noexcept
    {
        return Move{from, to, pawn, captured_pawn, MoveFlag::EnPassant};
    }

    // Castling (rook move is derived from flag by apply_move)
    [[nodiscard]] static Move castle(sq::Square king_from, sq::Square king_to,
                                     Piece king, bool kingside) noexcept
    {
        MoveFlag f = kingside ? MoveFlag::CastleKingside : MoveFlag::CastleQueenside;
        return Move{king_from, king_to, king, Piece::empty(), f};
    }

    // ── Queries ─────────────────────────────────────────────

    [[nodiscard]] constexpr bool is_valid()     const noexcept
    {
        return sq::is_valid(from) && sq::is_valid(to);
    }

    [[nodiscard]] constexpr bool is_capture()   const noexcept
    {
        return !captured.is_empty();
    }

    [[nodiscard]] constexpr bool is_en_passant() const noexcept
    {
        return flag == MoveFlag::EnPassant;
    }

    [[nodiscard]] constexpr bool is_castle()    const noexcept
    {
        return flag == MoveFlag::CastleKingside ||
               flag == MoveFlag::CastleQueenside;
    }

    [[nodiscard]] constexpr bool is_promotion() const noexcept
    {
        return flag == MoveFlag::PromoteQueen  ||
               flag == MoveFlag::PromoteRook   ||
               flag == MoveFlag::PromoteBishop ||
               flag == MoveFlag::PromoteKnight;
    }

    [[nodiscard]] constexpr bool is_double_push() const noexcept
    {
        return flag == MoveFlag::DoublePawnPush;
    }

    // Returns the piece type the pawn promotes to.
    // Asserts is_promotion() is true.
    [[nodiscard]] constexpr PieceType promotion_piece() const noexcept
    {
        switch (flag)
        {
            case MoveFlag::PromoteQueen:  return PieceType::Queen;
            case MoveFlag::PromoteRook:   return PieceType::Rook;
            case MoveFlag::PromoteBishop: return PieceType::Bishop;
            case MoveFlag::PromoteKnight: return PieceType::Knight;
            default:
                assert(false && "promotion_piece() called on non-promotion move");
                return PieceType::None;
        }
    }

    // ── Comparison ──────────────────────────────────────────

    [[nodiscard]] constexpr bool operator==(const Move& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(const Move& o) const noexcept = default;

    // ── Display ─────────────────────────────────────────────

    // Long algebraic notation: "e2e4", "e7e8q" for promotion.
    [[nodiscard]] std::string to_uci() const;

    // Short human-readable form for logs: "Pe2-e4", "Re1xh1", "O-O".
    [[nodiscard]] std::string to_string() const;
};

// ============================================================
//  Inline implementations
// ============================================================

namespace sq
{
    inline std::string to_string(Square s)
    {
        if (s == NONE) return "--";
        assert(is_valid(s));
        std::string r;
        r += static_cast<char>('a' + file_of(s));
        r += static_cast<char>('1' + rank_of(s));
        return r;
    }

    inline Square from_string(const std::string& s) noexcept
    {
        if (s.size() < 2) return NONE;
        int file = s[0] - 'a';
        int rank = s[1] - '1';
        if (file < 0 || file > 7 || rank < 0 || rank > 7) return NONE;
        return from_rf(rank, file);
    }
}

inline std::string Move::to_uci() const
{
    if (!is_valid()) return "0000";
    std::string r = sq::to_string(from) + sq::to_string(to);
    if (is_promotion())
    {
        char promo = '?';
        switch (flag)
        {
            case MoveFlag::PromoteQueen:  promo = 'q'; break;
            case MoveFlag::PromoteRook:   promo = 'r'; break;
            case MoveFlag::PromoteBishop: promo = 'b'; break;
            case MoveFlag::PromoteKnight: promo = 'n'; break;
            default: break;
        }
        r += promo;
    }
    return r;
}

inline std::string Move::to_string() const
{
    if (!is_valid()) return "<invalid>";

    if (flag == MoveFlag::CastleKingside)  return "O-O";
    if (flag == MoveFlag::CastleQueenside) return "O-O-O";

    std::string r;
    r += moving.to_char();
    r += sq::to_string(from);
    r += is_capture() ? 'x' : '-';
    r += sq::to_string(to);

    if (is_promotion())
    {
        r += '=';
        r += Piece::make(moving.color, promotion_piece()).to_char();
    }
    return r;
}