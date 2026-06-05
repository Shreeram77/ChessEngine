const path = require("path");
const express = require("express");
const cors = require("cors");
const { spawn } = require("child_process");

const app = express();

app.use(cors());
app.use(express.json());

app.post("/move", (req, res) => {
    const { fen, depth } = req.body;

    const engine = spawn(
        path.join(__dirname, "../oop_engine/chess_oop")
    );

    let output = "";

    engine.stdout.on("data", (data) => {
        output += data.toString();

        const lines = output.split("\n");

        for (const line of lines) {
            if (line.startsWith("bestmove")) {
                const bestMove =
                    line.split(" ")[1];

                res.json({
                    bestMove
                });

                engine.kill();
                return;
            }
        }
    });

    engine.stdin.write("uci\n");
    engine.stdin.write(
        `position fen ${fen}\n`
    );
    engine.stdin.write(
        `go depth ${depth}\n`
    );
});

app.listen(3000, () => {
    console.log(
        "Server running on port 3000"
    );
});