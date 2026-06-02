#include "ChessBoard.hpp"

#include <cassert>

// ============================================================
//  ChessBoard.cpp
//  Implements all rule logic: move generation, attack detection,
//  move application, and draw-by-insufficient-material.
// ============================================================

// ── is_attacked ─────────────────────────────────────────────

bool ChessBoard::is_attacked(sq::Square s, Color by) const
{
    // We reverse the attack: from square s, pretend we are each
    // piece type and see if we land on an enemy of that type.
    // This avoids iterating over all pieces.

    const Color opp = by;

    // ── Pawns ────────────────────────────────────────────────
    // Pawns attack diagonally forward (from their own perspective).
    // If `by` is White, white pawns attack upward, so an attacker on
    // (rank-1, file±1) would be a white pawn hitting s from below.
    {
        int rank = sq::rank_of(s);
        int file = sq::file_of(s);
        int pawn_rank = (opp == Color::White) ? rank - 1 : rank + 1;
        if (pawn_rank >= 0 && pawn_rank <= 7)
        {
            for (int df : {-1, 1})
            {
                int pf = file + df;
                if (pf >= 0 && pf <= 7)
                {
                    sq::Square ps = sq::from_rf(pawn_rank, pf);
                    const Piece& p = pos_.at(ps);
                    if (p.type == PieceType::Pawn && p.color == opp)
                        return true;
                }
            }
        }
    }

    // ── Knights ──────────────────────────────────────────────
    {
        static const int knight_offsets[8][2] = {
            {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
        };
        int rank = sq::rank_of(s);
        int file = sq::file_of(s);
        for (auto& off : knight_offsets)
        {
            int r = rank + off[0], f = file + off[1];
            if (r < 0 || r > 7 || f < 0 || f > 7) continue;
            const Piece& p = pos_.at(sq::from_rf(r, f));
            if (p.type == PieceType::Knight && p.color == opp)
                return true;
        }
    }

    // ── Sliding pieces (bishop / rook / queen) ────────────────
    {
        // Diagonal directions → bishop/queen
        static const int diag_dirs[4] = {
            sq::NORTH_EAST, sq::NORTH_WEST, sq::SOUTH_EAST, sq::SOUTH_WEST
        };
        for (int dir : diag_dirs)
        {
            int cur = s + dir;
            int prev_rank = sq::rank_of(s);
            int prev_file = sq::file_of(s);
            while (sq::is_valid(cur))
            {
                int cr = sq::rank_of(cur), cf = sq::file_of(cur);
                // Wrap detection: ensure we haven't crossed a board edge
                int dr = cr - prev_rank, df = cf - prev_file;
                if (dr < -1 || dr > 1 || df < -1 || df > 1) break;
                prev_rank = cr; prev_file = cf;

                const Piece& p = pos_.at(cur);
                if (!p.is_empty())
                {
                    if (p.color == opp &&
                        (p.type == PieceType::Bishop || p.type == PieceType::Queen))
                        return true;
                    break; // blocked
                }
                cur += dir;
            }
        }

        // Orthogonal directions → rook/queen
        static const int orth_dirs[4] = {
            sq::NORTH, sq::SOUTH, sq::EAST, sq::WEST
        };
        for (int dir : orth_dirs)
        {
            int cur = s + dir;
            int prev_rank = sq::rank_of(s);
            int prev_file = sq::file_of(s);
            while (sq::is_valid(cur))
            {
                int cr = sq::rank_of(cur), cf = sq::file_of(cur);
                int dr = cr - prev_rank, df = cf - prev_file;
                if (dr < -1 || dr > 1 || df < -1 || df > 1) break;
                prev_rank = cr; prev_file = cf;

                const Piece& p = pos_.at(cur);
                if (!p.is_empty())
                {
                    if (p.color == opp &&
                        (p.type == PieceType::Rook || p.type == PieceType::Queen))
                        return true;
                    break;
                }
                cur += dir;
            }
        }
    }

    // ── King ─────────────────────────────────────────────────
    {
        static const int king_offsets[8] = {
            sq::NORTH, sq::SOUTH, sq::EAST, sq::WEST,
            sq::NORTH_EAST, sq::NORTH_WEST, sq::SOUTH_EAST, sq::SOUTH_WEST
        };
        int rank = sq::rank_of(s);
        int file = sq::file_of(s);
        for (int off : king_offsets)
        {
            int ns = s + off;
            if (!sq::is_valid(ns)) continue;
            // Prevent wrap
            int nr = sq::rank_of(ns), nf = sq::file_of(ns);
            if (abs(nr - rank) > 1 || abs(nf - file) > 1) continue;
            const Piece& p = pos_.at(ns);
            if (p.type == PieceType::King && p.color == opp)
                return true;
        }
    }

    return false;
}

// ── gen_sliding_moves ────────────────────────────────────────

void ChessBoard::gen_sliding_moves(sq::Square from, const int* dirs, int dir_count,
                                   std::vector<Move>& out) const
{
    const Piece& mover = pos_.at(from);
    int from_rank = sq::rank_of(from);
    int from_file = sq::file_of(from);

    for (int i = 0; i < dir_count; ++i)
    {
        int dir = dirs[i];
        int cur = from + dir;
        int prev_rank = from_rank;
        int prev_file = from_file;

        while (sq::is_valid(cur))
        {
            int cr = sq::rank_of(cur), cf = sq::file_of(cur);
            // Wrap guard
            if (abs(cr - prev_rank) > 1 || abs(cf - prev_file) > 1) break;
            prev_rank = cr; prev_file = cf;

            const Piece& target = pos_.at(cur);
            if (target.is_empty())
            {
                out.push_back(Move::quiet(from, cur, mover));
            }
            else
            {
                if (target.color != mover.color)
                    out.push_back(Move::capture(from, cur, mover, target));
                break; // blocked either way
            }
            cur += dir;
        }
    }
}

// ── gen_pawn_moves ───────────────────────────────────────────

void ChessBoard::gen_pawn_moves(sq::Square from, std::vector<Move>& out) const
{
    const Piece& pawn = pos_.at(from);
    const Color  us   = pawn.color;
    const int    dir  = (us == Color::White) ? sq::NORTH : sq::SOUTH;
    const int    start_rank  = (us == Color::White) ? 1 : 6;
    const int    promote_rank = (us == Color::White) ? 7 : 0;
    const int    rank = sq::rank_of(from);
    const int    file = sq::file_of(from);

    auto push_with_promotion = [&](sq::Square to, bool is_capture, const Piece& cap = Piece::empty())
    {
        if (sq::rank_of(to) == promote_rank)
        {
            for (MoveFlag f : { MoveFlag::PromoteQueen, MoveFlag::PromoteRook,
                                MoveFlag::PromoteBishop, MoveFlag::PromoteKnight })
            {
                if (is_capture)
                    out.push_back(Move::capture(from, to, pawn, cap, f));
                else
                    out.push_back(Move::quiet(from, to, pawn, f));
            }
        }
        else
        {
            if (is_capture)
                out.push_back(Move::capture(from, to, pawn, cap));
            else
                out.push_back(Move::quiet(from, to, pawn));
        }
    };

    // Single push
    sq::Square one = from + dir;
    if (sq::is_valid(one) && pos_.is_empty(one))
    {
        push_with_promotion(one, false);

        // Double push from starting rank
        if (rank == start_rank)
        {
            sq::Square two = one + dir;
            if (sq::is_valid(two) && pos_.is_empty(two))
                out.push_back(Move::quiet(from, two, pawn, MoveFlag::DoublePawnPush));
        }
    }

    // Captures (including en passant)
    for (int df : {-1, 1})
    {
        int cf = file + df;
        if (cf < 0 || cf > 7) continue;
        sq::Square cap_sq = sq::from_rf(sq::rank_of(one), cf);
        if (!sq::is_valid(cap_sq)) continue;

        const Piece& target = pos_.at(cap_sq);

        // Normal capture
        if (!target.is_empty() && target.color != us)
            push_with_promotion(cap_sq, true, target);

        // En passant
        if (cap_sq == pos_.en_passant_sq)
        {
            // The captured pawn sits on the same rank as 'from'
            sq::Square captured_sq = sq::from_rf(rank, cf);
            const Piece& captured_pawn = pos_.at(captured_sq);
            out.push_back(Move::en_passant(from, cap_sq, pawn, captured_pawn));
        }
    }
}

// ── gen_knight_moves ─────────────────────────────────────────

void ChessBoard::gen_knight_moves(sq::Square from, std::vector<Move>& out) const
{
    const Piece& knight = pos_.at(from);
    int rank = sq::rank_of(from);
    int file = sq::file_of(from);

    static const int offsets[8][2] = {
        {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
    };

    for (auto& off : offsets)
    {
        int r = rank + off[0], f = file + off[1];
        if (r < 0 || r > 7 || f < 0 || f > 7) continue;
        sq::Square to = sq::from_rf(r, f);
        const Piece& target = pos_.at(to);
        if (target.is_empty())
            out.push_back(Move::quiet(from, to, knight));
        else if (target.color != knight.color)
            out.push_back(Move::capture(from, to, knight, target));
    }
}

// ── gen_bishop_moves ─────────────────────────────────────────

void ChessBoard::gen_bishop_moves(sq::Square from, std::vector<Move>& out) const
{
    static const int dirs[4] = {
        sq::NORTH_EAST, sq::NORTH_WEST, sq::SOUTH_EAST, sq::SOUTH_WEST
    };
    gen_sliding_moves(from, dirs, 4, out);
}

// ── gen_rook_moves ───────────────────────────────────────────

void ChessBoard::gen_rook_moves(sq::Square from, std::vector<Move>& out) const
{
    static const int dirs[4] = {
        sq::NORTH, sq::SOUTH, sq::EAST, sq::WEST
    };
    gen_sliding_moves(from, dirs, 4, out);
}

// ── gen_queen_moves ──────────────────────────────────────────

void ChessBoard::gen_queen_moves(sq::Square from, std::vector<Move>& out) const
{
    static const int dirs[8] = {
        sq::NORTH, sq::SOUTH, sq::EAST, sq::WEST,
        sq::NORTH_EAST, sq::NORTH_WEST, sq::SOUTH_EAST, sq::SOUTH_WEST
    };
    gen_sliding_moves(from, dirs, 8, out);
}

// ── gen_king_moves ───────────────────────────────────────────

void ChessBoard::gen_king_moves(sq::Square from, std::vector<Move>& out) const
{
    const Piece& king = pos_.at(from);
    const Color  us   = king.color;
    int rank = sq::rank_of(from);
    int file = sq::file_of(from);

    static const int offsets[8] = {
        sq::NORTH, sq::SOUTH, sq::EAST, sq::WEST,
        sq::NORTH_EAST, sq::NORTH_WEST, sq::SOUTH_EAST, sq::SOUTH_WEST
    };

    for (int off : offsets)
    {
        int to = from + off;
        if (!sq::is_valid(to)) continue;
        int tr = sq::rank_of(to), tf = sq::file_of(to);
        if (abs(tr - rank) > 1 || abs(tf - file) > 1) continue;

        const Piece& target = pos_.at(to);
        if (target.is_empty())
            out.push_back(Move::quiet(from, to, king));
        else if (target.color != us)
            out.push_back(Move::capture(from, to, king, target));
    }

    // ── Castling ─────────────────────────────────────────────
    // Conditions: rights exist, squares between are empty,
    // king not in check, king doesn't pass through attacked square.
    // Note: legality filter in legal_moves() will catch landing in check.
    Color enemy = opposite(us);

    if (pos_.castling.can_castle_kingside(us))
    {
        // Squares between king and rook must be empty
        sq::Square f_sq = from + sq::EAST;
        sq::Square g_sq = from + sq::EAST * 2;
        if (pos_.is_empty(f_sq) && pos_.is_empty(g_sq))
        {
            // King must not be in check or pass through f-square
            if (!is_attacked(from, enemy) && !is_attacked(f_sq, enemy))
                out.push_back(Move::castle(from, g_sq, king, true));
        }
    }

    if (pos_.castling.can_castle_queenside(us))
    {
        sq::Square d_sq = from + sq::WEST;
        sq::Square c_sq = from + sq::WEST * 2;
        sq::Square b_sq = from + sq::WEST * 3;
        if (pos_.is_empty(d_sq) && pos_.is_empty(c_sq) && pos_.is_empty(b_sq))
        {
            if (!is_attacked(from, enemy) && !is_attacked(d_sq, enemy))
                out.push_back(Move::castle(from, c_sq, king, false));
        }
    }
}

// ── pseudo_legal_moves_from ──────────────────────────────────

std::vector<Move> ChessBoard::pseudo_legal_moves_from(sq::Square from) const
{
    std::vector<Move> out;
    out.reserve(28);

    const Piece& p = pos_.at(from);
    if (p.is_empty() || p.color != pos_.side_to_move) return out;

    switch (p.type)
    {
        case PieceType::Pawn:   gen_pawn_moves  (from, out); break;
        case PieceType::Knight: gen_knight_moves(from, out); break;
        case PieceType::Bishop: gen_bishop_moves(from, out); break;
        case PieceType::Rook:   gen_rook_moves  (from, out); break;
        case PieceType::Queen:  gen_queen_moves (from, out); break;
        case PieceType::King:   gen_king_moves  (from, out); break;
        default: break;
    }

    return out;
}

// ── pseudo_legal_moves ───────────────────────────────────────

std::vector<Move> ChessBoard::pseudo_legal_moves() const
{
    std::vector<Move> out;
    out.reserve(40);

    for (int s = 0; s < 64; ++s)
    {
        const Piece& p = pos_.board[static_cast<size_t>(s)];
        if (p.is_empty() || p.color != pos_.side_to_move) continue;
        // pseudo_legal_moves_from(static_cast<sq::Square>(s));
        // (re-use the per-square generator into `out` directly)
        switch (p.type)
        {
            case PieceType::Pawn:   gen_pawn_moves  (s, out); break;
            case PieceType::Knight: gen_knight_moves(s, out); break;
            case PieceType::Bishop: gen_bishop_moves(s, out); break;
            case PieceType::Rook:   gen_rook_moves  (s, out); break;
            case PieceType::Queen:  gen_queen_moves (s, out); break;
            case PieceType::King:   gen_king_moves  (s, out); break;
            default: break;
        }
    }

    return out;
}

// ── apply_move_to ────────────────────────────────────────────

Position ChessBoard::apply_move_to(const Position& base, const Move& m) const
{
    Position next = base;

    const Color  us   = m.moving.color;
    const Color  them = opposite(us);

    // ── Perform the move on the board ─────────────────────────

    next.at(m.from) = Piece::empty();
    next.at(m.to)   = m.moving;

    // Promotion: replace the piece on the destination
    if (m.is_promotion())
        next.at(m.to) = Piece::make(us, m.promotion_piece());

    // En passant: remove the captured pawn (not on m.to)
    if (m.is_en_passant())
    {
        int ep_rank = sq::rank_of(m.from); // same rank as capturing pawn
        int ep_file = sq::file_of(m.to);
        next.at(sq::from_rf(ep_rank, ep_file)) = Piece::empty();
    }

    // Castling: also move the rook
    if (m.flag == MoveFlag::CastleKingside)
    {
        // Rook moves from h-file to f-file (same rank as king)
        int r = sq::rank_of(m.from);
        sq::Square rook_from = sq::from_rf(r, 7);
        sq::Square rook_to   = sq::from_rf(r, 5);
        next.at(rook_to)   = next.at(rook_from);
        next.at(rook_from) = Piece::empty();
    }
    else if (m.flag == MoveFlag::CastleQueenside)
    {
        int r = sq::rank_of(m.from);
        sq::Square rook_from = sq::from_rf(r, 0);
        sq::Square rook_to   = sq::from_rf(r, 3);
        next.at(rook_to)   = next.at(rook_from);
        next.at(rook_from) = Piece::empty();
    }

    // ── Update castling rights ────────────────────────────────

    // King moves: lose all rights for that color
    if (m.moving.type == PieceType::King)
        next.castling.remove_all(us);

    // Rook moves from original square: lose that side's right
    if (m.moving.type == PieceType::Rook)
    {
        int back_rank = (us == Color::White) ? 0 : 7;
        if (sq::rank_of(m.from) == back_rank)
        {
            if (sq::file_of(m.from) == 7) next.castling.remove_kingside(us);
            if (sq::file_of(m.from) == 0) next.castling.remove_queenside(us);
        }
    }

    // Opponent's rook captured on its home square: lose their right
    if (m.is_capture() && m.captured.type == PieceType::Rook)
    {
        int back_rank = (them == Color::White) ? 0 : 7;
        if (sq::rank_of(m.to) == back_rank)
        {
            if (sq::file_of(m.to) == 7) next.castling.remove_kingside(them);
            if (sq::file_of(m.to) == 0) next.castling.remove_queenside(them);
        }
    }

    // ── En passant square ─────────────────────────────────────

    if (m.flag == MoveFlag::DoublePawnPush)
    {
        // The en passant target is the square the pawn skipped over
        int ep_rank = (us == Color::White) ? 2 : 5;
        next.en_passant_sq = sq::from_rf(ep_rank, sq::file_of(m.from));
    }
    else
    {
        next.en_passant_sq = sq::NONE;
    }

    // ── Halfmove clock ────────────────────────────────────────

    if (m.moving.type == PieceType::Pawn || m.is_capture())
        next.halfmove_clock = 0;
    else
        ++next.halfmove_clock;

    // ── Fullmove number ───────────────────────────────────────

    if (us == Color::Black)
        ++next.fullmove_number;

    // ── Flip side to move ─────────────────────────────────────

    next.side_to_move = them;

    return next;
}

// ── is_insufficient_material ─────────────────────────────────

bool ChessBoard::is_insufficient_material() const
{
    // Collect all non-king pieces
    int white_bishops = 0, black_bishops = 0;
    int white_knights = 0, black_knights = 0;
    int white_bishop_color = -1; // 0 = light, 1 = dark
    int black_bishop_color = -1;

    for (int s = 0; s < 64; ++s)
    {
        const Piece& p = pos_.board[static_cast<size_t>(s)];
        if (p.is_empty() || p.type == PieceType::King) continue;

        // Any pawn, rook, or queen → sufficient material
        if (p.type == PieceType::Pawn ||
            p.type == PieceType::Rook ||
            p.type == PieceType::Queen)
            return false;

        if (p.type == PieceType::Bishop)
        {
            int sq_color = (sq::rank_of(s) + sq::file_of(s)) % 2;
            if (p.color == Color::White) { ++white_bishops; white_bishop_color = sq_color; }
            else                         { ++black_bishops; black_bishop_color = sq_color; }
        }
        else if (p.type == PieceType::Knight)
        {
            if (p.color == Color::White) ++white_knights;
            else                         ++black_knights;
        }
    }

    int white_minor = white_bishops + white_knights;
    int black_minor = black_bishops + black_knights;

    // K vs K
    if (white_minor == 0 && black_minor == 0) return true;

    // K+B vs K or K+N vs K
    if (white_minor == 1 && black_minor == 0) return true;
    if (black_minor == 1 && white_minor == 0) return true;

    // K+B vs K+B, same bishop color
    if (white_bishops == 1 && white_knights == 0 &&
        black_bishops == 1 && black_knights == 0 &&
        white_bishop_color == black_bishop_color)
        return true;

    return false;
}