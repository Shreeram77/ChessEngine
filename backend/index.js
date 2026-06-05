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

    let lastInfo = null;
    let responded = false;

    engine.stdout.on("data", (data) => {
        output += data.toString();

        const lines = output.split("\n");

        for (const line of lines) {

            console.log("LINE:", line);

            if (line.startsWith("info depth"))
            {
                lastInfo =
                    parseInfoLine(line);
            }

            console.log("FOUND BESTMOVE:", line);

            if (
                line.startsWith("bestmove")
                && !responded
            )
            {
                responded = true;

                const bestMove =
                    line.split(" ")[1];

                console.log("SENDING RESPONSE");
                
                res.json({
                    bestMove,

                    evaluation:
                        lastInfo
                        ? lastInfo.score / 100
                        : 0,

                    depth:
                        lastInfo
                        ? lastInfo.depth
                        : depth,

                    nodes:
                        lastInfo
                        ? lastInfo.nodes
                        : 0,

                    time:
                        lastInfo
                        ? lastInfo.time
                        : 0,
                });

                engine.kill();
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

function parseInfoLine(line) {
    const tokens = line.split(" ");

    const get = (key) => {
        const i = tokens.indexOf(key);
        return i !== -1
            ? tokens[i + 1]
            : null;
    };

    return {
        depth:
            parseInt(get("depth")) || 0,

        score:
            parseInt(get("cp")) || 0,

        nodes:
            parseInt(get("nodes")) || 0,

        time:
            parseInt(get("time")) || 0,
    };
}

app.listen(3000, () => {
    console.log(
        "Server running on port 3000"
    );
});