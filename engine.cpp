#include "engine.hpp"

#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

bool is_white_move = true;

// en passant

int enPassantRow = -1;
int enPassantCol = -1;

// castling info

bool moved[6];

// 0 -> white king
// 1 -> black king
// 2 -> white left rook
// 3 -> white right rook
// 4 -> black left rook
// 5 -> black right rook

char board[8][8] = {
    {'r','n','b','q','k','b','n','r'},
    {'p','p','p','p','p','p','p','p'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'P','P','P','P','P','P','P','P'},
    {'R','N','B','Q','K','B','N','R'}
};

// move piece

void move_piece(int a,int b,int x,int y){

    char piece = board[a][b];

    // castling moved info

    if(piece == 'K')
        moved[0] = true;

    if(piece == 'k')
        moved[1] = true;

    if(piece == 'R' &&
       a == 7 && b == 0)
        moved[2] = true;

    if(piece == 'R' &&
       a == 7 && b == 7)
        moved[3] = true;

    if(piece == 'r' &&
       a == 0 && b == 0)
        moved[4] = true;

    if(piece == 'r' &&
       a == 0 && b == 7)
        moved[5] = true;

    // castling rook move

    if(piece == 'K' &&
       a == 7 && b == 4 &&
       x == 7 && y == 6)
    {
        board[7][5] = 'R';
        board[7][7] = '.';
    }

    if(piece == 'K' &&
       a == 7 && b == 4 &&
       x == 7 && y == 2)
    {
        board[7][3] = 'R';
        board[7][0] = '.';
    }

    if(piece == 'k' &&
       a == 0 && b == 4 &&
       x == 0 && y == 6)
    {
        board[0][5] = 'r';
        board[0][7] = '.';
    }

    if(piece == 'k' &&
       a == 0 && b == 4 &&
       x == 0 && y == 2)
    {
        board[0][3] = 'r';
        board[0][0] = '.';
    }

    // reset en passant

    enPassantRow = -1;
    enPassantCol = -1;

    // en passant capture

    if(piece == 'P' &&
       y != b &&
       board[x][y] == '.')
    {
        board[x+1][y] = '.';
    }

    if(piece == 'p' &&
       y != b &&
       board[x][y] == '.')
    {
        board[x-1][y] = '.';
    }

    // actual move

    board[x][y] = piece;

    board[a][b] = '.';

    // create en passant square

    if(piece == 'P' &&
       a == 6 &&
       x == 4)
    {
        enPassantRow = 5;
        enPassantCol = y;
    }

    if(piece == 'p' &&
       a == 1 &&
       x == 3)
    {
        enPassantRow = 2;
        enPassantCol = y;
    }

    // promotion

    if(piece == 'P' &&
       x == 0)
    {
        board[x][y] = 'Q';
    }

    if(piece == 'p' &&
       x == 7)
    {
        board[x][y] = 'q';
    }

    is_white_move = !is_white_move;
}

// rook

bool check_for_rook(int a,int b,int x,int y){

    if(a==x){

        for(int j=min(b,y)+1;
            j<max(b,y);
            j++)
        {
            if(board[a][j] != '.')
                return false;
        }
    }

    else if(b==y){

        for(int i=min(a,x)+1;
            i<max(a,x);
            i++)
        {
            if(board[i][b] != '.')
                return false;
        }
    }

    else{
        return false;
    }

    return true;
}

// bishop

bool check_for_bishop(int a,int b,int x,int y){

    if(abs(y-b) != abs(x-a))
        return false;

    int dx = 1;
    int dy = 1;

    if(x<a)
        dx = -1;

    if(y<b)
        dy = -1;

    a += dx;
    b += dy;

    while(a!=x && b!=y){

        if(board[a][b] != '.')
            return false;

        a += dx;
        b += dy;
    }

    return true;
}

// knight

bool check_for_knight(int a,int b,int x,int y){

    vector<pair<int,int>> v{
        {1,2},
        {2,1},
        {-1,2},
        {-2,1},
        {1,-2},
        {2,-1},
        {-1,-2},
        {-2,-1},
    };

    for(auto it:v){

        int dx = it.first;
        int dy = it.second;

        if(a+dx == x &&
           b+dy == y)
        {
            return true;
        }
    }

    return false;
}

// pawn

bool check_for_pawn(int a,int b,int x,int y){

    // black pawn

    if(board[a][b] >= 'a' &&
       board[a][b] <= 'z')
    {
        // forward

        if(b==y){

            if(x-a==2 &&
               a==1 &&
               board[a+1][b] == '.' &&
               board[a+2][b] == '.')
            {
                return true;
            }

            if(x-a==1 &&
               board[x][y] == '.')
            {
                return true;
            }

            return false;
        }

        // normal capture

        if(abs(b-y)==1 &&
           a+1==x &&
           board[x][y] != '.')
        {
            return true;
        }

        // en passant

        if(abs(b-y)==1 &&
           a+1==x &&
           x == enPassantRow &&
           y == enPassantCol)
        {
            return true;
        }

        return false;
    }

    // white pawn

    else{

        // forward

        if(b==y){

            if(a-x==2 &&
               a==6 &&
               board[a-1][b] == '.' &&
               board[a-2][b] == '.')
            {
                return true;
            }

            if(a-x==1 &&
               board[x][y] == '.')
            {
                return true;
            }

            return false;
        }

        // normal capture

        if(abs(b-y)==1 &&
           a-1==x &&
           board[x][y] != '.')
        {
            return true;
        }

        // en passant

        if(abs(b-y)==1 &&
           a-1==x &&
           x == enPassantRow &&
           y == enPassantCol)
        {
            return true;
        }

        return false;
    }
}

// king

bool check_for_king(int a,int b,int x,int y){

    if(abs(x-a)<=1 &&
       abs(y-b)<=1)
    {
        return true;
    }

    // white castling

    if(board[a][b] == 'K' &&
       a == 7 &&
       b == 4)
    {
        // kingside

        if(x == 7 &&
           y == 6 &&
           !moved[0] &&
           !moved[3] &&
           board[7][5] == '.' &&
           board[7][6] == '.')
        {
            board[7][4] = '.';

            board[7][5] = 'K';

            bool check1 =
                is_in_check(true);

            board[7][5] = '.';

            board[7][6] = 'K';

            bool check2 =
                is_in_check(true);

            board[7][6] = '.';

            board[7][4] = 'K';

            if(!check1 &&
               !check2)
            {
                return true;
            }
        }

        // queenside

        if(x == 7 &&
           y == 2 &&
           !moved[0] &&
           !moved[2] &&
           board[7][1] == '.' &&
           board[7][2] == '.' &&
           board[7][3] == '.')
        {
            board[7][4] = '.';

            board[7][3] = 'K';

            bool check1 =
                is_in_check(true);

            board[7][3] = '.';

            board[7][2] = 'K';

            bool check2 =
                is_in_check(true);

            board[7][2] = '.';

            board[7][4] = 'K';

            if(!check1 &&
               !check2)
            {
                return true;
            }
        }
    }

    // black castling

    if(board[a][b] == 'k' &&
       a == 0 &&
       b == 4)
    {
        // kingside

        if(x == 0 &&
           y == 6 &&
           !moved[1] &&
           !moved[5] &&
           board[0][5] == '.' &&
           board[0][6] == '.')
        {
            board[0][4] = '.';

            board[0][5] = 'k';

            bool check1 =
                is_in_check(false);

            board[0][5] = '.';

            board[0][6] = 'k';

            bool check2 =
                is_in_check(false);

            board[0][6] = '.';

            board[0][4] = 'k';

            if(!check1 &&
               !check2)
            {
                return true;
            }
        }

        // queenside

        if(x == 0 &&
           y == 2 &&
           !moved[1] &&
           !moved[4] &&
           board[0][1] == '.' &&
           board[0][2] == '.' &&
           board[0][3] == '.')
        {
            board[0][4] = '.';

            board[0][3] = 'k';

            bool check1 =
                is_in_check(false);

            board[0][3] = '.';

            board[0][2] = 'k';

            bool check2 =
                is_in_check(false);

            board[0][2] = '.';

            board[0][4] = 'k';

            if(!check1 &&
               !check2)
            {
                return true;
            }
        }
    }

    return false;
}

// can move

bool can_move(int a,int b,int x,int y){

    if(min(a,b)<0 ||
       max(a,b)>7 ||
       min(x,y)<0 ||
       max(x,y)>7)
    {
        return false;
    }

    if(a==x && b==y)
        return false;

    if(board[a][b] == '.')
        return false;

    // same color collision

    if(board[a][b] >= 'a' &&
       board[a][b] <= 'z' &&
       board[x][y] >= 'a' &&
       board[x][y] <= 'z')
    {
        return false;
    }

    if(board[a][b] >= 'A' &&
       board[a][b] <= 'Z' &&
       board[x][y] >= 'A' &&
       board[x][y] <= 'Z')
    {
        return false;
    }

    // piece movement

    if(board[a][b] == 'R' ||
       board[a][b] == 'r')
    {
        return check_for_rook(a,b,x,y);
    }

    if(board[a][b] == 'B' ||
       board[a][b] == 'b')
    {
        return check_for_bishop(a,b,x,y);
    }

    if(board[a][b] == 'Q' ||
       board[a][b] == 'q')
    {
        return (
            check_for_rook(a,b,x,y) ||
            check_for_bishop(a,b,x,y)
        );
    }

    if(board[a][b] == 'N' ||
       board[a][b] == 'n')
    {
        return check_for_knight(a,b,x,y);
    }

    if(board[a][b] == 'P' ||
       board[a][b] == 'p')
    {
        return check_for_pawn(a,b,x,y);
    }

    if(board[a][b] == 'K' ||
       board[a][b] == 'k')
    {
        return check_for_king(a,b,x,y);
    }

    return false;
}

// check

bool is_in_check(bool white){

    int kingRow = -1;
    int kingCol = -1;

    char king =
        white ? 'K' : 'k';

    // find king

    for(int i=0;i<8;i++){

        for(int j=0;j<8;j++){

            if(board[i][j] == king){

                kingRow = i;
                kingCol = j;
            }
        }
    }

    // enemy attacks

    for(int i=0;i<8;i++){

        for(int j=0;j<8;j++){

            if(board[i][j] == '.')
                continue;

            if(white &&
               board[i][j] >= 'a' &&
               board[i][j] <= 'z')
            {
                if(can_move(
                    i,
                    j,
                    kingRow,
                    kingCol
                )){
                    return true;
                }
            }

            if(!white &&
               board[i][j] >= 'A' &&
               board[i][j] <= 'Z')
            {
                if(can_move(
                    i,
                    j,
                    kingRow,
                    kingCol
                )){
                    return true;
                }
            }
        }
    }

    return false;
}

// final valid

bool valid(int a,int b,int x,int y){

    // turn check

    if(board[a][b] >= 'A' &&
       board[a][b] <= 'Z' &&
       !is_white_move)
    {
        return false;
    }

    if(board[a][b] >= 'a' &&
       board[a][b] <= 'z' &&
       is_white_move)
    {
        return false;
    }

    // movement

    if(!can_move(a,b,x,y))
        return false;

    // temporary move

    char captured =
        board[x][y];

    char piece =
        board[a][b];

    board[x][y] = piece;

    board[a][b] = '.';

    bool white =
        (piece >= 'A' &&
         piece <= 'Z');

    bool check =
        is_in_check(white);

    // undo

    board[a][b] = piece;

    board[x][y] = captured;

    return !check;
}

// legal move exists

bool has_legal_move(bool white){

    bool currentTurn =
        is_white_move;

    is_white_move = white;

    for(int a=0;a<8;a++){

        for(int b=0;b<8;b++){

            if(board[a][b] == '.')
                continue;

            // white pieces

            if(white &&
               !(board[a][b] >= 'A' &&
                 board[a][b] <= 'Z'))
            {
                continue;
            }

            // black pieces

            if(!white &&
               !(board[a][b] >= 'a' &&
                 board[a][b] <= 'z'))
            {
                continue;
            }

            // try all squares

            for(int x=0;x<8;x++){

                for(int y=0;y<8;y++){

                    if(valid(a,b,x,y)){

                        is_white_move =
                            currentTurn;

                        return true;
                    }
                }
            }
        }
    }

    is_white_move =
        currentTurn;

    return false;
}

// checkmate

bool is_checkmate(bool white){

    if(
        is_in_check(white) &&
        !has_legal_move(white)
    ){
        return true;
    }

    return false;
}

// stalemate

bool is_stalemate(bool white){

    if(
        !is_in_check(white) &&
        !has_legal_move(white)
    ){
        return true;
    }

    return false;
}