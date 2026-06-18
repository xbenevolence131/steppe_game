const http = require("http");
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");

const rootDir = path.resolve(__dirname, "..");
const publicDir = path.join(rootDir, "public");
const latestMapPath = path.join(rootDir, "latest-map.json");
const scenariosDir = path.join(rootDir, "scenarios");
const trafficLogDir = path.join(rootDir, "logs", "engine-traffic");
const latestTrafficLogPath = path.join(trafficLogDir, "latest.jsonl");
const gameTraceLogDir = path.join(rootDir, "logs", "game-trace");
const latestGameTraceLogPath = path.join(gameTraceLogDir, "latest.jsonl");
const port = Number(process.env.PORT || 3000);
const daemonPort = Number(process.env.STEPPE_DAEMON_PORT || 4001);
const daemonUrl = `http://127.0.0.1:${daemonPort}`;

let daemonProcess = null;
let trafficSequence = 0;
const trafficSessionName = new Date().toISOString().replace(/[:.]/g, "-");
const sessionTrafficLogPath = path.join(trafficLogDir, `${trafficSessionName}.jsonl`);
const sessionGameTraceLogPath = path.join(gameTraceLogDir, `${trafficSessionName}.jsonl`);
const gameTraceStates = new Map();

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

function initializeGameTraceLogs() {
  fs.mkdirSync(gameTraceLogDir, { recursive: true });
  fs.writeFileSync(latestGameTraceLogPath, "", "utf8");
}

function ensureScenariosDir() {
  fs.mkdirSync(scenariosDir, { recursive: true });
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

function appendGameTraceLog(entry) {
  const line = `${JSON.stringify(entry)}\n`;
  try {
    fs.appendFileSync(latestGameTraceLogPath, line, "utf8");
    fs.appendFileSync(sessionGameTraceLogPath, line, "utf8");
  } catch (error) {
    console.error(`Could not write game trace log: ${error.message}`);
  }
}

function coordSummary(coord) {
  return coord && Number.isInteger(coord.q) && Number.isInteger(coord.r)
    ? { q: coord.q, r: coord.r }
    : null;
}

function unitSummary(unit) {
  if (!unit || !Number.isInteger(unit.id)) {
    return null;
  }
  return {
    id: unit.id,
    owner: unit.owner,
    faction: unit.faction,
    kind: unit.kind,
    name: unit.name,
    q: unit.q,
    r: unit.r,
    hp: unit.hp,
    maxHp: unit.maxHp,
    readiness: unit.readiness,
    remainingScaledMove: unit.remainingScaledMove,
    moveDone: unit.moveDone,
    combatDone: unit.combatDone,
    movedThisTurn: unit.movedThisTurn,
    population: unit.population,
    horses: unit.horses,
  };
}

function summarizeGameView(view) {
  if (!view || !view.game) {
    return null;
  }
  const game = view.game;
  return {
    round: game.round,
    activeFactionIndex: game.activeFactionIndex,
    activeOwner: game.activeOwner,
    stateVersion: game.stateVersion,
    turnOrder: Array.isArray(game.turnOrder) ? game.turnOrder.slice() : [],
    factions: Array.isArray(game.factions)
      ? game.factions.map((faction) => ({
        id: faction.id,
        key: faction.key,
        enabled: faction.enabled,
        ai: faction.ai,
        metal: faction.metal,
        treasure: faction.treasure,
        food: faction.food,
      }))
      : [],
    aiGroups: Array.isArray(game.aiGroups)
      ? game.aiGroups.map((group) => ({
        id: group.id,
        owner: group.owner,
        name: group.name,
        role: group.role,
        generated: Boolean(group.generated),
        unitIds: Array.isArray(group.unitIds) ? group.unitIds.slice() : [],
        directive: group.directive,
      }))
      : [],
    diplomacy: Array.isArray(game.diplomacy)
      ? game.diplomacy.map((relationship) => ({
        owner: relationship.owner,
        target: relationship.target,
        affinity: relationship.affinity,
        status: relationship.status,
        aiPosture: relationship.aiPosture,
      }))
      : [],
    units: Array.isArray(view.units) ? view.units.map(unitSummary).filter(Boolean) : [],
  };
}

function summarizeCommand(command) {
  const type = command && command.type;
  if (!type) {
    return { type: "" };
  }
  if (type === "load_game") {
    const state = command.state || {};
    return {
      type,
      width: state.width,
      height: state.height,
      units: Array.isArray(state.units) ? state.units.length : 0,
      hexes: Array.isArray(state.hexes) ? state.hexes.length : 0,
      aiGroups: Array.isArray(state.game?.aiGroups) ? state.game.aiGroups.length : 0,
      round: state.game?.round,
      activeOwner: state.game?.activeOwner,
    };
  }
  if (type === "generate_map") {
    return { ...command };
  }
  const summary = { type };
  for (const key of ["unitId", "attackerId", "defenderId", "groupId", "stance", "horses"]) {
    if (command[key] !== undefined) {
      summary[key] = command[key];
    }
  }
  if (command.to) {
    summary.to = coordSummary(command.to);
  }
  return summary;
}

function indexById(items) {
  const result = new Map();
  for (const item of items || []) {
    if (item && Number.isInteger(item.id)) {
      result.set(item.id, item);
    }
  }
  return result;
}

function unitChanged(prior, current) {
  return prior.q !== current.q
    || prior.r !== current.r
    || prior.owner !== current.owner
    || prior.hp !== current.hp
    || prior.readiness !== current.readiness
    || prior.remainingScaledMove !== current.remainingScaledMove
    || prior.moveDone !== current.moveDone
    || prior.combatDone !== current.combatDone
    || prior.movedThisTurn !== current.movedThisTurn
    || prior.population !== current.population
    || prior.horses !== current.horses;
}

function computeStatePatch(before, after) {
  if (!before || !after) {
    return [];
  }
  const patch = [];
  if (before.round !== after.round
    || before.activeOwner !== after.activeOwner
    || before.activeFactionIndex !== after.activeFactionIndex) {
    patch.push({
      type: "turn",
      before: {
        round: before.round,
        activeOwner: before.activeOwner,
        activeFactionIndex: before.activeFactionIndex,
      },
      after: {
        round: after.round,
        activeOwner: after.activeOwner,
        activeFactionIndex: after.activeFactionIndex,
      },
    });
  }

  const beforeUnits = indexById(before.units);
  const afterUnits = indexById(after.units);
  for (const unit of after.units) {
    const prior = beforeUnits.get(unit.id);
    if (!prior) {
      patch.push({ type: "unit_created", unit });
      continue;
    }
    if (unitChanged(prior, unit)) {
      patch.push({ type: "unit_changed", unitId: unit.id, before: prior, after: unit });
    }
  }
  for (const unit of before.units) {
    if (!afterUnits.has(unit.id)) {
      patch.push({ type: "unit_removed", unit });
    }
  }

  const beforeFactions = indexById(before.factions);
  for (const faction of after.factions) {
    const prior = beforeFactions.get(faction.id);
    if (!prior) {
      patch.push({ type: "faction_created", faction });
      continue;
    }
    if (prior.enabled !== faction.enabled
      || prior.ai !== faction.ai
      || prior.metal !== faction.metal
      || prior.treasure !== faction.treasure
      || prior.food !== faction.food) {
      patch.push({ type: "faction_changed", owner: faction.id, before: prior, after: faction });
    }
  }
  return patch;
}

function summarizeAiAnimation(animation) {
  return Array.isArray(animation)
    ? animation.map((step) => ({
      unitId: step.unitId,
      from: coordSummary(step.from),
      to: coordSummary(step.to),
      attacks: Array.isArray(step.attacks) ? step.attacks.map(coordSummary).filter(Boolean) : [],
      attackEvents: Array.isArray(step.attackEvents)
        ? step.attackEvents.map((event) => ({
          target: coordSummary(event.target),
          defenderId: event.defenderId,
          defenderFrom: coordSummary(event.defenderFrom),
          defenderTo: coordSummary(event.defenderTo),
          defenderMoved: Boolean(event.defenderMoved),
          attackerTo: coordSummary(event.attackerTo),
          attackerMoved: Boolean(event.attackerMoved),
        }))
        : [],
    }))
    : [];
}

function appendCommandGameTrace({ sequence, timestamp, durationMs, envelope, responseStatus, responseOk, payload, error }) {
  const gameId = envelope.gameId || "local-dev";
  const before = gameTraceStates.get(gameId) || null;
  const view = payload && payload.view;
  const after = summarizeGameView(view);
  const entry = {
    schema: "steppe-game-trace.v1",
    sequence,
    timestamp,
    durationMs,
    gameId,
    command: summarizeCommand(envelope.command || {}),
    response: {
      status: responseStatus,
      ok: responseOk,
      engineOk: payload ? payload.ok : undefined,
      error: error || (payload && payload.error) || undefined,
    },
    turn: {
      before: before
        ? {
          round: before.round,
          activeOwner: before.activeOwner,
          activeFactionIndex: before.activeFactionIndex,
          stateVersion: before.stateVersion,
        }
        : null,
      after: after
        ? {
          round: after.round,
          activeOwner: after.activeOwner,
          activeFactionIndex: after.activeFactionIndex,
          stateVersion: after.stateVersion,
        }
        : null,
    },
    events: Array.isArray(view?.events) ? view.events : [],
    statePatch: computeStatePatch(before, after),
    ai: {
      animation: summarizeAiAnimation(view?.aiAnimation),
      decisions: Array.isArray(view?.aiDecisionTrace) ? view.aiDecisionTrace : [],
    },
  };
  appendGameTraceLog(entry);
  if (after) {
    gameTraceStates.set(gameId, after);
  }
}

function payloadForClient(payload) {
  if (!payload || !payload.view || !Array.isArray(payload.view.aiDecisionTrace)) {
    return payload;
  }
  return {
    ...payload,
    view: {
      ...payload.view,
      aiDecisionTrace: undefined,
    },
  };
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

function scenarioFilename(rawName) {
  const requested = String(rawName || "").trim();
  if (!requested) {
    throw new Error("scenario filename is required");
  }
  const base = path.basename(requested).replace(/[^A-Za-z0-9._ -]/g, "_");
  const filename = base.toLowerCase().endsWith(".json") ? base : `${base}.json`;
  if (filename === ".json" || filename.includes("..")) {
    throw new Error("scenario filename is invalid");
  }
  return filename;
}

function scenarioFilePath(rawName) {
  const filename = scenarioFilename(rawName);
  const filePath = path.resolve(scenariosDir, filename);
  if (!filePath.startsWith(`${scenariosDir}${path.sep}`)) {
    throw new Error("scenario filename is invalid");
  }
  return { filename, filePath };
}

function powershellString(value) {
  return `'${String(value).replace(/'/g, "''")}'`;
}

function runPowershellJson(script) {
  return new Promise((resolve, reject) => {
    const child = spawn("powershell.exe", [
      "-NoProfile",
      "-STA",
      "-ExecutionPolicy",
      "Bypass",
      "-Command",
      script,
    ], { windowsHide: false });
    let stdout = "";
    let stderr = "";
    child.stdout.on("data", (chunk) => {
      stdout += chunk;
    });
    child.stderr.on("data", (chunk) => {
      stderr += chunk;
    });
    child.on("error", reject);
    child.on("close", (code) => {
      if (code !== 0) {
        reject(new Error(stderr.trim() || `PowerShell exited with code ${code}`));
        return;
      }
      const output = stdout.trim();
      if (!output) {
        resolve({ canceled: true });
        return;
      }
      try {
        resolve(JSON.parse(output));
      } catch {
        reject(new Error("file dialog returned invalid JSON"));
      }
    });
  });
}

async function pickScenarioOpenFile() {
  ensureScenariosDir();
  const script = `
Add-Type -AssemblyName System.Windows.Forms
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$dialog = New-Object System.Windows.Forms.OpenFileDialog
$dialog.Title = 'Load Scenario'
$dialog.InitialDirectory = ${powershellString(scenariosDir)}
$dialog.Filter = 'Scenario JSON (*.json)|*.json|All files (*.*)|*.*'
$dialog.Multiselect = $false
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
  @{ path = $dialog.FileName } | ConvertTo-Json -Compress
} else {
  @{ canceled = $true } | ConvertTo-Json -Compress
}
`;
  return runPowershellJson(script);
}

async function pickScenarioSaveFile(suggestedName) {
  ensureScenariosDir();
  const script = `
Add-Type -AssemblyName System.Windows.Forms
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$dialog = New-Object System.Windows.Forms.SaveFileDialog
$dialog.Title = 'Save Scenario'
$dialog.InitialDirectory = ${powershellString(scenariosDir)}
$dialog.FileName = ${powershellString(scenarioFilename(suggestedName || "scenario.json"))}
$dialog.DefaultExt = 'json'
$dialog.AddExtension = $true
$dialog.OverwritePrompt = $true
$dialog.Filter = 'Scenario JSON (*.json)|*.json|All files (*.*)|*.*'
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
  @{ path = $dialog.FileName } | ConvertTo-Json -Compress
} else {
  @{ canceled = $true } | ConvertTo-Json -Compress
}
`;
  return runPowershellJson(script);
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
    appendCommandGameTrace({
      sequence,
      timestamp: new Date(startedAt).toISOString(),
      durationMs: Date.now() - startedAt,
      envelope,
      responseStatus: response.status,
      responseOk: response.ok,
      payload,
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
      appendCommandGameTrace({
        sequence,
        timestamp: new Date(startedAt).toISOString(),
        durationMs: Date.now() - startedAt,
        envelope,
        responseOk: false,
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
    sendJson(res, result.ok ? 200 : 400, payloadForClient(result));
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

async function handleScenarioList(req, res) {
  try {
    ensureScenariosDir();
    const entries = await fs.promises.readdir(scenariosDir, { withFileTypes: true });
    const files = entries
      .filter((entry) => entry.isFile() && entry.name.toLowerCase().endsWith(".json"))
      .map((entry) => entry.name)
      .sort((first, second) => first.localeCompare(second));
    sendJson(res, 200, { files });
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleScenarioLoad(req, res) {
  try {
    ensureScenariosDir();
    const requestUrl = new URL(req.url, `http://${req.headers.host}`);
    const { filename, filePath } = scenarioFilePath(requestUrl.searchParams.get("name"));
    const content = await fs.promises.readFile(filePath, "utf8");
    sendJson(res, 200, { filename, scenario: JSON.parse(content) });
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleScenarioSave(req, res) {
  try {
    ensureScenariosDir();
    const payload = await readRequestJson(req);
    const { filename, filePath } = scenarioFilePath(payload.name || payload.filename);
    const scenario = payload.scenario;
    if (!scenario || typeof scenario !== "object" || Array.isArray(scenario)) {
      sendJson(res, 400, { error: "scenario snapshot is required" });
      return;
    }
    await fs.promises.writeFile(filePath, `${JSON.stringify(scenario, null, 2)}\n`, "utf8");
    sendJson(res, 200, { ok: true, filename });
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleScenarioPickOpen(req, res) {
  try {
    const picked = await pickScenarioOpenFile();
    if (picked.canceled) {
      sendJson(res, 200, { canceled: true });
      return;
    }
    const content = await fs.promises.readFile(picked.path, "utf8");
    sendJson(res, 200, {
      canceled: false,
      filename: path.basename(picked.path),
      scenario: JSON.parse(content),
    });
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

async function handleScenarioPickSave(req, res) {
  try {
    const payload = await readRequestJson(req);
    const scenario = payload.scenario;
    if (!scenario || typeof scenario !== "object" || Array.isArray(scenario)) {
      sendJson(res, 400, { error: "scenario snapshot is required" });
      return;
    }
    const picked = await pickScenarioSaveFile(payload.suggestedName || payload.name || payload.filename);
    if (picked.canceled) {
      sendJson(res, 200, { canceled: true });
      return;
    }
    await fs.promises.writeFile(picked.path, `${JSON.stringify(scenario, null, 2)}\n`, "utf8");
    sendJson(res, 200, {
      canceled: false,
      filename: path.basename(picked.path),
    });
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

function handleLatestGameTraceLog(req, res) {
  fs.readFile(latestGameTraceLogPath, "utf8", (error, content) => {
    if (error) {
      sendJson(res, 404, { error: "latest game trace log not found" });
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
      "Cache-Control": "no-store",
    });
    res.end(content);
  });
}

const server = http.createServer((req, res) => {
  const requestUrl = new URL(req.url, `http://${req.headers.host}`);
  if (req.method === "POST" && requestUrl.pathname === "/api/generate") {
    handleGenerate(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/command") {
    handleCommand(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/game/new") {
    handleGameNew(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/game/select") {
    handleGameSelect(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/game/move") {
    handleGameMove(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/game/attack") {
    handleGameAttack(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/game/end-turn") {
    handleGameEndTurn(req, res);
    return;
  }
  if (req.method === "GET" && requestUrl.pathname === "/api/scenarios") {
    handleScenarioList(req, res);
    return;
  }
  if (req.method === "GET" && requestUrl.pathname === "/api/scenarios/load") {
    handleScenarioLoad(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/scenarios/save") {
    handleScenarioSave(req, res);
    return;
  }
  if (req.method === "GET" && requestUrl.pathname === "/api/scenarios/pick-open") {
    handleScenarioPickOpen(req, res);
    return;
  }
  if (req.method === "POST" && requestUrl.pathname === "/api/scenarios/pick-save") {
    handleScenarioPickSave(req, res);
    return;
  }
  if (req.method === "GET" && requestUrl.pathname === "/api/debug/engine-traffic/latest") {
    handleLatestTrafficLog(req, res);
    return;
  }
  if (req.method === "GET" && requestUrl.pathname === "/api/debug/game-trace/latest") {
    handleLatestGameTraceLog(req, res);
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
initializeGameTraceLogs();
ensureScenariosDir();

server.listen(port, async () => {
  try {
    await ensureDaemon();
    console.log(`Steppe proxy listening at http://localhost:${port}; daemon at ${daemonUrl}`);
    console.log(`Engine traffic log: ${latestTrafficLogPath}`);
    console.log(`Game trace log: ${latestGameTraceLogPath}`);
  } catch (error) {
    console.error(error.message);
    console.log(`Steppe proxy listening at http://localhost:${port}; daemon unavailable`);
    console.log(`Engine traffic log: ${latestTrafficLogPath}`);
    console.log(`Game trace log: ${latestGameTraceLogPath}`);
  }
});
