const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const appShell = document.querySelector("#app-shell");
const mapPanel = document.querySelector("#map-panel");
const contextMenu = document.querySelector("#context-menu");
const combatPreview = document.querySelector("#combat-preview");
const detachHerdPopover = document.querySelector("#detach-herd-popover");
const detachHerdForm = document.querySelector("#detach-herd-form");
const detachHerdHorsesInput = document.querySelector("#detach-herd-horses");
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
const undoButton = document.querySelector("#undo-button");
const zoomInButton = document.querySelector("#zoom-in-button");
const zoomOutButton = document.querySelector("#zoom-out-button");
const fitButton = document.querySelector("#fit-button");
const editModeButton = document.querySelector("#edit-mode-button");
const editorModeSelect = document.querySelector("#editor-mode");
const editorEdgeFeatureSelect = document.querySelector("#editor-edge-feature");
const editorUnitTypeSelect = document.querySelector("#editor-unit-type");
const editorUnitSideSelect = document.querySelector("#editor-unit-side");
const terrainPalette = document.querySelector("#terrain-palette");
const endTurnButton = document.querySelector("#end-turn-button");
const controlEndTurnButton = document.querySelector("#control-end-turn-button");
const statusEndTurnButton = document.querySelector("#status-end-turn-button");
const turnCounter = document.querySelector(".turn-counter");
const activeFactionName = document.querySelector("#active-faction-name");
const statusActiveFactionName = document.querySelector("#status-active-faction-name");
const factionStatusName = document.querySelector("#faction-status-name");
const factionPopulationTotal = document.querySelector("#faction-population-total");
const factionHorsesTotal = document.querySelector("#faction-horses-total");
const factionMetalTotal = document.querySelector("#faction-metal-total");
const roundCount = document.querySelector("#round-count");
const turnStatusReadout = document.querySelector("#turn-status-readout");
const campCount = document.querySelector("#camp-count");
const herdCount = document.querySelector("#herd-count");
const horseArcherCount = document.querySelector("#horse-archer-count");
const hordeCount = document.querySelector("#horde-count");
const sidebarSelectionReadout = document.querySelector(".sidebar-selection-readout");
const unitRoster = document.querySelector("#unit-roster");
const unitName = document.querySelector("#unit-name");
const unitHp = document.querySelector("#unit-hp");
const unitAttack = document.querySelector("#unit-attack");
const unitDefense = document.querySelector("#unit-defense");
const unitReadiness = document.querySelector("#unit-readiness");
const unitMove = document.querySelector("#unit-move");
const unitResources = document.querySelector("#unit-resources");
const unitPopulation = document.querySelector("#unit-population");
const unitMetal = document.querySelector("#unit-metal");
const unitHorses = document.querySelector("#unit-horses");

let currentMap = null;
let appMode = "intro";
let isPanning = false;
let isEditing = false;
let isPainting = false;
let lastPointer = { x: 0, y: 0 };
let selectedTerrain = "lake";
let paintStrokeKeys = new Set();
let paintUndoRecorded = false;
let terrainUndo = new Map();
let localUndoStack = [];
let playUndoDepth = 0;
let currentTurn = 1;
let activeFactionIndex = 0;
let activeContextMenu = null;
let contextMenuRequestId = 0;
let combatPreviewRequestId = 0;
let combatPreviewTargetId = 0;
let detachHerdAmountContext = null;
let detachHerdPlacement = null;
let createUnitPlacement = null;

const viewport = {
  scale: 1,
  fitScale: 1,
  zoomLevelIndex: 0,
  offsetX: 0,
  offsetY: 0,
  width: 0,
  height: 0,
  minScale: 0.1,
  maxScale: 12,
};

const geometry = {
  margin: 22,
  size: 16,
  width: 0,
  height: 0,
};

const maxUndoDepth = 64;

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
  wall: {
    stroke: "#3a3328",
    highlight: "#c4b28a",
    gate: "#d6b45f",
    roadGate: "#f1f1e6",
    outline: "#221b13",
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
    owner: 0,
  },
  chinese: {
    name: "Chinese",
    color: "#c93632",
    owner: 2,
  },
  persian: {
    name: "Persian",
    color: "#8a4fb0",
    owner: 1,
  },
  neutral: {
    name: "Neutral",
    color: "#777777",
    owner: -1,
  },
};

const factionCount = 2;
const factionTurnOrder = ["mongol", "chinese", "persian"].slice(0, factionCount);

const unitTypeDefaults = {};
const unitDisplayKindNames = {
  horse_archer: "Horse Archers",
  chinese_cavalry: "Cavalry",
  chinese_militia: "Militia",
  mongol_lancer: "Lancers",
  infantry: "Infantry",
  horde: "Horde",
  herd: "Herd",
  camp: "Camp",
};
const unitKindLabels = {
  camp: "Camp",
  herd: "Herd",
  horse_archer: "Horse Archers",
  chinese_cavalry: "Chinese Cavalry",
  chinese_militia: "Chinese Militia",
  mongol_lancer: "Mongol Lancers",
  infantry: "Infantry",
  horde: "Horde",
};
const fallbackUnitDefaults = {
  hp: 1,
  attack: 0,
  defense: 1,
  readinessDamage: 0,
  readiness: 100,
  stance: "default",
  legalStances: ["default"],
  move: 0,
  projectsZoc: false,
  respectsZoc: false,
  population: 0,
  metal: 0,
  horses: 0,
};

const unitSpriteColumns = {
  infantry: "infantry",
  horde: "horde",
  herd: "herd",
  horse_archer: "horse_archer",
  chinese_cavalry: "chinese_cavalry",
  chinese_militia: "chinese_militia",
  mongol_lancer: "mongol_lancer",
  camp: "camp",
};
const unitSpriteZoomLevels = [
  { key: "small", pixelSize: 48, targetWidth: null, targetHeight: null, minFitMultiplier: 1 },
  { key: "medium", pixelSize: 96, targetWidth: 40, targetHeight: 20, minFitMultiplier: 1.4 },
  { key: "large", pixelSize: 128, targetWidth: 20, targetHeight: 10, minFitMultiplier: 2 },
];
const unitSpriteTintCache = new Map();
const bitmapUnitSpriteSources = {
  infantry: {
    small: "/unit-sprites/infantry_48.png",
    medium: "/unit-sprites/infantry_96.png",
    large: "/unit-sprites/infantry_128.png",
  },
  horde: {
    small: "/unit-sprites/horde_48.png",
    medium: "/unit-sprites/horde_96.png",
    large: "/unit-sprites/horde_128.png",
  },
  herd: {
    small: "/unit-sprites/herd_48.png",
    medium: "/unit-sprites/herd_96.png",
    large: "/unit-sprites/herd_128.png",
  },
  chinese_cavalry: {
    small: "/unit-sprites/heavy_cavalry_48.png",
    medium: "/unit-sprites/heavy_cavalry_96.png",
    large: "/unit-sprites/heavy_cavalry_128.png",
  },
  chinese_militia: {
    small: "/unit-sprites/militia_48.png",
    medium: "/unit-sprites/militia_96.png",
    large: "/unit-sprites/militia_128.png",
  },
  horse_archer: {
    small: "/unit-sprites/horse_archer_48.png",
    medium: "/unit-sprites/horse_archer_96.png",
    large: "/unit-sprites/horse_archer_128.png",
  },
  mongol_lancer: {
    small: "/unit-sprites/heavy_cavalry_48.png",
    medium: "/unit-sprites/heavy_cavalry_96.png",
    large: "/unit-sprites/heavy_cavalry_128.png",
  },
};
const bitmapUnitSpriteImages = {};
let unitSpriteSheetReady = false;

function terrainStyle(key) {
  const terrain = editorTerrains.find((entry) => entry.key === key);
  return {
    fill: terrain.fill,
    stroke: terrain.stroke,
    label: terrain.labelColor,
  };
}

function loadBitmapUnitSprites() {
  const entries = Object.entries(bitmapUnitSpriteSources)
    .flatMap(([kind, levels]) => Object.entries(levels).map(([key, src]) => ({ kind, key, src })));
  if (entries.length === 0) {
    unitSpriteSheetReady = true;
    return;
  }

  let loaded = 0;
  const markLoaded = () => {
    loaded += 1;
    if (loaded === entries.length) {
      unitSpriteSheetReady = true;
      if (currentMap) {
        drawMap();
      }
    }
  };

  for (const { kind, key, src } of entries) {
    const image = new Image();
    image.onload = markLoaded;
    image.onerror = markLoaded;
    image.src = src;
    if (!bitmapUnitSpriteImages[kind]) {
      bitmapUnitSpriteImages[kind] = {};
    }
    bitmapUnitSpriteImages[kind][key] = image;
  }
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

function worldSizeForHexSpan(width, height) {
  const clampedWidth = Math.max(1, width);
  const clampedHeight = Math.max(1, height);
  const hexHeight = Math.sqrt(3) * geometry.size;
  return {
    width: geometry.size * (1.5 * Math.max(1, clampedWidth - 1) + 2),
    height: hexHeight * (clampedHeight + 0.5),
  };
}

function fitMap() {
  if (!currentMap) {
    return;
  }
  resizeCanvas();
  updateGeometry(currentMap);
  const scaleX = viewport.width / geometry.width;
  const scaleY = viewport.height / geometry.height;
  viewport.fitScale = clamp(Math.min(scaleX, scaleY), viewport.minScale, viewport.maxScale);
  viewport.zoomLevelIndex = 0;
  viewport.scale = zoomScaleForLevel(viewport.zoomLevelIndex);
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

function zoomScaleForLevel(index) {
  const level = unitSpriteZoomLevels[clamp(index, 0, unitSpriteZoomLevels.length - 1)] || unitSpriteZoomLevels[0];
  const minimumTierScale = viewport.fitScale * (level.minFitMultiplier || 1);
  if (!level.targetWidth || !level.targetHeight) {
    return clamp(minimumTierScale, viewport.minScale, viewport.maxScale);
  }
  const target = worldSizeForHexSpan(
    Math.min(currentMap ? currentMap.width : level.targetWidth, level.targetWidth),
    Math.min(currentMap ? currentMap.height : level.targetHeight, level.targetHeight)
  );
  const targetScale = Math.min(viewport.width / target.width, viewport.height / target.height);
  return clamp(Math.max(viewport.fitScale, minimumTierScale, targetScale), viewport.minScale, viewport.maxScale);
}

function setZoomLevelAt(panelX, panelY, nextIndex) {
  if (!currentMap) {
    return;
  }

  const clampedIndex = clamp(nextIndex, 0, unitSpriteZoomLevels.length - 1);
  const nextScale = zoomScaleForLevel(clampedIndex);
  if (nextScale === viewport.scale && clampedIndex === viewport.zoomLevelIndex) {
    return;
  }

  const worldX = (panelX - viewport.offsetX) / viewport.scale;
  const worldY = (panelY - viewport.offsetY) / viewport.scale;
  viewport.zoomLevelIndex = clampedIndex;
  viewport.scale = nextScale;
  viewport.offsetX = panelX - worldX * viewport.scale;
  viewport.offsetY = panelY - worldY * viewport.scale;
  constrainViewport();
  drawMap();
}

function zoomStepAt(panelX, panelY, direction) {
  setZoomLevelAt(panelX, panelY, viewport.zoomLevelIndex + direction);
}

function zoomFromCenter(direction) {
  zoomStepAt(viewport.width / 2, viewport.height / 2, direction);
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
  paintUndoRecorded = false;
}

function activeFactionKey() {
  const clan = activeFaction();
  return clan.key || "neutral";
}

function activeOwner() {
  if (currentMap && currentMap.game && Number.isInteger(currentMap.game.activeOwner)) {
    return currentMap.game.activeOwner;
  }
  return null;
}

function activeFaction() {
  const owner = activeOwner();
  const clan = currentMap && currentMap.game && Array.isArray(currentMap.game.clans)
    ? currentMap.game.clans.find((candidate) => candidate.id === owner)
    : null;
  if (clan) {
    return clan;
  }
  return Object.values(factions).find((faction) => faction.owner === owner) || factions.neutral;
}

function countUnits(kind, owner = null) {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.filter((unit) => unit.kind === kind && (owner === null || unit.owner === owner)).length
    : 0;
}

function activeFactionResources(owner) {
  const totals = { population: 0, horses: 0, metal: 0 };
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return totals;
  }
  for (const unit of currentMap.units) {
    if (unit.owner !== owner || unit.kind !== "horde") {
      continue;
    }
    totals.population += Number.isInteger(unit.population) ? unit.population : 0;
    totals.horses += Number.isInteger(unit.horses) ? unit.horses : 0;
    totals.metal += Number.isInteger(unit.metal) ? unit.metal : 0;
  }
  return totals;
}

function selectedUnit() {
  const selectedUnitId = currentMap && currentMap.game && Number.isInteger(currentMap.game.selectedUnitId)
    ? currentMap.game.selectedUnitId
    : 0;
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.find((unit) => unit.id === selectedUnitId) || null
    : null;
}

function ensureGameMeta() {
  if (!currentMap) {
    return;
  }
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentTurn = Number.isInteger(currentMap.game.round) ? currentMap.game.round : 1;
  activeFactionIndex = Number.isInteger(currentMap.game.activeFactionIndex) ? currentMap.game.activeFactionIndex : 0;
}

function cloneMapState(map) {
  return map ? JSON.parse(JSON.stringify(map)) : null;
}

function canUndo() {
  return appMode === "play" ? playUndoDepth > 0 : localUndoStack.length > 0;
}

function syncUndoControls() {
  if (undoButton) {
    undoButton.disabled = !canUndo();
  }
}

function clearUndoHistory() {
  localUndoStack = [];
  playUndoDepth = 0;
  syncUndoControls();
}

function recordLocalUndo() {
  if (!currentMap) {
    return;
  }
  localUndoStack.push(cloneMapState(currentMap));
  if (localUndoStack.length > maxUndoDepth) {
    localUndoStack.shift();
  }
  syncUndoControls();
}

function recordPlayUndo() {
  playUndoDepth = Math.min(maxUndoDepth, playUndoDepth + 1);
  syncUndoControls();
}

function restoreLocalUndo(snapshot) {
  if (!snapshot) {
    return;
  }
  currentMap = refreshDerivedTopology(snapshot);
  terrainUndo = new Map();
  ensureGameMeta();
  syncModeControls();
  drawMap();
}

function normalizeUnitDefault(defaults) {
  const normalized = { ...fallbackUnitDefaults };
  if (!defaults || typeof defaults !== "object") {
    return normalized;
  }
  if (Number.isFinite(defaults.hp)) normalized.hp = Math.max(1, Math.trunc(defaults.hp));
  if (Number.isFinite(defaults.attack)) normalized.attack = Math.max(0, Math.trunc(defaults.attack));
  if (Number.isFinite(defaults.defense)) normalized.defense = Math.max(1, Math.trunc(defaults.defense));
  if (Number.isFinite(defaults.readinessDamage)) normalized.readinessDamage = Math.max(0, Math.trunc(defaults.readinessDamage));
  if (Number.isFinite(defaults.readiness)) normalized.readiness = Math.max(0, Math.trunc(defaults.readiness));
  if (typeof defaults.stance === "string") normalized.stance = defaults.stance;
  if (Array.isArray(defaults.legalStances)) {
    normalized.legalStances = defaults.legalStances.filter((stance) => typeof stance === "string");
  }
  if (Number.isFinite(defaults.move)) normalized.move = Math.max(0, defaults.move);
  normalized.projectsZoc = Boolean(defaults.projectsZoc);
  normalized.respectsZoc = Boolean(defaults.respectsZoc);
  if (Number.isFinite(defaults.population)) normalized.population = Math.max(0, Math.trunc(defaults.population));
  if (Number.isFinite(defaults.metal)) normalized.metal = Math.max(0, Math.trunc(defaults.metal));
  if (Number.isFinite(defaults.horses)) normalized.horses = Math.max(0, Math.trunc(defaults.horses));
  return normalized;
}

function unitDefaultsFor(kind) {
  return unitTypeDefaults[kind] || unitTypeDefaults.horse_archer || fallbackUnitDefaults;
}

function editorUnitDefaultsFor(kind) {
  const defaults = unitTypeDefaults[kind];
  if (!defaults) {
    throw new Error("Engine unit metadata has not loaded yet.");
  }
  return defaults;
}

function unitKindLabelForKind(kind) {
  return unitKindLabels[kind] || kind || "Unit";
}

function populateEditorUnitTypeOptions(kinds) {
  const previous = editorUnitTypeSelect.value;
  editorUnitTypeSelect.replaceChildren();
  for (const kind of kinds) {
    const option = document.createElement("option");
    option.value = kind;
    option.textContent = unitKindLabelForKind(kind);
    editorUnitTypeSelect.appendChild(option);
  }
  if (kinds.includes(previous)) {
    editorUnitTypeSelect.value = previous;
  } else if (kinds.length > 0) {
    editorUnitTypeSelect.value = kinds[0];
  }
}

async function loadUnitTypeDefaults() {
  const metadata = await postAppCommand({ type: "unit_defaults" });
  if (!metadata || typeof metadata !== "object" || !metadata.units || typeof metadata.units !== "object") {
    throw new Error("Engine unit metadata response is invalid.");
  }
  const entries = Object.entries(metadata.units);
  for (const [kind, defaults] of entries) {
    unitTypeDefaults[kind] = normalizeUnitDefault(defaults);
  }
  populateEditorUnitTypeOptions(entries.map(([kind]) => kind));
}

function normalizeUnit(unit, index) {
  const kind = typeof unit.kind === "string" ? (unit.kind === "cavalry" ? "horse_archer" : unit.kind) : "horse_archer";
  const defaults = unitDefaultsFor(kind);
  const owner = Number.isInteger(unit.owner) ? unit.owner : null;
  const ownerFaction = Object.entries(factions).find(([, faction]) => faction.owner === owner);
  const faction = typeof unit.faction === "string" && factions[unit.faction]
    ? unit.faction
    : (ownerFaction ? ownerFaction[0] : "mongol");
  const normalized = {
    id: Number.isInteger(unit.id) ? unit.id : index + 1,
    faction,
    kind,
    q: unit.q,
    r: unit.r,
  };
  if (owner !== null) normalized.owner = owner;
  if (Number.isFinite(unit.hp)) normalized.hp = unit.hp;
  if (Number.isFinite(unit.maxHp)) normalized.maxHp = unit.maxHp;
  normalized.attack = Number.isFinite(unit.attack) ? Math.max(0, Math.trunc(unit.attack)) : defaults.attack;
  normalized.defense = Number.isFinite(unit.defense) ? Math.max(1, Math.trunc(unit.defense)) : defaults.defense;
  normalized.readinessDamage = Number.isFinite(unit.readinessDamage)
    ? Math.max(0, Math.trunc(unit.readinessDamage))
    : defaults.readinessDamage;
  normalized.maxReadiness = Number.isFinite(unit.maxReadiness) ? Math.max(1, Math.trunc(unit.maxReadiness)) : 100;
  normalized.readiness = Number.isFinite(unit.readiness)
    ? Math.max(0, Math.min(Math.trunc(unit.readiness), normalized.maxReadiness))
    : defaults.readiness;
  normalized.stance = typeof unit.stance === "string" ? unit.stance : defaults.stance;
  if (Number.isFinite(unit.population)) normalized.population = Math.max(0, Math.trunc(unit.population));
  if (Number.isFinite(unit.metal)) normalized.metal = Math.max(0, Math.trunc(unit.metal));
  if (Number.isFinite(unit.horses)) normalized.horses = Math.max(0, Math.trunc(unit.horses));
  if (Number.isFinite(unit.scaledMove)) normalized.scaledMove = unit.scaledMove;
  if (Number.isFinite(unit.remainingScaledMove)) normalized.remainingScaledMove = unit.remainingScaledMove;
  if (Number.isFinite(unit.refMove)) {
    normalized.move = unit.refMove;
  } else if (Number.isFinite(unit.move)) {
    normalized.move = unit.move;
  }
  if (Number.isFinite(unit.remainingRefMove)) {
    normalized.remainingMove = unit.remainingRefMove;
  } else if (Number.isFinite(unit.remainingMove)) {
    normalized.remainingMove = unit.remainingMove;
  }
  if (unit.moveDone !== undefined) normalized.moveDone = Boolean(unit.moveDone);
  if (unit.movedThisTurn !== undefined) normalized.movedThisTurn = Boolean(unit.movedThisTurn);
  if (unit.combatDone !== undefined) normalized.combatDone = Boolean(unit.combatDone);
  if (unit.contactedEnemyThisTurn !== undefined) {
    normalized.contactedEnemyThisTurn = Boolean(unit.contactedEnemyThisTurn);
  }
  if (unit.projectsZoc !== undefined) normalized.projectsZoc = Boolean(unit.projectsZoc);
  if (unit.respectsZoc !== undefined) normalized.respectsZoc = Boolean(unit.respectsZoc);
  return normalized;
}

function applyGamePatch(payload) {
  if (!currentMap || !payload) {
    return;
  }
  if (Array.isArray(payload.units)) {
    currentMap.units = payload.units.map(normalizeUnit);
  }
  if (payload.game && typeof payload.game === "object") {
    currentMap.game = payload.game;
  }
  ensureGameMeta();
}

async function postAppCommand(command, gameId = "local-dev") {
  const response = await fetch("/api/command", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ gameId, command }),
  });
  const payload = await response.json();
  if (!response.ok) {
    throw new Error(payload.error || "app command failed");
  }
  if (!payload.ok) {
    throw new Error(payload.error || "engine command failed");
  }
  return payload.view;
}

async function postUndoableGameCommand(command) {
  const payload = await postAppCommand(command);
  recordPlayUndo();
  return payload;
}

function legalMoves() {
  return currentMap && currentMap.game && Array.isArray(currentMap.game.legalMoves)
    ? currentMap.game.legalMoves
    : [];
}

function legalAttacks() {
  return currentMap && currentMap.game && Array.isArray(currentMap.game.legalAttacks)
    ? currentMap.game.legalAttacks
    : [];
}

function legalMoveAt(coord) {
  return legalMoves().find((move) => move.q === coord.q && move.r === coord.r) || null;
}

function legalAttackForUnit(unit) {
  return unit ? legalAttacks().find((attack) => attack.unitId === unit.id) || null : null;
}

function hideCombatPreview() {
  combatPreviewRequestId += 1;
  combatPreviewTargetId = 0;
  combatPreview.hidden = true;
  combatPreview.replaceChildren();
}

function positionCombatPreview(clientX, clientY) {
  const rect = mapPanel.getBoundingClientRect();
  const margin = 8;
  const rawLeft = clientX - rect.left + 14;
  const rawTop = clientY - rect.top + 14;
  const maxLeft = Math.max(margin, rect.width - combatPreview.offsetWidth - margin);
  const maxTop = Math.max(margin, rect.height - combatPreview.offsetHeight - margin);
  combatPreview.style.left = `${clamp(rawLeft, margin, maxLeft)}px`;
  combatPreview.style.top = `${clamp(rawTop, margin, maxTop)}px`;
}

function combatantTitle(combatant) {
  return unitDisplayName({
    faction: combatant.faction,
    kind: combatant.kind,
  });
}

function appendCombatRow(parent, label, value) {
  const row = document.createElement("div");
  row.className = "combat-preview-row";
  const name = document.createElement("span");
  name.textContent = label;
  const score = document.createElement("strong");
  score.textContent = value;
  row.append(name, score);
  parent.appendChild(row);
}

function retreatStatusForSide(preview, mode) {
  if (preview.specialResolution === "feigned_retreat") {
    return mode === "defense" ? "Feigned" : "Pursues";
  }
  const retreatSide = mode === "attack" ? "attacker" : "defender";
  return preview.retreatOption === retreatSide ? "May retreat" : "No";
}

function appendCombatSide(parent, title, combatant, mode, preview) {
  const side = document.createElement("section");
  side.className = "combat-preview-side";
  const heading = document.createElement("h3");
  heading.textContent = title;
  side.appendChild(heading);
  appendCombatRow(side, "Unit", combatantTitle(combatant));
  appendCombatRow(side, "HP", `${combatant.hp}/${combatant.maxHp}`);
  appendCombatRow(side, mode === "attack" ? "Base attack" : "Base defense", String(
    mode === "attack" ? combatant.baseAttack : combatant.baseDefense
  ));
  appendCombatRow(side, "HP factor", `${combatant.hpPercent}%`);
  appendCombatRow(side, "Readiness", `${combatant.readiness}/${combatant.maxReadiness} (${combatant.readinessPercent}%)`);
  appendCombatRow(side, "RDY factor", `${combatant.readinessPercent}%`);
  appendCombatRow(side, "Retreat", retreatStatusForSide(preview, mode));
  appendCombatRow(side, "Terrain", mode === "attack" ? "-" : `${combatant.terrainDefensePercent}%`);
  appendCombatRow(side, "Flanking", mode === "defense"
    ? (preview.defenderFlanked ? `${preview.flankingDefensePercent}% defense` : "NO")
    : "-"
  );
  appendCombatRow(side, "Score", String(mode === "attack" ? combatant.effectiveAttack : combatant.effectiveDefense));
  appendCombatRow(side, mode === "attack" ? "Damage" : "Taken", String(
    mode === "attack" ? combatant.damageDealt : combatant.damageTaken
  ));
  appendCombatRow(side, mode === "attack" ? "RDY damage" : "RDY taken", String(
    mode === "attack" ? combatant.readinessDamageDealt : combatant.readinessDamageTaken
  ));
  appendCombatRow(side, "Pursuit RDY", mode === "attack" && preview.specialResolution === "feigned_retreat"
    ? String(preview.pursuitReadinessPenalty || combatant.readinessDamageTaken || 0)
    : "-"
  );

  const result = document.createElement("div");
  result.className = "combat-preview-result";
  appendCombatRow(result, "Result", `${combatant.resultHp}/${combatant.maxHp}${combatant.destroyed ? " destroyed" : ""}`);
  appendCombatRow(result, "Result RDY", `${combatant.resultReadiness}/${combatant.maxReadiness}`);
  side.appendChild(result);
  parent.appendChild(side);
}

function renderCombatPreview(preview, clientX, clientY) {
  combatPreview.replaceChildren();
  const title = document.createElement("div");
  title.className = "combat-preview-title";
  const titleParts = ["Combat Preview"];
  if (preview.specialResolution === "feigned_retreat") {
    titleParts.push("Feigned Retreat");
  }
  if (preview.defenderFlanked) {
    titleParts.push("Flanked");
  }
  if (!preview.defenderRetaliates) {
    titleParts.push("No Retaliation");
  }
  title.textContent = titleParts.join(" - ");
  const summary = document.createElement("div");
  summary.className = "combat-preview-summary";
  appendCombatRow(summary, "A-D", String(preview.baseDifferential));
  appendCombatRow(summary, "HP ratio", `${preview.hpRatioPercent}%`);
  appendCombatRow(summary, "RDY ratio", `${preview.readinessRatioPercent}%`);
  appendCombatRow(summary, "HP/RDY factor", `${preview.conditionRatioPercent}%`);
  appendCombatRow(summary, "CRT", String(preview.crtIndex));
  appendCombatRow(summary, "Retreat", preview.retreatOption === "none" ? "-" : preview.retreatOption);
  appendCombatRow(summary, "Resolution", preview.specialResolution === "feigned_retreat" ? "Feigned retreat" : "Normal");
  const impacts = document.createElement("div");
  impacts.className = "combat-preview-impact";
  appendCombatRow(impacts, "Readiness impact", preview.readinessImpact || "-");
  appendCombatRow(impacts, "Retreat impact", preview.retreatImpact || "-");
  if (preview.specialResolution === "feigned_retreat") {
    appendCombatRow(impacts, "Pursuit penalty", `${preview.pursuitReadinessPenalty || 0} RDY`);
  }
  const grid = document.createElement("div");
  grid.className = "combat-preview-grid";
  appendCombatSide(grid, "Attacker", preview.attacker, "attack", preview);
  appendCombatSide(grid, "Defender", preview.defender, "defense", preview);
  combatPreview.append(title, summary, impacts, grid);
  combatPreview.hidden = false;
  positionCombatPreview(clientX, clientY);
}

async function showCombatPreviewFor(event, defender) {
  const attacker = selectedUnit();
  if (!attacker || !defender || !legalAttackForUnit(defender)) {
    hideCombatPreview();
    return;
  }

  if (combatPreviewTargetId === defender.id && !combatPreview.hidden) {
    positionCombatPreview(event.clientX, event.clientY);
    return;
  }

  const requestId = combatPreviewRequestId + 1;
  combatPreviewRequestId = requestId;
  combatPreviewTargetId = defender.id;
  try {
    const preview = await postAppCommand({
      type: "combat_preview",
      attackerId: attacker.id,
      defenderId: defender.id,
    });
    if (requestId !== combatPreviewRequestId || combatPreviewTargetId !== defender.id) {
      return;
    }
    if (!preview || !preview.valid) {
      hideCombatPreview();
      return;
    }
    renderCombatPreview(preview, event.clientX, event.clientY);
  } catch {
    if (requestId === combatPreviewRequestId) {
      hideCombatPreview();
    }
  }
}

function updateCombatPreviewHover(event) {
  if (appMode !== "play" || isPanning || isPainting || detachHerdPlacement || createUnitPlacement) {
    hideCombatPreview();
    return;
  }
  const unit = findUnitAtPoint(panelToWorld(event));
  if (!unit || !legalAttackForUnit(unit)) {
    hideCombatPreview();
    return;
  }
  showCombatPreviewFor(event, unit);
}

const contextActionDefinitions = [
  {
    id: "select-unit",
    label: "Select",
    visible: ({ unit }) => Boolean(unit && unit.owner === activeOwner()),
    enabled: ({ unit, selected }) => Boolean(unit && (!selected || selected.id !== unit.id)),
    run: async ({ unit }) => selectUnit(unit),
  },
  {
    id: "stance-feigned-retreat",
    label: "Stance: Feigned Retreat",
    visible: ({ unit }) => Boolean(
      unit
      && unit.owner === activeOwner()
      && unit.kind === "horse_archer"
      && unit.stance !== "feigned_retreat"
    ),
    enabled: ({ unit }) => Boolean(unit && unit.kind === "horse_archer"),
    run: async ({ unit }) => setUnitStance(unit, "feigned_retreat"),
  },
  {
    id: "stance-defensive",
    label: "Stance: Defensive",
    visible: ({ unit }) => Boolean(
      unit
      && unit.owner === activeOwner()
      && unit.kind === "horse_archer"
      && unit.stance !== "defensive"
    ),
    enabled: ({ unit }) => Boolean(unit && unit.kind === "horse_archer"),
    run: async ({ unit }) => setUnitStance(unit, "defensive"),
  },
  {
    id: "move-here",
    label: "Move here",
    visible: ({ hex, selected }) => Boolean(hex && selected && legalMoveAt(hex)),
    enabled: ({ hex }) => Boolean(hex && legalMoveAt(hex)),
    run: async ({ hex }) => moveSelectedUnitTo(hex),
  },
  {
    id: "attack-unit",
    label: "Attack",
    visible: ({ unit }) => Boolean(unit && legalAttackForUnit(unit)),
    enabled: ({ unit }) => Boolean(unit && legalAttackForUnit(unit)),
    run: async ({ unit }) => attackSelectedUnit(unit),
  },
  {
    id: "detach-herd",
    label: "Detach herd",
    visible: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.detachHerd
      && Number.isInteger(unit.horses)
      && unit.horses > 0
    ),
    enabled: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.detachHerd
      && Number.isInteger(unit.horses)
      && unit.horses > 0
    ),
    run: async (context) => showDetachHerdAmount(context),
  },
  {
    id: "create-horse-archers",
    label: "Create Horse Archers",
    visible: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.createHorseArchers
      && Number.isInteger(unit.population)
      && Number.isInteger(unit.metal)
      && Number.isInteger(unit.horses)
      && unit.population >= 1
      && unit.metal >= 1
      && unit.horses >= 3
    ),
    enabled: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.createHorseArchers
      && Number.isInteger(unit.population)
      && Number.isInteger(unit.metal)
      && Number.isInteger(unit.horses)
      && unit.population >= 1
      && unit.metal >= 1
      && unit.horses >= 3
    ),
    run: async (context) => startCreateHorseArchersPlacement(context),
  },
  {
    id: "create-mongol-lancers",
    label: "Create Mongol Lancers",
    visible: ({ unit, actionAvailability }) => Boolean(
      unit
      && unit.faction === "mongol"
      && actionAvailability
      && actionAvailability.createMongolLancers
      && Number.isInteger(unit.population)
      && Number.isInteger(unit.metal)
      && Number.isInteger(unit.horses)
      && unit.population >= 1
      && unit.metal >= 2
      && unit.horses >= 3
    ),
    enabled: ({ unit, actionAvailability }) => Boolean(
      unit
      && unit.faction === "mongol"
      && actionAvailability
      && actionAvailability.createMongolLancers
      && Number.isInteger(unit.population)
      && Number.isInteger(unit.metal)
      && Number.isInteger(unit.horses)
      && unit.population >= 1
      && unit.metal >= 2
      && unit.horses >= 3
    ),
    run: async (context) => startCreateMongolLancersPlacement(context),
  },
];

function unitDisplayName(unit) {
  const faction = factions[unit.faction] || factions.mongol;
  const kind = unitDisplayKindNames[unit.kind] || unit.kind;
  return `${faction.name} ${kind}`;
}

function unitKindLabel(unit) {
  return unitKindLabelForKind(unit.kind);
}

function syncUnitRoster() {
  if (!unitRoster) {
    return;
  }
  unitRoster.replaceChildren();
  const units = currentMap && Array.isArray(currentMap.units) ? currentMap.units : [];
  const selected = selectedUnit();
  if (units.length === 0) {
    const empty = document.createElement("div");
    empty.className = "unit-roster-empty";
    empty.textContent = "No deployed units";
    unitRoster.appendChild(empty);
    return;
  }

  for (const unit of units) {
    const faction = factions[unit.faction] || factions.neutral;
    const item = document.createElement("button");
    item.type = "button";
    item.className = "unit-roster-item";
    item.classList.toggle("is-selected", Boolean(selected && selected.id === unit.id));
    item.style.setProperty("--unit-color", faction.color);
    item.addEventListener("click", () => {
      selectUnit(unit).catch((error) => window.alert(error.message));
    });

    const name = document.createElement("span");
    name.className = "unit-roster-name";
    name.textContent = unitDisplayName(unit);

    const meta = document.createElement("span");
    meta.className = "unit-roster-meta";
    const hp = Number.isFinite(unit.hp) && Number.isFinite(unit.maxHp) ? `${unit.hp}/${unit.maxHp}` : "-";
    meta.textContent = `${unitKindLabel(unit)} - HP ${hp}`;

    item.append(name, meta);
    unitRoster.appendChild(item);
  }
}

function syncUnitInspector() {
  const unit = selectedUnit();
  if (!unit) {
    sidebarSelectionReadout.textContent = "None";
    unitName.textContent = "None";
    unitHp.textContent = "-";
    unitAttack.textContent = "-";
    unitDefense.textContent = "-";
    unitReadiness.textContent = "-";
    unitMove.textContent = "-";
    unitResources.hidden = true;
    unitPopulation.textContent = "0";
    unitMetal.textContent = "0";
    unitHorses.textContent = "0";
    return;
  }
  sidebarSelectionReadout.textContent = unitDisplayName(unit);
  unitName.textContent = unitDisplayName(unit);
  unitHp.textContent = Number.isFinite(unit.hp) && Number.isFinite(unit.maxHp) ? `${unit.hp}/${unit.maxHp}` : "-";
  unitAttack.textContent = Number.isFinite(unit.attack) ? String(unit.attack) : "-";
  unitDefense.textContent = Number.isFinite(unit.defense) ? String(unit.defense) : "-";
  unitReadiness.textContent = Number.isFinite(unit.readiness) && Number.isFinite(unit.maxReadiness)
    ? `${unit.readiness}/${unit.maxReadiness}`
    : "-";
  unitMove.textContent = Number.isFinite(unit.remainingMove) && Number.isFinite(unit.move) ? `${unit.remainingMove}/${unit.move}` : "-";
  const showResources = unit.kind === "horde";
  unitResources.hidden = !showResources;
  unitPopulation.textContent = String(Number.isInteger(unit.population) ? unit.population : 0);
  unitMetal.textContent = String(Number.isInteger(unit.metal) ? unit.metal : 0);
  unitHorses.textContent = String(Number.isInteger(unit.horses) ? unit.horses : 0);
}

function syncPlayControls() {
  ensureGameMeta();
  const faction = activeFaction();
  const owner = activeOwner();
  turnCounter.textContent = `Round ${currentTurn} · ${faction.name} turn`;
  activeFactionName.textContent = faction.name;
  statusActiveFactionName.textContent = faction.name;
  factionStatusName.textContent = faction.name;
  roundCount.textContent = String(currentTurn);
  turnStatusReadout.textContent = `${faction.name} turn`;
  campCount.textContent = String(countUnits("camp", owner));
  herdCount.textContent = String(countUnits("herd", owner));
  horseArcherCount.textContent = String(countUnits("horse_archer", owner));
  hordeCount.textContent = String(countUnits("horde", owner));
  const resources = activeFactionResources(owner);
  factionPopulationTotal.textContent = String(resources.population);
  factionHorsesTotal.textContent = String(resources.horses);
  factionMetalTotal.textContent = String(resources.metal);
  syncUnitRoster();
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
  syncUndoControls();
}

function setAppMode(mode) {
  hideCombatPreview();
  appMode = mode;
  if (mode !== "scenario") {
    setEditMode(false);
  }
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

function drawWalls(walls) {
  if (!walls || walls.length === 0) {
    return;
  }

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  for (const wall of walls) {
    if (!Array.isArray(wall.edge_path) || wall.edge_path.length === 0) {
      continue;
    }
    ctx.strokeStyle = terrainStyles.wall.stroke;
    ctx.lineWidth = 8.5 / viewport.scale;
    for (const edge of wall.edge_path) {
      const boundary = edgeBoundaryPoints(edge);
      if (!boundary) {
        continue;
      }
      ctx.beginPath();
      ctx.moveTo(boundary[0][0], boundary[0][1]);
      ctx.lineTo(boundary[1][0], boundary[1][1]);
      ctx.stroke();
    }

    ctx.strokeStyle = terrainStyles.wall.highlight;
    ctx.lineWidth = 2 / viewport.scale;
    for (const edge of wall.edge_path) {
      const boundary = edgeBoundaryPoints(edge);
      if (!boundary) {
        continue;
      }
      ctx.beginPath();
      ctx.moveTo(boundary[0][0], boundary[0][1]);
      ctx.lineTo(boundary[1][0], boundary[1][1]);
      ctx.stroke();
    }
  }

  ctx.restore();
}

function drawWallGates(gates) {
  if (!gates || gates.length === 0) {
    return;
  }

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  for (const gate of gates) {
    if (!gate.edge || !gate.edge.a || !gate.edge.b) {
      continue;
    }
    const boundary = edgeBoundaryPoints(gate.edge);
    if (!boundary) {
      continue;
    }
    const x = (boundary[0][0] + boundary[1][0]) / 2;
    const y = (boundary[0][1] + boundary[1][1]) / 2;
    const radius = 4.2 / viewport.scale;
    ctx.fillStyle = terrainStyles.wall.outline;
    ctx.beginPath();
    ctx.arc(x, y, radius * 1.35, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = gate.kind === "silk_gate" || gate.kind === "road_gate"
      ? terrainStyles.wall.roadGate
      : terrainStyles.wall.gate;
    ctx.beginPath();
    ctx.arc(x, y, radius, 0, Math.PI * 2);
    ctx.fill();
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

function readinessPercentForUnit(unit) {
  if (!unit || !Number.isFinite(unit.readiness) || !Number.isFinite(unit.maxReadiness) || unit.maxReadiness <= 0) {
    return 100;
  }
  return clamp(Math.round((unit.readiness / unit.maxReadiness) * 100), 0, 100);
}

function unitHpReadinessColor(unit) {
  const readinessPercent = readinessPercentForUnit(unit);
  if (readinessPercent >= 65) {
    return "#237a3b";
  }
  if (readinessPercent >= 30) {
    return "#a97800";
  }
  return "#c93632";
}

function unitSpriteLevelForScale(scale) {
  return unitSpriteZoomLevels.reduce((best, level, index) => (
    Math.abs(zoomScaleForLevel(index) - scale) < Math.abs(zoomScaleForLevel(unitSpriteZoomLevels.indexOf(best)) - scale) ? level : best
  ), unitSpriteZoomLevels[0]);
}

function currentUnitSpriteLevel() {
  return unitSpriteZoomLevels[viewport.zoomLevelIndex] || unitSpriteZoomLevels[0];
}

function fillSpritePixel(spriteCtx, x, y, color) {
  spriteCtx.fillStyle = color;
  spriteCtx.fillRect(x, y, 1, 1);
}

function fillSpriteRect(spriteCtx, x, y, width, height, color) {
  spriteCtx.fillStyle = color;
  spriteCtx.fillRect(x, y, width, height);
}

function drawSpriteLine(spriteCtx, x0, y0, x1, y1, color) {
  let currentX = Math.round(x0);
  let currentY = Math.round(y0);
  const targetX = Math.round(x1);
  const targetY = Math.round(y1);
  const dx = Math.abs(targetX - currentX);
  const sx = currentX < targetX ? 1 : -1;
  const dy = -Math.abs(targetY - currentY);
  const sy = currentY < targetY ? 1 : -1;
  let error = dx + dy;

  while (true) {
    fillSpritePixel(spriteCtx, currentX, currentY, color);
    if (currentX === targetX && currentY === targetY) {
      break;
    }
    const doubledError = 2 * error;
    if (doubledError >= dy) {
      error += dy;
      currentX += sx;
    }
    if (doubledError <= dx) {
      error += dx;
      currentY += sy;
    }
  }
}

function drawSpriteEllipseOutline(spriteCtx, centerX, centerY, radiusX, radiusY, color) {
  const steps = Math.max(12, Math.round(Math.max(radiusX, radiusY) * 5));
  for (let i = 0; i < steps; i += 1) {
    const angle = (i / steps) * Math.PI * 2;
    const x = Math.round(centerX + Math.cos(angle) * radiusX);
    const y = Math.round(centerY + Math.sin(angle) * radiusY);
    fillSpritePixel(spriteCtx, x, y, color);
  }
}

function spriteScale(size) {
  return size / 32;
}

function px(value, size) {
  return Math.round(value * spriteScale(size));
}

function spriteRect(spriteCtx, x, y, width, height, size, color) {
  fillSpriteRect(
    spriteCtx,
    px(x, size),
    px(y, size),
    Math.max(1, px(width, size)),
    Math.max(1, px(height, size)),
    color
  );
}

function spriteLine(spriteCtx, x0, y0, x1, y1, size, color) {
  drawSpriteLine(spriteCtx, px(x0, size), px(y0, size), px(x1, size), px(y1, size), color);
}

function spriteEllipse(spriteCtx, centerX, centerY, radiusX, radiusY, size, color) {
  drawSpriteEllipseOutline(spriteCtx, px(centerX, size), px(centerY, size), px(radiusX, size), px(radiusY, size), color);
}

function drawInfantrySprite(spriteCtx, size, color) {
  spriteLine(spriteCtx, 8, 24, 24, 8, size, color);
  spriteLine(spriteCtx, 8, 8, 24, 24, size, color);
  spriteRect(spriteCtx, 14, 6, 4, 4, size, color);
  spriteLine(spriteCtx, 16, 11, 16, 25, size, color);
}

function drawHordeSprite(spriteCtx, size, color) {
  spriteLine(spriteCtx, 16, 6, 27, 26, size, color);
  spriteLine(spriteCtx, 27, 26, 5, 26, size, color);
  spriteLine(spriteCtx, 5, 26, 16, 6, size, color);
}

function drawHerdSprite(spriteCtx, size, color) {
  spriteEllipse(spriteCtx, 11, 17, 6, 4, size, color);
  spriteEllipse(spriteCtx, 16, 17, 6, 4, size, color);
  spriteEllipse(spriteCtx, 21, 17, 6, 4, size, color);
}

function drawChineseCavalrySprite(spriteCtx, size, color) {
  spriteEllipse(spriteCtx, 15, 19, 9, 5, size, color);
  spriteLine(spriteCtx, 23, 18, 27, 15, size, color);
  spriteLine(spriteCtx, 8, 23, 6, 27, size, color);
  spriteLine(spriteCtx, 13, 23, 14, 28, size, color);
  spriteLine(spriteCtx, 20, 23, 19, 28, size, color);
  spriteLine(spriteCtx, 24, 23, 27, 27, size, color);
  spriteLine(spriteCtx, 16, 8, 16, 24, size, color);
  spriteLine(spriteCtx, 10, 13, 22, 13, size, color);
}

function drawMongolLancerSprite(spriteCtx, size, color) {
  spriteEllipse(spriteCtx, 14, 20, 8, 4, size, color);
  spriteLine(spriteCtx, 21, 19, 25, 17, size, color);
  spriteLine(spriteCtx, 8, 23, 6, 27, size, color);
  spriteLine(spriteCtx, 14, 23, 15, 28, size, color);
  spriteLine(spriteCtx, 20, 23, 19, 28, size, color);
  spriteLine(spriteCtx, 7, 27, 28, 6, size, color);
  spriteLine(spriteCtx, 24, 6, 29, 6, size, color);
  spriteLine(spriteCtx, 29, 6, 27, 11, size, color);
}

function drawCampSprite(spriteCtx, size, color) {
  spriteLine(spriteCtx, 7, 26, 16, 6, size, color);
  spriteLine(spriteCtx, 16, 6, 25, 26, size, color);
  spriteLine(spriteCtx, 25, 26, 7, 26, size, color);
  spriteLine(spriteCtx, 13, 26, 13, 18, size, color);
  spriteLine(spriteCtx, 19, 26, 19, 18, size, color);
  spriteLine(spriteCtx, 13, 18, 19, 18, size, color);
}

const unitSpriteDrawers = {
  infantry: drawInfantrySprite,
  horde: drawHordeSprite,
  herd: drawHerdSprite,
  chinese_cavalry: drawChineseCavalrySprite,
  mongol_lancer: drawMongolLancerSprite,
  camp: drawCampSprite,
};

function tintedUnitSprite(kind, color, level) {
  const normalizedKind = kind === "cavalry" ? "horse_archer" : kind;
  if (!Object.prototype.hasOwnProperty.call(unitSpriteColumns, normalizedKind)) {
    return null;
  }
  const spriteLevel = level || currentUnitSpriteLevel();
  const cacheKey = `${normalizedKind}:${color}:${spriteLevel.key}`;
  const cached = unitSpriteTintCache.get(cacheKey);
  if (cached) {
    return cached;
  }

  const canvas = document.createElement("canvas");
  canvas.width = spriteLevel.pixelSize;
  canvas.height = spriteLevel.pixelSize;
  const spriteCtx = canvas.getContext("2d");
  if (!spriteCtx) {
    return null;
  }
  spriteCtx.imageSmoothingEnabled = false;
  const bitmapSources = bitmapUnitSpriteImages[normalizedKind];
  if (bitmapSources) {
    const source = bitmapSources[spriteLevel.key];
    if (!unitSpriteSheetReady || !source || !source.complete || source.naturalWidth === 0) {
      return null;
    }
    spriteCtx.drawImage(source, 0, 0, spriteLevel.pixelSize, spriteLevel.pixelSize);
    spriteCtx.globalCompositeOperation = "source-in";
    spriteCtx.fillStyle = color;
    spriteCtx.fillRect(0, 0, spriteLevel.pixelSize, spriteLevel.pixelSize);
  } else {
    const drawSprite = unitSpriteDrawers[normalizedKind];
    if (!drawSprite) {
      return null;
    }
    drawSprite(spriteCtx, spriteLevel.pixelSize, color);
  }
  unitSpriteTintCache.set(cacheKey, canvas);
  return canvas;
}

function unitCounterMetrics() {
  const hexHeight = Math.sqrt(3) * geometry.size;
  const width = geometry.size * 1.26;
  const height = hexHeight * 0.58;
  const dividerOffset = width * 0.52;
  return {
    width,
    height,
    dividerOffset,
    iconCenterX: dividerOffset * 0.5,
    hpX: dividerOffset + (width - dividerOffset) * 0.5,
    iconSize: Math.min(height * 0.94, dividerOffset * 0.9),
    hpFont: height * 0.34,
    cornerRadius: geometry.size * 0.18,
    strokeWidth: geometry.size * 0.1,
    selectedStrokeWidth: geometry.size * 0.17,
    selectedInset: geometry.size * 0.16,
  };
}

function drawUnitSpriteGlyph(unit, faction, x, y, metrics) {
  const level = currentUnitSpriteLevel();
  const sprite = tintedUnitSprite(unit.kind, faction.color, level);
  if (!sprite) {
    return false;
  }
  const size = metrics.iconSize;
  const centerX = x + metrics.iconCenterX;
  const centerY = y + metrics.height / 2;
  const smoothing = ctx.imageSmoothingEnabled;
  ctx.imageSmoothingEnabled = true;
  ctx.drawImage(sprite, centerX - size / 2, centerY - size / 2, size, size);
  ctx.imageSmoothingEnabled = smoothing;
  return true;
}

function drawFallbackUnitGlyph(unit, faction, x, y, dividerX, metrics) {
  ctx.strokeStyle = faction.color;
  ctx.lineWidth = metrics.strokeWidth;
  if (unit.kind === "infantry") {
    const iconInsetX = metrics.width * 0.13;
    const iconInsetY = metrics.height * 0.22;
    ctx.beginPath();
    ctx.moveTo(x + iconInsetX, y + iconInsetY);
    ctx.lineTo(dividerX - iconInsetX, y + metrics.height - iconInsetY);
    ctx.moveTo(dividerX - iconInsetX, y + iconInsetY);
    ctx.lineTo(x + iconInsetX, y + metrics.height - iconInsetY);
    ctx.stroke();
  } else if (unit.kind === "horde") {
    const left = x + metrics.width * 0.13;
    const right = dividerX - metrics.width * 0.1;
    const top = y + metrics.height * 0.22;
    const bottom = y + metrics.height * 0.78;
    ctx.beginPath();
    ctx.moveTo((left + right) / 2, top);
    ctx.lineTo(right, bottom);
    ctx.lineTo(left, bottom);
    ctx.closePath();
    ctx.stroke();
  } else if (unit.kind === "herd") {
    const ovalY = y + metrics.height / 2;
    const ovalCenters = [0.18, 0.26, 0.34].map((offset) => x + metrics.width * offset);
    for (const ovalX of ovalCenters) {
      ctx.beginPath();
      ctx.ellipse(ovalX, ovalY, metrics.width * 0.11, metrics.height * 0.13, 0, 0, Math.PI * 2);
      ctx.stroke();
    }
  } else if (unit.kind === "horse_archer") {
    ctx.beginPath();
    ctx.ellipse(x + metrics.iconCenterX, y + metrics.height / 2, metrics.width * 0.14, metrics.height * 0.15, 0, 0, Math.PI * 2);
    ctx.stroke();
  } else if (unit.kind === "chinese_cavalry") {
    const centerX = x + metrics.iconCenterX;
    const centerY = y + metrics.height / 2;
    ctx.beginPath();
    ctx.ellipse(centerX, centerY, metrics.width * 0.14, metrics.height * 0.15, 0, 0, Math.PI * 2);
    ctx.moveTo(centerX, centerY - metrics.height * 0.21);
    ctx.lineTo(centerX, centerY + metrics.height * 0.21);
    ctx.stroke();
  } else if (unit.kind === "mongol_lancer") {
    const left = x + metrics.width * 0.14;
    const right = dividerX - metrics.width * 0.12;
    const top = y + metrics.height * 0.21;
    const bottom = y + metrics.height * 0.79;
    ctx.beginPath();
    ctx.moveTo(left, bottom);
    ctx.lineTo(right, top);
    ctx.moveTo(left + metrics.width * 0.05, top);
    ctx.lineTo(right, top);
    ctx.lineTo(right - metrics.width * 0.08, top + metrics.height * 0.14);
    ctx.stroke();
  } else {
    ctx.beginPath();
    ctx.rect(x + metrics.width * 0.14, y + metrics.height * 0.25, metrics.width * 0.2, metrics.height * 0.42);
    ctx.fillStyle = faction.color;
    ctx.fill();
  }
}

function drawUnitCounters(units) {
  if (!units || units.length === 0) {
    return;
  }

  ctx.save();
  const selectedUnitId = currentMap && currentMap.game && Number.isInteger(currentMap.game.selectedUnitId)
    ? currentMap.game.selectedUnitId
    : 0;
  for (const unit of units) {
    const faction = factions[unit.faction] || factions.mongol;
    const metrics = unitCounterMetrics();
    const center = hexCenter(unit);
    const counterWidth = metrics.width;
    const counterHeight = metrics.height;
    const x = center.x - counterWidth / 2;
    const y = center.y - counterHeight / 2;
    const dividerX = x + metrics.dividerOffset;

    roundedRectPath(x, y, counterWidth, counterHeight, metrics.cornerRadius);
    ctx.fillStyle = "#fffdf8";
    ctx.fill();
    if (legalAttackForUnit(unit)) {
      ctx.fillStyle = "rgba(201, 54, 50, 0.24)";
      ctx.fill();
    }
    ctx.strokeStyle = faction.color;
    ctx.lineWidth = unit.id === selectedUnitId ? metrics.selectedStrokeWidth : metrics.strokeWidth;
    ctx.stroke();
    if (unit.id === selectedUnitId) {
      roundedRectPath(
        x - metrics.selectedInset,
        y - metrics.selectedInset,
        counterWidth + metrics.selectedInset * 2,
        counterHeight + metrics.selectedInset * 2,
        metrics.cornerRadius + metrics.selectedInset
      );
      ctx.strokeStyle = "#f4e48a";
      ctx.lineWidth = metrics.strokeWidth * 0.8;
      ctx.stroke();
    }

    ctx.beginPath();
    ctx.moveTo(dividerX, y + counterHeight * 0.13);
    ctx.lineTo(dividerX, y + counterHeight * 0.87);
    ctx.strokeStyle = "#b9ad96";
    ctx.lineWidth = metrics.strokeWidth * 0.45;
    ctx.stroke();

    if (!drawUnitSpriteGlyph(unit, faction, x, y, metrics)) {
      drawFallbackUnitGlyph(unit, faction, x, y, dividerX, metrics);
    }

    ctx.fillStyle = unitHpReadinessColor(unit);
    ctx.font = `${metrics.hpFont}px Segoe UI, Arial, sans-serif`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(unit.hp), x + metrics.hpX, y + counterHeight / 2 + metrics.height * 0.02);
  }
  ctx.restore();
}

function drawReachableHexes() {
  const moves = legalMoves();
  if (moves.length === 0) {
    return;
  }

  ctx.save();
  for (const move of moves) {
    const center = hexCenter(move);
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

function drawDetachDeploymentHexes() {
  const placement = detachHerdPlacement || createUnitPlacement;
  if (!placement || !Array.isArray(placement.deployableHexes)) {
    return;
  }

  ctx.save();
  for (const hex of placement.deployableHexes) {
    const center = hexCenter(hex);
    const points = hexPoints(center.x, center.y, geometry.size * 0.84);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(83, 148, 91, 0.42)";
    ctx.fill();
    ctx.strokeStyle = "#2d6936";
    ctx.lineWidth = 2 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function findUnitAtPoint(point) {
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return null;
  }
  const metrics = unitCounterMetrics();
  const counterWidth = metrics.width;
  const counterHeight = metrics.height;
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

function hideContextMenu() {
  contextMenuRequestId += 1;
  activeContextMenu = null;
  contextMenu.hidden = true;
  contextMenu.replaceChildren();
}

function contextForPointer(event) {
  const point = panelToWorld(event);
  const rect = mapPanel.getBoundingClientRect();
  return {
    point,
    panelX: event.clientX - rect.left,
    panelY: event.clientY - rect.top,
    hex: findNearestHex(point),
    unit: findUnitAtPoint(point),
    selected: selectedUnit(),
  };
}

function positionContextMenu(event) {
  const rect = mapPanel.getBoundingClientRect();
  const margin = 8;
  const rawLeft = event.clientX - rect.left;
  const rawTop = event.clientY - rect.top;
  const maxLeft = Math.max(margin, rect.width - contextMenu.offsetWidth - margin);
  const maxTop = Math.max(margin, rect.height - contextMenu.offsetHeight - margin);
  contextMenu.style.left = `${clamp(rawLeft, margin, maxLeft)}px`;
  contextMenu.style.top = `${clamp(rawTop, margin, maxTop)}px`;
}

async function hydrateContextActionAvailability(context) {
  context.actionAvailability = {
    detachHerd: false,
    createHorseArchers: false,
    createMongolLancers: false,
  };

  const unit = context.unit;
  if (!unit || unit.kind !== "horde" || unit.owner !== activeOwner()) {
    return;
  }

  const checks = [];
  if (Number.isInteger(unit.horses) && unit.horses > 0) {
    checks.push(postAppCommand({
      type: "detach_herd_options",
      unitId: unit.id,
      horses: 1,
    }).then((options) => {
      context.actionAvailability.detachHerd = Array.isArray(options.deployableHexes)
        && options.deployableHexes.length > 0;
    }).catch(() => {
      context.actionAvailability.detachHerd = false;
    }));
  }

  if (
    Number.isInteger(unit.population)
    && Number.isInteger(unit.metal)
    && Number.isInteger(unit.horses)
    && unit.population >= 1
    && unit.metal >= 1
    && unit.horses >= 3
  ) {
    checks.push(postAppCommand({
      type: "create_horse_archers_options",
      unitId: unit.id,
    }).then((options) => {
      context.actionAvailability.createHorseArchers = Array.isArray(options.deployableHexes)
        && options.deployableHexes.length > 0;
    }).catch(() => {
      context.actionAvailability.createHorseArchers = false;
    }));
  }

  if (
    unit.faction === "mongol"
    && Number.isInteger(unit.population)
    && Number.isInteger(unit.metal)
    && Number.isInteger(unit.horses)
    && unit.population >= 1
    && unit.metal >= 2
    && unit.horses >= 3
  ) {
    checks.push(postAppCommand({
      type: "create_mongol_lancers_options",
      unitId: unit.id,
    }).then((options) => {
      context.actionAvailability.createMongolLancers = Array.isArray(options.deployableHexes)
        && options.deployableHexes.length > 0;
    }).catch(() => {
      context.actionAvailability.createMongolLancers = false;
    }));
  }

  await Promise.all(checks);
}

async function showContextMenu(event) {
  const requestId = contextMenuRequestId + 1;
  contextMenuRequestId = requestId;
  activeContextMenu = null;
  contextMenu.hidden = true;
  contextMenu.replaceChildren();

  if (appMode !== "play" || !currentMap) {
    return false;
  }

  const context = contextForPointer(event);
  await hydrateContextActionAvailability(context);
  if (requestId !== contextMenuRequestId) {
    return false;
  }

  const actions = contextActionDefinitions.filter((action) => action.visible(context));
  if (actions.length === 0) {
    return false;
  }

  activeContextMenu = context;
  contextMenu.replaceChildren();
  for (const action of actions) {
    const button = document.createElement("button");
    button.type = "button";
    button.setAttribute("role", "menuitem");
    button.dataset.action = action.id;
    button.textContent = action.label;
    button.disabled = !action.enabled(context);
    button.addEventListener("click", async () => {
      if (button.disabled || !activeContextMenu) {
        return;
      }
      try {
        hideContextMenu();
        await action.run(context);
      } catch (error) {
        window.alert(error.message);
      }
    });
    contextMenu.appendChild(button);
  }
  contextMenu.hidden = false;
  positionContextMenu(event);
  return true;
}

function hideDetachHerdAmount() {
  detachHerdAmountContext = null;
  detachHerdPopover.hidden = true;
}

function cancelDetachHerdPlacement() {
  detachHerdPlacement = null;
  drawMap();
}

function cancelCreateUnitPlacement() {
  createUnitPlacement = null;
  drawMap();
}

function positionDetachHerdPopover(context) {
  const margin = 8;
  detachHerdPopover.hidden = false;
  const maxLeft = Math.max(margin, mapPanel.clientWidth - detachHerdPopover.offsetWidth - margin);
  const maxTop = Math.max(margin, mapPanel.clientHeight - detachHerdPopover.offsetHeight - margin);
  detachHerdPopover.style.left = `${clamp(context.panelX, margin, maxLeft)}px`;
  detachHerdPopover.style.top = `${clamp(context.panelY, margin, maxTop)}px`;
}

function showDetachHerdAmount(context) {
  if (!context.unit || context.unit.kind !== "horde" || !Number.isInteger(context.unit.horses) || context.unit.horses <= 0) {
    return false;
  }
  cancelDetachHerdPlacement();
  cancelCreateUnitPlacement();
  detachHerdAmountContext = context;
  detachHerdHorsesInput.min = "1";
  detachHerdHorsesInput.max = String(context.unit.horses);
  detachHerdHorsesInput.value = String(Math.min(1, context.unit.horses));
  positionDetachHerdPopover(context);
  detachHerdHorsesInput.focus();
  detachHerdHorsesInput.select();
  return true;
}

async function confirmDetachHerdAmount() {
  if (!detachHerdAmountContext || !detachHerdAmountContext.unit) {
    return;
  }
  const unit = selectedUnit() && selectedUnit().id === detachHerdAmountContext.unit.id
    ? selectedUnit()
    : detachHerdAmountContext.unit;
  const maxHorses = Number.isInteger(unit.horses) ? unit.horses : 0;
  const horses = Math.trunc(clamp(Number(detachHerdHorsesInput.value), 1, maxHorses));
  if (!Number.isInteger(horses) || horses <= 0) {
    return;
  }
  try {
    const options = await postAppCommand({
      type: "detach_herd_options",
      unitId: unit.id,
      horses,
    });
    if (!Array.isArray(options.deployableHexes) || options.deployableHexes.length === 0) {
      window.alert("No adjacent deployment hex is available.");
      hideDetachHerdAmount();
      return;
    }
    detachHerdPlacement = {
      unitId: unit.id,
      horses,
      deployableHexes: options.deployableHexes,
    };
    hideDetachHerdAmount();
    drawMap();
  } catch (error) {
    window.alert(error.message);
  }
}

function detachDeployableAt(coord) {
  return detachHerdPlacement && detachHerdPlacement.deployableHexes.find((hex) => hex.q === coord.q && hex.r === coord.r);
}

function createUnitDeployableAt(coord) {
  return createUnitPlacement
    && createUnitPlacement.deployableHexes.find((hex) => hex.q === coord.q && hex.r === coord.r);
}

async function deployDetachedHerdAt(coord) {
  if (!detachHerdPlacement || !detachDeployableAt(coord)) {
    return false;
  }
  const payload = await postUndoableGameCommand({
    type: "detach_herd",
    unitId: detachHerdPlacement.unitId,
    horses: detachHerdPlacement.horses,
    to: { q: coord.q, r: coord.r },
  });
  detachHerdPlacement = null;
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function startCreateHorseArchersPlacement(context) {
  return startCreateUnitPlacement(context, {
    optionsCommandType: "create_horse_archers_options",
    createCommandType: "create_horse_archers",
  });
}

async function startCreateMongolLancersPlacement(context) {
  return startCreateUnitPlacement(context, {
    optionsCommandType: "create_mongol_lancers_options",
    createCommandType: "create_mongol_lancers",
  });
}

async function startCreateUnitPlacement(context, config) {
  if (!context.unit || context.unit.kind !== "horde") {
    return false;
  }
  cancelDetachHerdPlacement();
  hideDetachHerdAmount();
  try {
    const options = await postAppCommand({
      type: config.optionsCommandType,
      unitId: context.unit.id,
    });
    if (!Array.isArray(options.deployableHexes) || options.deployableHexes.length === 0) {
      window.alert("No adjacent deployment hex is available.");
      createUnitPlacement = null;
      drawMap();
      return false;
    }
    createUnitPlacement = {
      unitId: context.unit.id,
      kind: options.kind,
      createCommandType: config.createCommandType,
      deployableHexes: options.deployableHexes,
      populationCost: options.populationCost,
      metalCost: options.metalCost,
      horsesCost: options.horsesCost,
    };
    drawMap();
    return true;
  } catch (error) {
    window.alert(error.message);
    return false;
  }
}

async function deployCreatedUnitAt(coord) {
  if (!createUnitPlacement || !createUnitDeployableAt(coord)) {
    return false;
  }
  const payload = await postUndoableGameCommand({
    type: createUnitPlacement.createCommandType,
    unitId: createUnitPlacement.unitId,
    to: { q: coord.q, r: coord.r },
  });
  createUnitPlacement = null;
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function selectUnit(unit) {
  if (!unit) {
    return false;
  }
  hideCombatPreview();
  const payload = await postAppCommand({
    type: "select_unit",
    unitId: unit.id,
  });
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function setUnitStance(unit, stance) {
  if (!unit || typeof stance !== "string") {
    return false;
  }
  hideCombatPreview();
  const payload = await postUndoableGameCommand({
    type: "set_unit_stance",
    unitId: unit.id,
    stance,
  });
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function moveSelectedUnitTo(coord) {
  const unit = selectedUnit();
  const move = legalMoveAt(coord);
  if (!move) {
    return false;
  }
  hideCombatPreview();
  const payload = await postUndoableGameCommand({
    type: "move_unit",
    unitId: unit.id,
    to: { q: coord.q, r: coord.r },
  });
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function attackSelectedUnit(defender) {
  const attacker = selectedUnit();
  if (!attacker || !defender || !legalAttackForUnit(defender)) {
    return false;
  }
  hideCombatPreview();
  const payload = await postUndoableGameCommand({
    type: "attack_unit",
    attackerId: attacker.id,
    defenderId: defender.id,
  });
  applyGamePatch(payload);
  syncModeControls();
  drawMap();
  return true;
}

async function advanceTurn() {
  hideCombatPreview();
  try {
    applyGamePatch(await postUndoableGameCommand({ type: "end_turn" }));
  } catch (error) {
    window.alert(error.message);
  }
  syncModeControls();
  drawMap();
}

async function undoLastAction() {
  hideCombatPreview();
  hideContextMenu();
  hideDetachHerdAmount();
  cancelDetachHerdPlacement();
  cancelCreateUnitPlacement();

  if (appMode === "play") {
    if (playUndoDepth <= 0) {
      syncUndoControls();
      return false;
    }
    try {
      const payload = await postAppCommand({ type: "undo" });
      playUndoDepth = Math.max(0, playUndoDepth - 1);
      applyGamePatch(payload);
      syncModeControls();
      drawMap();
      return true;
    } catch (error) {
      playUndoDepth = 0;
      syncUndoControls();
      window.alert(error.message);
      return false;
    }
  }

  const snapshot = localUndoStack.pop();
  if (!snapshot) {
    syncUndoControls();
    return false;
  }
  restoreLocalUndo(snapshot);
  return true;
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
  drawWalls(currentMap.walls);
  drawRiverEdges(currentMap.edges);
  drawWallGates(currentMap.wall_gates);
  drawCrossings(currentMap.crossings);
  drawMapMarkers(currentMap.river_sources, terrainStyles.river.source, 5);
  drawMapMarkers(currentMap.merge_points, terrainStyles.river.merge, 5);
  drawMapMarkers(currentMap.river_destinations, terrainStyles.river.destination, 5);
  drawReachableHexes();
  drawDetachDeploymentHexes();
  drawUnitCounters(currentMap.units);

  ctx.restore();
}

function syncEditorControls() {
  editModeButton.classList.toggle("is-active", isEditing);
  editModeButton.setAttribute("aria-pressed", String(isEditing));
  mapPanel.classList.toggle("is-editing", isEditing);
  const mode = editorModeSelect.value;
  terrainPalette.hidden = mode !== "terrain";
  editorEdgeFeatureSelect.hidden = mode !== "edges";
  editorUnitTypeSelect.hidden = mode !== "units";
  editorUnitSideSelect.hidden = mode !== "units";

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
      editorModeSelect.value = "terrain";
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

function roadEdgeKey(road) {
  if (!road || !Array.isArray(road.path) || road.path.length !== 2) {
    return "";
  }
  return edgeKey(canonicalEdge(road.path[0], road.path[1]));
}

function toggleRoadEdge(edge) {
  currentMap.roads = currentMap.roads || [];
  const key = edgeKey(edge);
  const index = currentMap.roads.findIndex((road) => road.feature === "editor_road" && roadEdgeKey(road) === key);
  if (index >= 0) {
    currentMap.roads.splice(index, 1);
    return;
  }
  const nextId = currentMap.roads.reduce((maxId, road) => Math.max(maxId, Number.isInteger(road.id) ? road.id : 0), 0) + 1;
  currentMap.roads.push({
    id: nextId,
    feature: "editor_road",
    path: [{ ...edge.a }, { ...edge.b }],
  });
}

function nextUnitId() {
  const units = currentMap && Array.isArray(currentMap.units) ? currentMap.units : [];
  return units.reduce((maxId, unit) => Math.max(maxId, Number.isInteger(unit.id) ? unit.id : 0), 0) + 1;
}

function makeEditorUnit(coord) {
  const kind = editorUnitTypeSelect.value;
  const factionKey = editorUnitSideSelect.value;
  const faction = factions[factionKey] || factions.mongol;
  const defaults = editorUnitDefaultsFor(kind);
  return {
    id: nextUnitId(),
    owner: faction.owner,
    faction: factionKey,
    kind,
    q: coord.q,
    r: coord.r,
    hp: defaults.hp,
    maxHp: defaults.hp,
    attack: defaults.attack,
    defense: defaults.defense,
    readinessDamage: defaults.readinessDamage,
    readiness: defaults.readiness,
    maxReadiness: 100,
    stance: defaults.stance,
    move: defaults.move,
    remainingMove: defaults.move,
    population: Number.isInteger(defaults.population) ? defaults.population : 0,
    metal: Number.isInteger(defaults.metal) ? defaults.metal : 0,
    horses: Number.isInteger(defaults.horses) ? defaults.horses : 0,
    projectsZoc: Boolean(defaults.projectsZoc),
    respectsZoc: Boolean(defaults.respectsZoc),
  };
}

function toggleEditorUnit(hex) {
  currentMap.units = Array.isArray(currentMap.units) ? currentMap.units : [];
  const kind = editorUnitTypeSelect.value;
  const factionKey = editorUnitSideSelect.value;
  const faction = factions[factionKey] || factions.mongol;
  const existingSelected = currentMap.units.findIndex((unit) => (
    unit.q === hex.q
    && unit.r === hex.r
    && unit.kind === kind
    && unit.owner === faction.owner
  ));
  if (existingSelected >= 0) {
    const removedId = currentMap.units[existingSelected].id;
    currentMap.units.splice(existingSelected, 1);
    if (currentMap.game && currentMap.game.selectedUnitId === removedId) {
      currentMap.game.selectedUnitId = 0;
    }
    return;
  }

  currentMap.units = currentMap.units.filter((unit) => !(unit.q === hex.q && unit.r === hex.r));
  currentMap.units.push(normalizeUnit(makeEditorUnit(hex), currentMap.units.length));
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
  const mode = editorModeSelect.value;
  if (mode === "edges") {
    const edge = findNearestEditableEdge(point);
    if (!edge) {
      return;
    }
    const key = `edge:${edgeKey(edge)}`;
    if (paintStrokeKeys.has(key)) {
      return;
    }
    paintStrokeKeys.add(key);
    if (!paintUndoRecorded) {
      recordLocalUndo();
      paintUndoRecorded = true;
    }
    if (editorEdgeFeatureSelect.value === "road") {
      toggleRoadEdge(edge);
    } else {
      if (event.shiftKey) {
        setRiverEdge(edge, false);
      } else {
        toggleRiverEdge(edge);
      }
    }
    refreshDerivedTopology();
    drawMap();
    return;
  }

  const hex = findNearestHex(point);
  if (!hex) {
    return;
  }
  if (mode === "units") {
    const key = `unit:${coordKey(hex)}`;
    if (paintStrokeKeys.has(key)) {
      return;
    }
    paintStrokeKeys.add(key);
    if (!paintUndoRecorded) {
      recordLocalUndo();
      paintUndoRecorded = true;
    }
    try {
      toggleEditorUnit(hex);
    } catch (error) {
      window.alert(error.message);
      return;
    }
    syncModeControls();
    drawMap();
    return;
  }

  const terrain = event.shiftKey ? "grassland" : selectedTerrain;
  const key = `hex:${coordKey(hex)}`;
  if (paintStrokeKeys.has(key)) {
    return;
  }
  paintStrokeKeys.add(key);
  if (!paintUndoRecorded) {
    recordLocalUndo();
    paintUndoRecorded = true;
  }
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
    currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
    terrainUndo = new Map();
    clearUndoHistory();
    ensureGameMeta();
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
    walls: [],
    wall_gates: [],
    crossings: [],
    units: [],
    metadata: {
      generator: typeof options.generator === "string" ? options.generator : "blank-editor",
      terrain_types: editorTerrains.map((terrain) => terrain.key),
      hex_label_model: "base-plus-generation-role.v1",
    },
  };
  terrainUndo = new Map();
  clearUndoHistory();
  currentMap.game = {};
  ensureGameMeta();
  refreshDerivedTopology();
  syncModeControls();
  fitMap();
}

async function createDefaultPlayScenario() {
  try {
    currentMap = await postAppCommand({
      type: "new_game",
      width: 80,
      height: 40,
      factions: factionCount,
    });
    currentMap.units = Array.isArray(currentMap.units) ? currentMap.units.map(normalizeUnit) : [];
    terrainUndo = new Map();
    clearUndoHistory();
    ensureGameMeta();
    refreshDerivedTopology();
    syncModeControls();
    fitMap();
  } catch (error) {
    window.alert(error.message);
  }
}

async function initializeApp() {
  try {
    await loadUnitTypeDefaults();
    await createDefaultPlayScenario();
  } catch (error) {
    window.alert(error.message);
  }
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
    walls: Array.isArray(map.walls) ? map.walls : [],
    wall_gates: Array.isArray(map.wall_gates) ? map.wall_gates : [],
    crossings: Array.isArray(map.crossings) ? map.crossings : [],
    units: Array.isArray(map.units)
      ? map.units
        .filter((unit) => unit && Number.isInteger(unit.q) && Number.isInteger(unit.r))
        .map(normalizeUnit)
      : [],
    game: map.game && typeof map.game === "object" ? map.game : {},
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

function isTextEditingTarget(target) {
  if (!target) {
    return false;
  }
  const tagName = target.tagName;
  return target.isContentEditable
    || tagName === "INPUT"
    || tagName === "TEXTAREA"
    || tagName === "SELECT";
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
    terrainUndo = new Map();
    clearUndoHistory();
    ensureGameMeta();
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
blankMapButton.addEventListener("click", () => createBlankMap());
editModeButton.addEventListener("click", () => setEditMode(!isEditing));
editorModeSelect.addEventListener("change", syncEditorControls);
editorEdgeFeatureSelect.addEventListener("change", syncEditorControls);
editorUnitTypeSelect.addEventListener("change", syncEditorControls);
editorUnitSideSelect.addEventListener("change", syncEditorControls);
saveButton.addEventListener("click", saveCurrentMap);
loadButton.addEventListener("click", chooseMapFile);
loadFileInput.addEventListener("change", () => loadMapFile(loadFileInput.files[0]));
undoButton.addEventListener("click", () => {
  undoLastAction().catch((error) => window.alert(error.message));
});
endTurnButton.addEventListener("click", advanceTurn);
controlEndTurnButton.addEventListener("click", advanceTurn);
statusEndTurnButton.addEventListener("click", advanceTurn);
detachHerdForm.addEventListener("submit", (event) => {
  event.preventDefault();
  confirmDetachHerdAmount();
});
contextMenu.addEventListener("pointerdown", (event) => event.stopPropagation());
contextMenu.addEventListener("click", (event) => event.stopPropagation());
detachHerdPopover.addEventListener("pointerdown", (event) => event.stopPropagation());
detachHerdPopover.addEventListener("click", (event) => event.stopPropagation());
zoomInButton.addEventListener("click", () => zoomFromCenter(1));
zoomOutButton.addEventListener("click", () => zoomFromCenter(-1));
fitButton.addEventListener("click", fitMap);

mapPanel.addEventListener("wheel", (event) => {
  if (!currentMap) {
    return;
  }
  event.preventDefault();
  const rect = mapPanel.getBoundingClientRect();
  zoomStepAt(event.clientX - rect.left, event.clientY - rect.top, event.deltaY < 0 ? 1 : -1);
}, { passive: false });

mapPanel.addEventListener("pointerdown", async (event) => {
  mapPanel.focus();
  hideCombatPreview();
  if (event.button === 2) {
    event.preventDefault();
    hideDetachHerdAmount();
    cancelDetachHerdPlacement();
    cancelCreateUnitPlacement();
    await showContextMenu(event);
    return;
  }
  hideContextMenu();
  hideDetachHerdAmount();
  const point = panelToWorld(event);
  if (appMode === "play" && event.button === 0 && detachHerdPlacement) {
    const hex = findNearestHex(point);
    if (hex && detachDeployableAt(hex)) {
      event.preventDefault();
      await deployDetachedHerdAt(hex);
      return;
    }
    event.preventDefault();
    return;
  }
  if (appMode === "play" && event.button === 0 && createUnitPlacement) {
    const hex = findNearestHex(point);
    if (hex && createUnitDeployableAt(hex)) {
      event.preventDefault();
      await deployCreatedUnitAt(hex);
      return;
    }
    event.preventDefault();
    return;
  }
  if (appMode === "play" && event.button === 0) {
    const unit = findUnitAtPoint(point);
    if (unit) {
      event.preventDefault();
      if (legalAttackForUnit(unit) && await attackSelectedUnit(unit)) {
        return;
      }
      await selectUnit(unit);
      return;
    }
    const hex = findNearestHex(point);
    if (hex && await moveSelectedUnitTo(hex)) {
      event.preventDefault();
      return;
    }
  }
  if (appMode === "scenario" && isEditing && event.button === 0) {
    event.preventDefault();
    isPainting = true;
    paintStrokeKeys = new Set();
    paintUndoRecorded = false;
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
    hideCombatPreview();
    event.preventDefault();
    paintAtPointer(event);
    return;
  }
  if (!isPanning) {
    updateCombatPreviewHover(event);
    return;
  }
  hideCombatPreview();
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

mapPanel.addEventListener("contextmenu", (event) => {
  if (appMode === "play") {
    event.preventDefault();
  }
});

mapPanel.addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    hideCombatPreview();
    hideContextMenu();
    hideDetachHerdAmount();
    cancelDetachHerdPlacement();
    cancelCreateUnitPlacement();
    return;
  }
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

window.addEventListener("keydown", (event) => {
  if (!(event.ctrlKey || event.metaKey) || event.shiftKey || event.altKey || event.key.toLowerCase() !== "z") {
    return;
  }
  if (isTextEditingTarget(event.target) || !canUndo()) {
    return;
  }
  event.preventDefault();
  undoLastAction().catch((error) => window.alert(error.message));
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
loadBitmapUnitSprites();
initializeApp();
