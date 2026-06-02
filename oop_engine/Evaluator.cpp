#include "Evaluator.hpp"
#include "Piece.hpp"

static int piece_value(PieceType type)
{
    switch (type)
    {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        case PieceType::King:   return 0;
        default:                return 0;
    }
}

int evaluate(const Position& pos)
{
    int score = 0;

    for (int rank = 0; rank < 8; ++rank)
    {
        for (int file = 0; file < 8; ++file)
        {
            const Piece& piece = pos.at(rank, file);

            if (piece.type == PieceType::None)
                continue;

            int value = piece_value(piece.type);

            if (piece.color == Color::White)
                score += value;
            else
                score -= value;
        }
    }

    return score;
}