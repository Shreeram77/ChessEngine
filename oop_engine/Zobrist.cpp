#include "Zobrist.hpp"

#include <array>
#include <random>

namespace
{
    std::array<std::array<uint64_t, 64>, 12>
        piece_keys;

    std::array<uint64_t, 16>
        castling_keys;

    std::array<uint64_t, 64>
        en_passant_keys;

    uint64_t side_key;

    int piece_index(const Piece& piece)
    {
        int base =
            piece.color == Color::White
            ? 0
            : 6;

        switch (piece.type)
        {
            case PieceType::Pawn:   return base + 0;
            case PieceType::Knight: return base + 1;
            case PieceType::Bishop: return base + 2;
            case PieceType::Rook:   return base + 3;
            case PieceType::Queen:  return base + 4;
            case PieceType::King:   return base + 5;

            default:
                return -1;
        }
    }

    int castling_index(
        const CastlingRights& c)
    {
        int value = 0;

        if (c.white_kingside)  value |= 1;
        if (c.white_queenside) value |= 2;
        if (c.black_kingside)  value |= 4;
        if (c.black_queenside) value |= 8;

        return value;
    }

    struct ZobristInitializer
    {
        ZobristInitializer()
        {
            std::mt19937_64 rng(
                0x123456789ULL);

            for (auto& row : piece_keys)
            {
                for (auto& key : row)
                    key = rng();
            }

            for (auto& key : castling_keys)
                key = rng();

            for (auto& key : en_passant_keys)
                key = rng();

            side_key = rng();
        }
    };

    ZobristInitializer initializer;
}

uint64_t compute_hash(
    const Position& pos)
{
    uint64_t hash = 0;

    for (int sq = 0; sq < 64; ++sq)
    {
        const Piece& piece =
            pos.board[sq];

        if (piece.is_empty())
            continue;

        hash ^=
            piece_keys
            [piece_index(piece)]
            [sq];
    }

    if (pos.side_to_move == Color::Black)
        hash ^= side_key;

    hash ^=
        castling_keys[
            castling_index(
                pos.castling)];

    if (pos.en_passant_sq != sq::NONE)
    {
        hash ^=
            en_passant_keys[
                static_cast<int>(
                    pos.en_passant_sq)];
    }

    return hash;
}