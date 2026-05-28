#pragma once

extern char board[8][8];

bool valid(
    int a,
    int b,
    int x,
    int y
);

void move_piece(
    int a,
    int b,
    int x,
    int y
);

bool is_in_check(bool white);

bool has_legal_move(bool white);

bool is_checkmate(bool white);

bool is_stalemate(bool white);