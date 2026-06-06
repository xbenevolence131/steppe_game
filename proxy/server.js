const http = require("http");
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");

const rootDir = path.resolve(__dirname, "..");
const publicDir = path.join(rootDir, "public");
const latestMapPath = path.join(rootDir, "latest-map.json");
const trafficLogDir = path.join(rootDir, "logs", "engine-traffic");
const latestTrafficLogPath = path.join(trafficLogDir, "latest.jsonl");
const port = Number(process.env.PORT || 3000);
const daemonPort = Number(process.env.STEPPE_DAEMON_PORT || 4001);
const daemonUrl = `http://127.0.0.1:${daemonPort}`;

let daemonProcess = null;
let trafficSequence = 0;
const trafficSessionName = new Date().toISOString().replace(/[:.]/g, "-");
const sessionTrafficLogPath = path.join(trafficLogDir, `${trafficSessionName}.jsonl`);

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".png": "image/png",
  ".svg": "image/svg+xml; charset=utf-8",
};

function daemonPath() {
  const candidates = [
    path.join(rootDir, "build", "steppe_daemon.exe"),
    path.join(rootDir, "build", "steppe_daemon"),
    path.join(rootDir, "build", "Debug", "steppe_daemon.exe"),
    path.join(rootDir, "build", "Release", "steppe_daemon.exe"),
  ];

  return candidates.find((candidate) => fs.existsSync(candidate));
}

function initializeTrafficLogs() {
  fs.mkdirSync(trafficLogDir, { recursive: true });
  fs.writeFileSync(latestTrafficLogPath, "", "utf8");
}

function appendTrafficLog(entry) {
  const line = `${JSON.stringify(entry)}\n`;
  try {
    fs.appendFileSync(latestTrafficLogPath, line, "utf8");
    fs.appendFileSync(sessionTrafficLogPath, line, "utf8");
  } catch (error) {
    console.error(`Could not write engine traffic log: ${error.message}`);
  }
}

function sendJson(res, statusCode, payload) {
  const body = JSON.stringify(payload);
  res.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(body),
  });
  res.end(body);
}

function readRequestJson(req) {
  return new Promise((resolve, reject) => {
    let body = "";
    req.on("data", (chunk) => {
      body += chunk;
      if (body.length > 8 * 1024 * 1024) {
        reject(new Error("request body is too large"));
        req.destroy();
      }
    });
    req.on("end", () => {
      try {
        resolve(body ? JSON.parse(body) : {});
      } catch {
        reject(new Error("request body must be valid JSON"));
      }
    });
    req.on("error", reject);
  });
}

function parseDimension(value, fallback, max) {
  const result = Number(value ?? fallback);
  if (!Number.isInteger(result) || result <= 0) {
    return fallback;
  }
  return Math.min(result, max);
}

function parseBoundedInteger(value, fallback, min, max) {
  const result = Number(value ?? fallback);
  if (!Number.isInteger(result)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, result));
}

function parseBoundedNumber(value, fallback, min, max) {
  const result = Number(value ?? fallback);
  if (!Number.isFinite(result)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, result));
}

function parseSeed(value) {
  const result = Number(value);
  if (!Number.isInteger(result) || result < 0 || result > 0xffffffff) {
    return Math.floor(Math.random() * 0x100000000);
  }
  return result;
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function daemonHealthy() {
  const sequence = ++trafficSequence;
  const startedAt = Date.now();
  try {
    const response = await fetch(`${daemonUrl}/health`);
    appendTrafficLog({
      sequence,
      timestamp: new Date(startedAt).toISOString(),
      durationMs: Date.now() - startedAt,
      replayable: false,
      transport: "proxy-daemon-http",
      request: {
        method: "GET",
        url: `${daemonUrl}/health`,
      },
      response: {
        status: response.status,
        ok: response.ok,
      },
    });
    return response.ok;
  } catch (error) {
    appendTrafficLog({
      sequence,
      timestamp: new Date(startedAt).toISOString(),
      durationMs: Date.now() - startedAt,
      replayable: false,
      transport: "proxy-daemon-http",
      request: {
        method: "GET",
        url: `${daemonUrl}/health`,
      },
      error: error.message,
    });
    return false;
  }
}

async function ensureDaemon() {
  if (await daemonHealthy()) {
    return;
  }

  const executable = daemonPath();
  if (!executable) {
    throw new Error("Daemon executable not found. Build it with: cmake --build build");
  }

  if (!daemonProcess || daemonProcess.exitCode !== null) {
    daemonProcess = spawn(executable, ["--port", String(daemonPort)], {
      cwd: rootDir,
      windowsHide: true,
      stdio: ["ignore", "pipe", "pipe"],
    });
    daemonProcess.stdout.on("data", (chunk) => process.stdout.write(chunk));
    daemonProcess.stderr.on("data", (chunk) => process.stderr.write(chunk));
    daemonProcess.on("exit", () => {
      daemonProcess = null;
    });
  }

  for (let attempt = 0; attempt < 40; attempt += 1) {
    if (await daemonHealthy()) {
      return;
    }
    await delay(100);
  }

  throw new Error(`Steppe daemon did not become ready at ${daemonUrl}`);
}

async function daemonCommand(command, gameId = "local-dev") {
  const envelope = { gameId, command };
  let logged = false;
  await ensureDaemon();
  const sequence = ++trafficSequence;
  const startedAt = Date.now();
  try {
    const response = await fetch(`${daemonUrl}/command`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(envelope),
    });
    const responseText = await response.text();
    let payload = null;
    try {
      payload = JSON.parse(responseText);
    } catch {
      // Logged below as rawBody; the caller still gets a useful exception.
    }

    appendTrafficLog({
      sequence,
      timestamp: new Date(startedAt).toISOString(),
      durationMs: Date.now() - startedAt,
      replayable: true,
      transport: "proxy-daemon-http",
      request: {
        method: "POST",
        url: `${daemonUrl}/command`,
        body: envelope,
      },
      response: {
        status: response.status,
        ok: response.ok,
        body: payload,
        rawBody: payload === null ? responseText : undefined,
      },
    });
    logged = true;

    if (payload === null) {
      throw new Error("daemon returned invalid JSON");
    }
    if (!response.ok) {
      throw new Error(payload.error || "daemon command failed");
    }
    return payload;
  } catch (error) {
    if (!logged) {
      appendTrafficLog({
        sequence,
        timestamp: new Date(startedAt).toISOString(),
        durationMs: Date.now() - startedAt,
        replayable: true,
        transport: "proxy-daemon-http",
        request: {
          method: "POST",
          url: `${daemonUrl}/command`,
          body: envelope,
        },
        error: error.message,
      });
    }
    throw error;
  }
}

function generationCommand(payload) {
  return {
    type: "generate_map",
    width: parseDimension(payload.width, 120, 120),
    height: parseDimension(payload.height, 80, 80),
    rivers: parseBoundedInteger(payload.rivers, 4, 0, 20),
    lakes: parseBoundedInteger(payload.lakes, 4, 0, 20),
    lakeSize: parseBoundedInteger(payload.lakeSize, 6, 1, 40),
    meanderForward: parseBoundedNumber(payload.meanderForward, 8, 0, 40),
    meanderForwardJitter: parseBoundedNumber(payload.meanderForwardJitter, 4, 0, 40),
    meanderLateral: parseBoundedNumber(payload.meanderLateral, 7, 0, 40),
    meanderLateralJitter: parseBoundedNumber(payload.meanderLateralJitter, 4, 0, 40),
    meanderStrength: parseBoundedNumber(payload.meanderStrength, 1, 0, 10),
    meanderReach: parseBoundedNumber(payload.meanderReach, 2, 0, 40),
    riverSlantStrength: parseBoundedNumber(payload.riverSlantStrength, 10, 0, 10),
    valleyThickness: parseBoundedNumber(payload.valleyThickness, 2, 0, 5),
    forestBlobs: parseBoundedInteger(payload.forestBlobs, 4, 0, 10),
    forestBlobRadius: parseBoundedNumber(payload.forestBlobRadius, 4, 0, 20),
    meanderTimeout: parseBoundedInteger(payload.meanderTimeout, 28, 1, 200),
    seed: parseSeed(payload.seed),
  };
}

async function handleGenerate(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand(generationCommand(payload));
    if (!result.ok) {
      sendJson(res, 500, result);
      return;
    }
    try {
      fs.writeFileSync(latestMapPath, `${JSON.stringify(result.view, null, 2)}\n`, "utf8");
    } catch (writeError) {
      sendJson(res, 500, { error: `Could not write latest-map.json: ${writeError.message}` });
      return;
    }
    sendJson(res, 200, result.view);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleCommand(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand(payload.command || {}, payload.gameId || "local-dev");
    sendJson(res, result.ok ? 200 : 400, result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleGameNew(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand({
      type: "new_game",
      width: parseDimension(payload.width, 10, 120),
      height: parseDimension(payload.height, 10, 80),
      factions: parseBoundedInteger(payload.factions, 2, 1, 3),
    });
    sendJson(res, result.ok ? 200 : 400, result.view || result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleGameSelect(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand({
      type: "select_unit",
      unitId: parseBoundedInteger(payload.unitId, 0, 0, 1000000),
    }, payload.gameId || "local-dev");
    sendJson(res, result.ok ? 200 : 400, result.view || result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleGameMove(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand({
      type: "move_unit",
      unitId: parseBoundedInteger(payload.unitId, 0, 0, 1000000),
      to: {
        q: parseBoundedInteger(payload.q, 0, 0, 1000000),
        r: parseBoundedInteger(payload.r, 0, 0, 1000000),
      },
    }, payload.gameId || "local-dev");
    sendJson(res, result.ok ? 200 : 400, result.view || result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleGameAttack(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand({
      type: "attack_unit",
      attackerId: parseBoundedInteger(payload.attackerId, 0, 0, 1000000),
      defenderId: parseBoundedInteger(payload.defenderId, 0, 0, 1000000),
    }, payload.gameId || "local-dev");
    sendJson(res, result.ok ? 200 : 400, result.view || result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleGameEndTurn(req, res) {
  try {
    const payload = await readRequestJson(req);
    const result = await daemonCommand({ type: "end_turn" }, payload.gameId || "local-dev");
    sendJson(res, result.ok ? 200 : 400, result.view || result);
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

function handleLatestTrafficLog(req, res) {
  fs.readFile(latestTrafficLogPath, "utf8", (error, content) => {
    if (error) {
      sendJson(res, 404, { error: "latest engine traffic log not found" });
      return;
    }
    res.writeHead(200, {
      "Content-Type": "application/x-ndjson; charset=utf-8",
      "Content-Length": Buffer.byteLength(content),
    });
    res.end(content);
  });
}

function serveStatic(req, res) {
  const requestPath = new URL(req.url, `http://${req.headers.host}`).pathname;
  const relativePath = requestPath === "/" ? "index.html" : requestPath.slice(1);
  const filePath = path.resolve(publicDir, relativePath);

  if (!filePath.startsWith(publicDir)) {
    res.writeHead(403);
    res.end("Forbidden");
    return;
  }

  fs.readFile(filePath, (error, content) => {
    if (error) {
      res.writeHead(404);
      res.end("Not found");
      return;
    }

    res.writeHead(200, {
      "Content-Type": mimeTypes[path.extname(filePath)] || "application/octet-stream",
      "Content-Length": content.length,
    });
    res.end(content);
  });
}

const server = http.createServer((req, res) => {
  if (req.method === "POST" && req.url === "/api/generate") {
    handleGenerate(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/command") {
    handleCommand(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/game/new") {
    handleGameNew(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/game/select") {
    handleGameSelect(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/game/move") {
    handleGameMove(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/game/attack") {
    handleGameAttack(req, res);
    return;
  }
  if (req.method === "POST" && req.url === "/api/game/end-turn") {
    handleGameEndTurn(req, res);
    return;
  }
  if (req.method === "GET" && req.url === "/api/debug/engine-traffic/latest") {
    handleLatestTrafficLog(req, res);
    return;
  }

  if (req.method === "GET" || req.method === "HEAD") {
    serveStatic(req, res);
    return;
  }

  res.writeHead(405);
  res.end("Method not allowed");
});

process.on("exit", () => {
  if (daemonProcess) {
    daemonProcess.kill();
  }
});

initializeTrafficLogs();

server.listen(port, async () => {
  try {
    await ensureDaemon();
    console.log(`Steppe proxy listening at http://localhost:${port}; daemon at ${daemonUrl}`);
    console.log(`Engine traffic log: ${latestTrafficLogPath}`);
  } catch (error) {
    console.error(error.message);
    console.log(`Steppe proxy listening at http://localhost:${port}; daemon unavailable`);
    console.log(`Engine traffic log: ${latestTrafficLogPath}`);
  }
});
