const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const widthInput = document.querySelector("#map-width");
const heightInput = document.querySelector("#map-height");
const riversInput = document.querySelector("#rivers");
const sourceVarianceInput = document.querySelector("#source-variance");
const horizontalBandInput = document.querySelector("#horizontal-band");
const generateButton = document.querySelector("#generate-button");
const saveButton = document.querySelector("#save-button");
const loadButton = document.querySelector("#load-button");

let currentMap = null;

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
  riverSource: {
    fill: "#2d79b8",
    stroke: "#9bc7e8",
    label: "#f3fbff",
  },
  riverDestination: {
    fill: "#145f77",
    stroke: "#8fd8e8",
    label: "#f3fbff",
  },
  riverMerge: {
    fill: "#1f8d7a",
    stroke: "#9ee3d6",
    label: "#f3fbff",
  },
};

function clampDimension(value, min, max, fallback) {
  const parsed = Number(value);
  if (!Number.isInteger(parsed)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, parsed));
}

function newSeed() {
  const values = new Uint32Array(1);
  window.crypto.getRandomValues(values);
  return values[0];
}

function resizeCanvas(width, height) {
  const dpr = window.devicePixelRatio || 1;
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;
  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);
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

function drawHex(cx, cy, size, label, terrain, marker) {
  const style = marker
    ? terrainStyles[marker]
    : terrainStyles[terrain] || terrainStyles.none;
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
  ctx.lineWidth = 1;
  ctx.stroke();

  ctx.fillStyle = style.label;
  ctx.font = `${Math.max(10, Math.min(13, size * 0.42))}px Segoe UI, Arial, sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, cx, cy);
}

function coordKey(coord) {
  return `${coord.q},${coord.r}`;
}

function sharedEdgeEndpoints(a, b, size) {
  const pointsA = hexPoints(a.cx, a.cy, size);
  const pointsB = hexPoints(b.cx, b.cy, size);
  const shared = [];

  for (const pointA of pointsA) {
    for (const pointB of pointsB) {
      if (Math.hypot(pointA[0] - pointB[0], pointA[1] - pointB[1]) < 1.25) {
        shared.push([
          (pointA[0] + pointB[0]) / 2,
          (pointA[1] + pointB[1]) / 2,
        ]);
      }
    }
  }

  if (shared.length >= 2) {
    return [
      { x: shared[0][0], y: shared[0][1] },
      { x: shared[1][0], y: shared[1][1] },
    ];
  }

  const dx = b.cx - a.cx;
  const dy = b.cy - a.cy;
  const length = Math.hypot(dx, dy) || 1;
  const nx = -dy / length;
  const ny = dx / length;
  const midX = (a.cx + b.cx) / 2;
  const midY = (a.cy + b.cy) / 2;
  const half = size * 0.42;

  return {
    0: { x: midX + nx * half, y: midY + ny * half },
    1: { x: midX - nx * half, y: midY - ny * half },
    length: 2,
  };
}

function drawRiverEdges(edges, centerForHex, size) {
  ctx.save();
  ctx.strokeStyle = "#5bb9ec";
  ctx.lineWidth = Math.max(4, size * 0.16);
  ctx.lineCap = "round";

  for (const edge of edges || []) {
    if (!edge.river) {
      continue;
    }

    const endpoints = sharedEdgeEndpoints(centerForHex(edge.a), centerForHex(edge.b), size);
    ctx.beginPath();
    ctx.moveTo(endpoints[0].x, endpoints[0].y);
    ctx.lineTo(endpoints[1].x, endpoints[1].y);
    ctx.stroke();
  }

  ctx.restore();
}

function drawMap(map) {
  const panel = canvas.parentElement;
  const margin = 22;
  const desiredWidth = Math.max(panel.clientWidth, 520);
  const maxSizeByWidth = (desiredWidth - margin * 2) / (1.5 * Math.max(1, map.width - 1) + 2);
  const maxSizeByHeight = 42;
  const size = Math.max(14, Math.min(maxSizeByWidth, maxSizeByHeight));
  const hexHeight = Math.sqrt(3) * size;
  const mapPixelWidth = margin * 2 + size * (1.5 * Math.max(1, map.width - 1) + 2);
  const mapPixelHeight = margin * 2 + hexHeight * (map.height + 0.5);

  resizeCanvas(Math.max(desiredWidth, mapPixelWidth), Math.max(360, mapPixelHeight));

  ctx.fillStyle = terrainStyles.none.fill;
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  const sourceKeys = new Set((map.river_sources || []).map((source) => coordKey(source)));
  const destinationKeys = new Set((map.river_destinations || []).map((destination) => coordKey(destination)));
  const mergeKeys = new Set((map.merge_points || []).map((mergePoint) => coordKey(mergePoint)));

  function centerForHex(hex) {
    const col = hex.q - 1;
    const row = hex.r - 1;
    return {
      cx: margin + size + col * size * 1.5,
      cy: margin + hexHeight / 2 + row * hexHeight + (col % 2) * hexHeight / 2,
    };
  }

  for (const hex of map.hexes) {
    const { cx, cy } = centerForHex(hex);
    drawHex(cx, cy, size, `${hex.q},${hex.r}`, hex.terrain, null);
  }

  drawRiverEdges(map.edges, centerForHex, size);

  for (const hex of map.hexes) {
    const key = coordKey(hex);
    const marker = sourceKeys.has(key)
      ? "riverSource"
      : mergeKeys.has(key)
        ? "riverMerge"
      : destinationKeys.has(key)
        ? "riverDestination"
        : null;

    if (marker) {
      const { cx, cy } = centerForHex(hex);
      drawHex(cx, cy, size, `${hex.q},${hex.r}`, hex.terrain, marker);
    }
  }
}

async function generateMap() {
  const width = clampDimension(widthInput.value, 1, 160, 100);
  const height = clampDimension(heightInput.value, 1, 80, 40);
  const rivers = clampDimension(riversInput.value, 0, 100, 3);
  const sourceVariance = clampDimension(sourceVarianceInput.value, 0, 100, 8);
  const horizontalBand = clampDimension(horizontalBandInput.value, 0, 100, 8);
  const seed = newSeed();
  widthInput.value = width;
  heightInput.value = height;
  riversInput.value = rivers;
  sourceVarianceInput.value = sourceVariance;
  horizontalBandInput.value = horizontalBand;

  generateButton.disabled = true;
  try {
    const response = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ width, height, seed, rivers, sourceVariance, horizontalBand }),
    });
    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload.error || "generation failed");
    }
    currentMap = payload;
    drawMap(currentMap);
  } catch (error) {
    window.alert(error.message);
  } finally {
    generateButton.disabled = false;
  }
}

generateButton.addEventListener("click", generateMap);
saveButton.addEventListener("click", () => window.alert("Save terrain is not implemented yet."));
loadButton.addEventListener("click", () => window.alert("Load terrain is not implemented yet."));
window.addEventListener("resize", () => {
  if (currentMap) {
    drawMap(currentMap);
  }
});

generateMap();
