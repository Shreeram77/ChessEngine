const path = require("path");
const express = require("express");
const cors = require("cors");
const { spawn } = require("child_process");

const app = express();

app.use(cors());
app.use(express.json());

// How long to wait for a bestmove before killing the engine (ms).
// Depth 6 on Render free tier takes ~5s; 30s is a safe ceiling.
const ENGINE_TIMEOUT_MS = 30000;
const MAX_DEPTH = 10;

const ENGINE_PATH = path.join(
    __dirname,
    "../oop_engine/chess_oop"
);

// Minimal FEN sanity check: must have 6 space-separated fields
// and a recognisable board rank section.
function isValidFen(fen) {
    if (typeof fen !== "string") return false;
    const parts = fen.trim().split(/\s+/);
    return parts.length === 6;
}

function isValidDepth(depth) {
    return (
        typeof depth === "number" &&
        Number.isInteger(depth) &&
        depth >= 1 &&
        depth <= MAX_DEPTH
    );
}

function parseInfoLine(line) {
    const tokens = line.split(" ");

    const get = (key) => {
        const i = tokens.indexOf(key);
        return i !== -1 ? tokens[i + 1] : null;
    };

    const pvIndex = tokens.indexOf("pv");
    const pv =
        pvIndex !== -1
            ? tokens.slice(pvIndex + 1).join(" ")
            : "";

    return {
        depth: parseInt(get("depth"))  || 0,
        score: parseInt(get("cp"))     || 0,
        nodes: parseInt(get("nodes"))  || 0,
        time:  parseInt(get("time"))   || 0,
        pv,
    };
}

app.post("/move", (req, res) => {
    const { fen, depth } = req.body;

    // ── Input validation ────────────────────────────────────
    if (!isValidFen(fen)) {
        return res
            .status(400)
            .json({ error: "Invalid or missing FEN string." });
    }

    if (!isValidDepth(depth)) {
        return res
            .status(400)
            .json({ error: "depth must be an integer between 1 and 10." });
    }

    // ── Spawn engine ────────────────────────────────────────
    let engine;
    try {
        engine = spawn(ENGINE_PATH);
    } catch (spawnErr) {
        console.error("Failed to spawn engine:", spawnErr.message);
        return res
            .status(500)
            .json({ error: "Engine process could not be started." });
    }

    let output      = "";
    let lastInfo    = null;
    let responded   = false;

    // ── Timeout guard ───────────────────────────────────────
    const timeout = setTimeout(() => {
        if (!responded) {
            responded = true;
            console.error("Engine timed out — killing process.");
            engine.kill();
            res.status(504).json({ error: "Engine timed out." });
        }
    }, ENGINE_TIMEOUT_MS);

    // ── Helper: send response and clean up ──────────────────
    function finish(payload, statusCode = 200) {
        if (responded) return;
        responded = true;
        clearTimeout(timeout);
        engine.kill();
        res.status(statusCode).json(payload);
    }

    // ── stdout: parse info lines and bestmove ───────────────
    engine.stdout.on("data", (data) => {
        output += data.toString();

        // Process only complete lines (split on newline, keep remainder)
        const lines = output.split("\n");
        output = lines.pop(); // incomplete last chunk back into buffer

        for (const line of lines) {
            if (line.startsWith("info depth")) {
                lastInfo = parseInfoLine(line);
            }

            if (line.startsWith("bestmove")) {
                const bestMove = line.split(" ")[1];

                if (!bestMove || bestMove === "(none)") {
                    finish(
                        { error: "Engine returned no legal move." },
                        422
                    );
                    return;
                }

                finish({
                    bestMove,
                    evaluation: lastInfo ? lastInfo.score / 100 : 0,
                    depth:      lastInfo ? lastInfo.depth        : depth,
                    nodes:      lastInfo ? lastInfo.nodes        : 0,
                    time:       lastInfo ? lastInfo.time         : 0,
                    pv:         lastInfo ? lastInfo.pv           : "",
                });
            }
        }
    });

    // ── stderr: log engine errors (crashes, assertions, etc.) ──
    engine.stderr.on("data", (data) => {
        console.error("Engine stderr:", data.toString().trim());
    });

    // ── close: handle engine exit before bestmove was received ─
    engine.on("close", (code) => {
        if (!responded) {
            console.error(`Engine exited with code ${code} before sending bestmove.`);
            finish(
                { error: "Engine exited unexpectedly." },
                500
            );
        }
    });

    engine.on("error", (err) => {
        console.error("Engine process error:", err.message);
        finish({ error: "Engine process error." }, 500);
    });

    // ── Send UCI commands ───────────────────────────────────
    try {
        engine.stdin.write("uci\n");
        engine.stdin.write(`position fen ${fen}\n`);
        engine.stdin.write(`go depth ${depth}\n`);
    } catch (err) {
        console.error("Failed to write to engine stdin:", err.message);

        finish(
            { error: "Failed to communicate with engine." },
            500
        );
    }
});

app.listen(3000, () => {
    console.log("Chess engine server running on port 3000");
});