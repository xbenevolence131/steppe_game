const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const mapPanel = document.querySelector("#map-panel");
const widthInput = document.querySelector("#map-width");
const heightInput = document.querySelector("#map-height");
const riversInput = document.querySelector("#river-sources");
const meanderForwardInput = document.querySelector("#meander-forward");
const meanderForwardJitterInput = document.querySelector("#meander-forward-jitter");
const meanderLateralInput = document.querySelector("#meander-lateral");
const meanderLateralJitterInput = document.querySelector("#meander-lateral-jitter");
const meanderStrengthInput = document.querySelector("#meander-strength");
const meanderReachInput = document.querySelector("#meander-reach");
const meanderTimeoutInput = document.querySelector("#meander-timeout");
const generateButton = document.querySelector("#generate-button");
const saveButton = document.querySelector("#save-button");
const loadButton = document.querySelector("#load-button");
const zoomInButton = document.querySelector("#zoom-in-button");
const zoomOutButton = document.querySelector("#zoom-out-button");
const fitButton = document.querySelector("#fit-button");

let currentMap = null;
let isPanning = false;
let lastPointer = { x: 0, y: 0 };

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

const terrainStyles = {
  none: {
    fill: "#383a3a",
    stroke: "#8b8f88",
    label: "#dedbd1",
  },
  grassland: {
    fill: "#d8c596",
    stroke: "#7e735f",
    label: "#29251d",
  },
  river: {
    stroke: "#2679a6",
    source: "#60c4e8",
    merge: "#f4e48a",
    destination: "#1f5f83",
  },
};

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

async function generateMap() {
  const width = clampDimension(widthInput.value, 1, 120, 120);
  const height = clampDimension(heightInput.value, 1, 80, 80);
  const rivers = clampDimension(riversInput.value, 0, 20, 4);
  const meanderForward = clampNumberInput(meanderForwardInput, 0, 40, 8);
  const meanderForwardJitter = clampNumberInput(meanderForwardJitterInput, 0, 40, 4);
  const meanderLateral = clampNumberInput(meanderLateralInput, 0, 40, 7);
  const meanderLateralJitter = clampNumberInput(meanderLateralJitterInput, 0, 40, 4);
  const meanderStrength = clampNumberInput(meanderStrengthInput, 0, 10, 1);
  const meanderReach = clampNumberInput(meanderReachInput, 0, 40, 2);
  const meanderTimeout = clampDimension(meanderTimeoutInput.value, 1, 200, 28);
  const seed = newSeed();
  widthInput.value = width;
  heightInput.value = height;
  riversInput.value = rivers;
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
        meanderForward,
        meanderForwardJitter,
        meanderLateral,
        meanderLateralJitter,
        meanderStrength,
        meanderReach,
        meanderTimeout,
        seed,
      }),
    });
    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload.error || "generation failed");
    }
    currentMap = payload;
    fitMap();
  } catch (error) {
    window.alert(error.message);
  } finally {
    generateButton.disabled = false;
  }
}

generateButton.addEventListener("click", generateMap);
saveButton.addEventListener("click", () => window.alert("Save terrain is not implemented yet."));
loadButton.addEventListener("click", () => window.alert("Load terrain is not implemented yet."));
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
  if (!isPanning) {
    return;
  }
  panBy(event.clientX - lastPointer.x, event.clientY - lastPointer.y);
  lastPointer = { x: event.clientX, y: event.clientY };
});

mapPanel.addEventListener("pointerup", (event) => {
  if (!isPanning) {
    return;
  }
  stopPanning();
  mapPanel.releasePointerCapture(event.pointerId);
});

mapPanel.addEventListener("pointercancel", stopPanning);
mapPanel.addEventListener("lostpointercapture", stopPanning);

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

generateMap();
