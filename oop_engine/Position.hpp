#pragma once

#include "Move.hpp"   // pulls in Piece.hpp and sq namespace
#include "Piece.hpp"

#include <array>
#include <string>
#include <cstdint>
#include <cassert>

// ============================================================
//  Position.hpp
//  The complete, self-contained state of a chess game at one
//  moment in time.
//
//  Design principles:
//  - Position is a VALUE TYPE: copy it freely, compare cheaply.
//  - It is PURE DATA: no chess-rule logic lives here.
//    ChessBoard is the rules engine that reads and writes Positions.
//  - Copying a Position costs exactly sizeof(Position) bytes
//    (~80 bytes). Minimax can copy thousands per second with no
//    heap allocation.
//  - operator== is provided for threefold-repetition detection.
//  - FEN import/export is here because FEN describes a Position,
//    not a game or a rule.
// ============================================================

// ------------------------------------------------------------
//  CastlingRights
// ------------------------------------------------------------

struct CastlingRights
{
    bool white_kingside  = true;
    bool white_queenside = true;
    bool black_kingside  = true;
    bool black_queenside = true;

    // ── Queries ─────────────────────────────────────────────

    [[nodiscard]] constexpr bool can_castle_kingside(Color c) const noexcept
    {
        return c == Color::White ? white_kingside : black_kingside;
    }

    [[nodiscard]] constexpr bool can_castle_queenside(Color c) const noexcept
    {
        return c == Color::White ? white_queenside : black_queenside;
    }

    [[nodiscard]] constexpr bool any() const noexcept
    {
        return white_kingside || white_queenside ||
               black_kingside || black_queenside;
    }

    // ── Mutators ────────────────────────────────────────────

    // Removes all castling rights for a color (e.g. when king moves).
    constexpr void remove_all(Color c) noexcept
    {
        if (c == Color::White) { white_kingside = false; white_queenside = false; }
        else                   { black_kingside = false; black_queenside = false; }
    }

    // Removes kingside right only (e.g. when h-rook moves).
    constexpr void remove_kingside(Color c) noexcept
    {
        if (c == Color::White) white_kingside  = false;
        else                   black_kingside  = false;
    }

    // Removes queenside right only (e.g. when a-rook moves).
    constexpr void remove_queenside(Color c) noexcept
    {
        if (c == Color::White) white_queenside = false;
        else                   black_queenside = false;
    }

    // ── Comparison & serialization ───────────────────────────

    [[nodiscard]] constexpr bool operator==(const CastlingRights&) const noexcept = default;

    // Returns FEN castling field: "KQkq", "Kq", "-", etc.
    [[nodiscard]] std::string to_fen() const;
};

// ------------------------------------------------------------
//  Position
// ------------------------------------------------------------

class Position
{
public:
    // ── Board storage ────────────────────────────────────────

    // 64-element flat array indexed by sq::Square (rank*8 + file).
    // board_[sq::e4] is the piece on e4.
    std::array<Piece, 64> board = {};

    // ── Game state ───────────────────────────────────────────

    Color          side_to_move    = Color::White;
    CastlingRights castling        = {};
    sq::Square     en_passant_sq   = sq::NONE;  // Target sq. for en-passant captures,
                                                 // sq::NONE if not available this move.
    int            halfmove_clock  = 0;          // Plies since last pawn move or capture
                                                 // (for 50-move rule: draw at 100).
    int            fullmove_number = 1;          // Starts at 1, increments after Black moves.

    // ── Construction ────────────────────────────────────────

    // Default: empty board, White to move, all castling rights, no ep.
    Position() = default;

    // Returns the standard chess starting position.
    [[nodiscard]] static Position starting();

    // Parses a FEN string. Throws std::invalid_argument on bad input.
    [[nodiscard]] static Position from_fen(const std::string& fen);

    // ── Board accessors ──────────────────────────────────────

    [[nodiscard]] Piece& at(sq::Square s) noexcept
    {
        assert(sq::is_valid(s));
        return board[static_cast<size_t>(s)];
    }

    [[nodiscard]] const Piece& at(sq::Square s) const noexcept
    {
        assert(sq::is_valid(s));
        return board[static_cast<size_t>(s)];
    }

    [[nodiscard]] Piece& at(int rank, int file) noexcept
    {
        return at(sq::from_rf(rank, file));
    }

    [[nodiscard]] const Piece& at(int rank, int file) const noexcept
    {
        return at(sq::from_rf(rank, file));
    }

    [[nodiscard]] bool is_empty(sq::Square s) const noexcept
    {
        return at(s).is_empty();
    }

    [[nodiscard]] bool is_occupied(sq::Square s) const noexcept
    {
        return !at(s).is_empty();
    }

    // ── Side-to-move helpers ─────────────────────────────────

    [[nodiscard]] Color opponent() const noexcept
    {
        return opposite(side_to_move);
    }

    // ── Comparison (required for repetition detection) ───────

    // Two positions are equal if every field relevant to repetition
    // matches: board layout, side to move, castling rights, en-passant.
    // halfmove_clock and fullmove_number are intentionally excluded
    // (they do not affect whether a position is a repetition).
    [[nodiscard]] bool operator==(const Position& o) const noexcept;
    [[nodiscard]] bool operator!=(const Position& o) const noexcept;

    // ── Serialization ────────────────────────────────────────

    // Exports this position as a FEN string.
    [[nodiscard]] std::string to_fen() const;

    // Renders the board as a human-readable ASCII diagram.
    // White pieces are uppercase; Black pieces lowercase; empty = '.'.
    //
    //   8 | r n b q k b n r
    //   7 | p p p p p p p p
    //   ...
    //   1 | R N B Q K B N R
    //       a b c d e f g h
    //
    [[nodiscard]] std::string to_ascii() const;

    // ── Validation ───────────────────────────────────────────

    // Returns true if the position passes basic sanity checks:
    // exactly one king per color, pawns not on rank 1 or 8, etc.
    // Not exhaustive — use for debug/testing.
    [[nodiscard]] bool is_sane() const noexcept;
};

// ============================================================
//  Implementations
// ============================================================

inline bool Position::operator==(const Position& o) const noexcept
{
    return board          == o.board          &&
           side_to_move   == o.side_to_move   &&
           castling       == o.castling       &&
           en_passant_sq  == o.en_passant_sq;
    // halfmove_clock and fullmove_number deliberately excluded.
}

inline bool Position::operator!=(const Position& o) const noexcept
{
    return !(*this == o);
}

inline std::string CastlingRights::to_fen() const
{
    std::string r;
    if (white_kingside)  r += 'K';
    if (white_queenside) r += 'Q';
    if (black_kingside)  r += 'k';
    if (black_queenside) r += 'q';
    if (r.empty())       r = "-";
    return r;
}

inline Position Position::starting()
{
    // Standard starting position via FEN. This is the canonical
    // source of truth — the FEN is the spec.
    return Position::from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );
}

inline Position Position::from_fen(const std::string& fen)
{
    Position pos;

    // Field 1: piece placement (rank 8 first, rank 1 last)
    int rank = 7;
    int file = 0;
    size_t i = 0;

    for (; i < fen.size() && fen[i] != ' '; ++i)
    {
        char c = fen[i];
        if (c == '/')
        {
            --rank;
            file = 0;
        }
        else if (c >= '1' && c <= '8')
        {
            file += (c - '0');
        }
        else
        {
            if (rank < 0 || rank > 7 || file < 0 || file > 7)
                throw std::invalid_argument("FEN: board coordinates out of range");
            pos.at(rank, file) = Piece::from_fen_char(c);
            ++file;
        }
    }

    if (i >= fen.size())
        throw std::invalid_argument("FEN: missing fields");
    ++i; // skip space

    // Field 2: active color
    if (i >= fen.size())
        throw std::invalid_argument("FEN: missing active color");
    pos.side_to_move = (fen[i] == 'b') ? Color::Black : Color::White;
    i += 2; // char + space

    // Field 3: castling availability
    pos.castling = {false, false, false, false};
    for (; i < fen.size() && fen[i] != ' '; ++i)
    {
        switch (fen[i])
        {
            case 'K': pos.castling.white_kingside  = true; break;
            case 'Q': pos.castling.white_queenside = true; break;
            case 'k': pos.castling.black_kingside  = true; break;
            case 'q': pos.castling.black_queenside = true; break;
            case '-': break;
            default:  throw std::invalid_argument("FEN: invalid castling character");
        }
    }
    ++i; // skip space

    // Field 4: en passant target square
    if (i >= fen.size())
        throw std::invalid_argument("FEN: missing en passant field");
    if (fen[i] == '-')
    {
        pos.en_passant_sq = sq::NONE;
        i += 2;
    }
    else
    {
        std::string ep_str = fen.substr(i, 2);
        pos.en_passant_sq = sq::from_string(ep_str);
        i += 3;
    }

    // Field 5: halfmove clock
    pos.halfmove_clock = 0;
    while (i < fen.size() && fen[i] != ' ')
    {
        pos.halfmove_clock = pos.halfmove_clock * 10 + (fen[i] - '0');
        ++i;
    }
    ++i;

    // Field 6: fullmove number
    pos.fullmove_number = 0;
    while (i < fen.size() && fen[i] != ' ' && fen[i] != '\0')
    {
        pos.fullmove_number = pos.fullmove_number * 10 + (fen[i] - '0');
        ++i;
    }

    return pos;
}

inline std::string Position::to_fen() const
{
    std::string fen;

    // Field 1: piece placement (rank 8 first)
    for (int r = 7; r >= 0; --r)
    {
        int empty_count = 0;
        for (int f = 0; f < 8; ++f)
        {
            const Piece& p = at(r, f);
            if (p.is_empty())
            {
                ++empty_count;
            }
            else
            {
                if (empty_count > 0)
                {
                    fen += static_cast<char>('0' + empty_count);
                    empty_count = 0;
                }
                fen += p.to_fen_char();
            }
        }
        if (empty_count > 0)
            fen += static_cast<char>('0' + empty_count);
        if (r > 0) fen += '/';
    }

    // Field 2: active color
    fen += (side_to_move == Color::White) ? " w " : " b ";

    // Field 3: castling
    fen += castling.to_fen();
    fen += ' ';

    // Field 4: en passant
    fen += sq::to_string(en_passant_sq);
    fen += ' ';

    // Field 5: halfmove clock
    fen += std::to_string(halfmove_clock);
    fen += ' ';

    // Field 6: fullmove number
    fen += std::to_string(fullmove_number);

    return fen;
}

inline std::string Position::to_ascii() const
{
    std::string out;
    for (int r = 7; r >= 0; --r)
    {
        out += static_cast<char>('1' + r);
        out += " | ";
        for (int f = 0; f < 8; ++f)
        {
            out += at(r, f).to_char();
            out += ' ';
        }
        out += '\n';
    }
    out += "    a b c d e f g h\n";
    out += "Side to move: ";
    out += color_name(side_to_move);
    out += '\n';
    return out;
}

inline bool Position::is_sane() const noexcept
{
    int white_kings = 0, black_kings = 0;
    for (int s = 0; s < 64; ++s)
    {
        const Piece& p = board[static_cast<size_t>(s)];
        if (p.is_empty()) continue;

        if (p.type == PieceType::King)
        {
            if (p.color == Color::White) ++white_kings;
            else                         ++black_kings;
        }

        // Pawns may not stand on rank 1 or rank 8
        if (p.type == PieceType::Pawn)
        {
            int r = sq::rank_of(s);
            if (r == 0 || r == 7) return false;
        }
    }

    return white_kings == 1 && black_kings == 1;
}