import { useState } from "react";
import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";

function App() {
  const [game, setGame] = useState(new Chess());
  const [history, setHistory] = useState([]);
  const [positions, setPositions] = useState([new Chess().fen()]);
  const [depth, setDepth] = useState(4);
  const [status, setStatus] = useState("");
  const [thinking, setThinking] = useState(false);
  const [boardOrientation, setBoardOrientation] =
  useState("white");
  const [playerColor, setPlayerColor] = useState("white");

  function newGame() {
    setGame(new Chess());
    setHistory([]);
    setPositions([new Chess().fen()]);
    setStatus("");
    setBoardOrientation("white");
  }

  async function onPieceDrop(source, target) {
    const gameCopy = new Chess(game.fen());

    const move = gameCopy.move({
      from: source,
      to: target,
      promotion: "q",
    });

    if (move === null) {
      return false;
    }

    // Human move save
    setHistory((prev) => [...prev, move.san]);

    // Human move immediately board par dikhao
    setGame(new Chess(gameCopy.fen()));

    if (gameCopy.isCheckmate()) {
      setStatus("Checkmate!");
    }
    else if (gameCopy.isStalemate()) {
      setStatus("Stalemate!");
    }
    else if (gameCopy.isDraw()) {
      setStatus("Draw!");
    }
    else {
      setStatus("");
    }

    setThinking(true);

    const response = await fetch(
      "https://chessengine-backend.onrender.com/move",
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          fen: gameCopy.fen(),
          depth: depth,
        }),
      }
    );

    const data = await response.json();

    setThinking(false);

    // Engine move
    const engineMove = gameCopy.move({
      from: data.bestMove.slice(0, 2),
      to: data.bestMove.slice(2, 4),
      promotion: "q",
    });

    if (engineMove) {
      setHistory((prev) => [...prev, engineMove.san]);
    }

    setGame(new Chess(gameCopy.fen()));

    if (gameCopy.isCheckmate()) {
      setStatus("Checkmate!");
    }
    else if (gameCopy.isStalemate()) {
      setStatus("Stalemate!");
    }
    else if (gameCopy.isDraw()) {
      setStatus("Draw!");
    }
    else {
      setStatus("");
    }

    setPositions((prev) => [
      ...prev,
      gameCopy.fen()
    ]);

    return true;
  }

  function undoMove() {
    if (positions.length <= 1) {
      return;
    }

    const newPositions = [...positions];
    newPositions.pop();

    const previousFen =
      newPositions[newPositions.length - 1];

    setPositions(newPositions);

    setGame(new Chess(previousFen));

    setHistory((prev) =>
    prev.slice(0, Math.max(0, prev.length - 2))
  );
  }

  return (
    <div style={{ width: "700px", margin: "40px auto" }}>
      <h1>Shreeram Chess Platform</h1>
      <button
        onClick={newGame}
        style={{
          marginLeft: "10px",
          padding: "8px 15px",
          cursor: "pointer",
        }}
        >
          New Game
        </button>

        <button
          onClick={() =>
            setBoardOrientation(
              boardOrientation === "white"
                ? "black"
                : "white"
            )
          }
          style={{
            marginLeft: "10px",
            padding: "8px 15px",
            cursor: "pointer",
          }}
        >
          Flip Board
        </button>
      <button
        onClick={undoMove}
        style={{
          marginBottom: "15px",
          padding: "8px 15px",
          cursor: "pointer",
        }}
      >
        Undo Move
      </button>

        <div style={{ marginBottom: "15px" }}>
          <label>Difficulty: </label>

          <select
            value={depth}
            onChange={(e) => setDepth(Number(e.target.value))}
          >
            <option value={2}>Easy</option>
            <option value={4}>Medium</option>
            <option value={6}>Hard</option>
          </select>
        </div>

      {thinking && (
        <h2>Engine Thinking...</h2>
      )}

      <Chessboard
        position={game.fen()}
        onPieceDrop={onPieceDrop}
        boardOrientation={boardOrientation}
        arePiecesDraggable={!thinking}
      />
      {status && (
        <h2 style={{ color: "red" }}>
          {status}
        </h2>
      )}

      <div style={{ marginTop: "20px" }}>
        <h3>Move History</h3>

        {history.map((move, index) => (
          <div key={index}>
            {index + 1}. {move}
          </div>
        ))}
      </div>
    </div>
  );
}

export default App;