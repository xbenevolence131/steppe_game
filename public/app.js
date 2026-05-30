const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const widthInput = document.querySelector("#map-width");
const heightInput = document.querySelector("#map-height");
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
  ctx.lineWidth = 1;
  ctx.stroke();

  ctx.fillStyle = style.label;
  ctx.font = `${Math.max(10, Math.min(13, size * 0.42))}px Segoe UI, Arial, sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, cx, cy);
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

  for (const hex of map.hexes) {
    const col = hex.q - 1;
    const row = hex.r - 1;
    const cx = margin + size + col * size * 1.5;
    const cy = margin + hexHeight / 2 + row * hexHeight + (col % 2) * hexHeight / 2;
    drawHex(cx, cy, size, `${hex.q},${hex.r}`, hex.terrain);
  }
}

async function generateMap() {
  const width = clampDimension(widthInput.value, 1, 120, 120);
  const height = clampDimension(heightInput.value, 1, 80, 80);
  const seed = newSeed();
  widthInput.value = width;
  heightInput.value = height;

  generateButton.disabled = true;
  try {
    const response = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ width, height, seed }),
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
