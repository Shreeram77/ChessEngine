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

static const int KNIGHT_PST[8][8] =
{
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

static const int PAWN_PST[8][8] =
{
    { 0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    { 5,  5, 10, 25, 25, 10,  5,  5},
    { 0,  0,  0, 20, 20,  0,  0,  0},
    { 5, -5,-10,  0,  0,-10, -5,  5},
    { 5, 10, 10,-20,-20, 10, 10,  5},
    { 0,  0,  0,  0,  0,  0,  0,  0}
};

static int pst_bonus(
    const Piece& piece,
    int rank,
    int file)
{
    int table_rank;

    if (piece.color == Color::White)
        table_rank = 7 - rank;
    else
        table_rank = rank;

    switch (piece.type)
    {
        case PieceType::Pawn:
            return PAWN_PST[table_rank][file];

        case PieceType::Knight:
            return KNIGHT_PST[table_rank][file];

        default:
            return 0;
    }

    if (piece.color == Color::White)
        table_rank = 7 - rank;
    else
        table_rank = rank;

    return KNIGHT_PST[table_rank][file];
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

            int value = 
                    piece_value(piece.type)
                    + pst_bonus(
                        piece,
                        rank,
                        file);

            if (piece.color == Color::White)
                score += value;
            else
                score -= value;
        }
    }

    return score;
}