#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

bool is_white_move = true;

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


void move_piece(int a,int b,int x,int y){
    board[x][y] = board[a][b];
    board[a][b] = '.';
}

bool check_for_rook(int a,int b,int x,int y){
    if(a==x){
        for(int j = min(b,y) + 1;j<max(b,y);j++){
            if(board[a][j] != '.') return false;
        }
    }
    else if(b==y){
        for(int i = min(a,x) + 1;i<max(a,x);i++){
            if(board[i][b] != '.') return false;
        }
    }
    else return false;
    return true;
}

bool check_for_bishop(int a,int b,int x,int y){
    if(abs(y-b) == abs(x-a)){
        int dx = 1,dy = 1;
        if(x<a) dx = -1;
        if(y<b) dy = -1;

        a += dx,b+= dy;
        while(a!=x && b!= y){
            if(board[a][b] != '.') return false;
            a += dx,b+= dy;
        }
        return true;
    }
    return false;
}

bool check_for_knight(int a,int b,int x,int y){
    vector<pair<int,int>> v{
        {1,2},{2,1},
        {-1,2},{-2,1},
        {1,-2},{2,-1},
        {-1,-2},{-2,-1},
    };
    for(auto it:v){
        int dx = it.first , dy = it.second;
        if(a+dx == x && b+dy == y) return true;
    }
    return false;
}

bool check_for_pawn(int a,int b,int x,int y){
    if(board[a][b]>='a' && board[a][b] <= 'z'){
        if(b==y){
            if(x-a==2 && a==1 && board[a+1][b] =='.' && board[a+2][b]=='.') return true;
            if(x-a==1 && board[x][y] == '.') return true;
            return false;
        }
        else if(abs(b-y)==1 && a+1 == x && board[x][y]!='.') return true;
        return false;
    }
    else{
        if(b==y){
            if(a-x==2 && a==6 && board[a-1][b] =='.' && board[a-2][b] == '.') return true;
            if(a-x==1 && board[x][y] == '.') return true;
            return false;
        }
        else if(abs(b-y)==1 && a-1==x && board[x][y]!='.') return true;
        return false;
    }
}

bool check_for_king(int a,int b,int x,int y){
    if(abs(x-a)<=1 && abs(y-b)<=1) return true;
    return false;
}

bool valid(int a,int b, int x,int y){
    if(min(a,b)<0 || max(a,b)>7 || min(x,y)<0 || max(x,y)>7) return false;
    if(a==x && b==y) return false;
    if(board[a][b] == '.') return false;
    if(board[a][b] >= 'A' && board[a][b] <= 'Z' && !is_white_move) return false;
    if(board[a][b] >= 'a' && board[a][b] <= 'z' && is_white_move) return false;
    if(board[a][b] >= 'a' && board[a][b] <= 'z' && board[x][y] >= 'a' && board[x][y] <= 'z' ) return false;
    if(board[a][b] >= 'A' && board[a][b] <= 'Z' && board[x][y] >= 'A' && board[x][y] <= 'Z' ) return false;

    if(board[a][b] == 'R' || board[a][b] == 'r') return check_for_rook(a,b,x,y);
    if(board[a][b] == 'B' || board[a][b] == 'b') return check_for_bishop(a,b,x,y);
    if(board[a][b] == 'Q' || board[a][b] == 'q') return (check_for_rook(a,b,x,y) || check_for_bishop(a,b,x,y));
    if(board[a][b] == 'N' || board[a][b] == 'n') return check_for_knight(a,b,x,y);
    if(board[a][b] == 'P' || board[a][b] == 'p') return check_for_pawn(a,b,x,y);
    if(board[a][b] == 'K' || board[a][b] == 'k') return check_for_king(a,b,x,y);
    return false;
}

void printBoard() {

    cout << "\n";

    for(int i=0;i<8;i++) {

        cout << 8-i << " ";

        for(int j=0;j<8;j++) {
            cout << board[i][j] << " ";
        }

        cout << "\n";
    }

    cout << "  a b c d e f g h\n";
}

int main() {
    printBoard();
    int t = 10;
    cin>>t;
    cin.ignore();
    while(t--){

        string s;
        getline(cin,s);

        int b = s[0] - 'a',a = 8 - (s[1] - '0'),d = s[3] - 'a',c = 8 - (s[4] -'0');
        
        if(valid(a,b,c,d)){
            move_piece(a,b,c,d);
            is_white_move = !is_white_move;
            printBoard();
        }
        else{
            cout<<endl;
            cout<<"wrong move.."<<endl;
            cout<<endl;
        }
    }
    return 0;
}