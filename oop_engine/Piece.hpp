#pragma once

#include <cstdint>
#include <string>
#include <cassert>

// ============================================================
//  Piece.hpp
//  Defines the atomic unit of the board: a colored piece type.
//
//  Design notes:
//  - Piece is a trivial struct (two enums). Zero heap, no vtable.
//  - PieceType::None + Color::None = empty square.
//  - Fits in 2 bytes; Position can store 64 of them cheaply.
//  - No game logic here — Piece knows nothing about moves or rules.
// ============================================================

// ------------------------------------------------------------
//  Enumerations
// ------------------------------------------------------------

enum class PieceType : uint8_t
{
    None   = 0,
    Pawn   = 1,
    Knight = 2,
    Bishop = 3,
    Rook   = 4,
    Queen  = 5,
    King   = 6
};

enum class Color : uint8_t
{
    None  = 0,
    White = 1,
    Black = 2
};

// ------------------------------------------------------------
//  Piece
// ------------------------------------------------------------

struct Piece
{
    PieceType type  = PieceType::None;
    Color     color = Color::None;

    // ── Factories ───────────────────────────────────────────

    static constexpr Piece make(Color c, PieceType t) noexcept
    {
        return Piece{t, c};
    }

    static constexpr Piece empty() noexcept
    {
        return Piece{};
    }

    // ── Queries ─────────────────────────────────────────────

    [[nodiscard]] constexpr bool is_empty()  const noexcept { return type == PieceType::None; }
    [[nodiscard]] constexpr bool is_white()  const noexcept { return color == Color::White; }
    [[nodiscard]] constexpr bool is_black()  const noexcept { return color == Color::Black; }
    [[nodiscard]] constexpr bool is_pawn()   const noexcept { return type == PieceType::Pawn; }
    [[nodiscard]] constexpr bool is_knight() const noexcept { return type == PieceType::Knight; }
    [[nodiscard]] constexpr bool is_bishop() const noexcept { return type == PieceType::Bishop; }
    [[nodiscard]] constexpr bool is_rook()   const noexcept { return type == PieceType::Rook; }
    [[nodiscard]] constexpr bool is_queen()  const noexcept { return type == PieceType::Queen; }
    [[nodiscard]] constexpr bool is_king()   const noexcept { return type == PieceType::King; }

    [[nodiscard]] constexpr bool is_sliding() const noexcept
    {
        return type == PieceType::Bishop ||
               type == PieceType::Rook   ||
               type == PieceType::Queen;
    }

    // ── Comparison ──────────────────────────────────────────

    [[nodiscard]] constexpr bool operator==(const Piece& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(const Piece& o) const noexcept = default;

    // ── Debug / display ─────────────────────────────────────

    // Returns single character: uppercase = White, lowercase = Black.
    // e.g. White Queen → 'Q', Black pawn → 'p', empty → '.'
    [[nodiscard]] char to_char() const noexcept;

    // Returns FEN character (same convention as to_char).
    [[nodiscard]] char to_fen_char() const noexcept;

    // Constructs a Piece from a FEN character.
    // Returns Piece::empty() for unrecognised input.
    [[nodiscard]] static Piece from_fen_char(char c) noexcept;
};

// ------------------------------------------------------------
//  Free helpers
// ------------------------------------------------------------

// Returns the opposite color. Undefined for Color::None.
[[nodiscard]] constexpr Color opposite(Color c) noexcept
{
    assert(c != Color::None);
    return c == Color::White ? Color::Black : Color::White;
}

// Human-readable name for display and debugging.
[[nodiscard]] const char* piece_type_name(PieceType t) noexcept;
[[nodiscard]] const char* color_name(Color c)          noexcept;

// ------------------------------------------------------------
//  Inline / constexpr implementations
// ------------------------------------------------------------

inline char Piece::to_char() const noexcept
{
    if (is_empty()) return '.';

    char base = '?';
    switch (type)
    {
        case PieceType::Pawn:   base = 'P'; break;
        case PieceType::Knight: base = 'N'; break;
        case PieceType::Bishop: base = 'B'; break;
        case PieceType::Rook:   base = 'R'; break;
        case PieceType::Queen:  base = 'Q'; break;
        case PieceType::King:   base = 'K'; break;
        default:                base = '?'; break;
    }

    return (color == Color::Black) ? static_cast<char>(base + 32) : base;
}

inline char Piece::to_fen_char() const noexcept
{
    return to_char(); // FEN uses the same convention
}

inline Piece Piece::from_fen_char(char c) noexcept
{
    Color color = (c >= 'a' && c <= 'z') ? Color::Black : Color::White;

    char upper = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c;

    PieceType type = PieceType::None;
    switch (upper)
    {
        case 'P': type = PieceType::Pawn;   break;
        case 'N': type = PieceType::Knight; break;
        case 'B': type = PieceType::Bishop; break;
        case 'R': type = PieceType::Rook;   break;
        case 'Q': type = PieceType::Queen;  break;
        case 'K': type = PieceType::King;   break;
        default:  return Piece::empty();
    }

    return Piece::make(color, type);
}

inline const char* piece_type_name(PieceType t) noexcept
{
    switch (t)
    {
        case PieceType::None:   return "None";
        case PieceType::Pawn:   return "Pawn";
        case PieceType::Knight: return "Knight";
        case PieceType::Bishop: return "Bishop";
        case PieceType::Rook:   return "Rook";
        case PieceType::Queen:  return "Queen";
        case PieceType::King:   return "King";
        default:                return "Unknown";
    }
}

inline const char* color_name(Color c) noexcept
{
    switch (c)
    {
        case Color::None:  return "None";
        case Color::White: return "White";
        case Color::Black: return "Black";
        default:           return "Unknown";
    }
}