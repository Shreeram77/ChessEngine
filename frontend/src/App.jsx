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
  const [engineData, setEngineData] = useState(null);
  const [arrows, setArrows] = useState([]);

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
      // "http://localhost:3000/move",
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

    setEngineData(data);

    setArrows([
      [
        data.bestMove.slice(0, 2),
        data.bestMove.slice(2, 4),
        "green",
      ],
    ]);

    console.log("ENGINE DATA:", data);

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
    <div
      style={{
        minHeight: "100vh",
        background: "#121212",
        color: "white",
        padding: "20px",
      }}
    >
    <div style={{ marginBottom: "25px" }}>
  <h1
    style={{
      fontSize: "42px",
      margin: 0,
    }}
  >
    ♟️ Shreeram Engine
  </h1>

  <p
    style={{
      color: "#999",
      marginTop: "8px",
    }}
  >
    Custom C++ Chess Engine • Alpha-Beta • TT • Zobrist • UCI
  </p>
</div>
    <div
      style={{
        marginBottom: "20px",
        display: "flex",
        gap: "10px",
        alignItems: "center",
        flexWrap: "wrap",
      }}
    >
      <button
        onClick={newGame}
        style={{
          padding: "8px 15px",
          cursor: "pointer",
          background: "#2a2a2a",
          color: "white",
          border: "1px solid #444",
          borderRadius: "8px",
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
            padding: "8px 15px",
            cursor: "pointer",
            background: "#2a2a2a",
            color: "white",
            border: "1px solid #444",
            borderRadius: "8px",
          }}
        >
        Flip Board
      </button>

      <button
        onClick={undoMove}
        style={{
          padding: "8px 15px",
          cursor: "pointer",
          background: "#2a2a2a",
          color: "white",
          border: "1px solid #444",
          borderRadius: "8px",
        }}
      >
        Undo Move
      </button>

      <div>
        <label>Difficulty: </label>

        <select
          value={depth}
          onChange={(e) =>
            setDepth(Number(e.target.value))
          }
        >
          <option value={2}>Easy</option>
          <option value={4}>Medium</option>
          <option value={6}>Hard</option>
        </select>
      </div>
    </div>

    {thinking && (
      <h2 style={{ color: "orange" }}>
        Engine Thinking...
      </h2>
    )}

    <div
      style={{
        background: "#2a2a2a",
        color: "white",
        border: "1px solid #444",
        borderRadius: "6px",
        padding: "5px",
      }}
    >
      {/* BOARD */}
    
      {/* BOARD */}
      <div
        style={{
          display: "flex",
          gap: "12px",
        }}
      >
        {/* Evaluation Bar */}
        <div
          style={{
            width: "35px",
            height: "600px",
            background: "#222",
            borderRadius: "8px",
            overflow: "hidden",
            border: "1px solid #444",
            position: "relative",
          }}
        >
          <div
            style={{
              position: "absolute",
              bottom: 0,
              width: "100%",
              background: "white",
              height: `${
                engineData
                  ? Math.max(
                      0,
                      Math.min(
                        100,
                        50 +
                          engineData.evaluation * 10
                      )
                    )
                  : 50
              }%`,
            }}
          />
        </div>

        <div>
          <Chessboard
            boardWidth={600}
            position={game.fen()}
            onPieceDrop={onPieceDrop}
            boardOrientation={boardOrientation}
            arePiecesDraggable={!thinking}
            customArrows={arrows}
          />

          {status && (
            <h2 style={{ color: "#ff4d4d" }}>
              {status}
            </h2>
          )}
        </div>
      </div>

      {/* ANALYSIS PANEL */}
      <div
        style={{
          minWidth: "320px",
          background: "#1e1e1e",
          border: "1px solid #444",
          borderRadius: "10px",
          padding: "15px",
        }}
      >
        <h2>Engine Analysis</h2>

        <h2 style={{ marginTop: 0 }}>
          Engine Analysis
        </h2>

        {engineData ? (
          <>
            <div
              style={{
                background: "#111",
                padding: "15px",
                borderRadius: "8px",
                marginBottom: "15px",
              }}
            >
              <div
                style={{
                  color: "#4ade80",
                  fontSize: "28px",
                  fontWeight: "bold",
                }}
              >
                {engineData.bestMove.slice(0, 2)}
                →
                {engineData.bestMove.slice(2, 4)}
              </div>

              <div
                style={{
                  color: "#999",
                  marginTop: "5px",
                }}
              >
                Suggested Move
              </div>
            </div>

            <div
              style={{
                display: "grid",
                gridTemplateColumns:
                  "1fr 1fr",
                gap: "10px",
              }}
            >
              <div>
                <strong>Eval</strong>
                <br />
                {engineData.evaluation}
              </div>

              <div>
                <strong>Depth</strong>
                <br />
                {engineData.depth}
              </div>

              <div>
                <strong>Nodes</strong>
                <br />
                {engineData.nodes.toLocaleString()}
              </div>

              <div>
                <strong>Time</strong>
                <br />
                {engineData.time} ms
              </div>
            </div>
          </>
        ) : (
          <p>No analysis yet.</p>
        )}

        <hr />

        <h2>Move History</h2>

        <div
          style={{
            maxHeight: "300px",
            overflowY: "auto",
          }}
        >
          {history.map((move, index) => (
            <div key={index}>
              {index + 1}. {move}
            </div>
          ))}
        </div>
      </div>
    </div>
  </div>
);
}
export default App;