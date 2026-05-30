const http = require("http");
const fs = require("fs");
const path = require("path");
const { execFile } = require("child_process");

const rootDir = path.resolve(__dirname, "..");
const publicDir = path.join(rootDir, "public");
const latestMapPath = path.join(rootDir, "latest-map.json");
const port = Number(process.env.PORT || 3000);

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
};

function enginePath() {
  const candidates = [
    path.join(rootDir, "build", "steppe_engine.exe"),
    path.join(rootDir, "build", "steppe_engine"),
    path.join(rootDir, "build", "Debug", "steppe_engine.exe"),
    path.join(rootDir, "build", "Release", "steppe_engine.exe"),
  ];

  return candidates.find((candidate) => fs.existsSync(candidate));
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
      if (body.length > 64 * 1024) {
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

function parseSeed(value) {
  const result = Number(value);
  if (!Number.isInteger(result) || result < 0 || result > 0xffffffff) {
    return Math.floor(Math.random() * 0x100000000);
  }
  return result;
}

async function handleGenerate(req, res) {
  let payload;
  try {
    payload = await readRequestJson(req);
  } catch (error) {
    sendJson(res, 400, { error: error.message });
    return;
  }

  const executable = enginePath();
  if (!executable) {
    sendJson(res, 500, {
      error: "Engine executable not found. Build it with: cmake -S . -B build -G Ninja && cmake --build build",
    });
    return;
  }

  const width = parseDimension(payload.width, 120, 120);
  const height = parseDimension(payload.height, 80, 80);
  const rivers = parseBoundedInteger(payload.rivers, 4, 0, 20);
  const seed = parseSeed(payload.seed);

  execFile(executable, [
    "generate",
    "--width", String(width),
    "--height", String(height),
    "--rivers", String(rivers),
    "--seed", String(seed),
  ], {
    cwd: rootDir,
    windowsHide: true,
    maxBuffer: 8 * 1024 * 1024,
  }, (error, stdout, stderr) => {
    if (error) {
      sendJson(res, 500, { error: stderr.trim() || error.message });
      return;
    }

    let map;
    try {
      map = JSON.parse(stdout);
    } catch {
      sendJson(res, 500, { error: "Engine returned invalid JSON" });
      return;
    }

    try {
      fs.writeFileSync(latestMapPath, `${JSON.stringify(map, null, 2)}\n`, "utf8");
    } catch (writeError) {
      sendJson(res, 500, { error: `Could not write latest-map.json: ${writeError.message}` });
      return;
    }

    sendJson(res, 200, map);
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

  if (req.method === "GET" || req.method === "HEAD") {
    serveStatic(req, res);
    return;
  }

  res.writeHead(405);
  res.end("Method not allowed");
});

server.listen(port, () => {
  console.log(`Steppe Terrain Generator proxy listening at http://localhost:${port}`);
});
