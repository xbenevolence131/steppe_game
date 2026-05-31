const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const mapPanel = document.querySelector("#map-panel");
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

let currentMap = null;
let isPanning = false;
let isEditing = false;
let isPainting = false;
let lastPointer = { x: 0, y: 0 };
let selectedTerrain = "lake";
let paintStrokeKeys = new Set();
let terrainUndo = new Map();

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
  { key: "woods", label: "Woods", fill: "#246b3b", stroke: "#133c22", labelColor: "#eef7e8" },
  { key: "marsh", label: "Marsh", fill: "#74794b", stroke: "#42462a", labelColor: "#f3eed0" },
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
  woods: terrainStyle("woods"),
  light_forest: terrainStyle("woods"),
  heavy_forest: terrainStyle("woods"),
  marsh: terrainStyle("marsh"),
  urban: terrainStyle("urban"),
  river: {
    stroke: "#2679a6",
    source: "#60c4e8",
    merge: "#f4e48a",
    destination: "#1f5f83",
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

function drawHex(cx, cy, size, label, terrain) {
  const style = terrainStyles[terrain] || terrainStyles.none;
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
    drawHex(center.x, center.y, geometry.size, `${hex.q},${hex.r}`, hex.terrain);
  }

  drawRiverEdges(currentMap.edges);
  drawMapMarkers(currentMap.river_sources, terrainStyles.river.source, 5);
  drawMapMarkers(currentMap.merge_points, terrainStyles.river.merge, 5);
  drawMapMarkers(currentMap.river_destinations, terrainStyles.river.destination, 5);

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
  isEditing = enabled;
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
    return;
  }

  if (!terrainUndo.has(key)) {
    terrainUndo.set(key, hex.terrain);
  }
  hex.terrain = terrain;
}

function paintAtPointer(event) {
  if (!isEditing || !currentMap) {
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
  const meanderTimeout = clampDimension(meanderTimeoutInput.value, 1, 200, 28);
  const seed = newSeed();
  widthInput.value = width;
  heightInput.value = height;
  riversInput.value = rivers;
  lakesInput.value = lakes;
  lakeSizeInput.value = lakeSize;
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
        meanderTimeout,
        seed,
      }),
    });
    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload.error || "generation failed");
    }
    currentMap = payload;
    terrainUndo = new Map();
    fitMap();
  } catch (error) {
    window.alert(error.message);
  } finally {
    generateButton.disabled = false;
  }
}

function createBlankMap() {
  const width = clampDimension(widthInput.value, 1, 120, 120);
  const height = clampDimension(heightInput.value, 1, 80, 80);
  widthInput.value = width;
  heightInput.value = height;

  const hexes = [];
  for (let r = 1; r <= height; r += 1) {
    for (let q = 1; q <= width; q += 1) {
      hexes.push({ q, r, terrain: "grassland" });
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
    roads: [],
    metadata: {
      generator: "blank-editor",
      terrain_types: editorTerrains.map((terrain) => terrain.key),
    },
  };
  terrainUndo = new Map();
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

  return {
    schema: typeof map.schema === "string" ? map.schema : "steppe-terrain.v1",
    seed: Number.isInteger(map.seed) ? map.seed : 0,
    width: map.width,
    height: map.height,
    hexes: map.hexes.map((hex) => ({
      q: Number(hex.q),
      r: Number(hex.r),
      terrain: typeof hex.terrain === "string" ? hex.terrain : "grassland",
    })).filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r)),
    river_sources: Array.isArray(map.river_sources) ? map.river_sources : [],
    river_destinations: Array.isArray(map.river_destinations) ? map.river_destinations : [],
    merge_points: Array.isArray(map.merge_points) ? map.merge_points : [],
    river_segments: Array.isArray(map.river_segments) ? map.river_segments : [],
    edges: Array.isArray(map.edges) ? map.edges : [],
    roads: Array.isArray(map.roads) ? map.roads : [],
    metadata: map.metadata && typeof map.metadata === "object"
      ? map.metadata
      : { generator: "loaded-editor", terrain_types: editorTerrains.map((terrain) => terrain.key) },
  };
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
    widthInput.value = currentMap.width;
    heightInput.value = currentMap.height;
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

generateButton.addEventListener("click", generateMap);
blankMapButton.addEventListener("click", createBlankMap);
editModeButton.addEventListener("click", () => setEditMode(!isEditing));
editorToolSelect.addEventListener("change", syncEditorControls);
saveButton.addEventListener("click", saveCurrentMap);
loadButton.addEventListener("click", chooseMapFile);
loadFileInput.addEventListener("change", () => loadMapFile(loadFileInput.files[0]));
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
  if (isEditing && event.button === 0) {
    event.preventDefault();
    isPainting = true;
    paintStrokeKeys = new Set();
    paintAtPointer(event);
    mapPanel.setPointerCapture(event.pointerId);
    return;
  }
  if (event.button !== 1) {
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
generateMap();
