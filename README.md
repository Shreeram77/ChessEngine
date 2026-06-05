# ♟️ Shreeram Engine

A full-stack chess platform powered by a custom chess engine written from scratch in C++20.

## Live Demo

Frontend:
https://chess-engine-theta.vercel.app/

Backend:
https://chessengine-backend.onrender.com/

---

## Overview

This project combines systems programming, search algorithms, protocol design, backend development, frontend development, and cloud deployment into a single application.

The chess engine is implemented entirely in C++ and communicates with a Node.js backend using the Universal Chess Interface (UCI) protocol. A React frontend allows users to play against the engine directly in the browser while viewing real-time engine analysis.

---

## Engine Architecture

Position
↓
ChessBoard (Legal Move Generation)
↓
Search
↓
UCI Interface
↓
Node.js Backend
↓
React Frontend

---

## Engine Features

### Move Generation

* Legal move generation
* Castling
* En passant
* Promotion
* Check / Checkmate
* Stalemate
* Threefold repetition
* Fifty-move rule
* FEN parsing

### Search

* Negamax
* Alpha-Beta Pruning
* Iterative Deepening
* Quiescence Search

### Search Optimizations

* Transposition Tables
* Zobrist Hashing
* Killer Move Heuristic
* History Heuristic
* Move Ordering

### Protocol Support

* Universal Chess Interface (UCI)

---

## Correctness Validation (Perft)

Move generation correctness was validated using standard Perft positions.

| Depth |     Nodes |
| ----- | --------: |
| 1     |        20 |
| 2     |       400 |
| 3     |     8,902 |
| 4     |   197,281 |
| 5     | 4,865,609 |

Perft testing was used to verify the correctness of legal move generation, castling, en passant, promotion handling, and check detection.

---

## Full Stack Architecture

Frontend

* React
* Vite
* react-chessboard
* chess.js

Backend

* Node.js
* Express

Engine

* C++20

Communication Flow

React Frontend
→ POST /move
→ Express Backend
→ Spawn UCI Engine Process
→ Search Position
→ Return Best Move
→ Update Board

---

## Engine Analysis

The frontend exposes engine search statistics:

* Best Move
* Evaluation Score
* Search Depth
* Nodes Searched
* Search Time

These metrics are produced directly by the search engine and transmitted through the backend API.

---

## Future Improvements

* Principal Variation Display
* Opening Book
* Endgame Tablebases
* PGN Export
* Game Persistence
* Search Streaming

---

## Author

Shreeram Goliya

* ICPC Chennai Regionals Rank 21
* ICPC India Preliminary AIR 175
* Codeforces Expert
* CodeChef 5★
