import { useState } from "react";
import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";

function App() {
  const [game, setGame] = useState(new Chess());
  const [history, setHistory] = useState([]);
  const [positions, setPositions] = useState([new Chess().fen()]);

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

    const response = await fetch(
      "http://localhost:3000/move",
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          fen: gameCopy.fen(),
          depth: 5,
        }),
      }
    );

    const data = await response.json();

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
        onClick={undoMove}
        style={{
          marginBottom: "15px",
          padding: "8px 15px",
          cursor: "pointer",
        }}
      >
        Undo Move
      </button>

      <Chessboard
        position={game.fen()}
        onPieceDrop={onPieceDrop}
      />

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