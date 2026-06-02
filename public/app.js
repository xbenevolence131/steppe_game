const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const appShell = document.querySelector("#app-shell");
const mapPanel = document.querySelector("#map-panel");
const scenarioModeButton = document.querySelector("#scenario-mode-button");
const playModeButton = document.querySelector("#play-mode-button");
const widthInput = document.querySelector("#map-width");
const heightInput = document.querySelector("#map-height");
const riversInput = document.querySelector("#river-sources");
const lakesInput = document.querySelector("#lake-count");
const lakeSizeInput = document.querySelector("#lake-size");
const meanderForwardInput = document.querySelector("#meander-forward");
const meanderForwardJitterInput = document.querySelector("#meander-forward-jitter");
const meanderLateralInput = document.querySelector("#meander-lateral");
const meanderLateralJitterInput = document.querySelector("#meander-lateral-jitter");
const meanderStrengthInput = document.querySelector("#meander-strength");
const meanderReachInput = document.querySelector("#meander-reach");
const riverSlantStrengthInput = document.querySelector("#river-slant-strength");
const valleyThicknessInput = document.querySelector("#valley-thickness");
const forestBlobsInput = document.querySelector("#forest-blobs");
const forestBlobRadiusInput = document.querySelector("#forest-blob-radius");
const meanderTimeoutInput = document.querySelector("#meander-timeout");
const generateButton = document.querySelector("#generate-button");
const blankMapButton = document.querySelector("#blank-map-button");
const saveButton = document.querySelector("#save-button");
const loadButton = document.querySelector("#load-button");
const loadFileInput = document.querySelector("#load-file-input");
const zoomInButton = document.querySelector("#zoom-in-button");
const zoomOutButton = document.querySelector("#zoom-out-button");
const fitButton = document.querySelector("#fit-button");
const editModeButton = document.querySelector("#edit-mode-button");
const editorToolSelect = document.querySelector("#editor-tool");
const terrainPalette = document.querySelector("#terrain-palette");
const endTurnButton = document.querySelector("#end-turn-button");
const controlEndTurnButton = document.querySelector("#control-end-turn-button");
const turnCounter = document.querySelector(".turn-counter");
const activeFactionName = document.querySelector("#active-faction-name");
const roundCount = document.querySelector("#round-count");
const turnStatusReadout = document.querySelector("#turn-status-readout");
const campCount = document.querySelector("#camp-count");
const herdCount = document.querySelector("#herd-count");
const cavalryCount = document.querySelector("#cavalry-count");
const sidebarSelectionReadout = document.querySelector(".sidebar-selection-readout");
const unitName = document.querySelector("#unit-name");
const unitHp = document.querySelector("#unit-hp");
const unitMove = document.querySelector("#unit-move");

let currentMap = null;
let appMode = "intro";
let isPanning = false;
let isEditing = false;
let isPainting = false;
let lastPointer = { x: 0, y: 0 };
let selectedTerrain = "lake";
let paintStrokeKeys = new Set();
let terrainUndo = new Map();
let currentTurn = 1;
let activeFactionIndex = 0;
let selectedUnitId = null;
let reachableHexes = new Map();

const viewport = {
  scale: 1,
  offsetX: 0,
  offsetY: 0,
  width: 0,
  height: 0,
  minScale: 0.1,
  maxScale: 3,
};

const geometry = {
  margin: 22,
  size: 16,
  width: 0,
  height: 0,
};

const editorTerrains = [
  { key: "grassland", label: "Grassland", fill: "#d8c596", stroke: "#7e735f", labelColor: "#29251d" },
  { key: "none", label: "None", fill: "#111111", stroke: "#5b5f5b", labelColor: "#eeeeee" },
  { key: "lake", label: "Lake", fill: "#82c7e6", stroke: "#245e78", labelColor: "#082c3a" },
  { key: "hill", label: "Hill", fill: "#b98c56", stroke: "#6f4f2f", labelColor: "#21170f" },
  { key: "mountain", label: "Mountain", fill: "#5b3724", stroke: "#2c1a11", labelColor: "#f1e7d8" },
  { key: "forest", label: "Forest", fill: "#246b3b", stroke: "#133c22", labelColor: "#eef7e8" },
  { key: "marsh", label: "Marsh", fill: "#74794b", stroke: "#42462a", labelColor: "#f3eed0" },
  { key: "desert", label: "Desert", fill: "#d3b766", stroke: "#80692d", labelColor: "#241c0d" },
  { key: "urban", label: "Urban", fill: "#8e8e8e", stroke: "#4e4e4e", labelColor: "#111111" },
];

const terrainStyles = {
  none: terrainStyle("none"),
  grassland: terrainStyle("grassland"),
  lake: terrainStyle("lake"),
  hill: terrainStyle("hill"),
  hills: terrainStyle("hill"),
  mountain: terrainStyle("mountain"),
  mountains: terrainStyle("mountain"),
  forest: terrainStyle("forest"),
  woods: terrainStyle("forest"),
  light_forest: terrainStyle("forest"),
  heavy_forest: terrainStyle("forest"),
  forest_blob: {
    fill: "#2f8f4e",
    stroke: "#0f4d27",
    label: "#eef7e8",
  },
  marsh: terrainStyle("marsh"),
  desert: terrainStyle("desert"),
  urban: terrainStyle("urban"),
  persian_town: {
    fill: "#8a4fb0",
    stroke: "#4b2468",
    label: "#f7eefb",
  },
  chinese_town: {
    fill: "#c93632",
    stroke: "#7a1515",
    label: "#fff0ea",
  },
  oasis: {
    fill: "#3f9f8a",
    stroke: "#1e5f55",
    label: "#ecfff9",
  },
  dzungarian_gate: {
    fill: "#d2a84a",
    stroke: "#7c5820",
    label: "#20170c",
  },
  river: {
    stroke: "#2679a6",
    source: "#60c4e8",
    merge: "#f4e48a",
    destination: "#1f5f83",
  },
  road: {
    stroke: "#8b5a2b",
    persian: "#7a4a92",
    chinese: "#aa3f2c",
    silk: "#c28a2c",
  },
  crossing: {
    bridge: "#b8b8b8",
    ford: "#f4f4f4",
    outline: "#2a2118",
  },
};

const factions = {
  mongol: {
    name: "Mongol",
    color: "#2368c4",
  },
  chinese: {
    name: "Chinese",
    color: "#c93632",
  },
  persian: {
    name: "Persian",
    color: "#8a4fb0",
  },
};

const factionCount = 2;
const factionTurnOrder = ["mongol", "chinese", "persian"].slice(0, factionCount);

const unitDefaults = {
  cavalry: {
    hp: 10,
    move: 4,
  },
};

function terrainStyle(key) {
  const terrain = editorTerrains.find((entry) => entry.key === key);
  return {
    fill: terrain.fill,
    stroke: terrain.stroke,
    label: terrain.labelColor,
  };
}

function clampDimension(value, min, max, fallback) {
  const parsed = Number(value);
  if (!Number.isInteger(parsed)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, parsed));
}

function clampNumberInput(input, min, max, fallback) {
  const parsed = Number(input.value);
  const value = Number.isFinite(parsed) ? Math.max(min, Math.min(max, parsed)) : fallback;
  input.value = value;
  return value;
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function newSeed() {
  const values = new Uint32Array(1);
  window.crypto.getRandomValues(values);
  return values[0];
}

function resizeCanvas() {
  const dpr = window.devicePixelRatio || 1;
  const rect = mapPanel.getBoundingClientRect();
  viewport.width = Math.max(1, Math.floor(rect.width));
  viewport.height = Math.max(1, Math.floor(rect.height));
  canvas.style.width = `${viewport.width}px`;
  canvas.style.height = `${viewport.height}px`;
  canvas.width = Math.round(viewport.width * dpr);
  canvas.height = Math.round(viewport.height * dpr);
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

function hexPoints(cx, cy, size) {
  const points = [];
  for (let i = 0; i < 6; i += 1) {
    const angle = Math.PI / 180 * (60 * i);
    points.push([
      cx + size * Math.cos(angle),
      cy + size * Math.sin(angle),
    ]);
  }
  return points;
}

function hexCenter(coord) {
  const col = coord.q - 1;
  const row = coord.r - 1;
  const hexHeight = Math.sqrt(3) * geometry.size;
  return {
    x: geometry.margin + geometry.size + col * geometry.size * 1.5,
    y: geometry.margin + hexHeight / 2 + row * hexHeight + (col % 2) * hexHeight / 2,
  };
}

function edgeBoundaryPoints(edge) {
  const firstCenter = hexCenter(edge.a);
  const secondCenter = hexCenter(edge.b);
  const firstCorners = hexPoints(firstCenter.x, firstCenter.y, geometry.size);
  const secondCorners = hexPoints(secondCenter.x, secondCenter.y, geometry.size);
  const shared = [];

  for (const first of firstCorners) {
    for (const second of secondCorners) {
      if (Math.hypot(first[0] - second[0], first[1] - second[1]) < 0.001) {
        shared.push(first);
      }
    }
  }

  return shared.length === 2 ? shared : null;
}

function coordKey(coord) {
  return `${coord.q},${coord.r}`;
}

function coordLess(first, second) {
  return first.r === second.r ? first.q < second.q : first.r < second.r;
}

function canonicalEdge(first, second) {
  return coordLess(second, first)
    ? { a: { ...second }, b: { ...first }, river: true }
    : { a: { ...first }, b: { ...second }, river: true };
}

function edgeKey(edge) {
  return `${coordKey(edge.a)}|${coordKey(edge.b)}`;
}

function topologyVertexKey(vertex) {
  return `${vertex.x},${vertex.y}`;
}

function compareTopologyVertexKeys(first, second) {
  const [firstX, firstY] = first.split(",").map(Number);
  const [secondX, secondY] = second.split(",").map(Number);
  return firstY === secondY ? firstX - secondX : firstY - secondY;
}

function topologyHexCorners(coord) {
  const scale = 1000000;
  const col = coord.q - 1;
  const row = coord.r - 1;
  const centerX = col * 1.5;
  const centerY = Math.sqrt(3) * (row + (col % 2) * 0.5);
  const corners = [];

  for (let i = 0; i < 6; i += 1) {
    const angle = Math.PI / 180 * (60 * i);
    corners.push({
      x: Math.round((centerX + Math.cos(angle)) * scale),
      y: Math.round((centerY + Math.sin(angle)) * scale),
    });
  }

  return corners;
}

function topologyEdgeBoundary(edge) {
  const firstCorners = topologyHexCorners(edge.a);
  const secondCorners = topologyHexCorners(edge.b);
  const secondKeys = new Map(secondCorners.map((corner) => [topologyVertexKey(corner), corner]));
  const shared = firstCorners.filter((corner) => secondKeys.has(topologyVertexKey(corner)));
  if (shared.length !== 2) {
    return null;
  }
  return shared.sort((first, second) => first.y === second.y ? first.x - second.x : first.y - second.y);
}

function addGraphEdge(graph, firstKey, secondKey) {
  if (!graph.has(firstKey)) {
    graph.set(firstKey, new Set());
  }
  if (!graph.has(secondKey)) {
    graph.set(secondKey, new Set());
  }
  graph.get(firstKey).add(secondKey);
  graph.get(secondKey).add(firstKey);
}

function deriveLakeRiverConnections(map) {
  if (!map || !Array.isArray(map.hexes) || !Array.isArray(map.edges)) {
    return [];
  }

  const verticesByKey = new Map();
  const lakeHexesByVertex = new Map();
  for (const hex of map.hexes) {
    if (hex.terrain !== "lake") {
      continue;
    }
    const coord = { q: Number(hex.q), r: Number(hex.r) };
    if (!Number.isInteger(coord.q) || !Number.isInteger(coord.r)) {
      continue;
    }
    for (const vertex of topologyHexCorners(coord)) {
      const key = topologyVertexKey(vertex);
      verticesByKey.set(key, vertex);
      if (!lakeHexesByVertex.has(key)) {
        lakeHexesByVertex.set(key, []);
      }
      lakeHexesByVertex.get(key).push(coord);
    }
  }

  const graph = new Map();
  const incidentEdges = new Map();
  for (const edge of map.edges) {
    if (!edge || !edge.a || !edge.b) {
      continue;
    }
    const canonical = canonicalEdge(edge.a, edge.b);
    const boundary = topologyEdgeBoundary(canonical);
    if (!boundary) {
      continue;
    }
    const firstKey = topologyVertexKey(boundary[0]);
    const secondKey = topologyVertexKey(boundary[1]);
    verticesByKey.set(firstKey, boundary[0]);
    verticesByKey.set(secondKey, boundary[1]);
    addGraphEdge(graph, firstKey, secondKey);
    if (!incidentEdges.has(firstKey)) {
      incidentEdges.set(firstKey, []);
    }
    if (!incidentEdges.has(secondKey)) {
      incidentEdges.set(secondKey, []);
    }
    incidentEdges.get(firstKey).push(canonical);
    incidentEdges.get(secondKey).push(canonical);
  }

  const connections = [];
  const seen = new Set();
  let componentId = 0;
  const sortedVertices = [...graph.keys()].sort(compareTopologyVertexKeys);
  for (const startKey of sortedVertices) {
    if (seen.has(startKey)) {
      continue;
    }
    componentId += 1;
    const stack = [startKey];
    const componentVertices = [];
    seen.add(startKey);

    while (stack.length > 0) {
      const current = stack.pop();
      componentVertices.push(current);
      for (const next of graph.get(current) || []) {
        if (!seen.has(next)) {
          seen.add(next);
          stack.push(next);
        }
      }
    }

    let terminalIndex = 0;
    const terminals = componentVertices
      .filter((key) => (graph.get(key) || new Set()).size === 1)
      .sort(compareTopologyVertexKeys);
    for (const terminalKey of terminals) {
      terminalIndex += 1;
      const lakeHexes = lakeHexesByVertex.get(terminalKey);
      if (!lakeHexes) {
        continue;
      }
      connections.push({
        id: connections.length + 1,
        kind: "river_terminal_to_lake_vertex",
        river_component: componentId,
        terminal_index: terminalIndex,
        vertex: verticesByKey.get(terminalKey),
        lake_hexes: lakeHexes.map((coord) => ({ ...coord })),
        river_edge: canonicalEdge(incidentEdges.get(terminalKey)[0].a, incidentEdges.get(terminalKey)[0].b),
      });
    }
  }

  return connections;
}

function refreshDerivedTopology(map = currentMap) {
  if (!map) {
    return null;
  }
  map.lake_river_connections = deriveLakeRiverConnections(map);
  map.metadata = map.metadata && typeof map.metadata === "object" ? map.metadata : {};
  map.metadata.lake_river_connection_model = "river-terminal-lake-vertex.v1";
  return map;
}

function neighborInDirection(coord, direction) {
  const shiftedDown = (coord.q - 1) % 2 === 1;
  switch (direction) {
    case 0: return { q: coord.q + 1, r: coord.r };
    case 1: return shiftedDown ? { q: coord.q + 1, r: coord.r + 1 } : { q: coord.q + 1, r: coord.r - 1 };
    case 2: return { q: coord.q, r: coord.r - 1 };
    case 3: return { q: coord.q - 1, r: coord.r };
    case 4: return shiftedDown ? { q: coord.q - 1, r: coord.r + 1 } : { q: coord.q - 1, r: coord.r - 1 };
    case 5: return { q: coord.q, r: coord.r + 1 };
    default: return coord;
  }
}

function inBounds(coord) {
  return currentMap && coord.q >= 1 && coord.q <= currentMap.width && coord.r >= 1 && coord.r <= currentMap.height;
}

function hexAtCoord(coord) {
  if (!currentMap || !Array.isArray(currentMap.hexes)) {
    return null;
  }
  return currentMap.hexes.find((hex) => hex.q === coord.q && hex.r === coord.r) || null;
}

function unitAtCoord(coord, exceptUnitId = null) {
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return null;
  }
  return currentMap.units.find((unit) => unit.id !== exceptUnitId && unit.q === coord.q && unit.r === coord.r) || null;
}

function movementCostForHex(hex) {
  return hex && hex.terrain === "grassland" ? 1 : Infinity;
}

function panelToWorld(event) {
  const rect = mapPanel.getBoundingClientRect();
  return {
    x: (event.clientX - rect.left - viewport.offsetX) / viewport.scale,
    y: (event.clientY - rect.top - viewport.offsetY) / viewport.scale,
  };
}

function distanceToSegment(point, first, second) {
  const dx = second.x - first.x;
  const dy = second.y - first.y;
  const lengthSquared = dx * dx + dy * dy;
  if (lengthSquared === 0) {
    return Math.hypot(point.x - first.x, point.y - first.y);
  }
  const t = clamp(((point.x - first.x) * dx + (point.y - first.y) * dy) / lengthSquared, 0, 1);
  const x = first.x + dx * t;
  const y = first.y + dy * t;
  return Math.hypot(point.x - x, point.y - y);
}

function findNearestHex(point) {
  if (!currentMap) {
    return null;
  }

  let best = null;
  let bestDistance = geometry.size * 0.95;
  for (const hex of currentMap.hexes) {
    const center = hexCenter(hex);
    const distance = Math.hypot(point.x - center.x, point.y - center.y);
    if (distance < bestDistance) {
      best = hex;
      bestDistance = distance;
    }
  }
  return best;
}

function findNearestEditableEdge(point) {
  if (!currentMap) {
    return null;
  }

  const hex = findNearestHex(point);
  if (!hex) {
    return null;
  }

  let best = null;
  let bestDistance = geometry.size * 0.42;
  for (let direction = 0; direction < 6; direction += 1) {
    const neighbor = neighborInDirection(hex, direction);
    if (!inBounds(neighbor)) {
      continue;
    }
    const edge = canonicalEdge(hex, neighbor);
    const boundary = edgeBoundaryPoints(edge);
    if (!boundary) {
      continue;
    }
    const distance = distanceToSegment(
      point,
      { x: boundary[0][0], y: boundary[0][1] },
      { x: boundary[1][0], y: boundary[1][1] },
    );
    if (distance < bestDistance) {
      best = edge;
      bestDistance = distance;
    }
  }
  return best;
}

function updateGeometry(map) {
  const hexHeight = Math.sqrt(3) * geometry.size;
  geometry.width = geometry.margin * 2 + geometry.size * (1.5 * Math.max(1, map.width - 1) + 2);
  geometry.height = geometry.margin * 2 + hexHeight * (map.height + 0.5);
}

function fitMap() {
  if (!currentMap) {
    return;
  }
  resizeCanvas();
  updateGeometry(currentMap);
  const scaleX = viewport.width / geometry.width;
  const scaleY = viewport.height / geometry.height;
  viewport.scale = clamp(Math.min(scaleX, scaleY), viewport.minScale, viewport.maxScale);
  viewport.offsetX = (viewport.width - geometry.width * viewport.scale) / 2;
  viewport.offsetY = (viewport.height - geometry.height * viewport.scale) / 2;
  drawMap();
}

function constrainViewport() {
  const scaledWidth = geometry.width * viewport.scale;
  const scaledHeight = geometry.height * viewport.scale;

  if (scaledWidth <= viewport.width) {
    viewport.offsetX = (viewport.width - scaledWidth) / 2;
  } else {
    viewport.offsetX = clamp(viewport.offsetX, viewport.width - scaledWidth, 0);
  }

  if (scaledHeight <= viewport.height) {
    viewport.offsetY = (viewport.height - scaledHeight) / 2;
  } else {
    viewport.offsetY = clamp(viewport.offsetY, viewport.height - scaledHeight, 0);
  }
}

function zoomAt(panelX, panelY, factor) {
  if (!currentMap) {
    return;
  }

  const nextScale = clamp(viewport.scale * factor, viewport.minScale, viewport.maxScale);
  if (nextScale === viewport.scale) {
    return;
  }

  const worldX = (panelX - viewport.offsetX) / viewport.scale;
  const worldY = (panelY - viewport.offsetY) / viewport.scale;
  viewport.scale = nextScale;
  viewport.offsetX = panelX - worldX * viewport.scale;
  viewport.offsetY = panelY - worldY * viewport.scale;
  constrainViewport();
  drawMap();
}

function zoomFromCenter(factor) {
  zoomAt(viewport.width / 2, viewport.height / 2, factor);
}

function panBy(dx, dy) {
  if (!currentMap) {
    return;
  }
  viewport.offsetX += dx;
  viewport.offsetY += dy;
  constrainViewport();
  drawMap();
}

function stopPanning() {
  isPanning = false;
  mapPanel.classList.remove("is-panning");
}

function stopPainting() {
  isPainting = false;
  paintStrokeKeys = new Set();
}

function activeFactionKey() {
  return factionTurnOrder[activeFactionIndex] || factionTurnOrder[0] || "mongol";
}

function activeFaction() {
  return factions[activeFactionKey()] || factions.mongol;
}

function canActWithUnit(unit) {
  return Boolean(unit && unit.faction === activeFactionKey());
}

function countUnits(kind, factionKey = null) {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.filter((unit) => unit.kind === kind && (!factionKey || unit.faction === factionKey)).length
    : 0;
}

function selectedUnit() {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.find((unit) => unit.id === selectedUnitId) || null
    : null;
}

function calculateReachableHexes(unit) {
  const reachable = new Map();
  if (!canActWithUnit(unit) || !Number.isFinite(unit.remainingMove) || unit.remainingMove <= 0) {
    return reachable;
  }

  const startKey = coordKey(unit);
  const bestCosts = new Map([[startKey, 0]]);
  const queue = [{ q: unit.q, r: unit.r, cost: 0 }];

  for (let index = 0; index < queue.length; index += 1) {
    const current = queue[index];
    for (let direction = 0; direction < 6; direction += 1) {
      const next = neighborInDirection(current, direction);
      if (!inBounds(next) || unitAtCoord(next, unit.id)) {
        continue;
      }
      const hex = hexAtCoord(next);
      const stepCost = movementCostForHex(hex);
      const nextCost = current.cost + stepCost;
      if (!Number.isFinite(stepCost) || nextCost > unit.remainingMove) {
        continue;
      }
      const key = coordKey(next);
      if (bestCosts.has(key) && bestCosts.get(key) <= nextCost) {
        continue;
      }
      bestCosts.set(key, nextCost);
      reachable.set(key, { coord: next, cost: nextCost });
      queue.push({ q: next.q, r: next.r, cost: nextCost });
    }
  }

  return reachable;
}

function refreshReachableHexes() {
  reachableHexes = appMode === "play" ? calculateReachableHexes(selectedUnit()) : new Map();
}

function resetMovementForFaction(factionKey) {
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return;
  }
  for (const unit of currentMap.units) {
    if (unit.faction === factionKey) {
      unit.remainingMove = unit.move;
    }
  }
}

function resetTurnState() {
  currentTurn = 1;
  activeFactionIndex = 0;
  resetMovementForFaction(activeFactionKey());
}

function createCavalryUnit(id, faction, q, r) {
  return {
    id,
    faction,
    kind: "cavalry",
    q,
    r,
    hp: unitDefaults.cavalry.hp,
    maxHp: unitDefaults.cavalry.hp,
    move: unitDefaults.cavalry.move,
    remainingMove: unitDefaults.cavalry.move,
  };
}

function normalizeUnit(unit, index) {
  const defaults = unitDefaults[unit.kind] || unitDefaults.cavalry;
  const move = Number.isFinite(unit.move) ? unit.move : defaults.move;
  const hp = Number.isFinite(unit.hp) ? unit.hp : defaults.hp;
  return {
    id: Number.isInteger(unit.id) ? unit.id : index + 1,
    faction: typeof unit.faction === "string" && factions[unit.faction] ? unit.faction : "mongol",
    kind: typeof unit.kind === "string" ? unit.kind : "cavalry",
    q: unit.q,
    r: unit.r,
    hp,
    maxHp: Number.isFinite(unit.maxHp) ? unit.maxHp : hp,
    move,
    remainingMove: Number.isFinite(unit.remainingMove) ? unit.remainingMove : move,
  };
}

function unitDisplayName(unit) {
  const faction = factions[unit.faction] || factions.mongol;
  const kind = unit.kind === "cavalry" ? "Cavalry" : unit.kind;
  return `${faction.name} ${kind}`;
}

function syncUnitInspector() {
  const unit = selectedUnit();
  if (!unit) {
    sidebarSelectionReadout.textContent = "None";
    unitName.textContent = "None";
    unitHp.textContent = "-";
    unitMove.textContent = "-";
    return;
  }
  sidebarSelectionReadout.textContent = unitDisplayName(unit);
  unitName.textContent = unitDisplayName(unit);
  unitHp.textContent = `${unit.hp}/${unit.maxHp}`;
  unitMove.textContent = `${unit.remainingMove}/${unit.move}`;
}

function syncPlayControls() {
  const faction = activeFaction();
  const factionKey = activeFactionKey();
  turnCounter.textContent = `Round ${currentTurn} · ${faction.name} turn`;
  activeFactionName.textContent = faction.name;
  roundCount.textContent = String(currentTurn);
  turnStatusReadout.textContent = `${faction.name} turn`;
  campCount.textContent = String(countUnits("camp", factionKey));
  herdCount.textContent = String(countUnits("herd", factionKey));
  cavalryCount.textContent = String(countUnits("cavalry", factionKey));
  syncUnitInspector();
}

function syncModeControls() {
  appShell.classList.toggle("is-intro", appMode === "intro");
  appShell.classList.toggle("is-scenario", appMode === "scenario");
  appShell.classList.toggle("is-play", appMode === "play");
  scenarioModeButton.classList.toggle("is-active", appMode === "scenario");
  playModeButton.classList.toggle("is-active", appMode === "play");
  scenarioModeButton.setAttribute("aria-pressed", String(appMode === "scenario"));
  playModeButton.setAttribute("aria-pressed", String(appMode === "play"));
  syncPlayControls();
}

function setAppMode(mode) {
  appMode = mode;
  if (mode !== "scenario") {
    setEditMode(false);
  }
  refreshReachableHexes();
  syncModeControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function styleForHex(hex) {
  const labels = hex.labels || [];
  if (hex.terrain === "urban" && labels.includes("persian_town")) {
    return terrainStyles.persian_town;
  }
  if (hex.terrain === "urban" && labels.includes("chinese_town")) {
    return terrainStyles.chinese_town;
  }
  if (hex.terrain === "urban" && labels.includes("oasis")) {
    return terrainStyles.oasis;
  }
  if (hex.terrain === "urban" && labels.includes("dzungarian_gate")) {
    return terrainStyles.dzungarian_gate;
  }
  if (labels.includes("forest_blob")) {
    return terrainStyles.forest_blob;
  }
  return terrainStyles[hex.terrain] || terrainStyles.none;
}

function drawHex(cx, cy, size, label, hex) {
  const style = styleForHex(hex);
  const points = hexPoints(cx, cy, size + 0.15);
  ctx.beginPath();
  points.forEach(([x, y], index) => {
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });
  ctx.closePath();
  ctx.fillStyle = style.fill;
  ctx.fill();
  ctx.strokeStyle = style.stroke;
  ctx.lineWidth = 1 / viewport.scale;
  ctx.stroke();

  const visibleSize = size * viewport.scale;
  if (visibleSize < 8) {
    return;
  }

  ctx.fillStyle = style.label;
  ctx.font = `${Math.max(8, Math.min(13, visibleSize * 0.42)) / viewport.scale}px Segoe UI, Arial, sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, cx, cy);
}

function drawRiverEdges(edges) {
  if (!edges || edges.length === 0) {
    return;
  }

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";
  ctx.strokeStyle = terrainStyles.river.stroke;
  ctx.lineWidth = 3.5 / viewport.scale;

  for (const edge of edges) {
    if (!edge.river) {
      continue;
    }
    const boundary = edgeBoundaryPoints(edge);
    if (!boundary) {
      continue;
    }
    ctx.beginPath();
    ctx.moveTo(boundary[0][0], boundary[0][1]);
    ctx.lineTo(boundary[1][0], boundary[1][1]);
    ctx.stroke();
  }

  ctx.restore();
}

function drawRoads(roads) {
  if (!roads || roads.length === 0) {
    return;
  }

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  for (const road of roads) {
    if (!Array.isArray(road.path) || road.path.length < 2) {
      continue;
    }
    if (road.feature === "silk_road") {
      ctx.strokeStyle = terrainStyles.road.silk;
      ctx.lineWidth = 4 / viewport.scale;
    } else {
      ctx.strokeStyle = road.feature === "persian_town"
        ? terrainStyles.road.persian
        : (road.feature === "chinese_town" ? terrainStyles.road.chinese : terrainStyles.road.stroke);
      ctx.lineWidth = 2.5 / viewport.scale;
    }
    const start = hexCenter(road.path[0]);
    ctx.beginPath();
    ctx.moveTo(start.x, start.y);
    for (const coord of road.path.slice(1)) {
      const center = hexCenter(coord);
      ctx.lineTo(center.x, center.y);
    }
    ctx.stroke();
  }

  ctx.restore();
}

function drawCrossings(crossings) {
  if (!crossings || crossings.length === 0) {
    return;
  }

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  for (const crossing of crossings) {
    if (!crossing.edge || !crossing.edge.a || !crossing.edge.b) {
      continue;
    }
    const start = hexCenter(crossing.edge.a);
    const end = hexCenter(crossing.edge.b);
    ctx.strokeStyle = terrainStyles.crossing.outline;
    ctx.lineWidth = 5.5 / viewport.scale;
    ctx.beginPath();
    ctx.moveTo(start.x, start.y);
    ctx.lineTo(end.x, end.y);
    ctx.stroke();

    ctx.strokeStyle = crossing.kind === "bridge"
      ? terrainStyles.crossing.bridge
      : terrainStyles.crossing.ford;
    ctx.lineWidth = 3.25 / viewport.scale;
    ctx.beginPath();
    ctx.moveTo(start.x, start.y);
    ctx.lineTo(end.x, end.y);
    ctx.stroke();
  }

  ctx.restore();
}

function drawMapMarkers(coords, fillStyle, radius) {
  if (!coords || coords.length === 0) {
    return;
  }

  ctx.save();
  ctx.fillStyle = fillStyle;
  ctx.strokeStyle = "#1c1d1b";
  ctx.lineWidth = 1 / viewport.scale;
  for (const coord of coords) {
    const center = hexCenter(coord);
    ctx.beginPath();
    ctx.arc(center.x, center.y, radius / viewport.scale, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
  }
  ctx.restore();
}

function roundedRectPath(x, y, width, height, radius) {
  const corner = Math.min(radius, width / 2, height / 2);
  ctx.beginPath();
  ctx.moveTo(x + corner, y);
  ctx.lineTo(x + width - corner, y);
  ctx.quadraticCurveTo(x + width, y, x + width, y + corner);
  ctx.lineTo(x + width, y + height - corner);
  ctx.quadraticCurveTo(x + width, y + height, x + width - corner, y + height);
  ctx.lineTo(x + corner, y + height);
  ctx.quadraticCurveTo(x, y + height, x, y + height - corner);
  ctx.lineTo(x, y + corner);
  ctx.quadraticCurveTo(x, y, x + corner, y);
  ctx.closePath();
}

function drawUnitCounters(units) {
  if (!units || units.length === 0) {
    return;
  }

  ctx.save();
  for (const unit of units) {
    const faction = factions[unit.faction] || factions.mongol;
    const center = hexCenter(unit);
    const counterWidth = 38 / viewport.scale;
    const counterHeight = 23 / viewport.scale;
    const x = center.x - counterWidth / 2;
    const y = center.y - counterHeight / 2;
    const dividerX = x + 19 / viewport.scale;

    roundedRectPath(x, y, counterWidth, counterHeight, 4 / viewport.scale);
    ctx.fillStyle = "#fffdf8";
    ctx.fill();
    ctx.strokeStyle = faction.color;
    ctx.lineWidth = (unit.id === selectedUnitId ? 4.5 : 2.5) / viewport.scale;
    ctx.stroke();
    if (unit.id === selectedUnitId) {
      roundedRectPath(x - 3 / viewport.scale, y - 3 / viewport.scale, counterWidth + 6 / viewport.scale, counterHeight + 6 / viewport.scale, 6 / viewport.scale);
      ctx.strokeStyle = "#f4e48a";
      ctx.lineWidth = 2 / viewport.scale;
      ctx.stroke();
    }

    ctx.beginPath();
    ctx.moveTo(dividerX, y + 3 / viewport.scale);
    ctx.lineTo(dividerX, y + counterHeight - 3 / viewport.scale);
    ctx.strokeStyle = "#b9ad96";
    ctx.lineWidth = 1 / viewport.scale;
    ctx.stroke();

    ctx.beginPath();
    ctx.ellipse(x + 9.5 / viewport.scale, y + counterHeight / 2, 5.5 / viewport.scale, 3.1 / viewport.scale, 0, 0, Math.PI * 2);
    ctx.fillStyle = faction.color;
    ctx.fill();

    ctx.fillStyle = "#22201b";
    ctx.font = `${12 / viewport.scale}px Segoe UI, Arial, sans-serif`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(unit.hp), x + 28.5 / viewport.scale, y + counterHeight / 2 + 0.5 / viewport.scale);
  }
  ctx.restore();
}

function drawReachableHexes() {
  if (reachableHexes.size === 0) {
    return;
  }

  ctx.save();
  for (const { coord } of reachableHexes.values()) {
    const center = hexCenter(coord);
    const points = hexPoints(center.x, center.y, geometry.size * 0.78);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(244, 228, 138, 0.38)";
    ctx.fill();
    ctx.strokeStyle = "#8a6f24";
    ctx.lineWidth = 1.5 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function findUnitAtPoint(point) {
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return null;
  }
  const counterWidth = 38 / viewport.scale;
  const counterHeight = 23 / viewport.scale;
  for (let index = currentMap.units.length - 1; index >= 0; index -= 1) {
    const unit = currentMap.units[index];
    const center = hexCenter(unit);
    if (
      point.x >= center.x - counterWidth / 2
      && point.x <= center.x + counterWidth / 2
      && point.y >= center.y - counterHeight / 2
      && point.y <= center.y + counterHeight / 2
    ) {
      return unit;
    }
  }
  return null;
}

function selectUnit(unit) {
  selectedUnitId = canActWithUnit(unit) ? unit.id : null;
  refreshReachableHexes();
  syncModeControls();
  drawMap();
}

function moveSelectedUnitTo(coord) {
  const unit = selectedUnit();
  if (!canActWithUnit(unit)) {
    return false;
  }
  const move = reachableHexes.get(coordKey(coord));
  if (!move) {
    return false;
  }
  unit.q = coord.q;
  unit.r = coord.r;
  unit.remainingMove = Math.max(0, unit.remainingMove - move.cost);
  refreshReachableHexes();
  syncModeControls();
  drawMap();
  return true;
}

function advanceTurn() {
  selectedUnitId = null;
  reachableHexes = new Map();
  activeFactionIndex = (activeFactionIndex + 1) % factionTurnOrder.length;
  if (activeFactionIndex === 0) {
    currentTurn += 1;
  }
  resetMovementForFaction(activeFactionKey());
  syncModeControls();
  drawMap();
}

function drawMap() {
  if (!currentMap) {
    return;
  }

  resizeCanvas();
  updateGeometry(currentMap);
  constrainViewport();

  ctx.fillStyle = terrainStyles.none.fill;
  ctx.fillRect(0, 0, viewport.width, viewport.height);
  ctx.save();
  ctx.translate(viewport.offsetX, viewport.offsetY);
  ctx.scale(viewport.scale, viewport.scale);

  for (const hex of currentMap.hexes) {
    const center = hexCenter(hex);
    drawHex(center.x, center.y, geometry.size, `${hex.q},${hex.r}`, hex);
  }

  drawRoads(currentMap.roads);
  drawRiverEdges(currentMap.edges);
  drawCrossings(currentMap.crossings);
  drawMapMarkers(currentMap.river_sources, terrainStyles.river.source, 5);
  drawMapMarkers(currentMap.merge_points, terrainStyles.river.merge, 5);
  drawMapMarkers(currentMap.river_destinations, terrainStyles.river.destination, 5);
  drawReachableHexes();
  drawUnitCounters(currentMap.units);

  ctx.restore();
}

function syncEditorControls() {
  editModeButton.classList.toggle("is-active", isEditing);
  editModeButton.setAttribute("aria-pressed", String(isEditing));
  mapPanel.classList.toggle("is-editing", isEditing);

  for (const button of terrainPalette.querySelectorAll(".terrain-button")) {
    button.classList.toggle("is-selected", button.dataset.terrain === selectedTerrain);
  }
}

function initializeTerrainPalette() {
  for (const terrain of editorTerrains) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "terrain-button";
    button.dataset.terrain = terrain.key;
    button.title = terrain.label;
    button.setAttribute("aria-label", terrain.label);
    button.style.background = terrain.fill;
    button.style.borderColor = terrain.stroke;
    button.addEventListener("click", () => {
      selectedTerrain = terrain.key;
      editorToolSelect.value = "hex";
      syncEditorControls();
    });
    terrainPalette.appendChild(button);
  }
  syncEditorControls();
}

function setEditMode(enabled) {
  isEditing = enabled && appMode === "scenario";
  if (!isEditing) {
    isPainting = false;
    paintStrokeKeys = new Set();
  }
  syncEditorControls();
}

function setRiverEdge(edge, enabled) {
  currentMap.edges = currentMap.edges || [];
  const key = edgeKey(edge);
  const index = currentMap.edges.findIndex((existing) => edgeKey(canonicalEdge(existing.a, existing.b)) === key);
  if (enabled && index < 0) {
    currentMap.edges.push(edge);
  } else if (!enabled && index >= 0) {
    currentMap.edges.splice(index, 1);
  }
}

function toggleRiverEdge(edge) {
  currentMap.edges = currentMap.edges || [];
  const key = edgeKey(edge);
  const index = currentMap.edges.findIndex((existing) => edgeKey(canonicalEdge(existing.a, existing.b)) === key);
  if (index >= 0) {
    currentMap.edges.splice(index, 1);
  } else {
    currentMap.edges.push(edge);
  }
}

function toggleHexTerrain(hex, terrain) {
  const key = coordKey(hex);
  if (hex.terrain === terrain) {
    hex.terrain = terrainUndo.get(key) || "grassland";
    terrainUndo.delete(key);
    hex.labels = editorLabelsForTerrain(hex.terrain);
    return;
  }

  if (!terrainUndo.has(key)) {
    terrainUndo.set(key, hex.terrain);
  }
  hex.terrain = terrain;
  hex.labels = editorLabelsForTerrain(hex.terrain);
}

function editorLabelsForTerrain(terrain) {
  if (terrain === "lake") {
    return [];
  }
  if (terrain === "grassland") {
    return ["base_steppe"];
  }
  if (terrain === "hill" || terrain === "mountain" || terrain === "forest" || terrain === "woods") {
    return ["wild_terrain"];
  }
  if (terrain === "urban") {
    return ["urban"];
  }
  if (terrain === "none") {
    return [];
  }
  return [];
}

function paintAtPointer(event) {
  if (appMode !== "scenario" || !isEditing || !currentMap) {
    return;
  }

  const point = panelToWorld(event);
  if (editorToolSelect.value === "edge") {
    const edge = findNearestEditableEdge(point);
    if (!edge) {
      return;
    }
    const key = `edge:${edgeKey(edge)}`;
    if (paintStrokeKeys.has(key)) {
      return;
    }
    paintStrokeKeys.add(key);
    if (event.shiftKey) {
      setRiverEdge(edge, false);
    } else {
      toggleRiverEdge(edge);
    }
    refreshDerivedTopology();
    drawMap();
    return;
  }

  const hex = findNearestHex(point);
  if (!hex) {
    return;
  }
  const terrain = event.shiftKey ? "grassland" : selectedTerrain;
  const key = `hex:${coordKey(hex)}`;
  if (paintStrokeKeys.has(key)) {
    return;
  }
  paintStrokeKeys.add(key);
  toggleHexTerrain(hex, terrain);
  refreshDerivedTopology();
  drawMap();
}

async function generateMap() {
  const width = clampDimension(widthInput.value, 1, 120, 120);
  const height = clampDimension(heightInput.value, 1, 80, 80);
  const rivers = clampDimension(riversInput.value, 0, 20, 4);
  const lakes = clampDimension(lakesInput.value, 0, 20, 4);
  const lakeSize = clampDimension(lakeSizeInput.value, 1, 40, 6);
  const meanderForward = clampNumberInput(meanderForwardInput, 0, 40, 8);
  const meanderForwardJitter = clampNumberInput(meanderForwardJitterInput, 0, 40, 4);
  const meanderLateral = clampNumberInput(meanderLateralInput, 0, 40, 7);
  const meanderLateralJitter = clampNumberInput(meanderLateralJitterInput, 0, 40, 4);
  const meanderStrength = clampNumberInput(meanderStrengthInput, 0, 10, 1);
  const meanderReach = clampNumberInput(meanderReachInput, 0, 40, 2);
  const riverSlantStrength = clampNumberInput(riverSlantStrengthInput, 0, 10, 10);
  const valleyThickness = clampNumberInput(valleyThicknessInput, 0, 5, 2);
  const forestBlobs = clampDimension(forestBlobsInput.value, 0, 10, 4);
  const forestBlobRadius = clampNumberInput(forestBlobRadiusInput, 0, 20, 4);
  const meanderTimeout = clampDimension(meanderTimeoutInput.value, 1, 200, 28);
  const seed = newSeed();
  widthInput.value = width;
  heightInput.value = height;
  riversInput.value = rivers;
  lakesInput.value = lakes;
  lakeSizeInput.value = lakeSize;
  forestBlobsInput.value = forestBlobs;
  meanderTimeoutInput.value = meanderTimeout;

  generateButton.disabled = true;
  try {
    const response = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        width,
        height,
        rivers,
        lakes,
        lakeSize,
        meanderForward,
        meanderForwardJitter,
        meanderLateral,
        meanderLateralJitter,
        meanderStrength,
        meanderReach,
        riverSlantStrength,
        valleyThickness,
        forestBlobs,
        forestBlobRadius,
        meanderTimeout,
        seed,
      }),
    });
    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload.error || "generation failed");
    }
    currentMap = payload;
    currentMap.units = Array.isArray(currentMap.units) ? currentMap.units : [];
    selectedUnitId = null;
    reachableHexes = new Map();
    terrainUndo = new Map();
    resetTurnState();
    refreshDerivedTopology();
    syncModeControls();
    fitMap();
  } catch (error) {
    window.alert(error.message);
  } finally {
    generateButton.disabled = false;
  }
}

function createBlankMap(options = {}) {
  const requestedWidth = Number.isInteger(options.width) ? options.width : widthInput.value;
  const requestedHeight = Number.isInteger(options.height) ? options.height : heightInput.value;
  const width = clampDimension(requestedWidth, 1, 120, 120);
  const height = clampDimension(requestedHeight, 1, 80, 80);
  widthInput.value = width;
  heightInput.value = height;

  const hexes = [];
  for (let r = 1; r <= height; r += 1) {
    for (let q = 1; q <= width; q += 1) {
      hexes.push({ q, r, terrain: "grassland", labels: ["base_steppe"] });
    }
  }

  currentMap = {
    schema: "steppe-terrain.v1",
    seed: 0,
    width,
    height,
    hexes,
    river_sources: [],
    river_destinations: [],
    merge_points: [],
    river_segments: [],
    edges: [],
    towns: [],
    roads: [],
    crossings: [],
    units: [],
    metadata: {
      generator: typeof options.generator === "string" ? options.generator : "blank-editor",
      terrain_types: editorTerrains.map((terrain) => terrain.key),
      hex_label_model: "base-plus-generation-role.v1",
    },
  };
  selectedUnitId = null;
  reachableHexes = new Map();
  terrainUndo = new Map();
  resetTurnState();
  refreshDerivedTopology();
  syncModeControls();
  fitMap();
}

function createDefaultPlayScenario() {
  createBlankMap({ width: 10, height: 10, generator: "default-play-sandbox" });
  currentMap.units = [
    createCavalryUnit(1, "mongol", 3, 5),
    createCavalryUnit(2, "mongol", 3, 7),
    createCavalryUnit(3, "chinese", 8, 5),
    createCavalryUnit(4, "chinese", 8, 7),
  ];
  selectedUnitId = null;
  reachableHexes = new Map();
  resetTurnState();
  syncModeControls();
  fitMap();
}

function normalizeLoadedMap(map) {
  if (!map || typeof map !== "object") {
    throw new Error("Loaded file is not a terrain map.");
  }
  if (!Number.isInteger(map.width) || !Number.isInteger(map.height) || map.width < 1 || map.height < 1) {
    throw new Error("Loaded map has invalid dimensions.");
  }
  if (!Array.isArray(map.hexes)) {
    throw new Error("Loaded map is missing hex terrain.");
  }

  return refreshDerivedTopology({
    schema: typeof map.schema === "string" ? map.schema : "steppe-terrain.v1",
    seed: Number.isInteger(map.seed) ? map.seed : 0,
    width: map.width,
    height: map.height,
    hexes: map.hexes.map((hex) => ({
      q: Number(hex.q),
      r: Number(hex.r),
      terrain: typeof hex.terrain === "string" ? hex.terrain : "grassland",
      labels: Array.isArray(hex.labels)
        ? hex.labels.filter((label) => typeof label === "string")
        : editorLabelsForTerrain(typeof hex.terrain === "string" ? hex.terrain : "grassland"),
    })).filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r)),
    river_sources: Array.isArray(map.river_sources) ? map.river_sources : [],
    river_destinations: Array.isArray(map.river_destinations) ? map.river_destinations : [],
    merge_points: Array.isArray(map.merge_points) ? map.merge_points : [],
    river_segments: Array.isArray(map.river_segments) ? map.river_segments : [],
    edges: Array.isArray(map.edges) ? map.edges : [],
    towns: Array.isArray(map.towns) ? map.towns : [],
    roads: Array.isArray(map.roads) ? map.roads : [],
    crossings: Array.isArray(map.crossings) ? map.crossings : [],
    units: Array.isArray(map.units)
      ? map.units
        .filter((unit) => unit && Number.isInteger(unit.q) && Number.isInteger(unit.r))
        .map(normalizeUnit)
      : [],
    metadata: map.metadata && typeof map.metadata === "object"
      ? map.metadata
      : { generator: "loaded-editor", terrain_types: editorTerrains.map((terrain) => terrain.key) },
  });
}

function defaultMapFilename() {
  const seed = currentMap && Number.isInteger(currentMap.seed) ? currentMap.seed : 0;
  const width = currentMap ? currentMap.width : widthInput.value;
  const height = currentMap ? currentMap.height : heightInput.value;
  return `steppe-terrain-${width}x${height}-${seed}.json`;
}

function mapFilePickerTypes() {
  return [{
    description: "Steppe terrain JSON",
    accept: { "application/json": [".json"] },
  }];
}

function mapBlob() {
  refreshDerivedTopology();
  return new Blob([`${JSON.stringify(currentMap, null, 2)}\n`], { type: "application/json" });
}

function downloadMapFallback() {
  const url = URL.createObjectURL(mapBlob());
  const link = document.createElement("a");
  link.href = url;
  link.download = defaultMapFilename();
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

async function saveCurrentMap() {
  if (!currentMap) {
    window.alert("No terrain map is loaded.");
    return;
  }

  if (!window.showSaveFilePicker) {
    downloadMapFallback();
    return;
  }

  try {
    const handle = await window.showSaveFilePicker({
      suggestedName: defaultMapFilename(),
      types: mapFilePickerTypes(),
    });
    const writable = await handle.createWritable();
    await writable.write(mapBlob());
    await writable.close();
  } catch (error) {
    if (error.name !== "AbortError") {
      window.alert(error.message);
    }
  }
}

async function loadMapText(text) {
  try {
    currentMap = normalizeLoadedMap(JSON.parse(text));
    selectedUnitId = null;
    reachableHexes = new Map();
    terrainUndo = new Map();
    resetTurnState();
    widthInput.value = currentMap.width;
    heightInput.value = currentMap.height;
    syncModeControls();
    fitMap();
  } catch (error) {
    window.alert(error.message);
  }
}

async function loadMapFile(file) {
  if (!file) {
    return;
  }

  try {
    await loadMapText(await file.text());
  } catch (error) {
    window.alert(error.message);
  } finally {
    loadFileInput.value = "";
  }
}

async function chooseMapFile() {
  if (!window.showOpenFilePicker) {
    loadFileInput.click();
    return;
  }

  try {
    const [handle] = await window.showOpenFilePicker({
      multiple: false,
      types: mapFilePickerTypes(),
    });
    await loadMapFile(await handle.getFile());
  } catch (error) {
    if (error.name !== "AbortError") {
      window.alert(error.message);
    }
  }
}

scenarioModeButton.addEventListener("click", () => setAppMode("scenario"));
playModeButton.addEventListener("click", () => setAppMode("play"));
generateButton.addEventListener("click", generateMap);
blankMapButton.addEventListener("click", createBlankMap);
editModeButton.addEventListener("click", () => setEditMode(!isEditing));
editorToolSelect.addEventListener("change", syncEditorControls);
saveButton.addEventListener("click", saveCurrentMap);
loadButton.addEventListener("click", chooseMapFile);
loadFileInput.addEventListener("change", () => loadMapFile(loadFileInput.files[0]));
endTurnButton.addEventListener("click", advanceTurn);
controlEndTurnButton.addEventListener("click", advanceTurn);
zoomInButton.addEventListener("click", () => zoomFromCenter(1.25));
zoomOutButton.addEventListener("click", () => zoomFromCenter(0.8));
fitButton.addEventListener("click", fitMap);

mapPanel.addEventListener("wheel", (event) => {
  if (!currentMap) {
    return;
  }
  event.preventDefault();
  const rect = mapPanel.getBoundingClientRect();
  zoomAt(event.clientX - rect.left, event.clientY - rect.top, event.deltaY < 0 ? 1.12 : 1 / 1.12);
}, { passive: false });

mapPanel.addEventListener("pointerdown", (event) => {
  mapPanel.focus();
  const point = panelToWorld(event);
  if (appMode === "play" && event.button === 0) {
    const unit = findUnitAtPoint(point);
    if (unit) {
      event.preventDefault();
      selectUnit(unit);
      return;
    }
    const hex = findNearestHex(point);
    if (hex && moveSelectedUnitTo(hex)) {
      event.preventDefault();
      return;
    }
  }
  if (appMode === "scenario" && isEditing && event.button === 0) {
    event.preventDefault();
    isPainting = true;
    paintStrokeKeys = new Set();
    paintAtPointer(event);
    mapPanel.setPointerCapture(event.pointerId);
    return;
  }
  if (event.button !== 0 && event.button !== 1) {
    return;
  }
  event.preventDefault();
  isPanning = true;
  lastPointer = { x: event.clientX, y: event.clientY };
  mapPanel.classList.add("is-panning");
  mapPanel.setPointerCapture(event.pointerId);
});

mapPanel.addEventListener("pointermove", (event) => {
  if (isPainting) {
    event.preventDefault();
    paintAtPointer(event);
    return;
  }
  if (!isPanning) {
    return;
  }
  panBy(event.clientX - lastPointer.x, event.clientY - lastPointer.y);
  lastPointer = { x: event.clientX, y: event.clientY };
});

mapPanel.addEventListener("pointerup", (event) => {
  if (isPainting) {
    stopPainting();
    mapPanel.releasePointerCapture(event.pointerId);
    return;
  }
  if (!isPanning) {
    return;
  }
  stopPanning();
  mapPanel.releasePointerCapture(event.pointerId);
});

mapPanel.addEventListener("pointercancel", () => {
  stopPainting();
  stopPanning();
});
mapPanel.addEventListener("lostpointercapture", () => {
  stopPainting();
  stopPanning();
});

mapPanel.addEventListener("auxclick", (event) => {
  if (event.button === 1) {
    event.preventDefault();
  }
});

mapPanel.addEventListener("keydown", (event) => {
  const step = event.shiftKey ? 80 : 32;
  const moves = {
    ArrowLeft: [step, 0],
    ArrowRight: [-step, 0],
    ArrowUp: [0, step],
    ArrowDown: [0, -step],
  };
  const move = moves[event.key];
  if (!move) {
    return;
  }
  event.preventDefault();
  panBy(move[0], move[1]);
});

window.addEventListener("resize", () => {
  if (currentMap) {
    drawMap();
  }
});

new ResizeObserver(() => {
  if (currentMap) {
    drawMap();
  }
}).observe(mapPanel);

initializeTerrainPalette();
syncModeControls();
createDefaultPlayScenario();
