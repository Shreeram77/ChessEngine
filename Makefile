CXX = g++
CXXFLAGS = -O2 -std=c++20

ENGINE_SRC = \
oop_engine/ChessBoard.cpp \
oop_engine/Evaluator.cpp \
oop_engine/Game.cpp \
oop_engine/Perft.cpp \
oop_engine/Search.cpp \
oop_engine/TranspositionTable.cpp \
oop_engine/UCI.cpp \
oop_engine/Zobrist.cpp \
oop_engine/main.cpp

all:
	g++ -O2 -std=c++20 $(ENGINE_SRC) -o oop_engine/chess_oop