import { useState } from "react";
import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";
import "./App.css";

// ── Constants ───────────────────────────────────────────────

const API_URL = "https://chessengine-backend.onrender.com/move";
// const API_URL = "http://localhost:3000/move"; // uncomment for local dev

// Converts centipawn score to a 0–100 percentage for the eval bar.
// White advantage → above 50, Black → below 50.
// Clamped to [0, 100]. Each 100cp ≈ 5 bar units (20cp per unit).
function evalToBarPercent(pawns) {
    const percent = 50 - (pawns ?? 0) * 5;

    return Math.max(
        0,
        Math.min(100, percent)
    );
}

// ── AnalysisPanel ────────────────────────────────────────────

function AnalysisPanel({ engineData, fetchError }) {
    return (
        <div className="panel-card">
            <div className="panel-title">Engine Analysis</div>

            {fetchError && (
                <div className="error-message">{fetchError}</div>
            )}

            {engineData ? (
                <>
                    <div className="best-move-display">
                        <div className="best-move-uci">
                            {engineData.bestMove.slice(0, 2)}
                            {" → "}
                            {engineData.bestMove.slice(2, 4)}
                        </div>
                        <div className="best-move-label">Best Move</div>
                    </div>

                    <div className="stats-grid">
                        <div className="stat-item">
                            <div className="stat-label">Eval</div>
                            <div className="stat-value">
                                {engineData.evaluation > 0 ? "+" : ""}
                                {engineData.evaluation.toFixed(2)}
                            </div>
                        </div>

                        <div className="stat-item">
                            <div className="stat-label">Depth</div>
                            <div className="stat-value">{engineData.depth}</div>
                        </div>

                        <div className="stat-item">
                            <div className="stat-label">Nodes</div>
                            <div className="stat-value">
                                {engineData.nodes.toLocaleString()}
                            </div>
                        </div>

                        <div className="stat-item">
                            <div className="stat-label">Time</div>
                            <div className="stat-value">{engineData.time} ms</div>
                        </div>
                    </div>

                    {engineData.pv && (
                        <div className="pv-line">
                            <div className="pv-label">Principal Variation</div>
                            <div className="pv-text">{engineData.pv}</div>
                        </div>
                    )}
                </>
            ) : (
                !fetchError && (
                    <p className="no-analysis">Make a move to see analysis.</p>
                )
            )}
        </div>
    );
}

// ── MoveHistory ──────────────────────────────────────────────

// history is a flat array of SAN strings: [white1, black1, white2, black2, ...]
// We display them as move pairs: 1. e4 e5 / 2. Nf3 Nc6 etc.
function MoveHistory({ history }) {
    // Build pairs: [[white, black?], ...]
    const pairs = [];
    for (let i = 0; i < history.length; i += 2) {
        pairs.push([history[i], history[i + 1] ?? ""]);
    }

    return (
        <div className="panel-card">
            <div className="panel-title">Move History</div>

            {pairs.length === 0 ? (
                <p className="no-moves">No moves yet.</p>
            ) : (
                <div className="move-history-scroll">
                    {pairs.map(([white, black], idx) => (
                        <div key={idx} className="move-pair">
                            <span className="move-number">{idx + 1}.</span>
                            <span className="move-san">{white}</span>
                            <span className="move-san">{black}</span>
                        </div>
                    ))}
                </div>
            )}
        </div>
    );
}

// ── App ──────────────────────────────────────────────────────

export default function App() {
    const [game, setGame]               = useState(new Chess());
    const [history, setHistory]         = useState([]);       // flat SAN array
    const [positions, setPositions]     = useState([new Chess().fen()]); // FEN snapshots for undo
    const [depth, setDepth]             = useState(4);
    const [status, setStatus]           = useState("");
    const [thinking, setThinking]       = useState(false);
    const [boardOrientation, setBoardOrientation] = useState("white");
    const [engineData, setEngineData]   = useState(null);
    const [fetchError, setFetchError]   = useState("");
    const [arrows, setArrows]           = useState([]);

    // ── Game-over detection ────────────────────────────────

    function getGameStatus(chess) {
        if (chess.isCheckmate())  return "Checkmate!";
        if (chess.isStalemate())  return "Stalemate!";
        if (chess.isDraw())       return "Draw!";
        return "";
    }

    // ── New game ───────────────────────────────────────────

    function newGame() {
        const fresh = new Chess();
        setGame(fresh);
        setHistory([]);
        setPositions([fresh.fen()]);
        setStatus("");
        setEngineData(null);
        setFetchError("");
        setArrows([]);
        setBoardOrientation("white");
    }

    // ── Undo (removes last full move: human + engine) ──────

    function undoMove() {
        if (positions.length <= 1) return;

        const newPositions = positions.slice(0, -1);
        const previousFen  = newPositions[newPositions.length - 1];

        setPositions(newPositions);
        setGame(new Chess(previousFen));
        setHistory((prev) => prev.slice(0, Math.max(0, prev.length - 2)));
        setStatus("");
        setArrows([]);
    }

    // ── Apply a move to a Chess instance, return new instance ─

    function applyMove(chess, from, to) {
        const copy = new Chess(chess.fen());
        const move = copy.move({ from, to, promotion: "q" });
        return move ? { chess: copy, move } : { chess: null, move: null };
    }

    // ── Fetch engine move ──────────────────────────────────

    async function fetchEngineMove(fen) {
        const response = await fetch(API_URL, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ fen, depth }),
        });

        if (!response.ok) {
            const err = await response.json().catch(() => ({}));
            throw new Error(err.error || `Server error: ${response.status}`);
        }

        return response.json();
    }

    // ── onPieceDrop ────────────────────────────────────────

    async function onPieceDrop(source, target) {
        // Apply human move
        const { chess: afterHuman, move: humanMove } = applyMove(game, source, target);
        if (!afterHuman) return false;

        const humanSan = humanMove.san;

        setGame(afterHuman);
        setHistory((prev) => [...prev, humanSan]);
        setStatus(getGameStatus(afterHuman));
        setFetchError("");

        if (afterHuman.isGameOver()) 
          { setPositions((prev) => [
              ...prev,
              afterHuman.fen()
          ]);

    return true;
}

        setThinking(true);
        setArrows([]);

        let data;
        try {
            data = await fetchEngineMove(afterHuman.fen());
        } catch (err) {
            setFetchError(err.message || "Engine request failed.");
            setThinking(false);
            return true;
        }

        setEngineData(data);
        setThinking(false);

        // Show best-move arrow
        setArrows([
            [
                data.bestMove.slice(0, 2),
                data.bestMove.slice(2, 4),
                "green",
            ],
        ]);

        // Apply engine move
        const { chess: afterEngine, move: engineMove } = applyMove(
            afterHuman,
            data.bestMove.slice(0, 2),
            data.bestMove.slice(2, 4)
        );

        if (!afterEngine) {
            setFetchError("Engine returned an illegal move.");
            return true;
        }

        setGame(afterEngine);
        setHistory((prev) => [...prev, engineMove.san]);
        setStatus(getGameStatus(afterEngine));
        setPositions((prev) => [...prev, afterEngine.fen()]);

        return true;
    }

    // ── Eval bar height ────────────────────────────────────

    const evalBarHeight = evalToBarPercent(engineData ? engineData.evaluation : 0);

    // ── Render ─────────────────────────────────────────────

    return (
        <div className="app">

            {/* Header */}
            <header className="app-header">
                <h1 className="app-title">♟ Shreeram Engine</h1>
                <p className="app-subtitle">
                    C++ Engine · Alpha-Beta · Quiescence · TT · Zobrist · UCI
                </p>
            </header>

            {/* Toolbar */}
            <div className="toolbar">
                <button className="btn" onClick={newGame}>
                    New Game
                </button>

                <button
                    className="btn"
                    onClick={() =>
                        setBoardOrientation((o) =>
                            o === "white" ? "black" : "white"
                        )
                    }
                >
                    Flip Board
                </button>

                <button className="btn" onClick={undoMove}>
                    Undo Move
                </button>

                <div>
                    <span className="difficulty-label">Difficulty:</span>
                    <select
                        className="difficulty-select"
                        value={depth}
                        onChange={(e) => setDepth(Number(e.target.value))}
                    >
                        <option value={2}>Easy</option>
                        <option value={4}>Medium</option>
                        <option value={6}>Hard</option>
                    </select>
                </div>
            </div>

            {/* Thinking indicator (reserved height to prevent layout shift) */}
            <div className="thinking-indicator">
                {thinking && "Engine thinking…"}
            </div>

            {/* Board row */}
            <div className="board-and-sidebar">

                {/* Evaluation bar */}
                <div className="eval-bar-wrap">
                    <div
                        className="eval-bar-fill"
                        style={{ height: `${evalBarHeight}%` }}
                    />
                </div>

                {/* Board */}
                <div className="board-column">
                    <div className="board-wrapper">
                        <Chessboard
                            boardWidth={600}
                            position={game.fen()}
                            onPieceDrop={onPieceDrop}
                            boardOrientation={boardOrientation}
                            arePiecesDraggable={!thinking}
                            customArrows={arrows}
                        />
                    </div>

                    {status && (
                        <div className="game-status">{status}</div>
                    )}
                </div>

                {/* Sidebar */}
                <div className="sidebar">
                    <AnalysisPanel
                        engineData={engineData}
                        fetchError={fetchError}
                    />
                    <MoveHistory history={history} />
                </div>

            </div>
        </div>
    );
}
