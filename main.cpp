#include <iostream>
#include <SFML/Graphics.hpp>
#include "engine.hpp"
#include <optional>
#include <map>

using namespace std;

// selection

bool pieceSelected = false;

int selectedRow = -1;
int selectedCol = -1;

int main(){

    // window

    sf::RenderWindow window(
        sf::VideoMode({800,800}),
        "Chess"
    );

    // textures

    map<char,sf::Texture> textures;

    textures['P'].loadFromFile("pieces-basic-svg/pawn-w.png");
    textures['R'].loadFromFile("pieces-basic-svg/rook-w.png");
    textures['N'].loadFromFile("pieces-basic-svg/knight-w.png");
    textures['B'].loadFromFile("pieces-basic-svg/bishop-w.png");
    textures['Q'].loadFromFile("pieces-basic-svg/queen-w.png");
    textures['K'].loadFromFile("pieces-basic-svg/king-w.png");

    textures['p'].loadFromFile("pieces-basic-svg/pawn-b.png");
    textures['r'].loadFromFile("pieces-basic-svg/rook-b.png");
    textures['n'].loadFromFile("pieces-basic-svg/knight-b.png");
    textures['b'].loadFromFile("pieces-basic-svg/bishop-b.png");
    textures['q'].loadFromFile("pieces-basic-svg/queen-b.png");
    textures['k'].loadFromFile("pieces-basic-svg/king-b.png");

    // game loop

    while(window.isOpen()){

        // events

        while(const optional event =
              window.pollEvent()){

            // close

            if(event->is<sf::Event::Closed>()){
                window.close();
            }

            // mouse click

            if(const auto* mousePressed =
                event->getIf<
                sf::Event::MouseButtonPressed>())
            {
                if(mousePressed->button ==
                    sf::Mouse::Button::Left)
                {
                    int x =
                        mousePressed->position.x;

                    int y =
                        mousePressed->position.y;

                    int row = y / 100;
                    int col = x / 100;

                    // first click

                    if(!pieceSelected){

                        if(board[row][col] != '.'){

                            selectedRow = row;
                            selectedCol = col;

                            pieceSelected = true;
                        }
                    }

                    // second click

                    else{

                    if(valid(
                        selectedRow,
                        selectedCol,
                        row,
                        col
                    )){
                        move_piece(
                            selectedRow,
                            selectedCol,
                            row,
                            col
                        );

                        // black checkmate

                        if(is_checkmate(false)){
                            cout<<"WHITE WINS"<<endl;
                        }

                        // white checkmate

                        if(is_checkmate(true)){
                            cout<<"BLACK WINS"<<endl;
                        }

                        // stalemate

                        if(
                            is_stalemate(true) ||
                            is_stalemate(false)
                        ){
                            cout<<"DRAW"<<endl;
                        }

                        // insufficient material

                        if(insufficient_material()){
                            cout<<"DRAW"<<endl;
                        }

                        // 50 move rule

                        if(fifty_move_rule()){
                            cout<<"DRAW"<<endl;
                        }

                        // repetition

                        if(threefold_repetition()){
                            cout<<"DRAW"<<endl;
                        }
                    }

                        pieceSelected = false;
                    }
                }
            }
        }

        window.clear();

        // draw board

        for(int i=0;i<8;i++){

            for(int j=0;j<8;j++){

                sf::RectangleShape square;

                square.setSize(
                    sf::Vector2f(100.f,100.f)
                );

                square.setPosition(
                    sf::Vector2f(
                        j * 100,
                        i * 100
                    )
                );

                // normal colors

                if((i+j)%2==0){
                    square.setFillColor(
                        sf::Color::White
                    );
                }
                else{
                    square.setFillColor(
                        sf::Color(100,100,100)
                    );
                }

                // selected square

                if(
                    pieceSelected &&
                    i == selectedRow &&
                    j == selectedCol
                ){
                    square.setFillColor(
                        sf::Color(100,200,100)
                    );
                }

                // capture square

                if(
                    pieceSelected &&
                    valid(
                        selectedRow,
                        selectedCol,
                        i,
                        j
                    ) &&
                    board[i][j] != '.'
                ){
                    square.setFillColor(
                        sf::Color(200,100,100)
                    );
                }

                // check square

                if(
                    board[i][j] == 'K' &&
                    is_in_check(true)
                ){
                    square.setFillColor(
                        sf::Color(255,100,100)
                    );
                }

                if(
                    board[i][j] == 'k' &&
                    is_in_check(false)
                ){
                    square.setFillColor(
                        sf::Color(255,100,100)
                    );
                }

                window.draw(square);

                // legal move dots

                if(
                    pieceSelected &&
                    valid(
                        selectedRow,
                        selectedCol,
                        i,
                        j
                    )
                ){

                    sf::CircleShape circle;

                    circle.setRadius(15.f);

                    if(board[i][j] != '.'){
                        circle.setFillColor(
                            sf::Color(255,0,0,180)
                        );
                    }
                    else{
                        circle.setFillColor(
                            sf::Color(50,50,50,150)
                        );
                    }

                    circle.setPosition(
                        sf::Vector2f(
                            j * 100 + 35,
                            i * 100 + 35
                        )
                    );

                    window.draw(circle);
                }
            }
        }

        // draw pieces

        for(int i=0;i<8;i++){

            for(int j=0;j<8;j++){

                if(board[i][j] == '.')
                    continue;

                sf::Sprite piece(
                    textures[board[i][j]]
                );

                sf::Vector2u textureSize =
                    textures[
                        board[i][j]
                    ].getSize();

                piece.setScale(
                    sf::Vector2f(
                        100.f / textureSize.x,
                        100.f / textureSize.y
                    )
                );

                piece.setPosition(
                    sf::Vector2f(
                        j * 100,
                        i * 100
                    )
                );

                window.draw(piece);
            }
        }

        // display

        window.display();
    }

    return 0;
}