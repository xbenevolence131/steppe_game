const canvas = document.querySelector("#map-canvas");
const ctx = canvas.getContext("2d");
const appShell = document.querySelector("#app-shell");
const mapSection = document.querySelector(".map-section");
const mapPanel = document.querySelector("#map-panel");
const diplomacyPanel = document.querySelector("#diplomacy-panel");
const diplomacyTable = document.querySelector("#diplomacy-table");
const contextMenu = document.querySelector("#context-menu");
const combatPreview = document.querySelector("#combat-preview");
const detachHerdPopover = document.querySelector("#detach-herd-popover");
const detachHerdForm = document.querySelector("#detach-herd-form");
const detachHerdHorsesInput = document.querySelector("#detach-herd-horses");
const scenarioModeButton = document.querySelector("#scenario-mode-button");
const playModeButton = document.querySelector("#play-mode-button");
const scenarioResizeHandle = document.querySelector("#scenario-resize-handle");
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
const scenarioFileDialog = document.querySelector("#scenario-file-dialog");
const scenarioFileForm = document.querySelector("#scenario-file-form");
const scenarioFileTitle = document.querySelector("#scenario-file-title");
const scenarioFileNameField = document.querySelector("#scenario-file-name-field");
const scenarioFileNameInput = document.querySelector("#scenario-file-name");
const scenarioFileList = document.querySelector("#scenario-file-list");
const scenarioFileCancel = document.querySelector("#scenario-file-cancel");
const scenarioFileConfirm = document.querySelector("#scenario-file-confirm");
const undoButton = document.querySelector("#undo-button");
const zoomInButton = document.querySelector("#zoom-in-button");
const zoomOutButton = document.querySelector("#zoom-out-button");
const fitButton = document.querySelector("#fit-button");
const mapAiToggleButton = document.querySelector("#map-ai-toggle-button");
const pastureViewButton = document.querySelector("#pasture-view-button");
const territoryViewButton = document.querySelector("#territory-view-button");
const unitsViewButton = document.querySelector("#units-view-button");
const coordinatesViewButton = document.querySelector("#coordinates-view-button");
const detailsToggleButton = document.querySelector("#details-toggle-button");
const scenarioNameInput = document.querySelector("#scenario-name");
const foodConsumptionEnabledInput = document.querySelector("#food-consumption-enabled");
const scenarioRegions = Array.from(document.querySelectorAll(".scenario-region"));
const scenarioToolButtons = Array.from(document.querySelectorAll("[data-scenario-activate]"));
const scenarioCollapseButtons = Array.from(document.querySelectorAll("[data-scenario-toggle]"));
const scenarioToolPanels = Array.from(document.querySelectorAll(".scenario-tool-panel"));
const unitToolButtons = Array.from(document.querySelectorAll(".unit-tool-button"));
const scenarioFactionRows = Array.from(document.querySelectorAll(".scenario-faction-table tbody tr"));
const terrainEditModeButtons = Array.from(document.querySelectorAll(".terrain-edit-mode-button"));
const edgeFeatureButtons = Array.from(document.querySelectorAll(".edge-feature-button"));
const ownershipToolButtons = Array.from(document.querySelectorAll(".ownership-tool-button"));
const editorEdgeFeatureSelect = document.querySelector("#editor-edge-feature");
const edgeFeatureChoices = document.querySelector("#edge-feature-choices");
const editorUnitTypeSelect = document.querySelector("#editor-unit-type");
const editorUnitSideSelect = document.querySelector("#editor-unit-side");
const ownershipFactionSelect = document.querySelector("#ownership-faction-select");
const ownershipFactionSwatch = document.querySelector("#ownership-faction-swatch");
const terrainPalette = document.querySelector("#terrain-palette");
const addAiGroupButton = document.querySelector("#add-ai-group-button");
const factionRelationshipsContainer = document.querySelector("#faction-relationships");
const aiGroupsContainer = document.querySelector("#ai-groups");
const strategicAiGroupsContainer = document.querySelector("#strategic-ai-groups");
const strategicAiToggleButton = document.querySelector("#strategic-ai-toggle-button");
const strategicAiSelectNoneButton = document.querySelector("#strategic-ai-select-none-button");
const strategicAiPanelSummary = document.querySelector("#strategic-ai-panel-summary");
const endTurnButton = document.querySelector("#end-turn-button");
const controlEndTurnButton = document.querySelector("#control-end-turn-button");
const statusEndTurnButton = document.querySelector("#status-end-turn-button");
const executeAiGroupButton = document.querySelector("#execute-ai-group-button");
const prevUnitButton = document.querySelector("#prev-unit-button");
const nextUnitButton = document.querySelector("#next-unit-button");
const diplomacyToggleButton = document.querySelector("#diplomacy-toggle-button");
const turnCounter = document.querySelector(".turn-counter");
const activeFactionName = document.querySelector("#active-faction-name");
const statusActiveFactionName = document.querySelector("#status-active-faction-name");
const factionStatusName = document.querySelector("#faction-status-name");
const factionPopulationTotal = document.querySelector("#faction-population-total");
const factionHorsesTotal = document.querySelector("#faction-horses-total");
const factionFoodTotal = document.querySelector("#faction-food-total");
const factionMetalTotal = document.querySelector("#faction-metal-total");
const factionTreasureTotal = document.querySelector("#faction-treasure-total");
const roundCount = document.querySelector("#round-count");
const turnStatusReadout = document.querySelector("#turn-status-readout");
const herdCount = document.querySelector("#herd-count");
const horseArcherCount = document.querySelector("#horse-archer-count");
const hordeCount = document.querySelector("#horde-count");
const sidebarFactionMetal = document.querySelector("#sidebar-faction-metal");
const sidebarSelectionReadout = document.querySelector(".sidebar-selection-readout");
const unitInspectorInfoToggle = document.querySelector("#unit-inspector-info-toggle");
const unitName = document.querySelector("#unit-name");
const unitNameInput = document.querySelector("#unit-name-input");
const unitType = document.querySelector("#unit-type");
const unitTypeInput = document.querySelector("#unit-type-input");
const unitHp = document.querySelector("#unit-hp");
const unitHpInput = document.querySelector("#unit-hp-input");
const unitAttack = document.querySelector("#unit-attack");
const unitDefense = document.querySelector("#unit-defense");
const unitReadinessDamage = document.querySelector("#unit-readiness-damage");
const unitReadiness = document.querySelector("#unit-readiness");
const unitReadinessInput = document.querySelector("#unit-readiness-input");
const unitMove = document.querySelector("#unit-move");
const unitZoc = document.querySelector("#unit-zoc");
const unitSpecialProperties = document.querySelector("#unit-special-properties");
const unitStatsView = document.querySelector("#unit-stats-view");
const unitStance = document.querySelector("#unit-stance");
const unitStanceInput = document.querySelector("#unit-stance-input");
const unitResources = document.querySelector("#unit-resources");
const unitPopulationRow = document.querySelector("#unit-population-row");
const unitPopulation = document.querySelector("#unit-population");
const unitPopulationInput = document.querySelector("#unit-population-input");
const unitHorsesRow = document.querySelector("#unit-horses-row");
const unitHorses = document.querySelector("#unit-horses");
const unitHorsesInput = document.querySelector("#unit-horses-input");
const unitStarvationRow = document.querySelector("#unit-starvation-row");
const unitStarvation = document.querySelector("#unit-starvation");
const unitProductionRow = document.querySelector("#unit-production-row");
const unitProduction = document.querySelector("#unit-production");
const hexTitleCoordinate = document.querySelector("#hex-title-coordinate");
const hexName = document.querySelector("#hex-name");
const hexTerrain = document.querySelector("#hex-terrain");
const hexDefense = document.querySelector("#hex-defense");
const hexMoveCost = document.querySelector("#hex-move-cost");
const hexRoadCost = document.querySelector("#hex-road-cost");
const hexSupplyState = document.querySelector("#hex-supply-state");
const hexPastureRow = document.querySelector("#hex-pasture-row");
const hexPasture = document.querySelector("#hex-pasture");

let currentMap = null;
const engineProjection = {
  version: 0,
  hash: "",
  lastSyncReason: "",
};
let appMode = "intro";
let isPanning = false;
let isPainting = false;
let lastPointer = { x: 0, y: 0 };
let selectedTerrain = "lake";
let selectedOwnershipOwner = -1;
let ownershipTool = "owner";
let scenarioTool = "session";
let unitTool = "place";
let editorSelectedUnitId = 0;
let aiPickMode = null;
let activeAiGroupId = 0;
let activeStrategicAiGroupId = 0;
let nextStrategicAiGroupIndex = 0;
let unitSelectionRequestId = 0;
let unitSelectionInProgress = false;
let collapsedAiGroupIds = new Set();
let collapsedStrategicAiGroupIds = new Set();
let strategicAiCollapsed = false;
let openScenarioRegions = new Set(["session", "factions", "terrain", "ownership", "units", "ai"]);
let paintStrokeKeys = new Set();
let paintUndoRecorded = false;
let terrainUndo = new Map();
let localUndoStack = [];
let playUndoDepth = 0;
let playModeEngineSync = Promise.resolve();
let scenarioEditorEngineSync = Promise.resolve();
let scenarioEditorSyncVersion = 0;
let currentTurn = 1;
let activeFactionIndex = 0;
let activeContextMenu = null;
let contextMenuRequestId = 0;
let combatPreviewRequestId = 0;
let combatPreviewTargetId = 0;
let detachHerdAmountContext = null;
let detachHerdPlacement = null;
let createUnitPlacement = null;
let scenarioFileMode = "load";
let selectedScenarioFile = "";
let appInitialization = null;
let pastureViewEnabled = false;
let territoryViewEnabled = false;
let unitsViewEnabled = true;
let coordinateLabelsEnabled = true;
let unitInspectorStatsVisible = false;
let scenarioDetailsCollapsed = false;
let scenarioPanelHeight = 156;
let selectedHexCoord = null;
let aiAnimationState = null;
let aiAnimationInProgress = false;
let playSurfaceMode = "map";

const hordeHorseCapacity = 20;

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
const moveScale = 8;
const scenarioDirectoryDbName = "steppe-scenario-files";
const scenarioDirectoryStoreName = "handles";
const scenarioDirectoryHandleKey = "scenario-directory";
const scenarioPanelHeightStorageKey = "steppe.scenarioPanelHeight";
const defaultScenarioPanelHeight = 156;
const minScenarioPanelHeight = 112;

const editorTerrains = [
  { key: "grassland", label: "Grassland", fill: "#d8c596", stroke: "#7e735f", labelColor: "#29251d" },
  { key: "farmland", label: "Farmland", fill: "#c8ad62", stroke: "#806f35", labelColor: "#241f0d" },
  { key: "none", label: "None", fill: "#111111", stroke: "#5b5f5b", labelColor: "#eeeeee" },
  { key: "lake", label: "Lake", fill: "#82c7e6", stroke: "#245e78", labelColor: "#082c3a" },
  { key: "hill", label: "Hill", fill: "#b98c56", stroke: "#6f4f2f", labelColor: "#21170f" },
  { key: "mountain", label: "Mountain", fill: "#5b3724", stroke: "#2c1a11", labelColor: "#f1e7d8" },
  { key: "forest", label: "Forest", fill: "#2f8a4c", stroke: "#176032", labelColor: "#eef7e8" },
  { key: "marsh", label: "Marsh", fill: "#969b6a", stroke: "#5f633d", labelColor: "#211f12" },
  { key: "desert", label: "Desert", fill: "#e1c84f", stroke: "#8b7624", labelColor: "#211b08" },
  { key: "urban", label: "Urban", fill: "#8e8e8e", stroke: "#4e4e4e", labelColor: "#111111" },
];

const terrainStyles = {
  none: terrainStyle("none"),
  grassland: terrainStyle("grassland"),
  farmland: terrainStyle("farmland"),
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
    archetype: "steppe_nomad",
    color: "#d6a21a",
    owner: 0,
    metal: 4,
    treasure: 0,
    food: 20,
    enabled: true,
    ai: false,
  },
  chinese: {
    name: "Chinese",
    archetype: "chinese",
    color: "#c93632",
    owner: 2,
    metal: 4,
    treasure: 0,
    food: 20,
    enabled: true,
    ai: false,
  },
  persian: {
    name: "Persian",
    archetype: "persian",
    color: "#1f4fa3",
    owner: 1,
    metal: 4,
    treasure: 0,
    food: 20,
    enabled: false,
    ai: false,
  },
  jurchen: {
    name: "Jurchen",
    archetype: "jurchen",
    color: "#1f6f68",
    owner: 3,
    metal: 4,
    treasure: 0,
    food: 20,
    enabled: false,
    ai: true,
  },
  forest_nomad: {
    name: "Forest Nomads",
    archetype: "forest_nomad",
    color: "#2f6b35",
    owner: 4,
    metal: 2,
    treasure: 0,
    food: 20,
    enabled: false,
    ai: true,
  },
  neutral: {
    name: "Neutral",
    archetype: "neutral",
    color: "#777777",
    owner: -1,
    metal: 0,
    treasure: 0,
    food: 0,
    enabled: false,
    ai: false,
  },
};

const factionCount = 2;
const defaultFactionKeys = ["mongol", "chinese", "jurchen", "forest_nomad", "persian"];
const factionTurnOrder = defaultFactionKeys.slice(0, factionCount);

const unitTypeDefaults = {};
const unitDisplayKindNames = {
  horse_archer: "Horse Archers",
  chinese_cavalry: "Cavalry",
  chinese_militia: "Militia",
  mongol_lancer: "Lancers",
  infantry: "Chinese Infantry",
  persian_infantry: "Persian Infantry",
  persian_cavalry: "Persian Cavalry",
  jurchen_infantry: "Jurchen Infantry",
  jurchen_cavalry: "Jurchen Cavalry",
  forest_warband: "Forest Warband",
  forest_raiders: "Forest Raiders",
  horde: "Horde",
  herd: "Herd",
};
const unitKindLabels = {
  herd: "Herd",
  horse_archer: "Horse Archers",
  chinese_cavalry: "Chinese Cavalry",
  chinese_militia: "Chinese Militia",
  mongol_lancer: "Lancers",
  infantry: "Chinese Infantry",
  persian_infantry: "Persian Infantry",
  persian_cavalry: "Persian Cavalry",
  jurchen_infantry: "Jurchen Infantry",
  jurchen_cavalry: "Jurchen Cavalry",
  forest_warband: "Forest Warband",
  forest_raiders: "Forest Raiders",
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
  persian_infantry: "persian_infantry",
  persian_cavalry: "persian_cavalry",
  jurchen_infantry: "jurchen_infantry",
  jurchen_cavalry: "jurchen_cavalry",
  forest_warband: "forest_warband",
  forest_raiders: "forest_raiders",
};
const unitSpriteZoomLevels = [
  { key: "small", pixelSize: 48, visibleHexColumns: null },
  { key: "medium", pixelSize: 96, visibleHexColumns: 40 },
  { key: "large", pixelSize: 128, visibleHexColumns: 20 },
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
  persian_infantry: {
    small: "/unit-sprites/infantry_48.png",
    medium: "/unit-sprites/infantry_96.png",
    large: "/unit-sprites/infantry_128.png",
  },
  persian_cavalry: {
    small: "/unit-sprites/heavy_cavalry_48.png",
    medium: "/unit-sprites/heavy_cavalry_96.png",
    large: "/unit-sprites/heavy_cavalry_128.png",
  },
  jurchen_infantry: {
    small: "/unit-sprites/infantry_48.png",
    medium: "/unit-sprites/infantry_96.png",
    large: "/unit-sprites/infantry_128.png",
  },
  jurchen_cavalry: {
    small: "/unit-sprites/heavy_cavalry_48.png",
    medium: "/unit-sprites/heavy_cavalry_96.png",
    large: "/unit-sprites/heavy_cavalry_128.png",
  },
  forest_warband: {
    small: "/unit-sprites/militia_48.png",
    medium: "/unit-sprites/militia_96.png",
    large: "/unit-sprites/militia_128.png",
  },
  forest_raiders: {
    small: "/unit-sprites/horse_archer_48.png",
    medium: "/unit-sprites/horse_archer_96.png",
    large: "/unit-sprites/horse_archer_128.png",
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

function titleCaseToken(value) {
  return String(value || "")
    .split("_")
    .filter(Boolean)
    .map((part) => `${part.charAt(0).toUpperCase()}${part.slice(1)}`)
    .join(" ");
}

function defaultPastureForTerrain(terrain) {
  const capacity = terrain === "grassland" ? 100 : 0;
  return { capacity, remaining: capacity };
}

function roundPastureValue(value) {
  return Math.round(value * 100) / 100;
}

function formatPastureValue(value) {
  return Number.isFinite(value) ? value.toFixed(2) : "0.00";
}

function normalizePasture(rawPasture, terrain) {
  const fallback = defaultPastureForTerrain(terrain);
  if (!rawPasture || typeof rawPasture !== "object") {
    return fallback;
  }
  const capacity = Number.isFinite(rawPasture.capacity)
    ? roundPastureValue(clamp(rawPasture.capacity, 0, 100))
    : fallback.capacity;
  const remaining = Number.isFinite(rawPasture.remaining)
    ? roundPastureValue(clamp(rawPasture.remaining, 0, capacity))
    : Math.min(fallback.remaining, capacity);
  return { capacity, remaining };
}

function supplyStateLabel(hex) {
  if (!hex || !territoryClaimableHex(hex)) {
    return "-";
  }
  const owner = territoryOwnerForHex(hex);
  const archetype = factionArchetypeForOwner(owner);
  if (owner === factions.neutral.owner || archetype === "steppe_nomad" || archetype === "forest_nomad" || archetype === "neutral") {
    return "None";
  }
  if (hex.supplySource) {
    return "Source";
  }
  if (hex.supplied === true) {
    return "Supplied";
  }
  if (hex.supplied === false) {
    return "Out";
  }
  return "Unknown";
}

function terrainDefensePercent(terrain) {
  switch (terrain) {
    case "hill":
    case "forest":
      return 125;
    case "mountain":
    case "urban":
      return 150;
    case "marsh":
      return 115;
    case "desert":
      return 90;
    case "grassland":
    case "farmland":
    case "none":
    case "lake":
    default:
      return 100;
  }
}

function terrainMovementCostScaled(terrain) {
  switch (terrain) {
    case "grassland":
    case "farmland":
    case "desert":
      return 8;
    case "hill":
    case "forest":
      return 12;
    case "urban":
    case "marsh":
    case "mountain":
      return 16;
    case "none":
    case "lake":
    default:
      return Infinity;
  }
}

function roadModifiedMovementCostScaled(terrainCost, mounted) {
  if (!Number.isFinite(terrainCost)) {
    return Infinity;
  }
  const footRoadCost = Math.floor((terrainCost + 1) / 2);
  return mounted ? footRoadCost + 1 : footRoadCost;
}

function formatMoveCost(scaledCost) {
  if (!Number.isFinite(scaledCost)) {
    return "Blocked";
  }
  const refCost = scaledCost / moveScale;
  return Number.isInteger(refCost) ? String(refCost) : refCost.toFixed(2).replace(/0$/, "");
}

function normalizeMapHex(hex) {
  const terrain = typeof hex.terrain === "string" ? hex.terrain : "grassland";
  const normalized = {
    q: Number(hex.q),
    r: Number(hex.r),
    terrain,
    name: normalizeHexName(hex.name),
    labels: Array.isArray(hex.labels)
      ? hex.labels.filter((label) => typeof label === "string")
      : editorLabelsForTerrain(terrain),
    pasture: normalizePasture(hex.pasture, terrain),
  };
  if (Number.isInteger(hex.owner)) {
    normalized.owner = hex.owner;
  }
  if (typeof hex.supplySource === "boolean") {
    normalized.supplySource = hex.supplySource;
  } else if (typeof hex.supply_source === "boolean") {
    normalized.supplySource = hex.supply_source;
  }
  if (typeof hex.supplied === "boolean") {
    normalized.supplied = hex.supplied;
  }
  return normalized;
}

function normalizeHexName(name) {
  return typeof name === "string" && name.trim() ? name.trim() : "None";
}

function townName(town) {
  if (!town || typeof town !== "object") {
    return "None";
  }
  if (typeof town.name === "string" && town.name.trim()) {
    return town.name.trim();
  }
  switch (town.feature) {
    case "persian_town":
      return "Persian Town";
    case "chinese_town":
      return "Chinese Town";
    case "dzungarian_gate":
      return "Dzungarian Gate";
    case "oasis":
      return "Oasis";
    case "grassland_water":
      return "Water Town";
    default:
      return "Town";
  }
}

function hexNameFromLabels(labels) {
  if (!Array.isArray(labels)) {
    return "None";
  }
  if (labels.includes("persian_town")) {
    return "Persian Town";
  }
  if (labels.includes("chinese_town")) {
    return "Chinese Town";
  }
  if (labels.includes("dzungarian_gate")) {
    return "Dzungarian Gate";
  }
  if (labels.includes("oasis")) {
    return "Oasis";
  }
  if (labels.includes("water_adjacent_town") || labels.includes("grassland_water")) {
    return "Water Town";
  }
  return labels.includes("urban") ? "Town" : "None";
}

function syncHexNamesFromTowns(map) {
  if (!map || !Array.isArray(map.hexes)) {
    return;
  }
  for (const hex of map.hexes) {
    hex.name = normalizeHexName(hex.name);
    if (hex.name === "None") {
      hex.name = hexNameFromLabels(hex.labels);
    }
  }
  if (!Array.isArray(map.towns)) {
    return;
  }
  const hexesByCoord = new Map(map.hexes.map((hex) => [coordKey(hex), hex]));
  for (const town of map.towns) {
    if (!town || !Number.isInteger(town.q) || !Number.isInteger(town.r)) {
      continue;
    }
    const hex = hexesByCoord.get(coordKey(town));
    if (!hex) {
      continue;
    }
    const currentName = normalizeHexName(hex.name);
    const labelName = hexNameFromLabels(hex.labels);
    const hasExplicitTownName = typeof town.name === "string" && town.name.trim();
    if (!hasExplicitTownName && currentName !== "None" && currentName !== labelName) {
      continue;
    }
    hex.name = townName(town);
  }
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
  syncHexNamesFromTowns(map);
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

function hexAtCoord(coord) {
  if (!currentMap || !coord || !Array.isArray(currentMap.hexes)) {
    return null;
  }
  return currentMap.hexes.find((hex) => hex.q === coord.q && hex.r === coord.r) || null;
}

function selectedHex() {
  return hexAtCoord(selectedHexCoord);
}

function selectHex(hex, options = {}) {
  selectedHexCoord = hex ? { q: hex.q, r: hex.r } : null;
  syncHexInspector();
  if (options.draw !== false) {
    drawMap();
  }
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
  if (!level.visibleHexColumns || !currentMap) {
    return clamp(viewport.fitScale, viewport.minScale, viewport.maxScale);
  }
  const targetColumns = Math.max(1, Math.min(currentMap.width, level.visibleHexColumns));
  const target = worldSizeForHexSpan(targetColumns, 1);
  const targetScale = viewport.width / target.width;
  return clamp(Math.max(viewport.fitScale, targetScale), viewport.minScale, viewport.maxScale);
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

function screenPointForWorldPoint(point) {
  return {
    x: viewport.offsetX + point.x * viewport.scale,
    y: viewport.offsetY + point.y * viewport.scale,
  };
}

function viewportCenterWorldPoint() {
  return {
    x: (viewport.width / 2 - viewport.offsetX) / viewport.scale,
    y: (viewport.height / 2 - viewport.offsetY) / viewport.scale,
  };
}

function centerViewportOnWorldPoint(point) {
  if (!currentMap || !point || !Number.isFinite(point.x) || !Number.isFinite(point.y)) {
    return false;
  }
  const before = { x: viewport.offsetX, y: viewport.offsetY };
  viewport.offsetX = viewport.width / 2 - point.x * viewport.scale;
  viewport.offsetY = viewport.height / 2 - point.y * viewport.scale;
  constrainViewport();
  const moved = Math.abs(viewport.offsetX - before.x) > 0.5 || Math.abs(viewport.offsetY - before.y) > 0.5;
  if (moved) {
    drawMap();
  }
  return moved;
}

function pointNearViewportCenter(point) {
  if (!point || viewport.width <= 0 || viewport.height <= 0) {
    return true;
  }
  const screen = screenPointForWorldPoint(point);
  return Math.abs(screen.x - viewport.width / 2) <= viewport.width * 0.28
    && Math.abs(screen.y - viewport.height / 2) <= viewport.height * 0.28;
}

function aiAnimationStepFocusPoint(step) {
  const coords = [];
  const addCoord = (coord) => {
    if (coord && Number.isInteger(coord.q) && Number.isInteger(coord.r)) {
      coords.push(coord);
    }
  };
  addCoord(step && step.from);
  addCoord(step && step.to);
  for (const attack of step && Array.isArray(step.attacks) ? step.attacks : []) {
    addCoord(attack);
  }
  for (const event of step && Array.isArray(step.attackEvents) ? step.attackEvents : []) {
    addCoord(event.target);
    addCoord(event.defenderFrom);
    addCoord(event.defenderTo);
    addCoord(event.attackerTo);
  }
  if (coords.length === 0) {
    return viewportCenterWorldPoint();
  }
  const points = coords.map(hexCenter);
  const minX = Math.min(...points.map((point) => point.x));
  const maxX = Math.max(...points.map((point) => point.x));
  const minY = Math.min(...points.map((point) => point.y));
  const maxY = Math.max(...points.map((point) => point.y));
  return {
    x: (minX + maxX) / 2,
    y: (minY + maxY) / 2,
  };
}

async function focusAiAnimationStep(step) {
  const focus = aiAnimationStepFocusPoint(step);
  if (!pointNearViewportCenter(focus) && centerViewportOnWorldPoint(focus)) {
    await sleep(180);
  }
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

function scenarioEditingActive() {
  return appMode === "scenario"
    && (scenarioTool === "terrain" || scenarioTool === "ownership" || scenarioTool === "units" || scenarioTool === "ai");
}

function terrainEditingActive() {
  return appMode === "scenario" && scenarioTool === "terrain";
}

function ownershipEditingActive() {
  return appMode === "scenario" && scenarioTool === "ownership";
}

function unitPlaceActive() {
  return appMode === "scenario" && scenarioTool === "units" && unitTool === "place";
}

function unitEditActive() {
  return appMode === "scenario" && scenarioTool === "units" && unitTool === "edit";
}

function aiEditingActive() {
  return appMode === "scenario" && scenarioTool === "ai";
}

function aiMemberPickActive() {
  return aiEditingActive() && aiPickMode === "members";
}

function aiTargetPickActive() {
  return aiEditingActive() && aiPickMode === "target";
}

function scenarioName() {
  const value = scenarioNameInput ? scenarioNameInput.value.trim() : "";
  return value || "Untitled Scenario";
}

function foodConsumptionEnabled() {
  return foodConsumptionEnabledInput ? foodConsumptionEnabledInput.checked : true;
}

function updateFoodConsumptionSettingFromInput() {
  if (!currentMap) {
    return;
  }
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.foodConsumption = foodConsumptionEnabled();
  void syncScenarioEditorToEngine();
}

function selectedScenarioFactionsFromControls() {
  const selected = scenarioFactionRows.map((row) => {
    const key = row.dataset.factionKey || "mongol";
    return normalizeScenarioFaction({
      key,
      enabled: Boolean(row.querySelector(".scenario-faction-enabled")?.checked),
      ai: Boolean(row.querySelector(".scenario-faction-ai")?.checked),
    }, key);
  });
  return selected.length > 0 ? selected : defaultScenarioFactions();
}

function syncScenarioParameterControls() {
  if (scenarioNameInput) {
    scenarioNameInput.value = currentMap && typeof currentMap.name === "string" && currentMap.name.trim()
      ? currentMap.name
      : "Untitled Scenario";
  }
  if (foodConsumptionEnabledInput) {
    foodConsumptionEnabledInput.checked = !currentMap
      || !currentMap.game
      || currentMap.game.foodConsumption !== false;
  }
  const scenarioFactions = currentMap && Array.isArray(currentMap.factions)
    ? normalizeScenarioFactions(currentMap.factions, currentMap.units)
    : defaultScenarioFactions();
  for (const row of scenarioFactionRows) {
    const key = row.dataset.factionKey || "mongol";
    const faction = scenarioFactions.find((candidate) => candidate.key === key) || normalizeScenarioFaction(key, key);
    const enabled = row.querySelector(".scenario-faction-enabled");
    const ai = row.querySelector(".scenario-faction-ai");
    if (enabled) {
      enabled.checked = Boolean(faction.enabled);
    }
    if (ai) {
      ai.checked = Boolean(faction.ai);
      ai.disabled = !Boolean(faction.enabled);
    }
  }
}

function optionLabelForFaction(faction) {
  return faction.name || factions[faction.key]?.name || faction.key || `Owner ${faction.id}`;
}

function appendFactionOptions(select, selectedOwner) {
  select.replaceChildren();
  for (const faction of currentMap && Array.isArray(currentMap.factions) ? currentMap.factions : defaultScenarioFactions()) {
    if (faction.id === factions.neutral.owner) {
      continue;
    }
    const option = document.createElement("option");
    option.value = String(faction.id);
    option.textContent = optionLabelForFaction(faction);
    select.appendChild(option);
  }
  select.value = String(selectedOwner);
}

function ownershipFactionChoices() {
  return [
    { ...factions.neutral, id: factions.neutral.owner, key: "neutral" },
    ...(currentMap && Array.isArray(currentMap.factions) ? currentMap.factions : defaultScenarioFactions()),
  ];
}

function syncOwnershipEditorControls() {
  if (!ownershipFactionSelect) {
    return;
  }
  for (const button of ownershipToolButtons) {
    const active = button.dataset.ownershipTool === ownershipTool;
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-pressed", String(active));
  }
  const prior = Number.isInteger(selectedOwnershipOwner) ? selectedOwnershipOwner : factions.neutral.owner;
  ownershipFactionSelect.replaceChildren();
  for (const faction of ownershipFactionChoices()) {
    const option = document.createElement("option");
    option.value = String(faction.id);
    option.textContent = faction.id === factions.neutral.owner ? "None" : optionLabelForFaction(faction);
    ownershipFactionSelect.appendChild(option);
  }
  const validOwners = new Set(Array.from(ownershipFactionSelect.options).map((option) => Number.parseInt(option.value, 10)));
  selectedOwnershipOwner = validOwners.has(prior) ? prior : factions.neutral.owner;
  ownershipFactionSelect.value = String(selectedOwnershipOwner);
  if (ownershipFactionSwatch) {
    ownershipFactionSwatch.style.borderColor = territoryOutlineColor(selectedOwnershipOwner);
    ownershipFactionSwatch.style.background = selectedOwnershipOwner === factions.neutral.owner
      ? "transparent"
      : territoryOutlineColor(selectedOwnershipOwner);
  }
  const ownershipField = ownershipFactionSelect.closest(".ownership-field");
  if (ownershipField) {
    ownershipField.hidden = ownershipTool !== "owner";
  }
}

function appendDirectiveOptions(select, selectedType) {
  select.replaceChildren();
  for (const directive of aiDirectiveTypes) {
    const option = document.createElement("option");
    option.value = directive.key;
    option.textContent = directive.label;
    select.appendChild(option);
  }
  select.value = selectedType;
}

function mutateAiGroup(groupId, mutator, options = {}) {
  if (!currentMap) {
    return;
  }
  const groups = aiGroups();
  const group = groups.find((candidate) => candidate.id === groupId);
  if (!group) {
    return;
  }
  recordLocalUndo();
  mutator(group);
  currentMap.game.aiGroups = normalizeAiGroups(groups);
  if (options.resync !== false) {
    syncEditorControls();
  }
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
  if (options.syncEngine !== false) {
    void syncAiGroupsToEngine();
  }
}

function removeAiGroup(groupId) {
  if (!currentMap) {
    return;
  }
  const groups = aiGroups();
  const nextGroups = groups.filter((group) => group.id !== groupId);
  if (nextGroups.length === groups.length) {
    return;
  }
  recordLocalUndo();
  currentMap.game.aiGroups = normalizeAiGroups(nextGroups);
  collapsedAiGroupIds.delete(groupId);
  if (activeAiGroupId === groupId) {
    activeAiGroupId = 0;
    aiPickMode = null;
  }
  syncEditorControls();
  requestAnimationFrame(drawMap);
  void syncAiGroupsToEngine();
}

function syncFactionRelationshipControls() {
  if (!factionRelationshipsContainer) {
    return;
  }
  factionRelationshipsContainer.replaceChildren();
  if (!currentMap) {
    return;
  }
  const diplomacyFactions = enabledScenarioFactions()
    .filter((faction) => faction.id !== factions.neutral.owner);
  if (diplomacyFactions.length < 2) {
    return;
  }

  const title = document.createElement("div");
  title.className = "faction-relationships-title";
  title.textContent = "Relationships";

  const table = document.createElement("table");
  table.className = "faction-relationship-table";
  const thead = document.createElement("thead");
  const headRow = document.createElement("tr");
  const corner = document.createElement("th");
  corner.scope = "col";
  corner.textContent = "Toward";
  headRow.appendChild(corner);
  for (const targetFaction of diplomacyFactions) {
    const th = document.createElement("th");
    th.scope = "col";
    th.textContent = optionLabelForFaction(targetFaction);
    headRow.appendChild(th);
  }
  thead.appendChild(headRow);

  const tbody = document.createElement("tbody");
  for (const ownerFaction of diplomacyFactions) {
    const row = document.createElement("tr");
    const rowHeader = document.createElement("th");
    rowHeader.scope = "row";
    rowHeader.textContent = optionLabelForFaction(ownerFaction);
    row.appendChild(rowHeader);
    for (const targetFaction of diplomacyFactions) {
      const cell = document.createElement("td");
      if (ownerFaction.id === targetFaction.id) {
        cell.textContent = "-";
      } else {
        const dispositionInput = document.createElement("input");
        dispositionInput.type = "number";
        dispositionInput.min = "0";
        dispositionInput.max = "100";
        dispositionInput.step = "1";
        dispositionInput.className = "faction-disposition-input";
        dispositionInput.value = String(diplomacyDisposition(ownerFaction.id, targetFaction.id));
        dispositionInput.setAttribute("aria-label", `${optionLabelForFaction(ownerFaction)} disposition toward ${optionLabelForFaction(targetFaction)}`);
        dispositionInput.addEventListener("change", () => {
          updateDiplomacyDisposition(ownerFaction.id, targetFaction.id, dispositionInput.value);
        });

        const cellStack = document.createElement("div");
        cellStack.className = "faction-relationship-cell";
        cellStack.append(dispositionInput);
        cell.appendChild(cellStack);
      }
      row.appendChild(cell);
    }
    tbody.appendChild(row);
  }
  table.append(thead, tbody);
  factionRelationshipsContainer.append(title, table);
}

function syncAiEditorControls() {
  if (!aiGroupsContainer) {
    return;
  }
  aiGroupsContainer.replaceChildren();
  if (!currentMap) {
    const empty = document.createElement("p");
    empty.className = "ai-empty-state";
    empty.textContent = "No scenario loaded.";
    aiGroupsContainer.appendChild(empty);
    return;
  }
  const groups = aiGroups().filter((group) => !group.generated);
  if (groups.length === 0) {
    const empty = document.createElement("p");
    empty.className = "ai-empty-state";
    empty.textContent = "No scenario AI groups.";
    aiGroupsContainer.appendChild(empty);
    return;
  }

  for (const group of groups) {
    const card = document.createElement("article");
    card.className = "ai-group-card";
    card.classList.toggle("is-picking", activeAiGroupId === group.id && aiPickMode !== null);

    const deleteButton = document.createElement("button");
    deleteButton.type = "button";
    deleteButton.className = "ai-group-delete";
    deleteButton.setAttribute("aria-label", `Delete ${group.name}`);
    deleteButton.title = `Delete ${group.name}`;
    deleteButton.textContent = "-";
    deleteButton.addEventListener("click", () => removeAiGroup(group.id));
    const groupRow = document.createElement("div");
    groupRow.className = "ai-group-edit-row";

    const nameLabel = document.createElement("label");
    nameLabel.className = "ai-field";
    nameLabel.textContent = "Name";
    const nameInput = document.createElement("input");
    nameInput.type = "text";
    nameInput.className = "ai-group-name";
    nameInput.value = group.name;
    nameInput.addEventListener("change", () => {
      mutateAiGroup(group.id, (draft) => {
        draft.name = nameInput.value.trim() || draft.name;
      });
    });
    nameLabel.appendChild(nameInput);

    const ownerLabel = document.createElement("label");
    ownerLabel.className = "ai-field";
    ownerLabel.textContent = "Faction";
    const ownerSelect = document.createElement("select");
    ownerSelect.className = "ai-group-owner";
    appendFactionOptions(ownerSelect, group.owner);
    ownerSelect.addEventListener("change", () => {
      const nextOwner = Number.parseInt(ownerSelect.value, 10);
      mutateAiGroup(group.id, (draft) => {
        draft.owner = nextOwner;
        draft.unitIds = draft.unitIds.filter((unitId) => {
          const unit = currentMap.units.find((candidate) => candidate.id === unitId);
          return unit && unit.owner === nextOwner;
        });
      });
    });
    ownerLabel.appendChild(ownerSelect);

    const roleLabel = document.createElement("label");
    roleLabel.className = "ai-field";
    roleLabel.textContent = "Role";
    const roleSelect = document.createElement("select");
    roleSelect.className = "ai-group-role";
    appendAiGroupRoleOptions(roleSelect, group.role);
    roleSelect.addEventListener("change", () => {
      mutateAiGroup(group.id, (draft) => {
        draft.role = roleSelect.value;
      });
    });
    roleLabel.appendChild(roleSelect);

    const membersButton = document.createElement("button");
    membersButton.type = "button";
    membersButton.className = "ai-members-button";
    membersButton.textContent = "Members";
    membersButton.addEventListener("click", () => setAiPickMode(group.id, "members"));

    const memberCount = document.createElement("span");
    memberCount.className = "ai-member-count";
    memberCount.textContent = `${group.unitIds.length} units`;

    const directiveLabel = document.createElement("label");
    directiveLabel.className = "ai-field";
    directiveLabel.textContent = "Directive";
    const directiveSelect = document.createElement("select");
    directiveSelect.className = "ai-directive-type";
    appendDirectiveOptions(directiveSelect, group.directive.type);
    directiveSelect.addEventListener("change", () => {
      mutateAiGroup(group.id, (draft) => {
        draft.directive.type = directiveSelect.value;
      });
    });
    directiveLabel.appendChild(directiveSelect);

    const targetButton = document.createElement("button");
    targetButton.type = "button";
    targetButton.className = "ai-target-button";
    targetButton.textContent = "Target Hex";
    targetButton.addEventListener("click", () => setAiPickMode(group.id, "target"));

    const targetFactionLabel = document.createElement("label");
    targetFactionLabel.className = "ai-field";
    targetFactionLabel.textContent = "Target Faction";
    const targetFactionSelect = document.createElement("select");
    targetFactionSelect.className = "ai-target-faction";
    appendFactionOptions(targetFactionSelect, group.directive.owner);
    targetFactionSelect.addEventListener("change", () => {
      const targetOwner = Number.parseInt(targetFactionSelect.value, 10);
      mutateAiGroup(group.id, (draft) => {
        draft.directive.owner = targetOwner;
        draft.directive.faction = factionKeyForOwner(targetOwner);
      });
    });
    targetFactionLabel.appendChild(targetFactionSelect);

    const targetReadout = document.createElement("span");
    targetReadout.className = "ai-target-readout";
    targetReadout.textContent = `${group.directive.target.q},${group.directive.target.r}`;

    groupRow.append(
      nameLabel,
      ownerLabel,
      roleLabel,
      membersButton,
      memberCount,
      directiveLabel,
      targetButton,
      targetFactionLabel,
      targetReadout,
      deleteButton
    );
    card.append(groupRow);
    aiGroupsContainer.appendChild(card);
  }
}

function aiDirectiveLabel(type) {
  return aiDirectiveTypes.find((directive) => directive.key === type)?.label || titleCaseToken(type);
}

const aiGroupRoles = [
  { key: "manual_directive", label: "Manual Directive" },
  { key: "garrison", label: "Garrison" },
  { key: "field_army", label: "Field Army" },
  { key: "raiding_force", label: "Raiding Force" },
  { key: "reserve", label: "Reserve" },
];

function aiGroupRoleLabel(role) {
  return aiGroupRoles.find((candidate) => candidate.key === role)?.label || titleCaseToken(role);
}

function appendAiGroupRoleOptions(select, selectedRole = "manual_directive") {
  select.replaceChildren();
  for (const role of aiGroupRoles) {
    const option = document.createElement("option");
    option.value = role.key;
    option.textContent = role.label;
    option.selected = role.key === selectedRole;
    select.appendChild(option);
  }
}

function strategicAiGroups() {
  return aiGroups().filter((group) => group.unitIds.length > 0);
}

function activeStrategicAiGroup() {
  const groups = aiGroups();
  if (groups.length === 0 || activeStrategicAiGroupId === 0) {
    activeStrategicAiGroupId = 0;
    return null;
  }
  const group = groups.find((candidate) => candidate.id === activeStrategicAiGroupId);
  if (!group) {
    activeStrategicAiGroupId = 0;
    return null;
  }
  return group;
}

function selectStrategicAiGroup(groupId) {
  activeStrategicAiGroupId = groupId;
  syncStrategicAiDashboard();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function clearStrategicAiSelection() {
  activeStrategicAiGroupId = 0;
  syncStrategicAiDashboard();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function syncStrategicAiDashboard() {
  if (!strategicAiGroupsContainer) {
    return;
  }
  if (strategicAiSelectNoneButton) {
    strategicAiSelectNoneButton.disabled = activeStrategicAiGroupId === 0;
  }
  strategicAiGroupsContainer.replaceChildren();
  if (appMode !== "play" || !currentMap) {
    if (strategicAiPanelSummary) {
      strategicAiPanelSummary.textContent = "No AI groups";
    }
    return;
  }

  const groups = aiGroups();
  if (strategicAiPanelSummary) {
    const groupText = groups.length === 1 ? "1 AI group" : `${groups.length} AI groups`;
    const unitCount = groups.reduce((sum, group) => sum + group.unitIds.length, 0);
    strategicAiPanelSummary.textContent = groups.length > 0
      ? `${groupText} / ${unitCount} units`
      : "No AI groups";
  }
  if (groups.length === 0) {
    activeStrategicAiGroupId = 0;
    const empty = document.createElement("p");
    empty.className = "ai-empty-state";
    empty.textContent = "No AI groups.";
    strategicAiGroupsContainer.appendChild(empty);
    return;
  }

  activeStrategicAiGroup();
  for (const group of groups) {
    const collapsed = false;
    const card = document.createElement("article");
    card.className = "ai-group-card is-dashboard-row";
    card.classList.toggle("is-active", activeStrategicAiGroupId === group.id);
    card.tabIndex = 0;
    card.setAttribute("role", "button");
    card.setAttribute("aria-pressed", String(activeStrategicAiGroupId === group.id));
    card.addEventListener("click", () => selectStrategicAiGroup(group.id));
    card.addEventListener("keydown", (event) => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        selectStrategicAiGroup(group.id);
      }
    });

    const collapseButton = document.createElement("button");
    collapseButton.type = "button";
    collapseButton.className = "ai-group-collapse";
    collapseButton.setAttribute("aria-expanded", String(!collapsed));
    collapseButton.setAttribute("aria-label", `${collapsed ? "Expand" : "Collapse"} ${group.name}`);
    collapseButton.textContent = collapsed ? "▸" : "▾";
    collapseButton.addEventListener("click", (event) => {
      event.stopPropagation();
      if (collapsedStrategicAiGroupIds.has(group.id)) {
        collapsedStrategicAiGroupIds.delete(group.id);
      } else {
        collapsedStrategicAiGroupIds.add(group.id);
      }
      syncStrategicAiDashboard();
    });

    const summary = document.createElement("span");
    summary.className = "ai-group-summary";
    summary.textContent = `${group.name} / ${aiGroupRoleLabel(group.role)} / ${optionLabelForFaction(factionForOwner(group.owner))} / ${group.unitIds.length} units / ${aiDirectiveLabel(group.directive.type)}`;

    const groupRow = document.createElement("div");
    groupRow.className = "ai-group-edit-row";

    const statusField = document.createElement("div");
    statusField.className = "ai-field";
    statusField.textContent = "AI Status";
    const statusBadge = document.createElement("span");
    statusBadge.className = "ai-status-badge";
    statusBadge.textContent = group.generated ? "Auto AI" : "Manual AI";
    statusField.appendChild(statusBadge);

    const nameField = document.createElement("div");
    nameField.className = "ai-field";
    nameField.textContent = "Name";
    const nameValue = document.createElement("span");
    nameValue.className = "ai-target-readout";
    nameValue.textContent = group.name;
    nameField.appendChild(nameValue);

    const ownerField = document.createElement("div");
    ownerField.className = "ai-field";
    ownerField.textContent = "Faction";
    const ownerValue = document.createElement("span");
    ownerValue.className = "ai-target-readout";
    ownerValue.textContent = optionLabelForFaction(factionForOwner(group.owner));
    ownerField.appendChild(ownerValue);

    const roleField = document.createElement("div");
    roleField.className = "ai-field";
    roleField.textContent = "Role";
    const roleValue = document.createElement("span");
    roleValue.className = "ai-target-readout";
    roleValue.textContent = aiGroupRoleLabel(group.role);
    roleField.appendChild(roleValue);

    const membersField = document.createElement("div");
    membersField.className = "ai-field ai-members-field";
    membersField.textContent = "Members";
    const membersLabel = document.createElement("span");
    membersLabel.className = "ai-member-count ai-member-pill";
    membersLabel.textContent = `${group.unitIds.length} units`;
    membersField.appendChild(membersLabel);

    const directiveField = document.createElement("div");
    directiveField.className = "ai-field";
    directiveField.textContent = "Directive";
    const directiveValue = document.createElement("span");
    directiveValue.className = "ai-target-readout";
    directiveValue.textContent = aiDirectiveLabel(group.directive.type);
    directiveField.appendChild(directiveValue);

    const targetHex = strategicAiTargetHex(group);
    const targetField = document.createElement("div");
    targetField.className = "ai-field ai-target-field";
    targetField.textContent = "Target Hex";
    const targetButton = document.createElement("button");
    targetButton.type = "button";
    targetButton.className = "ai-target-jump-button";
    targetButton.textContent = targetHex ? `${targetHex.q},${targetHex.r}` : "-";
    targetButton.disabled = !targetHex;
    targetButton.addEventListener("click", (event) => {
      event.stopPropagation();
      selectStrategicAiGroup(group.id);
      if (!targetHex) {
        return;
      }
      playSurfaceMode = "map";
      syncModeControls();
      centerViewportOnWorldPoint(hexCenter(targetHex));
    });
    targetField.appendChild(targetButton);

    const targetFactionField = document.createElement("div");
    targetFactionField.className = "ai-field";
    targetFactionField.textContent = "Target Faction";
    const targetFactionValue = document.createElement("span");
    targetFactionValue.className = "ai-target-readout";
    targetFactionValue.textContent = optionLabelForFaction(factionForOwner(group.directive.owner));
    targetFactionField.appendChild(targetFactionValue);

    groupRow.append(
      statusField,
      nameField,
      ownerField,
      roleField,
      membersField,
      directiveField,
      targetField,
      targetFactionField
    );
    card.append(groupRow);
    strategicAiGroupsContainer.appendChild(card);
  }
}

function syncDiplomacyScreen() {
  if (!diplomacyTable) {
    return;
  }
  diplomacyTable.replaceChildren();
  if (!currentMap) {
    return;
  }

  const factionsForDiplomacy = enabledScenarioFactions()
    .filter((faction) => faction.id !== factions.neutral.owner);
  const thead = document.createElement("thead");
  const headRow = document.createElement("tr");
  const corner = document.createElement("th");
  corner.textContent = "Views";
  headRow.appendChild(corner);
  for (const targetFaction of factionsForDiplomacy) {
    const th = document.createElement("th");
    th.scope = "col";
    th.textContent = optionLabelForFaction(targetFaction);
    headRow.appendChild(th);
  }
  thead.appendChild(headRow);

  const tbody = document.createElement("tbody");
  for (const ownerFaction of factionsForDiplomacy) {
    const row = document.createElement("tr");
    const rowHeader = document.createElement("th");
    rowHeader.scope = "row";
    rowHeader.textContent = optionLabelForFaction(ownerFaction);
    row.appendChild(rowHeader);
    for (const targetFaction of factionsForDiplomacy) {
      const cell = document.createElement("td");
      if (ownerFaction.id === targetFaction.id) {
        cell.textContent = "-";
      } else {
        const disposition = diplomacyDisposition(ownerFaction.id, targetFaction.id);
        cell.textContent = `${dispositionLabel(disposition)} / ${disposition}`;
        cell.className = `is-${dispositionBand(disposition)}`;
      }
      row.appendChild(cell);
    }
    tbody.appendChild(row);
  }

  diplomacyTable.append(thead, tbody);
}

function syncScenarioToolControls() {
  for (const region of scenarioRegions) {
    const key = region.dataset.scenarioRegion;
    const open = openScenarioRegions.has(key);
    region.classList.toggle("is-active", key === scenarioTool);
    region.classList.toggle("is-collapsed", !open);
  }
  for (const button of scenarioToolButtons) {
    const active = button.dataset.scenarioActivate === scenarioTool;
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-pressed", String(active));
  }
  for (const button of scenarioCollapseButtons) {
    const key = button.dataset.scenarioToggle;
    const open = openScenarioRegions.has(key);
    button.setAttribute("aria-expanded", String(open));
    button.textContent = open ? "-" : "+";
    button.setAttribute("aria-label", `${open ? "Collapse" : "Expand"} ${key}`);
  }
  for (const panel of scenarioToolPanels) {
    panel.hidden = !openScenarioRegions.has(panel.dataset.scenarioPanel);
  }
  for (const button of unitToolButtons) {
    const active = button.dataset.unitTool === unitTool;
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-pressed", String(active));
  }
  const placingUnit = openScenarioRegions.has("units") && unitTool === "place";
  editorUnitSideSelect.hidden = !placingUnit;
  editorUnitTypeSelect.hidden = !placingUnit;
  const terrainOpen = openScenarioRegions.has("terrain");
  const terrainMode = editorEdgeFeatureSelect.value === "terrain";
  editorEdgeFeatureSelect.hidden = true;
  terrainPalette.hidden = !terrainOpen || !terrainMode;
  edgeFeatureChoices.hidden = !terrainOpen || terrainMode;
  for (const button of terrainEditModeButtons) {
    const active = button.dataset.terrainEditMode === (terrainMode ? "terrain" : "edge");
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-pressed", String(active));
  }
  for (const button of edgeFeatureButtons) {
    const active = button.dataset.edgeFeature === editorEdgeFeatureSelect.value;
    button.classList.toggle("is-active", active);
    button.setAttribute("aria-pressed", String(active));
  }
  mapPanel.classList.toggle("is-editing", scenarioEditingActive());
  for (const button of terrainPalette.querySelectorAll(".terrain-button")) {
    button.classList.toggle("is-selected", button.dataset.terrain === selectedTerrain);
  }
  syncOwnershipEditorControls();
  syncAiEditorControls();
}

function setScenarioTool(tool) {
  scenarioTool = ["session", "factions", "terrain", "ownership", "units", "ai"].includes(tool) ? tool : "session";
  openScenarioRegions.add(scenarioTool);
  if (scenarioTool !== "ai") {
    aiPickMode = null;
  }
  isPainting = false;
  paintStrokeKeys = new Set();
  paintUndoRecorded = false;
  syncEditorControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function toggleScenarioRegion(region) {
  if (!["session", "factions", "terrain", "ownership", "units", "ai"].includes(region)) {
    return;
  }
  if (openScenarioRegions.has(region)) {
    openScenarioRegions.delete(region);
  } else {
    openScenarioRegions.add(region);
  }
  syncEditorControls();
}

function setTerrainEditMode(mode) {
  editorEdgeFeatureSelect.value = mode === "edge" ? "river" : "terrain";
  setScenarioTool("terrain");
}

function setEditorEdgeFeature(feature) {
  editorEdgeFeatureSelect.value = ["river", "road", "bridge", "wall", "wall_pass"].includes(feature) ? feature : "river";
  setScenarioTool("terrain");
}

function setOwnershipTool(tool) {
  ownershipTool = tool === "source" ? "source" : "owner";
  setScenarioTool("ownership");
  syncOwnershipEditorControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function setUnitTool(tool) {
  unitTool = tool === "edit" ? "edit" : "place";
  scenarioTool = "units";
  openScenarioRegions.add("units");
  aiPickMode = null;
  isPainting = false;
  paintStrokeKeys = new Set();
  paintUndoRecorded = false;
  if (unitTool === "place") {
    editorSelectedUnitId = 0;
    if (currentMap && currentMap.game) {
      currentMap.game.selectedUnitId = 0;
    }
  }
  syncEditorControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function setAiPickMode(groupId, mode) {
  if (!currentMap) {
    return;
  }
  const group = aiGroups().find((candidate) => candidate.id === groupId);
  if (!group) {
    return;
  }
  scenarioTool = "ai";
  openScenarioRegions.add("ai");
  activeAiGroupId = group.id;
  aiPickMode = mode === "target" ? "target" : "members";
  isPainting = false;
  paintStrokeKeys = new Set();
  paintUndoRecorded = false;
  syncEditorControls();
  requestAnimationFrame(drawMap);
}

function addAiGroup() {
  if (!currentMap) {
    return;
  }
  ensureGameMeta();
  recordLocalUndo();
  const id = nextAiGroupId();
  const owner = defaultAiOwner();
  currentMap.game.aiGroups.push(normalizeAiGroup({
    id,
    owner,
    name: `AI Group ${id}`,
    role: "manual_directive",
    unitIds: [],
    directive: {
      type: "hunt",
      target: normalizeAiTarget(null),
      owner: factions.mongol.owner,
      faction: "mongol",
    },
  }, currentMap.game.aiGroups.length));
  activeAiGroupId = id;
  aiPickMode = null;
  scenarioTool = "ai";
  openScenarioRegions.add("ai");
  syncModeControls();
  drawMap();
  void syncAiGroupsToEngine();
}

function updateScenarioNameFromInput() {
  if (!currentMap) {
    return;
  }
  currentMap.name = scenarioName();
}

function updateScenarioFactionsFromControls() {
  if (!currentMap) {
    return;
  }
  const enabled = scenarioFactionRows.filter((row) => row.querySelector(".scenario-faction-enabled")?.checked);
  if (enabled.length === 0 && scenarioFactionRows.length > 0) {
    const firstEnabled = scenarioFactionRows[0].querySelector(".scenario-faction-enabled");
    if (firstEnabled) {
      firstEnabled.checked = true;
    }
  }
  const selectedFactions = selectedScenarioFactionsFromControls();
  currentMap.factions = selectedFactions;
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.factions = selectedFactions.map((faction) => ({
    id: faction.id,
    key: faction.key,
    name: faction.name,
    color: faction.color,
    metal: faction.metal,
    treasure: faction.treasure,
    food: faction.food,
    enabled: faction.enabled,
    ai: faction.ai,
  }));
  currentMap.game.turnOrder = selectedFactions
    .filter((faction) => faction.enabled)
    .map((faction) => faction.id);
  ensureGameMeta();
  syncModeControls();
  drawMap();
  void syncScenarioEditorToEngine();
}

function normalizeScenarioFaction(faction, fallbackKey = "mongol") {
    const key = typeof faction === "string"
        ? faction
    : (faction && typeof faction.key === "string" ? faction.key : fallbackKey);
  const defaults = factions[key] || factions[fallbackKey] || factions.mongol;
  return {
    id: Number.isInteger(faction && faction.id) ? faction.id : defaults.owner,
    key,
    name: typeof (faction && faction.name) === "string" ? faction.name : defaults.name,
    color: typeof (faction && faction.color) === "string" ? faction.color : defaults.color,
    metal: Number.isFinite(faction && faction.metal) ? Math.max(0, Math.trunc(faction.metal)) : defaults.metal,
    treasure: Number.isFinite(faction && faction.treasure) ? Math.max(0, Math.trunc(faction.treasure)) : defaults.treasure,
    food: Number.isFinite(faction && faction.food) ? Math.max(0, Math.trunc(faction.food)) : defaults.food,
    enabled: faction && faction.enabled !== undefined ? Boolean(faction.enabled) : Boolean(defaults.enabled),
    ai: faction && faction.ai !== undefined ? Boolean(faction.ai) : Boolean(defaults.ai),
  };
}

function defaultScenarioFactions() {
  return defaultFactionKeys.map((key) => normalizeScenarioFaction({
    key,
    enabled: factionTurnOrder.includes(key),
    ai: false,
  }, key));
}

function normalizeScenarioFactions(rawFactions, units = []) {
  const normalized = [];
  const seenOwners = new Set();
  const addFaction = (entry, fallbackKey = "mongol") => {
    const faction = normalizeScenarioFaction(entry, fallbackKey);
    if (faction.id === factions.neutral.owner || seenOwners.has(faction.id)) {
      return;
    }
    seenOwners.add(faction.id);
    normalized.push(faction);
  };

  if (Array.isArray(rawFactions)) {
    rawFactions.forEach((entry) => addFaction(entry));
  }
  if (normalized.length === 0 && Array.isArray(units)) {
    for (const unit of units) {
      if (!unit || unit.owner === factions.neutral.owner || seenOwners.has(unit.owner)) {
        continue;
      }
      addFaction(unit.faction || Object.keys(factions).find((key) => factions[key].owner === unit.owner) || "mongol");
    }
  }
  if (normalized.length === 0) {
    return defaultScenarioFactions();
  }
  for (const key of defaultFactionKeys) {
    const defaults = normalizeScenarioFaction({ key, enabled: false, ai: false }, key);
    if (!seenOwners.has(defaults.id)) {
      normalized.push(defaults);
    }
  }
  return normalized;
}

const aiDirectiveTypes = [
  { key: "inactive", label: "Inactive" },
  { key: "hold_hex", label: "Hold Hex" },
  { key: "hunt", label: "Hunt" },
  { key: "defend_hex", label: "Defend Hex" },
  { key: "hunt_horde", label: "Hunt Horde" },
  { key: "capture_hex", label: "Capture Hex" },
];

function factionKeyForOwner(owner) {
  const scenarioFaction = currentMap && Array.isArray(currentMap.factions)
    ? currentMap.factions.find((candidate) => candidate.id === owner)
    : null;
  if (scenarioFaction) {
    return scenarioFaction.key;
  }
  const fallback = Object.entries(factions).find(([, faction]) => faction.owner === owner);
  return fallback ? fallback[0] : "mongol";
}

function ownerForFactionKey(key) {
  const scenarioFaction = currentMap && Array.isArray(currentMap.factions)
    ? currentMap.factions.find((candidate) => candidate.key === key)
    : null;
  if (scenarioFaction) {
    return scenarioFaction.id;
  }
  return factions[key]?.owner ?? factions.mongol.owner;
}

function enabledScenarioFactions() {
  if (!currentMap || !Array.isArray(currentMap.factions)) {
    return defaultScenarioFactions().filter((faction) => faction.enabled);
  }
  const enabled = currentMap.factions.filter((faction) => faction.enabled);
  return enabled.length > 0 ? enabled : currentMap.factions.slice(0, 1);
}

function defaultAiOwner() {
  const aiFaction = enabledScenarioFactions().find((faction) => faction.ai);
  if (aiFaction) {
    return aiFaction.id;
  }
  const chinese = enabledScenarioFactions().find((faction) => faction.key === "chinese");
  return chinese ? chinese.id : enabledScenarioFactions()[0]?.id ?? factions.mongol.owner;
}

function normalizeAiTarget(target) {
  const width = currentMap && Number.isInteger(currentMap.width) ? currentMap.width : 1;
  const height = currentMap && Number.isInteger(currentMap.height) ? currentMap.height : 1;
  const fallback = { q: Math.floor(width / 2), r: Math.floor(height / 2) };
  return {
    q: Number.isInteger(target && target.q) ? target.q : fallback.q,
    r: Number.isInteger(target && target.r) ? target.r : fallback.r,
  };
}

function normalizeAiDirective(directive) {
  const type = aiDirectiveTypes.some((candidate) => candidate.key === (directive && directive.type))
    ? directive.type
    : (aiDirectiveTypes.some((candidate) => candidate.key === (directive && directive.kind)) ? directive.kind : "hunt");
  const owner = Number.isInteger(directive && directive.owner)
    ? directive.owner
    : (Number.isInteger(directive && directive.targetOwner) ? directive.targetOwner : ownerForFactionKey(directive && directive.faction));
  const faction = typeof (directive && directive.faction) === "string"
    ? directive.faction
    : factionKeyForOwner(owner);
  return {
    type,
    target: normalizeAiTarget(directive && directive.target),
    faction,
    owner: ownerForFactionKey(faction),
  };
}

function normalizeAiGroup(group, index = 0, usedIds = new Set()) {
  let id = Number.isInteger(group && group.id) ? group.id : index + 1;
  while (usedIds.has(id)) {
    id += 1;
  }
  usedIds.add(id);
  const owner = Number.isInteger(group && group.owner) ? group.owner : defaultAiOwner();
  const unitIds = Array.isArray(group && group.unitIds)
    ? Array.from(new Set(group.unitIds.filter(Number.isInteger)))
    : (Array.isArray(group && group.unit_ids) ? Array.from(new Set(group.unit_ids.filter(Number.isInteger))) : []);
  return {
    id,
    owner,
    name: typeof (group && group.name) === "string" && group.name.trim() ? group.name.trim() : `AI Group ${index + 1}`,
    role: aiGroupRoles.some((candidate) => candidate.key === (group && group.role)) ? group.role : "manual_directive",
    generated: group && group.generated !== undefined ? Boolean(group.generated) : false,
    unitIds,
    directive: normalizeAiDirective(group && group.directive),
  };
}

function normalizeAiGroups(rawGroups) {
  const usedIds = new Set();
  return Array.isArray(rawGroups)
    ? rawGroups.map((group, index) => normalizeAiGroup(group, index, usedIds))
    : [];
}

function aiGroups() {
  if (!currentMap) {
    return [];
  }
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.aiGroups = normalizeAiGroups(currentMap.game.aiGroups);
  return currentMap.game.aiGroups;
}

function diplomacyOwnerKey(owner, target) {
  return `${owner}->${target}`;
}

function clampDisposition(value, fallback = 25) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? Math.max(0, Math.min(100, Math.trunc(parsed))) : fallback;
}

function legacyDisposition(relationship) {
  if (Number.isFinite(relationship.disposition)) {
    return clampDisposition(relationship.disposition);
  }
  let disposition = clampDisposition(relationship.affinity, 50);
  if (relationship.status === "peace") {
    disposition = Math.max(disposition, 50);
  } else {
    disposition = Math.min(disposition, 25);
  }
  switch (relationship.aiPosture) {
    case "raiding":
      return Math.min(disposition, 10);
    case "aggressive":
      return Math.min(disposition, 20);
    case "defensive":
      return Math.min(disposition, 35);
    case "passive":
      return Math.max(disposition, 60);
    default:
      return disposition;
  }
}

function dispositionBand(disposition) {
  const value = clampDisposition(disposition);
  if (value <= 15) return "hostile";
  if (value <= 35) return "threat";
  if (value < 65) return "neutral";
  return "friendly";
}

function dispositionLabel(disposition) {
  const value = clampDisposition(disposition);
  if (value <= 15) return "Hostile";
  if (value <= 35) return "Unfriendly";
  if (value < 65) return "Neutral";
  return "Friendly";
}

function normalizeDiplomacy(rawDiplomacy) {
  const factionsForDiplomacy = enabledScenarioFactions()
    .filter((faction) => faction.id !== factions.neutral.owner);
  const prior = new Map();
  if (Array.isArray(rawDiplomacy)) {
    for (const relationship of rawDiplomacy) {
      if (!relationship || typeof relationship !== "object") {
        continue;
      }
      const owner = Number.isInteger(relationship.owner)
        ? relationship.owner
        : ownerForFactionKey(relationship.faction);
      const target = Number.isInteger(relationship.target)
        ? relationship.target
        : ownerForFactionKey(relationship.targetFaction);
      if (owner === target || owner === factions.neutral.owner || target === factions.neutral.owner) {
        continue;
      }
      const key = diplomacyOwnerKey(owner, target);
      prior.set(key, legacyDisposition(relationship));
    }
  }

  const normalized = [];
  for (const ownerFaction of factionsForDiplomacy) {
    for (const targetFaction of factionsForDiplomacy) {
      if (ownerFaction.id === targetFaction.id) {
        continue;
      }
      const key = diplomacyOwnerKey(ownerFaction.id, targetFaction.id);
      normalized.push({
        owner: ownerFaction.id,
        faction: ownerFaction.key,
        target: targetFaction.id,
        targetFaction: targetFaction.key,
        disposition: prior.has(key) ? prior.get(key) : 25,
      });
    }
  }
  return normalized;
}

function diplomacyDisposition(owner, target) {
  const relationship = currentMap
    && currentMap.game
    && Array.isArray(currentMap.game.diplomacy)
    ? currentMap.game.diplomacy.find((candidate) => candidate.owner === owner && candidate.target === target)
    : null;
  return relationship ? clampDisposition(relationship.disposition) : 25;
}

function updateDiplomacyDisposition(owner, target, value) {
  if (!currentMap || owner === target) {
    return;
  }
  ensureGameMeta();
  const disposition = clampDisposition(value);
  currentMap.game.diplomacy = normalizeDiplomacy(currentMap.game.diplomacy);
  const relationship = currentMap.game.diplomacy.find((candidate) => candidate.owner === owner && candidate.target === target);
  if (!relationship) {
    return;
  }
  recordLocalUndo();
  relationship.disposition = disposition;
  currentMap.game.diplomacy = normalizeDiplomacy(currentMap.game.diplomacy);
  syncFactionRelationshipControls();
  syncDiplomacyScreen();
  void syncScenarioEditorToEngine();
}

function activeAiGroup() {
  return aiGroups().find((group) => group.id === activeAiGroupId) || null;
}

function nextAiGroupId() {
  const ids = aiGroups().filter((group) => !group.generated && group.id > 0).map((group) => group.id);
  return ids.length > 0 ? Math.max(...ids) + 1 : 1;
}

function factionForOwner(owner) {
  const scenarioFaction = currentMap && Array.isArray(currentMap.factions)
    ? currentMap.factions.find((candidate) => candidate.id === owner)
    : null;
  if (scenarioFaction) {
    return scenarioFaction;
  }
  return Object.values(factions).find((faction) => faction.owner === owner) || factions.neutral;
}

function factionArchetypeForOwner(owner) {
  const builtIn = Object.values(factions).find((faction) => faction.owner === owner);
  return builtIn ? builtIn.archetype : factions.neutral.archetype;
}

function activeFactionKey() {
  const faction = activeFaction();
  return faction.key || "neutral";
}

function activeOwner() {
  if (currentMap && currentMap.game && Number.isInteger(currentMap.game.activeOwner)) {
    return currentMap.game.activeOwner;
  }
  const order = currentMap && currentMap.game && Array.isArray(currentMap.game.turnOrder)
    ? currentMap.game.turnOrder
    : [];
  const index = Math.max(0, Math.min(activeFactionIndex, order.length - 1));
  return Number.isInteger(order[index]) ? order[index] : null;
}

function activeFaction() {
  const owner = activeOwner();
  const faction = currentMap && currentMap.game && Array.isArray(currentMap.game.factions)
    ? currentMap.game.factions.find((candidate) => candidate.id === owner)
    : null;
  if (faction) {
    return faction;
  }
  return factionForOwner(owner);
}

function activeFactionAiControlled() {
  const faction = activeFaction();
  return Boolean(faction && faction.ai);
}

function countUnits(kind, owner = null) {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.filter((unit) => unit.kind === kind && (owner === null || unit.owner === owner)).length
    : 0;
}

function activeFactionUnits() {
  const owner = activeOwner();
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units
      .filter((unit) => unit.owner === owner && (!Number.isFinite(unit.hp) || unit.hp > 0))
      .slice()
      .sort((first, second) => first.id - second.id)
    : [];
}

function activeFactionResources(owner) {
  const faction = activeFaction();
  const totals = {
    population: 0,
    horses: 0,
    food: Number.isInteger(faction.food) ? faction.food : 0,
    metal: Number.isInteger(faction.metal) ? faction.metal : 0,
    treasure: Number.isInteger(faction.treasure) ? faction.treasure : 0,
  };
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return totals;
  }
  for (const unit of currentMap.units) {
    if (unit.owner !== owner || unit.kind !== "horde") {
      continue;
    }
    totals.population += Number.isInteger(unit.population) ? unit.population : 0;
    totals.horses += Number.isInteger(unit.horses) ? unit.horses : 0;
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
  currentMap.name = typeof currentMap.name === "string" && currentMap.name.trim()
    ? currentMap.name
    : "Untitled Scenario";
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.foodConsumption = currentMap.game.foodConsumption !== false;
  currentMap.units = Array.isArray(currentMap.units) ? currentMap.units.map(normalizeUnit) : [];

  const engineAuthoredGame = Number.isInteger(currentMap.game.stateVersion)
    && Array.isArray(currentMap.game.factions);
  if (engineAuthoredGame) {
    currentMap.factions = currentMap.game.factions.map((faction) => normalizeScenarioFaction(faction, faction.key));
    currentMap.game.factions = currentMap.factions.map((faction) => ({
      id: faction.id,
      key: faction.key,
      name: faction.name,
      color: faction.color,
      metal: faction.metal,
      treasure: faction.treasure,
      food: faction.food,
      enabled: faction.enabled,
      ai: faction.ai,
    }));
    if (!Array.isArray(currentMap.game.turnOrder) || currentMap.game.turnOrder.length === 0) {
      currentMap.game.turnOrder = currentMap.factions
        .filter((faction) => faction.enabled)
        .map((faction) => faction.id);
    }
    currentTurn = Number.isInteger(currentMap.game.round) ? currentMap.game.round : 1;
    const maxIndex = Math.max(0, currentMap.game.turnOrder.length - 1);
    activeFactionIndex = Number.isInteger(currentMap.game.activeFactionIndex)
      ? Math.max(0, Math.min(currentMap.game.activeFactionIndex, maxIndex))
      : 0;
    if (!Number.isInteger(currentMap.game.activeOwner)) {
      currentMap.game.activeOwner = currentMap.game.turnOrder[activeFactionIndex] ?? null;
    }
    currentMap.game.legalMoves = Array.isArray(currentMap.game.legalMoves) ? currentMap.game.legalMoves : [];
    currentMap.game.legalAttacks = Array.isArray(currentMap.game.legalAttacks) ? currentMap.game.legalAttacks : [];
    currentMap.game.legalRaids = Array.isArray(currentMap.game.legalRaids) ? currentMap.game.legalRaids : [];
    currentMap.game.aiGroups = normalizeAiGroups(currentMap.game.aiGroups);
    currentMap.game.diplomacy = normalizeDiplomacy(currentMap.game.diplomacy);
    return;
  }

  const priorGameFactions = Array.isArray(currentMap.game.factions) ? currentMap.game.factions : [];
  currentMap.factions = normalizeScenarioFactions(currentMap.factions, currentMap.units).map((faction) => {
    const prior = priorGameFactions.find((candidate) => candidate.id === faction.id);
    return prior
      ? normalizeScenarioFaction({
        ...prior,
        ...faction,
        metal: Number.isFinite(prior.metal) ? prior.metal : faction.metal,
        treasure: Number.isFinite(prior.treasure) ? prior.treasure : faction.treasure,
        food: Number.isFinite(prior.food) ? prior.food : faction.food,
      }, faction.key)
      : faction;
  });
  currentMap.game.turnOrder = currentMap.factions
    .filter((faction) => faction.enabled)
    .map((faction) => faction.id);
  if (currentMap.game.turnOrder.length === 0) {
    currentMap.factions[0].enabled = true;
    currentMap.game.turnOrder = [currentMap.factions[0].id];
  }
  currentMap.game.factions = currentMap.factions.map((faction) => ({
    id: faction.id,
    key: faction.key,
    name: faction.name,
    color: faction.color,
    metal: faction.metal,
    treasure: faction.treasure,
    food: faction.food,
    enabled: faction.enabled,
    ai: faction.ai,
  }));
  currentMap.game.aiGroups = normalizeAiGroups(currentMap.game.aiGroups);
  currentMap.game.diplomacy = normalizeDiplomacy(currentMap.game.diplomacy);
  const unitsById = new Map(currentMap.units.map((unit) => [unit.id, unit]));
  for (const group of currentMap.game.aiGroups) {
    group.unitIds = group.unitIds.filter((unitId) => {
      const unit = unitsById.get(unitId);
      return unit && unit.owner === group.owner;
    });
  }
  currentTurn = Number.isInteger(currentMap.game.round) ? currentMap.game.round : 1;
  const maxIndex = Math.max(0, currentMap.game.turnOrder.length - 1);
  activeFactionIndex = Number.isInteger(currentMap.game.activeFactionIndex)
    ? Math.max(0, Math.min(currentMap.game.activeFactionIndex, maxIndex))
    : 0;
  const activeOwner = currentMap.game.turnOrder[activeFactionIndex] ?? null;
  currentMap.game.round = currentTurn;
  currentMap.game.activeFactionIndex = activeFactionIndex;
  currentMap.game.activeOwner = currentMap.game.turnOrder[activeFactionIndex] ?? null;
  currentMap.game.legalMoves = Array.isArray(currentMap.game.legalMoves) ? currentMap.game.legalMoves : [];
  currentMap.game.legalAttacks = Array.isArray(currentMap.game.legalAttacks) ? currentMap.game.legalAttacks : [];
  currentMap.game.legalRaids = Array.isArray(currentMap.game.legalRaids) ? currentMap.game.legalRaids : [];
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
  editorSelectedUnitId = currentMap.game && Number.isInteger(currentMap.game.selectedUnitId)
    ? currentMap.game.selectedUnitId
    : 0;
  syncModeControls();
  drawMap();
  void syncScenarioEditorToEngine();
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
  if (Number.isFinite(defaults.horses)) normalized.horses = Math.max(0, Math.trunc(defaults.horses));
  normalized.allowedFactions = Array.isArray(defaults.allowedFactions)
    ? defaults.allowedFactions.filter((key) => typeof key === "string")
    : [];
  normalized.allowedArchetypes = Array.isArray(defaults.allowedArchetypes)
    ? defaults.allowedArchetypes.filter((key) => typeof key === "string")
    : [];
  return normalized;
}

function clampUnitHorses(kind, horses) {
  const normalized = Math.max(0, Math.trunc(horses));
  return kind === "horde" ? Math.min(normalized, hordeHorseCapacity) : normalized;
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

function populateUnitTypeSelect(select, kinds, previous = select.value) {
  select.replaceChildren();
  for (const kind of kinds) {
    const option = document.createElement("option");
    option.value = kind;
    option.textContent = unitKindLabelForKind(kind);
    select.appendChild(option);
  }
  if (kinds.includes(previous)) {
    select.value = previous;
  } else if (kinds.length > 0) {
    select.value = kinds[0];
  }
}

function unitKindAllowedForFaction(kind, factionKey) {
  const defaults = unitTypeDefaults[kind];
  if (!defaults) {
    return true;
  }
  const fallbackFaction = factions[factionKey];
  const archetype = fallbackFaction ? fallbackFaction.archetype : "";
  if (archetype && Array.isArray(defaults.allowedArchetypes) && defaults.allowedArchetypes.length > 0) {
    return defaults.allowedArchetypes.includes(archetype);
  }
  if (!Array.isArray(defaults.allowedFactions) || defaults.allowedFactions.length === 0) {
    return true;
  }
  return defaults.allowedFactions.includes(factionKey);
}

function unitKindsForFaction(factionKey) {
  const kinds = Object.keys(unitTypeDefaults).filter((kind) => unitKindAllowedForFaction(kind, factionKey));
  return kinds.length > 0 ? kinds : Object.keys(unitTypeDefaults);
}

function refreshEditorUnitTypeOptions() {
  populateUnitTypeSelect(editorUnitTypeSelect, unitKindsForFaction(editorUnitSideSelect.value));
}

function populateEditorUnitTypeOptions(kinds) {
  populateUnitTypeSelect(editorUnitTypeSelect, kinds.filter((kind) => unitKindAllowedForFaction(kind, editorUnitSideSelect.value)));
  populateUnitTypeSelect(unitTypeInput, kinds);
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
  if (typeof unit.name === "string") normalized.name = unit.name;
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
  if (Number.isFinite(unit.horses)) normalized.horses = clampUnitHorses(kind, unit.horses);
  if (Number.isFinite(unit.starvationTurns)) normalized.starvationTurns = Math.max(0, Math.trunc(unit.starvationTurns));
  if (typeof unit.productionState === "string") normalized.productionState = unit.productionState;
  if (typeof unit.productionKind === "string") normalized.productionKind = unit.productionKind;
  if (Number.isFinite(unit.productionTurnsRemaining)) {
    normalized.productionTurnsRemaining = Math.max(0, Math.trunc(unit.productionTurnsRemaining));
  }
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

function normalizeEngineView(view) {
  const normalized = refreshDerivedTopology(view);
  normalized.hexes = Array.isArray(normalized.hexes)
    ? normalized.hexes.map(normalizeMapHex).filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r))
    : [];
  normalized.units = Array.isArray(normalized.units) ? normalized.units.map(normalizeUnit) : [];
  if (normalized.game
    && Array.isArray(normalized.game.factions)
    && (!Array.isArray(normalized.factions) || normalized.factions.length === 0)) {
    normalized.factions = normalized.game.factions.map((faction) => normalizeScenarioFaction(faction, faction.key));
  }
  const previousMap = currentMap;
  try {
    currentMap = normalized;
    ensureGameMeta();
  } finally {
    currentMap = previousMap;
  }
  return normalized;
}

function replaceProjectionFromEngine(view, reason = "replace") {
  currentMap = normalizeEngineView(view);
  syncEngineProjectionMeta(currentMap, reason);
  return currentMap;
}

function applyEnginePatch(payload, reason = "patch") {
  if (!currentMap || !payload) {
    return;
  }
  if (Array.isArray(payload.hexes)) {
    currentMap.hexes = payload.hexes.map(normalizeMapHex)
      .filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r));
  }
  if (Array.isArray(payload.units)) {
    currentMap.units = payload.units.map(normalizeUnit);
  }
  if (Array.isArray(payload.edges)) {
    currentMap.edges = payload.edges;
  }
  if (Array.isArray(payload.roads)) {
    currentMap.roads = payload.roads;
  }
  if (Array.isArray(payload.walls)) {
    currentMap.walls = payload.walls;
  }
  if (Array.isArray(payload.wall_gates)) {
    currentMap.wall_gates = payload.wall_gates;
  }
  if (Array.isArray(payload.crossings)) {
    currentMap.crossings = payload.crossings;
  }
  if (payload.game && typeof payload.game === "object") {
    currentMap.game = payload.game;
    if (Array.isArray(payload.game.factions)) {
      currentMap.factions = payload.game.factions.map((faction) => normalizeScenarioFaction(faction, faction.key));
    }
  }
  ensureGameMeta();
  syncEngineProjectionMeta(currentMap, reason);
}

function applyGamePatch(payload) {
  applyEnginePatch(payload);
}

function syncEngineProjectionMeta(map, reason) {
  const game = map && map.game && typeof map.game === "object" ? map.game : null;
  engineProjection.version = Number.isInteger(game && game.stateVersion) ? game.stateVersion : 0;
  engineProjection.hash = typeof (game && game.stateHash) === "string" ? game.stateHash : "";
  engineProjection.lastSyncReason = reason;
}

function hasEngineProjection() {
  return engineProjection.version > 0 && engineProjection.hash.length > 0;
}

function resetScenarioEditorSyncQueue() {
  scenarioEditorSyncVersion += 1;
  scenarioEditorEngineSync = Promise.resolve(false);
}

function syncScenarioEditorToEngine() {
  if (!currentMap) {
    return Promise.resolve(false);
  }
  scenarioEditorSyncVersion += 1;
  const syncVersion = scenarioEditorSyncVersion;
  scenarioEditorEngineSync = scenarioEditorEngineSync
    .catch(() => false)
    .then(async () => {
      const commandType = hasEngineProjection() ? "set_editor_state" : "load_game";
      const snapshot = scenarioSnapshot();
      let command = { type: commandType, state: snapshot };
      let payload;
      try {
        payload = await postAppCommand(command);
      } catch (error) {
        if (commandType !== "set_editor_state" || !String(error && error.message).includes("unknown gameId")) {
          throw error;
        }
        command = { type: "load_game", state: snapshot };
        payload = await postAppCommand(command);
      }
      if (syncVersion === scenarioEditorSyncVersion) {
        replaceProjectionFromEngine(payload, command.type);
        syncModeControls();
        drawMap();
      }
      return true;
    })
    .catch((error) => {
      window.alert(error.message);
      return false;
    });
  return scenarioEditorEngineSync;
}

function syncAiGroupsToEngine() {
  return syncScenarioEditorToEngine();
}

function projectionComparableState(map) {
  if (!map || typeof map !== "object") {
    return null;
  }
  const game = map.game && typeof map.game === "object" ? map.game : {};
  return {
    width: map.width,
    height: map.height,
    hexes: Array.isArray(map.hexes)
      ? map.hexes.map((hex) => ({
        q: hex.q,
        r: hex.r,
        terrain: hex.terrain,
        name: normalizeHexName(hex.name),
        owner: Number.isInteger(hex.owner) ? hex.owner : factions.neutral.owner,
        supplySource: Boolean(hex.supplySource),
        supplied: Boolean(hex.supplied),
        pasture: normalizePasture(hex.pasture, hex.terrain),
      }))
      : [],
    units: Array.isArray(map.units)
      ? map.units.map(normalizeUnit)
      : [],
    game: {
      stateVersion: Number.isInteger(game.stateVersion) ? game.stateVersion : 0,
      round: Number.isInteger(game.round) ? game.round : 1,
      activeFactionIndex: Number.isInteger(game.activeFactionIndex) ? game.activeFactionIndex : 0,
      activeOwner: Number.isInteger(game.activeOwner) ? game.activeOwner : null,
      selectedUnitId: Number.isInteger(game.selectedUnitId) ? game.selectedUnitId : 0,
      foodConsumption: game.foodConsumption !== false,
      legalMoves: Array.isArray(game.legalMoves) ? game.legalMoves : [],
      legalAttacks: Array.isArray(game.legalAttacks) ? game.legalAttacks : [],
      legalRaids: Array.isArray(game.legalRaids) ? game.legalRaids : [],
      turnOrder: Array.isArray(game.turnOrder) ? game.turnOrder : [],
      factions: Array.isArray(game.factions) ? game.factions : [],
      aiGroups: normalizeAiGroups(game.aiGroups),
      diplomacy: normalizeDiplomacy(game.diplomacy),
    },
  };
}

async function resyncEngineProjection(reason) {
  const snapshot = await postAppCommand({ type: "get_state" });
  replaceProjectionFromEngine(snapshot, reason);
  syncModeControls();
  drawMap();
  return currentMap;
}

async function validateEngineProjection(reason) {
  if (appMode !== "play" || !currentMap) {
    return true;
  }
  const authoritative = await postAppCommand({ type: "get_state" });
  const normalizedAuthoritative = normalizeEngineView(authoritative);
  const localComparable = projectionComparableState(currentMap);
  const authoritativeComparable = projectionComparableState(normalizedAuthoritative);
  if (JSON.stringify(localComparable) === JSON.stringify(authoritativeComparable)) {
    syncEngineProjectionMeta(currentMap, reason);
    return true;
  }
  console.warn("Engine projection desync; resyncing browser state", {
    reason,
    local: currentMap.game && {
      stateVersion: currentMap.game.stateVersion,
      stateHash: currentMap.game.stateHash,
    },
    authoritative: normalizedAuthoritative.game && {
      stateVersion: normalizedAuthoritative.game.stateVersion,
      stateHash: normalizedAuthoritative.game.stateHash,
    },
  });
  replaceProjectionFromEngine(normalizedAuthoritative, `resync:${reason}`);
  syncModeControls();
  drawMap();
  return false;
}

function applyAiStepHexSnapshot(step) {
  if (!currentMap || !Array.isArray(currentMap.hexes) || !step || !Array.isArray(step.hexes) || step.hexes.length === 0) {
    return false;
  }
  const currentHexes = new Map(currentMap.hexes.map((hex) => [coordKey(hex), hex]));
  let changed = false;
  for (const update of step.hexes) {
    const hex = currentHexes.get(coordKey(update));
    if (!hex) {
      continue;
    }
    if (Number.isInteger(update.owner) && hex.owner !== update.owner) {
      hex.owner = update.owner;
      changed = true;
    }
    if (typeof update.supplySource === "boolean" && hex.supplySource !== update.supplySource) {
      hex.supplySource = update.supplySource;
      changed = true;
    }
    if (typeof update.supplied === "boolean" && hex.supplied !== update.supplied) {
      hex.supplied = update.supplied;
      changed = true;
    }
  }
  if (changed) {
    syncHexInspector();
  }
  return changed;
}

function normalizeAnimationCoord(coord) {
  if (!coord || !Number.isInteger(coord.q) || !Number.isInteger(coord.r)) {
    return null;
  }
  return { q: coord.q, r: coord.r };
}

function normalizeAnimationHexState(hex) {
  if (!hex || !Number.isInteger(hex.q) || !Number.isInteger(hex.r)) {
    return null;
  }
  const normalized = { q: hex.q, r: hex.r };
  if (Number.isInteger(hex.owner)) {
    normalized.owner = hex.owner;
  }
  if (typeof hex.supplySource === "boolean") {
    normalized.supplySource = hex.supplySource;
  } else if (typeof hex.supply_source === "boolean") {
    normalized.supplySource = hex.supply_source;
  }
  if (typeof hex.supplied === "boolean") {
    normalized.supplied = hex.supplied;
  }
  return normalized;
}

function normalizeAiAttackEvent(event) {
  if (!event || typeof event !== "object") {
    return null;
  }
  const target = normalizeAnimationCoord(event.target);
  const defenderFrom = normalizeAnimationCoord(event.defenderFrom);
  const defenderTo = normalizeAnimationCoord(event.defenderTo);
  const attackerTo = normalizeAnimationCoord(event.attackerTo);
  if (!target || !defenderFrom || !defenderTo || !attackerTo) {
    return null;
  }
  return {
    target,
    defenderId: Number.isInteger(event.defenderId) ? event.defenderId : 0,
    defenderFrom,
    defenderTo,
    defenderMoved: Boolean(event.defenderMoved),
    attackerTo,
    attackerMoved: Boolean(event.attackerMoved),
  };
}

function normalizeAiAnimationStep(step) {
  if (!step || !Number.isInteger(step.unitId)) {
    return null;
  }
  const from = normalizeAnimationCoord(step.from);
  const to = normalizeAnimationCoord(step.to);
  if (!from || !to) {
    return null;
  }
  return {
    unitId: step.unitId,
    from,
    to,
    hexes: Array.isArray(step.hexes)
      ? step.hexes.map(normalizeAnimationHexState).filter(Boolean)
      : [],
    units: Array.isArray(step.units)
      ? step.units.map(normalizeUnit)
      : [],
    attacks: Array.isArray(step.attacks)
      ? step.attacks.map(normalizeAnimationCoord).filter(Boolean)
      : [],
    attackEvents: Array.isArray(step.attackEvents)
      ? step.attackEvents.map(normalizeAiAttackEvent).filter(Boolean)
      : [],
  };
}

function normalizeAiAnimation(rawAnimation) {
  return Array.isArray(rawAnimation)
    ? rawAnimation.map(normalizeAiAnimationStep).filter(Boolean)
    : [];
}

function sleep(milliseconds) {
  return new Promise((resolve) => window.setTimeout(resolve, milliseconds));
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
  await playModeEngineSync;
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

function legalRaids() {
  return currentMap && currentMap.game && Array.isArray(currentMap.game.legalRaids)
    ? currentMap.game.legalRaids
    : [];
}

function legalMoveAt(coord) {
  return legalMoves().find((move) => move.q === coord.q && move.r === coord.r) || null;
}

function legalAttackForUnit(unit) {
  return unit ? legalAttacks().find((attack) => attack.unitId === unit.id) || null : null;
}

function legalRaidAt(coord) {
  return coord ? legalRaids().find((raid) => raid.q === coord.q && raid.r === coord.r) || null : null;
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
  if (preview.retreatOption === retreatSide && preview.retreatBlocked) {
    return "Blocked";
  }
  return preview.retreatOption === retreatSide ? "Retreats" : "No";
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
  } else if (preview.specialResolution === "horse_stealing") {
    titleParts.push("Horse Stealing");
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
  appendCombatRow(summary, "Retreat", preview.retreatOption === "none"
    ? "-"
    : `${preview.retreatOption}${preview.retreatBlocked ? " blocked" : ""}`
  );
  const resolutionLabel = preview.specialResolution === "feigned_retreat"
    ? "Feigned retreat"
    : (preview.specialResolution === "horse_stealing" ? "Horse stealing" : "Normal");
  appendCombatRow(summary, "Resolution", resolutionLabel);
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
    id: "raid-hex",
    label: "Raid farmland",
    visible: ({ hex }) => Boolean(hex && legalRaidAt(hex)),
    enabled: ({ hex }) => Boolean(hex && legalRaidAt(hex)),
    run: async ({ hex }) => raidSelectedHex(hex),
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
    label: "Train Horse Archers",
    visible: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.createHorseArchers
    ),
    enabled: ({ unit, actionAvailability }) => Boolean(
      unit
      && actionAvailability
      && actionAvailability.createHorseArchers
    ),
    run: async (context) => startHordeUnitProduction(context, "create_horse_archers"),
  },
  {
    id: "create-mongol-lancers",
    label: "Train Lancers",
    visible: ({ unit, actionAvailability }) => Boolean(
      unit
      && factionArchetypeForOwner(unit.owner) === "steppe_nomad"
      && actionAvailability
      && actionAvailability.createMongolLancers
    ),
    enabled: ({ unit, actionAvailability }) => Boolean(
      unit
      && factionArchetypeForOwner(unit.owner) === "steppe_nomad"
      && actionAvailability
      && actionAvailability.createMongolLancers
    ),
    run: async (context) => startHordeUnitProduction(context, "create_mongol_lancers"),
  },
];

function unitDisplayName(unit) {
  if (unit && typeof unit.name === "string" && unit.name.trim()) {
    return unit.name.trim();
  }
  return defaultUnitDisplayNameFor(unit);
}

function unitKindLabel(unit) {
  return unitKindLabelForKind(unit.kind);
}

function stanceLabel(stance) {
  return String(stance || "default")
    .split("_")
    .map((part) => part ? `${part[0].toUpperCase()}${part.slice(1)}` : part)
    .join(" ");
}

function unitZocLabel(unit) {
  const parts = [];
  if (unit && unit.projectsZoc) {
    parts.push("Projects");
  }
  parts.push(unit && unit.respectsZoc ? "Respects" : "Ignores");
  return parts.join(" / ");
}

function unitSpecialPropertyLabels(unit) {
  if (!unit || unit.kind === "horde" || unit.kind === "herd") {
    return [];
  }
  const labels = [];
  const stances = legalStancesForUnit(unit);
  if (stances.includes("feigned_retreat")) {
    labels.push("Feigned Retreat");
  }
  if (unit.respectsZoc === false) {
    labels.push("Ignores ZOC");
  }
  return labels;
}

function syncUnitInspectorView() {
  const unitInspector = document.querySelector(".unit-inspector");
  const statsVisible = unitInspectorStatsVisible && Boolean(selectedUnit());
  unitInspector.classList.toggle("is-stats-view", statsVisible);
  if (unitStatsView) {
    unitStatsView.hidden = !statsVisible;
  }
  if (unitInspectorInfoToggle) {
    unitInspectorInfoToggle.setAttribute("aria-pressed", String(statsVisible));
    unitInspectorInfoToggle.setAttribute("aria-label", statsVisible ? "Show unit parameters" : "Show unit stats");
    unitInspectorInfoToggle.title = statsVisible ? "Show unit parameters" : "Show unit stats";
  }
}

function productionLabel(unit) {
  if (!unit || unit.kind !== "horde" || unit.productionState !== "building") {
    return "Idle";
  }
  const kind = unitKindLabelForKind(unit.productionKind || "horse_archer");
  const turns = Number.isInteger(unit.productionTurnsRemaining) ? unit.productionTurnsRemaining : 0;
  return `${kind}: ${turns} turn${turns === 1 ? "" : "s"}`;
}

function legalStancesForUnit(unit) {
  const defaults = unitDefaultsFor(unit ? unit.kind : "");
  return defaults.legalStances && defaults.legalStances.length > 0 ? defaults.legalStances : ["default"];
}

function resourceFieldsForUnit(unit) {
  if (!unit) {
    return [];
  }
  if (unit.kind === "horde") {
    return ["population", "horses", "starvation"];
  }
  if (unit.kind === "herd") {
    return ["horses", "starvation"];
  }
  return [];
}

function defaultUnitDisplayNameFor(unit, kind = unit.kind) {
  const owner = Number.isInteger(unit && unit.owner)
    ? unit.owner
    : (factions[unit && unit.faction] ? factions[unit.faction].owner : factions.neutral.owner);
  const faction = factionForOwner(owner);
  const factionName = typeof faction.name === "string" && faction.name.trim()
    ? faction.name.trim()
    : (typeof (unit && unit.faction) === "string" && unit.faction.trim() ? unit.faction.trim() : "unit");
  const prefixMatch = factionName.match(/[A-Za-z0-9]/);
  const prefix = prefixMatch ? prefixMatch[0].toLowerCase() : "u";
  const unitKind = typeof kind === "string" && kind.trim() ? kind.trim() : "unit";
  const id = Number.isInteger(unit && unit.id) ? unit.id : 0;
  return `${prefix}_${unitKind}_${id}`;
}

function editorInspectorUnit() {
  return unitEditActive() ? selectedUnit() : null;
}

function applyUnitTypeDefaults(unit, kind) {
  const defaults = unitDefaultsFor(kind);
  const hadCustomName = typeof unit.name === "string" && unit.name.trim()
    && unit.name.trim() !== defaultUnitDisplayNameFor(unit, unit.kind);
  unit.kind = kind;
  unit.attack = defaults.attack;
  unit.defense = defaults.defense;
  unit.readinessDamage = defaults.readinessDamage;
  unit.maxHp = defaults.hp;
  unit.hp = Math.max(0, Math.min(Number.isFinite(unit.hp) ? Math.trunc(unit.hp) : defaults.hp, defaults.hp));
  unit.maxReadiness = 100;
  unit.readiness = Math.max(0, Math.min(Number.isFinite(unit.readiness) ? Math.trunc(unit.readiness) : defaults.readiness, 100));
  unit.move = defaults.move;
  unit.remainingMove = defaults.move;
  unit.projectsZoc = Boolean(defaults.projectsZoc);
  unit.respectsZoc = Boolean(defaults.respectsZoc);
  if (Number.isInteger(defaults.population)) unit.population = defaults.population;
  if (Number.isInteger(defaults.horses)) unit.horses = clampUnitHorses(kind, defaults.horses);
  const stances = defaults.legalStances && defaults.legalStances.length > 0 ? defaults.legalStances : ["default"];
  unit.stance = stances.includes(unit.stance) ? unit.stance : defaults.stance;
  if (!hadCustomName) {
    delete unit.name;
  }
}

function mutateEditorInspectorUnit(mutator) {
  const unit = editorInspectorUnit();
  if (!unit) {
    return;
  }
  recordLocalUndo();
  mutator(unit);
  ensureGameMeta();
  syncModeControls();
  drawMap();
  void syncScenarioEditorToEngine();
}

function updateEditorUnitName() {
  mutateEditorInspectorUnit((unit) => {
    const name = unitNameInput.value.trim();
    const fallbackName = defaultUnitDisplayNameFor(unit);
    if (!name || name === fallbackName) {
      delete unit.name;
    } else {
      unit.name = name;
    }
  });
}

function updateEditorUnitType() {
  mutateEditorInspectorUnit((unit) => {
    const kind = unitTypeInput.value;
    if (!unitKindAllowedForFaction(kind, unit.faction)) {
      unitTypeInput.value = unit.kind;
      return;
    }
    applyUnitTypeDefaults(unit, kind);
  });
}

function updateEditorUnitHp() {
  mutateEditorInspectorUnit((unit) => {
    const maxHp = Number.isFinite(unit.maxHp) ? unit.maxHp : unitDefaultsFor(unit.kind).hp;
    const hp = Number.parseInt(unitHpInput.value, 10);
    unit.hp = Number.isFinite(hp) ? Math.max(0, Math.min(hp, maxHp)) : unit.hp;
  });
}

function updateEditorUnitReadiness() {
  mutateEditorInspectorUnit((unit) => {
    const readiness = Number.parseInt(unitReadinessInput.value, 10);
    unit.maxReadiness = 100;
    unit.readiness = Number.isFinite(readiness) ? Math.max(0, Math.min(readiness, 100)) : unit.readiness;
  });
}

function updateEditorUnitStance() {
  mutateEditorInspectorUnit((unit) => {
    const stances = legalStancesForUnit(unit);
    unit.stance = stances.includes(unitStanceInput.value) ? unitStanceInput.value : stances[0];
  });
}

function updateEditorUnitResource(field, input) {
  mutateEditorInspectorUnit((unit) => {
    if (!resourceFieldsForUnit(unit).includes(field)) {
      return;
    }
    const value = Number.parseInt(input.value, 10);
    const fallback = Number.isInteger(unit[field]) ? unit[field] : 0;
    unit[field] = Number.isFinite(value) ? Math.max(0, value) : fallback;
    if (field === "horses") {
      unit[field] = clampUnitHorses(unit.kind, unit[field]);
    }
  });
}

function syncUnitInspector() {
  const unit = selectedUnit();
  const editing = unitEditActive() && Boolean(unit);
  const unitInspector = document.querySelector(".unit-inspector");
  unitInspector.classList.toggle("is-editing", editing);
  if (!unit) {
    if (sidebarSelectionReadout) {
      sidebarSelectionReadout.textContent = "None";
    }
    unitName.textContent = "None";
    unitNameInput.value = "";
    unitType.textContent = "-";
    unitTypeInput.value = unitTypeInput.options.length > 0 ? unitTypeInput.options[0].value : "";
    unitHp.textContent = "-";
    unitHpInput.value = "";
    unitAttack.textContent = "-";
    unitDefense.textContent = "-";
    unitReadinessDamage.textContent = "-";
    unitReadiness.textContent = "-";
    unitReadinessInput.value = "";
    unitMove.textContent = "-";
    unitZoc.textContent = "-";
    unitSpecialProperties.textContent = "None";
    unitStance.textContent = "-";
    unitStanceInput.replaceChildren();
    unitResources.hidden = true;
    unitPopulationRow.hidden = false;
    unitHorsesRow.hidden = false;
    unitStarvationRow.hidden = true;
    unitPopulation.textContent = "0";
    unitPopulationInput.value = "";
    unitHorses.textContent = "0";
    unitHorsesInput.value = "";
    unitStarvation.textContent = "0";
    unitProductionRow.hidden = true;
    unitProduction.textContent = "Idle";
    syncUnitInspectorView();
    return;
  }
  if (sidebarSelectionReadout) {
    sidebarSelectionReadout.textContent = unitDisplayName(unit);
  }
  unitName.textContent = unitDisplayName(unit);
  unitNameInput.value = unitDisplayName(unit);
  unitType.textContent = unitKindLabel(unit);
  populateUnitTypeSelect(unitTypeInput, unitKindsForFaction(unit.faction), unit.kind);
  unitHp.textContent = Number.isFinite(unit.hp) && Number.isFinite(unit.maxHp) ? String(unit.hp) : "-";
  unitHpInput.value = Number.isFinite(unit.hp) ? String(unit.hp) : "";
  unitAttack.textContent = Number.isFinite(unit.attack) ? String(unit.attack) : "-";
  unitDefense.textContent = Number.isFinite(unit.defense) ? String(unit.defense) : "-";
  unitReadinessDamage.textContent = Number.isFinite(unit.readinessDamage) ? String(unit.readinessDamage) : "-";
  unitReadiness.textContent = Number.isFinite(unit.readiness) ? String(unit.readiness) : "-";
  unitReadinessInput.value = Number.isFinite(unit.readiness) ? String(unit.readiness) : "";
  unitMove.textContent = Number.isFinite(unit.move) ? String(unit.move) : "-";
  unitZoc.textContent = unitZocLabel(unit);
  const specialProperties = unitSpecialPropertyLabels(unit);
  unitSpecialProperties.textContent = specialProperties.length > 0 ? specialProperties.join(", ") : "None";
  unitStance.textContent = stanceLabel(unit.stance);
  const stances = legalStancesForUnit(unit);
  const previousStance = unitStanceInput.value;
  unitStanceInput.replaceChildren();
  for (const stance of stances) {
    const option = document.createElement("option");
    option.value = stance;
    option.textContent = stanceLabel(stance);
    unitStanceInput.appendChild(option);
  }
  unitStanceInput.value = stances.includes(unit.stance)
    ? unit.stance
    : (stances.includes(previousStance) ? previousStance : stances[0]);
  const resourceFields = resourceFieldsForUnit(unit);
  unitResources.hidden = resourceFields.length === 0;
  unitPopulationRow.hidden = !resourceFields.includes("population");
  unitHorsesRow.hidden = !resourceFields.includes("horses");
  unitStarvationRow.hidden = !resourceFields.includes("starvation");
  const population = Number.isInteger(unit.population) ? unit.population : 0;
  const horses = Number.isInteger(unit.horses) ? unit.horses : 0;
  const starvationTurns = Number.isInteger(unit.starvationTurns) ? unit.starvationTurns : 0;
  unitPopulation.textContent = String(population);
  unitPopulationInput.value = String(population);
  unitHorses.textContent = String(horses);
  unitHorsesInput.value = String(horses);
  if (unit.kind === "horde") {
    unitHorsesInput.max = String(hordeHorseCapacity);
  } else {
    unitHorsesInput.removeAttribute("max");
  }
  unitStarvation.textContent = String(starvationTurns);
  unitProductionRow.hidden = unit.kind !== "horde";
  unitProduction.textContent = productionLabel(unit);
  syncUnitInspectorView();
}

function syncPlayControls() {
  ensureGameMeta();
  const faction = activeFaction();
  const owner = activeOwner();
  if (turnCounter) {
    turnCounter.textContent = `Round ${currentTurn} · ${faction.name} turn`;
  }
  if (activeFactionName) {
    activeFactionName.textContent = faction.name;
  }
  statusActiveFactionName.textContent = faction.name;
  factionStatusName.textContent = faction.name;
  roundCount.textContent = String(currentTurn);
  turnStatusReadout.textContent = `${faction.name} turn`;
  if (herdCount) {
    herdCount.textContent = String(countUnits("herd", owner));
  }
  if (horseArcherCount) {
    horseArcherCount.textContent = String(countUnits("horse_archer", owner));
  }
  if (hordeCount) {
    hordeCount.textContent = String(countUnits("horde", owner));
  }
  const resources = activeFactionResources(owner);
  factionPopulationTotal.textContent = String(resources.population);
  factionHorsesTotal.textContent = String(resources.horses);
  if (factionFoodTotal) {
    factionFoodTotal.textContent = String(resources.food);
  }
  factionMetalTotal.textContent = String(resources.metal);
  factionTreasureTotal.textContent = String(resources.treasure);
  if (sidebarFactionMetal) {
    sidebarFactionMetal.textContent = String(resources.metal);
  }
  if (executeAiGroupButton) {
    executeAiGroupButton.disabled = appMode !== "play" || aiAnimationInProgress || strategicAiGroups().length === 0;
  }
  const canCycleUnits = appMode === "play"
    && !aiAnimationInProgress
    && !unitSelectionInProgress
    && activeFactionUnits().length > 0;
  if (prevUnitButton) {
    prevUnitButton.disabled = !canCycleUnits;
  }
  if (nextUnitButton) {
    nextUnitButton.disabled = !canCycleUnits;
  }
  syncUnitInspector();
  syncStrategicAiDashboard();
}

function syncPastureViewButton() {
  pastureViewButton.classList.toggle("is-active", pastureViewEnabled);
  pastureViewButton.setAttribute("aria-pressed", String(pastureViewEnabled));
}

function syncTerritoryViewButton() {
  if (!territoryViewButton) {
    return;
  }
  territoryViewButton.classList.toggle("is-active", territoryViewEnabled);
  territoryViewButton.setAttribute("aria-pressed", String(territoryViewEnabled));
}

function syncUnitsViewButton() {
  if (!unitsViewButton) {
    return;
  }
  unitsViewButton.classList.toggle("is-active", unitsViewEnabled);
  unitsViewButton.setAttribute("aria-pressed", String(unitsViewEnabled));
}

function syncCoordinateViewButton() {
  if (!coordinatesViewButton) {
    return;
  }
  coordinatesViewButton.classList.toggle("is-active", coordinateLabelsEnabled);
  coordinatesViewButton.setAttribute("aria-pressed", String(coordinateLabelsEnabled));
}

function syncHexInspector() {
  const hex = selectedHex();
  if (!hex) {
    if (selectedHexCoord && currentMap) {
      selectedHexCoord = null;
    }
    if (hexTitleCoordinate) {
      hexTitleCoordinate.textContent = "None";
    }
    hexName.textContent = "None";
    hexTerrain.textContent = "-";
    hexDefense.textContent = "-";
    hexMoveCost.textContent = "-";
    if (hexRoadCost) {
      hexRoadCost.textContent = "-";
    }
    hexSupplyState.textContent = "-";
    hexPastureRow.hidden = true;
    hexPasture.textContent = "-";
    return;
  }

  const terrainCost = terrainMovementCostScaled(hex.terrain);
  const pasture = normalizePasture(hex.pasture, hex.terrain);
  const coordinateLabel = `${hex.q},${hex.r}`;
  if (hexTitleCoordinate) {
    hexTitleCoordinate.textContent = coordinateLabel;
  }
  hexName.textContent = normalizeHexName(hex.name);
  hexTerrain.textContent = titleCaseToken(hex.terrain);
  hexDefense.textContent = `${terrainDefensePercent(hex.terrain)}%`;
  hexMoveCost.textContent = Number.isFinite(terrainCost)
    ? `${formatMoveCost(terrainCost)}; road F ${formatMoveCost(roadModifiedMovementCostScaled(terrainCost, false))} / M ${formatMoveCost(roadModifiedMovementCostScaled(terrainCost, true))}`
    : "Blocked";
  if (hexRoadCost) {
    hexRoadCost.textContent = Number.isFinite(terrainCost)
      ? `Foot ${formatMoveCost(roadModifiedMovementCostScaled(terrainCost, false))} / Mounted ${formatMoveCost(roadModifiedMovementCostScaled(terrainCost, true))}`
      : "Blocked";
  }
  hexSupplyState.textContent = supplyStateLabel(hex);
  hexPastureRow.hidden = pasture.capacity <= 0;
  hexPasture.textContent = pasture.capacity > 0
    ? `${formatPastureValue(pasture.remaining)}/${formatPastureValue(pasture.capacity)}`
    : "-";
}

function setPlaySurfaceMode(mode) {
  playSurfaceMode = mode === "diplomacy" ? "diplomacy" : "map";
  syncModeControls();
  if (playSurfaceMode === "map" && currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function toggleDiplomacySurface() {
  setPlaySurfaceMode(playSurfaceMode === "diplomacy" ? "map" : "diplomacy");
}

function toggleStrategicAiPanel() {
  if (appMode !== "play") {
    return;
  }
  strategicAiCollapsed = !strategicAiCollapsed;
  syncModeControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function toggleScenarioDetailsPanel() {
  if (appMode !== "scenario") {
    return;
  }
  scenarioDetailsCollapsed = !scenarioDetailsCollapsed;
  syncModeControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function maxScenarioPanelHeight() {
  const viewportHeight = Number.isFinite(window.innerHeight) ? window.innerHeight : 720;
  const halfViewport = Math.round(viewportHeight * 0.55);
  const mapPreservingMax = viewportHeight - 220;
  return Math.max(minScenarioPanelHeight, Math.min(halfViewport, mapPreservingMax));
}

function clampScenarioPanelHeight(value) {
  const numeric = Number.isFinite(value) ? value : defaultScenarioPanelHeight;
  return Math.max(minScenarioPanelHeight, Math.min(maxScenarioPanelHeight(), Math.round(numeric)));
}

function applyScenarioPanelHeight(value, persist = true) {
  scenarioPanelHeight = clampScenarioPanelHeight(value);
  appShell.style.setProperty("--scenario-panel-height", `${scenarioPanelHeight}px`);
  if (scenarioResizeHandle) {
    scenarioResizeHandle.setAttribute("aria-valuemin", String(minScenarioPanelHeight));
    scenarioResizeHandle.setAttribute("aria-valuemax", String(maxScenarioPanelHeight()));
    scenarioResizeHandle.setAttribute("aria-valuenow", String(scenarioPanelHeight));
  }
  if (persist) {
    try {
      window.localStorage.setItem(scenarioPanelHeightStorageKey, String(scenarioPanelHeight));
    } catch {
      // Ignore storage failures in private or restricted contexts.
    }
  }
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

function loadScenarioPanelHeight() {
  try {
    const stored = Number.parseInt(window.localStorage.getItem(scenarioPanelHeightStorageKey), 10);
    if (Number.isFinite(stored)) {
      scenarioPanelHeight = stored;
    }
  } catch {
    scenarioPanelHeight = defaultScenarioPanelHeight;
  }
  applyScenarioPanelHeight(scenarioPanelHeight, false);
}

function resetScenarioPanelHeight() {
  applyScenarioPanelHeight(defaultScenarioPanelHeight);
}

function startScenarioPanelResize(event) {
  if (appMode !== "scenario" || !scenarioResizeHandle) {
    return;
  }
  event.preventDefault();
  const startY = event.clientY;
  const startHeight = document.querySelector("#scenario-controls")?.getBoundingClientRect().height || scenarioPanelHeight;
  scenarioResizeHandle.setPointerCapture(event.pointerId);
  scenarioResizeHandle.classList.add("is-dragging");

  const onPointerMove = (moveEvent) => {
    applyScenarioPanelHeight(startHeight + moveEvent.clientY - startY);
  };
  const onPointerUp = (upEvent) => {
    if (scenarioResizeHandle.hasPointerCapture(upEvent.pointerId)) {
      scenarioResizeHandle.releasePointerCapture(upEvent.pointerId);
    }
    scenarioResizeHandle.classList.remove("is-dragging");
    scenarioResizeHandle.removeEventListener("pointermove", onPointerMove);
    scenarioResizeHandle.removeEventListener("pointerup", onPointerUp);
    scenarioResizeHandle.removeEventListener("pointercancel", onPointerUp);
  };

  scenarioResizeHandle.addEventListener("pointermove", onPointerMove);
  scenarioResizeHandle.addEventListener("pointerup", onPointerUp);
  scenarioResizeHandle.addEventListener("pointercancel", onPointerUp);
}

function nudgeScenarioPanelHeight(delta) {
  if (appMode !== "scenario") {
    return;
  }
  applyScenarioPanelHeight(scenarioPanelHeight + delta);
}

function syncModeControls() {
  appShell.classList.toggle("is-intro", appMode === "intro");
  appShell.classList.toggle("is-scenario", appMode === "scenario");
  appShell.classList.toggle("is-play", appMode === "play");
  appShell.classList.toggle("scenario-details-collapsed", appMode === "scenario" && scenarioDetailsCollapsed);
  appShell.classList.toggle("strategic-ai-collapsed", appMode === "play" && strategicAiCollapsed);
  mapSection.classList.toggle("is-diplomacy", appMode === "play" && playSurfaceMode === "diplomacy");
  if (diplomacyPanel) {
    diplomacyPanel.hidden = appMode !== "play" || playSurfaceMode !== "diplomacy";
  }
  appShell.classList.toggle("show-editor-inspector", appMode === "scenario" && unitEditActive() && Boolean(selectedUnit()));
  scenarioModeButton.classList.toggle("is-active", appMode === "scenario");
  playModeButton.classList.toggle("is-active", appMode === "play");
  scenarioModeButton.setAttribute("aria-pressed", String(appMode === "scenario"));
  playModeButton.setAttribute("aria-pressed", String(appMode === "play"));
  if (endTurnButton) {
    endTurnButton.disabled = appMode !== "play" || aiAnimationInProgress;
  }
  controlEndTurnButton.disabled = appMode !== "play" || aiAnimationInProgress;
  statusEndTurnButton.disabled = appMode !== "play" || aiAnimationInProgress;
  if (executeAiGroupButton) {
    executeAiGroupButton.disabled = appMode !== "play" || aiAnimationInProgress || strategicAiGroups().length === 0;
  }
  if (strategicAiToggleButton) {
    strategicAiToggleButton.hidden = appMode !== "play";
    strategicAiToggleButton.textContent = strategicAiCollapsed ? "+" : "-";
    strategicAiToggleButton.title = strategicAiCollapsed ? "Show Strategic AI" : "Hide Strategic AI";
    strategicAiToggleButton.setAttribute("aria-expanded", String(!strategicAiCollapsed));
    strategicAiToggleButton.setAttribute("aria-label", strategicAiCollapsed ? "Expand Strategic AI" : "Collapse Strategic AI");
  }
  if (strategicAiSelectNoneButton) {
    strategicAiSelectNoneButton.hidden = appMode !== "play";
    strategicAiSelectNoneButton.disabled = activeStrategicAiGroupId === 0;
  }
  if (mapAiToggleButton) {
    mapAiToggleButton.hidden = appMode !== "play";
    mapAiToggleButton.classList.toggle("is-active", appMode === "play" && !strategicAiCollapsed);
    mapAiToggleButton.setAttribute("aria-pressed", String(appMode === "play" && !strategicAiCollapsed));
  }
  if (diplomacyToggleButton) {
    diplomacyToggleButton.disabled = appMode !== "play" || aiAnimationInProgress;
    diplomacyToggleButton.classList.toggle("is-active", appMode === "play" && playSurfaceMode === "diplomacy");
    diplomacyToggleButton.setAttribute("aria-pressed", String(appMode === "play" && playSurfaceMode === "diplomacy"));
    diplomacyToggleButton.textContent = playSurfaceMode === "diplomacy" ? "Map" : "Diplomacy";
  }
  if (detailsToggleButton) {
    const scenarioMode = appMode === "scenario";
    detailsToggleButton.hidden = !scenarioMode;
    detailsToggleButton.textContent = scenarioDetailsCollapsed ? "Show Panel" : "Hide Panel";
    detailsToggleButton.title = scenarioDetailsCollapsed ? "Show bottom panel" : "Hide bottom panel";
    detailsToggleButton.setAttribute("aria-expanded", String(!scenarioDetailsCollapsed));
  }
  syncEditorControls();
  syncScenarioParameterControls();
  syncFactionRelationshipControls();
  syncPlayControls();
  syncDiplomacyScreen();
  syncUndoControls();
  syncPastureViewButton();
  syncTerritoryViewButton();
  syncUnitsViewButton();
  syncCoordinateViewButton();
  syncHexInspector();
}

function setAppMode(mode) {
  hideCombatPreview();
  appMode = mode;
  if (mode !== "play") {
    playSurfaceMode = "map";
  }
  if (mode !== "scenario") {
    isPainting = false;
    paintStrokeKeys = new Set();
    paintUndoRecorded = false;
    editorSelectedUnitId = 0;
    aiPickMode = null;
  }
  syncModeControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
}

async function enterPlayMode() {
  const sync = (async () => {
    appMode = "play";
    strategicAiCollapsed = true;
    territoryViewEnabled = true;
    isPainting = false;
    paintStrokeKeys = new Set();
    paintUndoRecorded = false;
    editorSelectedUnitId = 0;
    aiPickMode = null;
    syncModeControls();
    if (appInitialization) {
      await appInitialization;
    }
    if (currentMap) {
      try {
        const payload = await postAppCommand({ type: "load_game", state: scenarioSnapshot() });
        replaceProjectionFromEngine(payload, "load_game");
        terrainUndo = new Map();
        clearUndoHistory();
        ensureGameMeta();
        syncEngineProjectionMeta(currentMap, "load_game");
      } catch (error) {
        window.alert(error.message);
        return;
      }
    }
    setAppMode("play");
  })();
  playModeEngineSync = sync.catch(() => false);
  return sync;
}

function enterScenarioMode() {
  setAppMode("scenario");
}

function pastureStyleForHex(hex) {
  if (!pastureViewEnabled || hex.terrain !== "grassland") {
    return null;
  }
  const pasture = normalizePasture(hex.pasture, hex.terrain);
  const ratio = pasture.capacity > 0 ? pasture.remaining / pasture.capacity : 0;
  if (ratio >= 0.75) {
    return { fill: "#2f8f4e", stroke: "#15552d", label: "#eef7e8" };
  }
  if (ratio >= 0.5) {
    return { fill: "#d5c83a", stroke: "#7c7219", label: "#25230c" };
  }
  if (ratio >= 0.25) {
    return { fill: "#d9822b", stroke: "#874715", label: "#221105" };
  }
  return { fill: "#b83f2f", stroke: "#672118", label: "#fff0ec" };
}

function styleForHex(hex) {
  const pastureStyle = pastureStyleForHex(hex);
  if (pastureStyle) {
    return pastureStyle;
  }
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
  if (!coordinateLabelsEnabled || visibleSize < 8) {
    return;
  }

  ctx.fillStyle = style.label;
  ctx.font = `${Math.max(8, Math.min(13, visibleSize * 0.42)) / viewport.scale}px Segoe UI, Arial, sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, cx, cy);
}

function drawSelectedHex() {
  const hex = selectedHex();
  if (!hex) {
    return;
  }
  const center = hexCenter(hex);
  const points = hexPoints(center.x, center.y, geometry.size * 0.98);
  ctx.save();
  ctx.beginPath();
  points.forEach(([x, y], index) => {
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });
  ctx.closePath();
  ctx.strokeStyle = "#f6f0d8";
  ctx.lineWidth = 5 / viewport.scale;
  ctx.stroke();
  ctx.strokeStyle = "#1c1d1b";
  ctx.lineWidth = 2 / viewport.scale;
  ctx.stroke();
  ctx.restore();
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

function territoryClaimableHex(hex) {
  return hex && ["grassland", "farmland", "desert", "hill", "forest", "marsh", "urban"].includes(hex.terrain);
}

function territoryOwnerForHex(hex) {
  return Number.isInteger(hex && hex.owner) ? hex.owner : factions.neutral.owner;
}

function territoryOutlineColor(owner) {
  if (owner === factions.neutral.owner) {
    return "#050505";
  }
  return factionForOwner(owner).color || "#111111";
}

function drawTerritoryOutlines() {
  if ((!territoryViewEnabled && !ownershipEditingActive()) || !currentMap || !Array.isArray(currentMap.hexes)) {
    return;
  }

  const visited = new Set();
  const components = [];
  const hexByKey = new Map();
  for (const hex of currentMap.hexes) {
    hexByKey.set(coordKey(hex), hex);
  }
  for (const hex of currentMap.hexes) {
    if (!territoryClaimableHex(hex)) {
      continue;
    }
    const key = coordKey(hex);
    if (visited.has(key)) {
      continue;
    }

    const owner = territoryOwnerForHex(hex);
    const queue = [hex];
    const boundaries = new Map();
    let queueIndex = 0;
    visited.add(key);

    while (queueIndex < queue.length) {
      const current = queue[queueIndex];
      queueIndex += 1;
      for (let direction = 0; direction < 6; direction += 1) {
        const neighborCoord = neighborInDirection(current, direction);
        const neighbor = inBounds(neighborCoord) ? hexByKey.get(coordKey(neighborCoord)) : null;
        const sameRegion = territoryClaimableHex(neighbor) && territoryOwnerForHex(neighbor) === owner;
        if (sameRegion) {
          const neighborKey = coordKey(neighbor);
          if (!visited.has(neighborKey)) {
            visited.add(neighborKey);
            queue.push(neighbor);
          }
          continue;
        }
        const edge = canonicalEdge(current, neighborCoord);
        boundaries.set(edgeKey(edge), { edge, inside: current });
      }
    }

    components.push({ owner, boundaries: Array.from(boundaries.values()) });
  }

  components.sort((first, second) => {
    if (first.owner === factions.neutral.owner && second.owner !== factions.neutral.owner) {
      return -1;
    }
    if (first.owner !== factions.neutral.owner && second.owner === factions.neutral.owner) {
      return 1;
    }
    return first.owner - second.owner;
  });

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";
  for (const component of components) {
    ctx.strokeStyle = territoryOutlineColor(component.owner);
    ctx.lineWidth = 1.8 / viewport.scale;
    for (const segment of component.boundaries) {
      const boundary = edgeBoundaryPoints(segment.edge);
      if (!boundary) {
        continue;
      }
      const center = hexCenter(segment.inside);
      const inset = 6.4 / viewport.scale;
      const gap = 3.2 / viewport.scale;
      const midpoint = {
        x: (boundary[0][0] + boundary[1][0]) / 2,
        y: (boundary[0][1] + boundary[1][1]) / 2,
      };
      const inwardLength = Math.hypot(center.x - midpoint.x, center.y - midpoint.y) || 1;
      const inward = {
        x: ((center.x - midpoint.x) / inwardLength) * inset,
        y: ((center.y - midpoint.y) / inwardLength) * inset,
      };
      const dx = boundary[1][0] - boundary[0][0];
      const dy = boundary[1][1] - boundary[0][1];
      const edgeLength = Math.hypot(dx, dy) || 1;
      const trim = Math.min(gap, edgeLength * 0.22);
      const along = {
        x: (dx / edgeLength) * trim,
        y: (dy / edgeLength) * trim,
      };
      const start = {
        x: boundary[0][0] + inward.x + along.x,
        y: boundary[0][1] + inward.y + along.y,
      };
      const end = {
        x: boundary[1][0] + inward.x - along.x,
        y: boundary[1][1] + inward.y - along.y,
      };
      ctx.beginPath();
      ctx.moveTo(start.x, start.y);
      ctx.lineTo(end.x, end.y);
      ctx.stroke();
    }
  }
  ctx.restore();
}

function drawSupplySourceMarkers() {
  if ((!territoryViewEnabled && !ownershipEditingActive()) || !currentMap || !Array.isArray(currentMap.hexes)) {
    return;
  }

  ctx.save();
  for (const hex of currentMap.hexes) {
    if (!hex.supplySource) {
      continue;
    }
    const center = hexCenter(hex);
    const radius = 4.4 / viewport.scale;
    ctx.fillStyle = "#fffdf8";
    ctx.strokeStyle = "#1c1d1b";
    ctx.lineWidth = 1.4 / viewport.scale;
    ctx.beginPath();
    ctx.arc(center.x, center.y, radius * 1.35, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
    ctx.fillStyle = "#c93632";
    ctx.beginPath();
    ctx.moveTo(center.x, center.y - radius);
    ctx.lineTo(center.x + radius * 0.86, center.y + radius * 0.5);
    ctx.lineTo(center.x - radius * 0.86, center.y + radius * 0.5);
    ctx.closePath();
    ctx.fill();
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

const unitSpriteDrawers = {
  infantry: drawInfantrySprite,
  persian_infantry: drawInfantrySprite,
  jurchen_infantry: drawInfantrySprite,
  forest_warband: drawInfantrySprite,
  horde: drawHordeSprite,
  herd: drawHerdSprite,
  chinese_cavalry: drawChineseCavalrySprite,
  persian_cavalry: drawChineseCavalrySprite,
  jurchen_cavalry: drawChineseCavalrySprite,
  forest_raiders: drawChineseCavalrySprite,
  mongol_lancer: drawMongolLancerSprite,
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

function unitUsesSupply(unit) {
  if (!unit || !Number.isInteger(unit.owner)) {
    return false;
  }
  const archetype = factionArchetypeForOwner(unit.owner);
  return archetype === "chinese" || archetype === "persian" || archetype === "jurchen";
}

function unitOutOfSupply(unit) {
  if (!unitUsesSupply(unit)) {
    return false;
  }
  const hex = mapHexAt(unit);
  if (!hex || typeof hex.supplied !== "boolean") {
    return false;
  }
  return hex.owner !== unit.owner || !hex.supplied;
}

function drawUnitCounters(units) {
  if (!units || units.length === 0) {
    return;
  }

  ctx.save();
  const selectedUnitId = currentMap && currentMap.game && Number.isInteger(currentMap.game.selectedUnitId)
    ? currentMap.game.selectedUnitId
    : 0;
  const strategicMemberIds = activeStrategicAiMemberIds();
  const editorAiMemberIds = activeEditorAiMemberIds();
  for (const unit of units) {
    const faction = factions[unit.faction] || factions.mongol;
    const metrics = unitCounterMetrics();
    const center = hexCenter(unit);
    const counterWidth = metrics.width;
    const counterHeight = metrics.height;
    const x = center.x - counterWidth / 2;
    const y = center.y - counterHeight / 2;
    const dividerX = x + metrics.dividerOffset;
    const isStrategicMember = strategicMemberIds.has(unit.id);
    const isEditorAiMember = editorAiMemberIds.has(unit.id);

    roundedRectPath(x, y, counterWidth, counterHeight, metrics.cornerRadius);
    ctx.fillStyle = isEditorAiMember || isStrategicMember ? "#bfe8bf" : "#fffdf8";
    ctx.fill();
    if (unitOutOfSupply(unit)) {
      ctx.fillStyle = "rgba(201, 54, 50, 0.18)";
      ctx.fill();
    }
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

function aiAnimationUnitPositionMap() {
  const positions = new Map();
  if (!aiAnimationState || !Array.isArray(aiAnimationState.unitPositions)) {
    return positions;
  }
  for (const position of aiAnimationState.unitPositions) {
    if (Number.isInteger(position.id) && Number.isInteger(position.q) && Number.isInteger(position.r)) {
      positions.set(position.id, { q: position.q, r: position.r });
    }
  }
  return positions;
}

function unitsForMapRender() {
  if (!currentMap || !Array.isArray(currentMap.units)) {
    return [];
  }
  const positions = aiAnimationUnitPositionMap();
  if (positions.size === 0) {
    return currentMap.units;
  }
  return currentMap.units.map((unit) => {
    const position = positions.get(unit.id);
    return position ? { ...unit, q: position.q, r: position.r } : unit;
  });
}

function setAiAnimationUnitPosition(unitId, coord, attackTarget = null) {
  const positions = aiAnimationUnitPositionMap();
  if (Number.isInteger(unitId) && coord && Number.isInteger(coord.q) && Number.isInteger(coord.r)) {
    positions.set(unitId, { q: coord.q, r: coord.r });
  }
  aiAnimationState = {
    unitId,
    unitCoord: coord && Number.isInteger(coord.q) && Number.isInteger(coord.r) ? { q: coord.q, r: coord.r } : null,
    attackTarget,
    unitPositions: Array.from(positions.entries()).map(([id, position]) => ({ id, q: position.q, r: position.r })),
  };
}

function drawReachableHexes() {
  if (appMode !== "play" || aiAnimationInProgress) {
    return;
  }
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

function drawRaidableHexes() {
  if (appMode !== "play" || aiAnimationInProgress) {
    return;
  }
  const raids = legalRaids();
  if (raids.length === 0) {
    return;
  }

  ctx.save();
  for (const raid of raids) {
    const center = hexCenter(raid);
    const points = hexPoints(center.x, center.y, geometry.size * 0.82);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(187, 68, 36, 0.28)";
    ctx.fill();
    ctx.strokeStyle = "#9d301d";
    ctx.lineWidth = 1.8 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function drawEditorUnitMoveHexes() {
  if (!unitEditActive()) {
    return;
  }
  const moves = editorUnitMoveHexes();
  if (moves.length === 0) {
    return;
  }

  ctx.save();
  for (const move of moves) {
    const center = hexCenter(move);
    const points = hexPoints(center.x, center.y, geometry.size * 0.74);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(47, 111, 115, 0.28)";
    ctx.fill();
    ctx.strokeStyle = "#1f4f52";
    ctx.lineWidth = 1.4 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function drawAiEditorHighlights() {
  if (!aiEditingActive()) {
    return;
  }
  const group = activeAiGroup();
  if (!group) {
    return;
  }
  const memberIds = new Set(group.unitIds);
  ctx.save();
  for (const unit of currentMap.units || []) {
    if (!memberIds.has(unit.id)) {
      continue;
    }
    const center = hexCenter(unit);
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
    ctx.fillStyle = "rgba(191, 232, 191, 0.28)";
    ctx.fill();
    ctx.strokeStyle = "#4f8f4f";
    ctx.lineWidth = (aiMemberPickActive() ? 2.4 : 1.6) / viewport.scale;
    ctx.stroke();
  }
  const target = group.directive && group.directive.target;
  if (target && Number.isInteger(target.q) && Number.isInteger(target.r)) {
    const center = hexCenter(target);
    const points = hexPoints(center.x, center.y, geometry.size * 0.92);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(217, 35, 190, 0.2)";
    ctx.fill();
    ctx.strokeStyle = "#d923be";
    ctx.lineWidth = 4.2 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function mapHexAt(coord) {
  if (!currentMap || !coord || !Number.isInteger(coord.q) || !Number.isInteger(coord.r)) {
    return null;
  }
  return (currentMap.hexes || []).find((hex) => hex.q === coord.q && hex.r === coord.r) || null;
}

function strategicAiTargetHex(group) {
  if (!group || !currentMap) {
    return null;
  }
  const directive = group.directive || {};
  if (directive.type !== "capture_hex" && directive.type !== "defend_hex" && directive.type !== "hold_hex") {
    return null;
  }
  return mapHexAt(directive.target);
}

function activeStrategicAiMemberIds() {
  const group = appMode === "play" ? activeStrategicAiGroup() : null;
  return new Set(group ? group.unitIds : []);
}

function activeEditorAiMemberIds() {
  const group = aiEditingActive() ? activeAiGroup() : null;
  return new Set(group ? group.unitIds : []);
}

function drawStrategicAiHighlights() {
  if (appMode !== "play") {
    return;
  }
  const group = activeStrategicAiGroup();
  if (!group) {
    return;
  }

  ctx.save();
  const target = strategicAiTargetHex(group);
  if (target) {
    const center = hexCenter(target);
    const points = hexPoints(center.x, center.y, geometry.size * 0.92);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(217, 35, 190, 0.2)";
    ctx.fill();
    ctx.strokeStyle = "#d923be";
    ctx.lineWidth = 4.2 / viewport.scale;
    ctx.stroke();
  }
  ctx.restore();
}

function drawAiTurnAnimationHighlights() {
  if (!aiAnimationState) {
    return;
  }
  ctx.save();
  if (aiAnimationState.unitCoord) {
    const center = hexCenter(aiAnimationState.unitCoord);
    const points = hexPoints(center.x, center.y, geometry.size * 0.88);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(52, 173, 202, 0.16)";
    ctx.fill();
    ctx.strokeStyle = "#34adca";
    ctx.lineWidth = 3 / viewport.scale;
    ctx.stroke();
  }
  if (aiAnimationState.attackTarget) {
    const center = hexCenter(aiAnimationState.attackTarget);
    const points = hexPoints(center.x, center.y, geometry.size * 0.9);
    ctx.beginPath();
    points.forEach(([x, y], index) => {
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.closePath();
    ctx.fillStyle = "rgba(201, 54, 50, 0.36)";
    ctx.fill();
    ctx.strokeStyle = "#c93632";
    ctx.lineWidth = 2.8 / viewport.scale;
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

function unitAdjacentToEnemy(unit) {
  if (!unit || !currentMap || !Array.isArray(currentMap.units)) {
    return false;
  }
  const neighbors = Array.from({ length: 6 }, (_, direction) => neighborInDirection(unit, direction));
  return currentMap.units.some((candidate) => (
    candidate
    && candidate.id !== unit.id
    && candidate.owner !== unit.owner
    && neighbors.some((neighbor) => neighbor.q === candidate.q && neighbor.r === candidate.r)
  ));
}

function contextForPointer(event) {
  const point = panelToWorld(event);
  const rect = mapPanel.getBoundingClientRect();
  const unit = findUnitAtPoint(point);
  const hordeCanUseResources = Boolean(
    unit
    && unit.kind === "horde"
    && unit.owner === activeOwner()
    && unit.productionState !== "building"
    && !unit.moveDone
    && !unit.movedThisTurn
      && !unitAdjacentToEnemy(unit)
  );
  return {
    point,
    panelX: event.clientX - rect.left,
    panelY: event.clientY - rect.top,
    hex: findNearestHex(point),
    unit,
    selected: selectedUnit(),
    actionAvailability: {
      detachHerd: Boolean(hordeCanUseResources && Number.isInteger(unit.horses) && unit.horses > 0),
      createHorseArchers: Boolean(
        hordeCanUseResources
      ),
      createMongolLancers: Boolean(
        hordeCanUseResources
        && unit.faction === "mongol"
      ),
    },
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

async function showContextMenu(event) {
  const requestId = contextMenuRequestId + 1;
  contextMenuRequestId = requestId;
  activeContextMenu = null;
  contextMenu.hidden = true;
  contextMenu.replaceChildren();

  if (appMode !== "play" || !currentMap) {
    return false;
  }

  await playModeEngineSync;
  if (requestId !== contextMenuRequestId || appMode !== "play" || !currentMap) {
    return false;
  }

  const context = contextForPointer(event);
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
  selectedHexCoord = { q: coord.q, r: coord.r };
  const payload = await postUndoableGameCommand({
    type: "detach_herd",
    unitId: detachHerdPlacement.unitId,
    horses: detachHerdPlacement.horses,
    to: { q: coord.q, r: coord.r },
  });
  detachHerdPlacement = null;
  applyEnginePatch(payload, "detach_herd");
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

async function startHordeUnitProduction(context, commandType) {
  if (!context.unit || context.unit.kind !== "horde") {
    return false;
  }
  cancelDetachHerdPlacement();
  cancelCreateUnitPlacement();
  hideDetachHerdAmount();
  try {
    const payload = await postUndoableGameCommand({
      type: commandType,
      unitId: context.unit.id,
    });
    applyEnginePatch(payload, commandType);
    syncModeControls();
    drawMap();
    return true;
  } catch (error) {
    window.alert(error.message);
    return false;
  }
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
  selectedHexCoord = { q: coord.q, r: coord.r };
  const commandType = createUnitPlacement.createCommandType;
  const payload = await postUndoableGameCommand({
    type: commandType,
    unitId: createUnitPlacement.unitId,
    to: { q: coord.q, r: coord.r },
  });
  createUnitPlacement = null;
  applyEnginePatch(payload, commandType);
  syncModeControls();
  drawMap();
  return true;
}

async function selectUnit(unit) {
  if (!unit) {
    return false;
  }
  const requestId = unitSelectionRequestId + 1;
  unitSelectionRequestId = requestId;
  unitSelectionInProgress = true;
  hideCombatPreview();
  selectedHexCoord = { q: unit.q, r: unit.r };
  syncModeControls();
  drawMap();
  try {
    await playModeEngineSync;
    if (requestId !== unitSelectionRequestId) {
      return false;
    }
    const syncedUnit = currentMap && Array.isArray(currentMap.units)
      ? currentMap.units.find((candidate) => candidate.id === unit.id)
      : null;
    if (!syncedUnit) {
      return false;
    }
    unit = syncedUnit;
    selectedHexCoord = { q: unit.q, r: unit.r };
    syncModeControls();
    drawMap();
    const payload = await postAppCommand({
      type: "select_unit",
      unitId: unit.id,
    });
    if (requestId !== unitSelectionRequestId) {
      return false;
    }
    applyEnginePatch(payload, "select_unit");
    return true;
  } finally {
    if (requestId === unitSelectionRequestId) {
      unitSelectionInProgress = false;
      syncModeControls();
      drawMap();
    }
  }
}

async function cycleActiveFactionUnit(direction) {
  if (appMode !== "play" || aiAnimationInProgress || unitSelectionInProgress) {
    return false;
  }
  const units = activeFactionUnits();
  if (units.length === 0) {
    return false;
  }
  const selected = selectedUnit() || (selectedHexCoord && currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.find((unit) => unit.q === selectedHexCoord.q && unit.r === selectedHexCoord.r) || null
    : null);
  const selectedIndex = selected ? units.findIndex((unit) => unit.id === selected.id) : -1;
  const step = direction < 0 ? -1 : 1;
  const nextIndex = selectedIndex < 0
    ? (step < 0 ? units.length - 1 : 0)
    : (selectedIndex + step + units.length) % units.length;
  return selectUnit(units[nextIndex]);
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
  applyEnginePatch(payload, "set_unit_stance");
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
  selectedHexCoord = { q: coord.q, r: coord.r };
  const payload = await postUndoableGameCommand({
    type: "move_unit",
    unitId: unit.id,
    to: { q: coord.q, r: coord.r },
  });
  applyEnginePatch(payload, "move_unit");
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
  selectedHexCoord = { q: defender.q, r: defender.r };
  const payload = await postUndoableGameCommand({
    type: "attack_unit",
    attackerId: attacker.id,
    defenderId: defender.id,
  });
  applyEnginePatch(payload, "attack_unit");
  syncModeControls();
  drawMap();
  return true;
}

async function raidSelectedHex(hex) {
  const unit = selectedUnit();
  if (!unit || !hex || !legalRaidAt(hex)) {
    return false;
  }
  hideCombatPreview();
  selectedHexCoord = { q: hex.q, r: hex.r };
  const payload = await postUndoableGameCommand({
    type: "raid_hex",
    unitId: unit.id,
    target: { q: hex.q, r: hex.r },
  });
  applyEnginePatch(payload, "raid_hex");
  syncModeControls();
  drawMap();
  return true;
}

async function animateAiPatch(payload) {
  const steps = normalizeAiAnimation(payload && payload.aiAnimation);
  if (steps.length === 0 || !currentMap || !Array.isArray(currentMap.units)) {
    applyEnginePatch(payload, "ai_patch");
    syncModeControls();
    drawMap();
    return;
  }

  aiAnimationInProgress = true;
  syncModeControls();
  try {
    for (const step of steps) {
      await focusAiAnimationStep(step);
      setAiAnimationUnitPosition(step.unitId, step.from);
      drawMap();
      await sleep(260);

      if (step.from.q !== step.to.q || step.from.r !== step.to.r) {
        setAiAnimationUnitPosition(step.unitId, step.to);
        drawMap();
        await sleep(320);
        if (applyAiStepHexSnapshot(step)) {
          drawMap();
          await sleep(180);
        }
      } else {
        await sleep(160);
      }

      const attackEvents = step.attackEvents.length > 0
        ? step.attackEvents
        : step.attacks.map((target) => ({
          target,
          defenderId: 0,
          defenderFrom: target,
          defenderTo: target,
          defenderMoved: false,
          attackerTo: step.to,
          attackerMoved: false,
        }));
      for (const event of attackEvents) {
        setAiAnimationUnitPosition(step.unitId, step.to, event.target);
        drawMap();
        await sleep(320);

        if (event.defenderMoved) {
          setAiAnimationUnitPosition(event.defenderId, event.defenderTo);
          drawMap();
          await sleep(320);
        }

        if (event.attackerMoved) {
          setAiAnimationUnitPosition(step.unitId, event.attackerTo);
          drawMap();
          await sleep(320);
        }
      }

      applyAiStepHexSnapshot(step);
      setAiAnimationUnitPosition(step.unitId, step.to);
      drawMap();
      await sleep(160);
    }
    applyEnginePatch(payload, "ai_patch");
    syncModeControls();
    drawMap();
  } finally {
    aiAnimationState = null;
    aiAnimationInProgress = false;
    syncModeControls();
    drawMap();
  }
}

async function advanceTurn() {
  hideCombatPreview();
  try {
    const payload = await postUndoableGameCommand({ type: "end_turn" });
    applyEnginePatch(payload, "end_turn");
    syncModeControls();
    drawMap();
    await stepAiTurnsUntilHuman();
    await validateEngineProjection("end_turn");
  } catch (error) {
    aiAnimationState = null;
    aiAnimationInProgress = false;
    window.alert(error.message);
    syncModeControls();
    drawMap();
  }
}

async function stepAiTurnsUntilHuman() {
  let guard = 0;
  while (appMode === "play" && activeFactionAiControlled() && guard < 128) {
    guard += 1;
    await animateAiPatch(await postUndoableGameCommand({ type: "step_ai_turn" }));
  }
  if (guard >= 128) {
    throw new Error("AI turn did not complete after 128 steps.");
  }
}

async function executeNextAiGroupTurn() {
  hideCombatPreview();
  const groups = strategicAiGroups();
  if (groups.length === 0) {
    syncModeControls();
    return;
  }
  const group = groups[nextStrategicAiGroupIndex % groups.length];
  nextStrategicAiGroupIndex = (nextStrategicAiGroupIndex + 1) % groups.length;
  activeStrategicAiGroupId = group.id;
  try {
    await animateAiPatch(await postUndoableGameCommand({ type: "execute_ai_group", groupId: group.id }));
  } catch (error) {
    aiAnimationState = null;
    aiAnimationInProgress = false;
    window.alert(error.message);
    syncModeControls();
    drawMap();
  }
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
      applyEnginePatch(payload, "undo");
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
  if (!currentMap || (appMode === "play" && playSurfaceMode === "diplomacy")) {
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
  drawTerritoryOutlines();
  drawSupplySourceMarkers();
  drawMapMarkers(currentMap.river_sources, terrainStyles.river.source, 5);
  drawMapMarkers(currentMap.merge_points, terrainStyles.river.merge, 5);
  drawMapMarkers(currentMap.river_destinations, terrainStyles.river.destination, 5);
  drawReachableHexes();
  drawRaidableHexes();
  drawEditorUnitMoveHexes();
  drawAiEditorHighlights();
  drawStrategicAiHighlights();
  drawAiTurnAnimationHighlights();
  drawDetachDeploymentHexes();
  drawSelectedHex();
  if (unitsViewEnabled) {
    drawUnitCounters(unitsForMapRender());
  }

  ctx.restore();
}

function syncEditorControls() {
  syncScenarioToolControls();
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
      editorEdgeFeatureSelect.value = "terrain";
      setScenarioTool("terrain");
      syncEditorControls();
    });
    terrainPalette.appendChild(button);
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

function wallEdgeKey(edge) {
  if (!edge || !edge.a || !edge.b) {
    return "";
  }
  return edgeKey(canonicalEdge(edge.a, edge.b));
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

function crossingEdgeKey(crossing) {
  return crossing && crossing.edge ? wallEdgeKey(crossing.edge) : "";
}

function toggleCrossingEdge(edge, kind) {
  currentMap.crossings = Array.isArray(currentMap.crossings) ? currentMap.crossings : [];
  const key = edgeKey(edge);
  const index = currentMap.crossings.findIndex((crossing) => (
    crossing.kind === kind && crossingEdgeKey(crossing) === key
  ));
  if (index >= 0) {
    currentMap.crossings.splice(index, 1);
    return;
  }

  const nextId = currentMap.crossings.reduce((maxId, crossing) => (
    Math.max(maxId, Number.isInteger(crossing.id) ? crossing.id : 0)
  ), 0) + 1;
  currentMap.crossings.push({
    id: nextId,
    kind,
    edge: { a: { ...edge.a }, b: { ...edge.b } },
  });
}

function editableGreatWall() {
  currentMap.walls = Array.isArray(currentMap.walls) ? currentMap.walls : [];
  let wall = currentMap.walls.find((candidate) => candidate.feature === "great_wall");
  if (!wall) {
    const nextId = currentMap.walls.reduce((maxId, candidate) => (
      Math.max(maxId, Number.isInteger(candidate.id) ? candidate.id : 0)
    ), 0) + 1;
    wall = {
      id: nextId,
      feature: "great_wall",
      edge_path: [],
    };
    currentMap.walls.push(wall);
  }
  wall.edge_path = Array.isArray(wall.edge_path) ? wall.edge_path : [];
  return wall;
}

function toggleWallEdge(edge) {
  const wall = editableGreatWall();
  const key = edgeKey(edge);
  const index = wall.edge_path.findIndex((existing) => wallEdgeKey(existing) === key);
  if (index >= 0) {
    wall.edge_path.splice(index, 1);
    currentMap.wall_gates = Array.isArray(currentMap.wall_gates)
      ? currentMap.wall_gates.filter((gate) => wallEdgeKey(gate.edge) !== key)
      : [];
    if (wall.edge_path.length === 0) {
      currentMap.walls = currentMap.walls.filter((candidate) => candidate !== wall);
    }
    return;
  }
  wall.edge_path.push({ a: { ...edge.a }, b: { ...edge.b } });
}

function toggleWallPass(edge) {
  currentMap.wall_gates = Array.isArray(currentMap.wall_gates) ? currentMap.wall_gates : [];
  const key = edgeKey(edge);
  const index = currentMap.wall_gates.findIndex((gate) => wallEdgeKey(gate.edge) === key);
  if (index >= 0) {
    currentMap.wall_gates.splice(index, 1);
    return;
  }

  const wall = editableGreatWall();
  if (!wall.edge_path.some((existing) => wallEdgeKey(existing) === key)) {
    wall.edge_path.push({ a: { ...edge.a }, b: { ...edge.b } });
  }
  const nextId = currentMap.wall_gates.reduce((maxId, gate) => (
    Math.max(maxId, Number.isInteger(gate.id) ? gate.id : 0)
  ), 0) + 1;
  currentMap.wall_gates.push({
    id: nextId,
    kind: "gate",
    edge: { a: { ...edge.a }, b: { ...edge.b } },
  });
}

function nextUnitId() {
  const units = currentMap && Array.isArray(currentMap.units) ? currentMap.units : [];
  return units.reduce((maxId, unit) => Math.max(maxId, Number.isInteger(unit.id) ? unit.id : 0), 0) + 1;
}

function makeEditorUnit(coord) {
  const kind = editorUnitTypeSelect.value;
  const factionKey = editorUnitSideSelect.value;
  if (!unitKindAllowedForFaction(kind, factionKey)) {
    throw new Error(`${unitKindLabelForKind(kind)} is not available to ${factions[factionKey]?.name || factionKey}.`);
  }
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
    horses: Number.isInteger(defaults.horses) ? defaults.horses : 0,
    projectsZoc: Boolean(defaults.projectsZoc),
    respectsZoc: Boolean(defaults.respectsZoc),
  };
}

function toggleEditorUnit(hex) {
  selectedHexCoord = { q: hex.q, r: hex.r };
  currentMap.units = Array.isArray(currentMap.units) ? currentMap.units : [];
  const removedUnits = currentMap.units.filter((unit) => unit.q === hex.q && unit.r === hex.r);
  if (removedUnits.length > 0) {
    currentMap.units = currentMap.units.filter((unit) => !(unit.q === hex.q && unit.r === hex.r));
    const removedIds = new Set(removedUnits.map((unit) => unit.id));
    if (currentMap.game && removedIds.has(currentMap.game.selectedUnitId)) {
      currentMap.game.selectedUnitId = 0;
    }
    if (removedIds.has(editorSelectedUnitId)) {
      editorSelectedUnitId = 0;
    }
    return;
  }

  const unit = normalizeUnit(makeEditorUnit(hex), currentMap.units.length);
  currentMap.units.push(unit);
  editorSelectedUnitId = unit.id;
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.selectedUnitId = unit.id;
}

function editorSelectedUnit() {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.find((unit) => unit.id === editorSelectedUnitId) || null
    : null;
}

function unitAtCoord(coord, ignoreUnitId = 0) {
  return currentMap && Array.isArray(currentMap.units)
    ? currentMap.units.find((unit) => unit.id !== ignoreUnitId && unit.q === coord.q && unit.r === coord.r) || null
    : null;
}

function editorHexPassable(hex) {
  return Boolean(hex && hex.terrain !== "lake" && hex.terrain !== "none");
}

function editorUnitMoveHexes() {
  const unit = editorSelectedUnit();
  if (!currentMap || !unit || !Array.isArray(currentMap.hexes)) {
    return [];
  }
  return currentMap.hexes.filter((hex) => (
    editorHexPassable(hex)
    && !(hex.q === unit.q && hex.r === unit.r)
    && !unitAtCoord(hex, unit.id)
  ));
}

function editorUnitMoveAt(coord) {
  return editorUnitMoveHexes().find((hex) => hex.q === coord.q && hex.r === coord.r) || null;
}

function selectEditorUnit(unit) {
  editorSelectedUnitId = unit ? unit.id : 0;
  selectedHexCoord = unit ? { q: unit.q, r: unit.r } : selectedHexCoord;
  currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
  currentMap.game.selectedUnitId = editorSelectedUnitId;
  syncModeControls();
  drawMap();
}

function handleUnitEditClick(event) {
  if (!currentMap) {
    return;
  }
  const point = panelToWorld(event);
  const clickedUnit = findUnitAtPoint(point);
  if (clickedUnit) {
    selectEditorUnit(clickedUnit);
    return;
  }
  const hex = findNearestHex(point);
  const unit = editorSelectedUnit();
  if (hex && unit && editorUnitMoveAt(hex)) {
    recordLocalUndo();
    unit.q = hex.q;
    unit.r = hex.r;
    selectedHexCoord = { q: hex.q, r: hex.r };
    unit.remainingMove = unit.move;
    currentMap.game.selectedUnitId = unit.id;
    ensureGameMeta();
    syncModeControls();
    drawMap();
    void syncScenarioEditorToEngine();
    return;
  }
  if (hex) {
    selectedHexCoord = { q: hex.q, r: hex.r };
  }
  selectEditorUnit(null);
}

function handleAiMemberPickClick(event) {
  const group = activeAiGroup();
  if (!currentMap || !group) {
    return;
  }
  const unit = findUnitAtPoint(panelToWorld(event));
  if (!unit || unit.owner !== group.owner) {
    return;
  }
  recordLocalUndo();
  const unitIds = new Set(group.unitIds);
  if (unitIds.has(unit.id)) {
    unitIds.delete(unit.id);
  } else {
    unitIds.add(unit.id);
  }
  group.unitIds = Array.from(unitIds);
  editorSelectedUnitId = unit.id;
  selectedHexCoord = { q: unit.q, r: unit.r };
  currentMap.game.selectedUnitId = unit.id;
  ensureGameMeta();
  syncModeControls();
  drawMap();
  void syncAiGroupsToEngine();
}

function handleAiTargetPickClick(event) {
  const group = activeAiGroup();
  if (!currentMap || !group) {
    return;
  }
  const hex = findNearestHex(panelToWorld(event));
  if (!hex) {
    return;
  }
  recordLocalUndo();
  selectedHexCoord = { q: hex.q, r: hex.r };
  group.directive.target = { q: hex.q, r: hex.r };
  ensureGameMeta();
  syncModeControls();
  drawMap();
  void syncAiGroupsToEngine();
}

function toggleHexTerrain(hex, terrain) {
  const key = coordKey(hex);
  if (hex.terrain === terrain) {
    hex.terrain = terrainUndo.get(key) || "grassland";
    terrainUndo.delete(key);
    hex.labels = editorLabelsForTerrain(hex.terrain);
    hex.pasture = defaultPastureForTerrain(hex.terrain);
    return;
  }

  if (!terrainUndo.has(key)) {
    terrainUndo.set(key, hex.terrain);
  }
  hex.terrain = terrain;
  hex.labels = editorLabelsForTerrain(hex.terrain);
  hex.pasture = defaultPastureForTerrain(hex.terrain);
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

function paintHexOwnership(hex, owner) {
  if (!hex || hex.owner === owner) {
    return false;
  }
  hex.owner = owner;
  return true;
}

function paintHexSupplySource(hex, supplySource) {
  if (!hex || hex.supplySource === supplySource) {
    return false;
  }
  hex.supplySource = supplySource;
  return true;
}

function paintSupplySourceAtPointer(event) {
  if (!ownershipEditingActive() || !currentMap) {
    return;
  }
  const hex = findNearestHex(panelToWorld(event));
  if (!hex) {
    return;
  }
  const supplySource = !event.shiftKey;
  const key = `supply:${coordKey(hex)}`;
  if (paintStrokeKeys.has(key)) {
    return;
  }
  paintStrokeKeys.add(key);
  selectedHexCoord = { q: hex.q, r: hex.r };
  if (hex.supplySource === supplySource) {
    drawMap();
    return;
  }
  if (!paintUndoRecorded) {
    recordLocalUndo();
    paintUndoRecorded = true;
  }
  paintHexSupplySource(hex, supplySource);
  drawMap();
  void syncScenarioEditorToEngine();
}

function paintOwnershipAtPointer(event) {
  if (!ownershipEditingActive() || !currentMap) {
    return;
  }
  if (ownershipTool === "source") {
    paintSupplySourceAtPointer(event);
    return;
  }
  const hex = findNearestHex(panelToWorld(event));
  if (!hex) {
    return;
  }
  const owner = event.shiftKey ? factions.neutral.owner : selectedOwnershipOwner;
  const key = `owner:${coordKey(hex)}`;
  if (paintStrokeKeys.has(key)) {
    return;
  }
  paintStrokeKeys.add(key);
  if (hex.owner === owner) {
    selectedHexCoord = { q: hex.q, r: hex.r };
    drawMap();
    return;
  }
  if (!paintUndoRecorded) {
    recordLocalUndo();
    paintUndoRecorded = true;
  }
  selectedHexCoord = { q: hex.q, r: hex.r };
  paintHexOwnership(hex, owner);
  drawMap();
  void syncScenarioEditorToEngine();
}

function paintAtPointer(event) {
  if (!terrainEditingActive() || !currentMap) {
    return;
  }

  const point = panelToWorld(event);
  if (["river", "road", "bridge", "wall", "wall_pass"].includes(editorEdgeFeatureSelect.value)) {
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
    } else if (editorEdgeFeatureSelect.value === "bridge") {
      toggleCrossingEdge(edge, "bridge");
    } else if (editorEdgeFeatureSelect.value === "wall") {
      toggleWallEdge(edge);
    } else if (editorEdgeFeatureSelect.value === "wall_pass") {
      toggleWallPass(edge);
    } else {
      if (event.shiftKey) {
        setRiverEdge(edge, false);
      } else {
        toggleRiverEdge(edge);
      }
    }
    refreshDerivedTopology();
    drawMap();
    void syncScenarioEditorToEngine();
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
  selectedHexCoord = { q: hex.q, r: hex.r };
  if (!paintUndoRecorded) {
    recordLocalUndo();
    paintUndoRecorded = true;
  }
  toggleHexTerrain(hex, terrain);
  refreshDerivedTopology();
  drawMap();
  void syncScenarioEditorToEngine();
}

function placeEditorUnitAtPointer(event) {
  if (!unitPlaceActive() || !currentMap) {
    return;
  }
  const point = panelToWorld(event);
  const hex = findNearestHex(point);
  if (!hex) {
    return;
  }
  if (!editorHexPassable(hex)) {
    window.alert("Units cannot be placed on blocked terrain.");
    return;
  }
  recordLocalUndo();
  try {
    toggleEditorUnit(hex);
  } catch (error) {
    window.alert(error.message);
    return;
  }
  syncModeControls();
  drawMap();
  void syncScenarioEditorToEngine();
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
    currentMap.name = scenarioName();
    currentMap.hexes = Array.isArray(currentMap.hexes)
      ? currentMap.hexes.map(normalizeMapHex).filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r))
      : [];
    currentMap.factions = selectedScenarioFactionsFromControls();
    currentMap.units = Array.isArray(currentMap.units) ? currentMap.units : [];
    currentMap.game = currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {};
    currentMap.game.foodConsumption = foodConsumptionEnabled();
    selectedHexCoord = null;
    terrainUndo = new Map();
    clearUndoHistory();
    ensureGameMeta();
    syncEngineProjectionMeta(currentMap, "new_game");
    refreshDerivedTopology();
    syncModeControls();
    fitMap();
    void syncScenarioEditorToEngine();
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
      hexes.push(normalizeMapHex({ q, r, terrain: "grassland", labels: ["base_steppe"] }));
    }
  }

  currentMap = {
    schema: "steppe-scenario.v1",
    name: scenarioName(),
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
    factions: selectedScenarioFactionsFromControls(),
    units: [],
    metadata: {
      generator: typeof options.generator === "string" ? options.generator : "new-scenario",
      terrain_types: editorTerrains.map((terrain) => terrain.key),
      hex_label_model: "base-plus-generation-role.v1",
    },
  };
  selectedHexCoord = null;
  terrainUndo = new Map();
  clearUndoHistory();
  currentMap.game = { foodConsumption: foodConsumptionEnabled() };
  ensureGameMeta();
  refreshDerivedTopology();
  syncModeControls();
  fitMap();
  void syncScenarioEditorToEngine();
}

async function createDefaultPlayScenario() {
  try {
    currentMap = await postAppCommand({
      type: "new_game",
      width: 80,
      height: 40,
      factions: factionCount,
    });
    currentMap.hexes = Array.isArray(currentMap.hexes)
      ? currentMap.hexes.map(normalizeMapHex).filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r))
      : [];
    currentMap.units = Array.isArray(currentMap.units) ? currentMap.units.map(normalizeUnit) : [];
    selectedHexCoord = null;
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

async function loadScenarioByName(filename) {
  const payload = await scenarioApi(`/api/scenarios/load?name=${encodeURIComponent(filename)}`);
  resetScenarioEditorSyncQueue();
  currentMap = normalizeLoadedMap(payload.scenario);
  selectedHexCoord = null;
  terrainUndo = new Map();
  clearUndoHistory();
  ensureGameMeta();
  syncEngineProjectionMeta(currentMap, "load_scenario");
  widthInput.value = currentMap.width;
  heightInput.value = currentMap.height;
  syncModeControls();
  fitMap();
}

async function initializeApp() {
  try {
    await loadUnitTypeDefaults();
    await loadScenarioByName("main1.json");
  } catch (error) {
    window.alert(error.message);
  }
}

function normalizeLoadedMap(map) {
  if (!map || typeof map !== "object") {
    throw new Error("Loaded file is not a scenario.");
  }
  if (!Number.isInteger(map.width) || !Number.isInteger(map.height) || map.width < 1 || map.height < 1) {
    throw new Error("Loaded scenario has invalid dimensions.");
  }
  if (!Array.isArray(map.hexes)) {
    throw new Error("Loaded scenario is missing hex terrain.");
  }

  return refreshDerivedTopology({
    schema: typeof map.schema === "string" ? map.schema : "steppe-scenario.v1",
    name: typeof map.name === "string" && map.name.trim() ? map.name : "Untitled Scenario",
    seed: Number.isInteger(map.seed) ? map.seed : 0,
    width: map.width,
    height: map.height,
    hexes: map.hexes.map(normalizeMapHex)
      .filter((hex) => Number.isInteger(hex.q) && Number.isInteger(hex.r)),
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
    factions: normalizeScenarioFactions(map.factions, map.units),
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
  return `steppe-scenario-${width}x${height}-${seed}.json`;
}

function mapFilePickerTypes() {
  return [{
    description: "Steppe scenario JSON",
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

function openScenarioDirectoryDb() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(scenarioDirectoryDbName, 1);
    request.onupgradeneeded = () => {
      request.result.createObjectStore(scenarioDirectoryStoreName);
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
  });
}

async function readStoredScenarioDirectoryHandle() {
  if (!window.indexedDB) {
    return null;
  }
  const db = await openScenarioDirectoryDb();
  return new Promise((resolve, reject) => {
    const transaction = db.transaction(scenarioDirectoryStoreName, "readonly");
    const store = transaction.objectStore(scenarioDirectoryStoreName);
    const request = store.get(scenarioDirectoryHandleKey);
    request.onsuccess = () => resolve(request.result || null);
    request.onerror = () => reject(request.error);
    transaction.oncomplete = () => db.close();
    transaction.onerror = () => {
      db.close();
      reject(transaction.error);
    };
  });
}

async function storeScenarioDirectoryHandle(handle) {
  if (!window.indexedDB || !handle) {
    return;
  }
  const db = await openScenarioDirectoryDb();
  await new Promise((resolve, reject) => {
    const transaction = db.transaction(scenarioDirectoryStoreName, "readwrite");
    const store = transaction.objectStore(scenarioDirectoryStoreName);
    store.put(handle, scenarioDirectoryHandleKey);
    transaction.oncomplete = () => resolve();
    transaction.onerror = () => reject(transaction.error);
  });
  db.close();
}

async function hasScenarioDirectoryPermission(handle, mode) {
  if (!handle || !handle.queryPermission || !handle.requestPermission) {
    return false;
  }
  const options = { mode };
  if (await handle.queryPermission(options) === "granted") {
    return true;
  }
  return await handle.requestPermission(options) === "granted";
}

async function scenarioDirectoryHandle(mode) {
  if (!window.showDirectoryPicker) {
    return null;
  }
  const storedHandle = await readStoredScenarioDirectoryHandle().catch(() => null);
  if (storedHandle && await hasScenarioDirectoryPermission(storedHandle, mode)) {
    return storedHandle;
  }
  const pickedHandle = await window.showDirectoryPicker({
    id: "steppe-scenarios-directory",
    mode,
    startIn: "documents",
  });
  await storeScenarioDirectoryHandle(pickedHandle);
  return pickedHandle;
}

function scenarioSnapshot() {
  ensureGameMeta();
  refreshDerivedTopology();
  return JSON.parse(JSON.stringify({
    ...currentMap,
    schema: "steppe-scenario.v1",
    name: scenarioName(),
    factions: normalizeScenarioFactions(currentMap.factions, currentMap.units),
    units: Array.isArray(currentMap.units) ? currentMap.units.map(normalizeUnit) : [],
    game: currentMap.game && typeof currentMap.game === "object" ? currentMap.game : {},
    metadata: {
      ...(currentMap.metadata && typeof currentMap.metadata === "object" ? currentMap.metadata : {}),
      snapshot: "full-scenario",
    },
  }));
}

function mapBlob() {
  return new Blob([`${JSON.stringify(scenarioSnapshot(), null, 2)}\n`], { type: "application/json" });
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
    window.alert("No scenario is loaded.");
    return;
  }

  if (!window.showSaveFilePicker) {
    downloadMapFallback();
    return;
  }

  try {
    const directoryHandle = await scenarioDirectoryHandle("readwrite");
    const handle = await window.showSaveFilePicker({
      id: "steppe-scenario",
      startIn: directoryHandle || "documents",
      suggestedName: defaultMapFilename(),
      types: mapFilePickerTypes(),
    });
    const writable = await handle.createWritable();
    await writable.write(mapBlob());
    await writable.close();
  } catch (error) {
    if (error.name === "AbortError") {
      return;
    }
    window.alert(error.message);
  }
}

async function scenarioApi(pathname, options = {}) {
  const response = await fetch(pathname, options);
  const payload = await response.json();
  if (!response.ok) {
    throw new Error(payload.error || "scenario file operation failed");
  }
  return payload;
}

async function listScenarioFiles() {
  const payload = await scenarioApi("/api/scenarios");
  return Array.isArray(payload.files) ? payload.files : [];
}

function renderScenarioFileList(files) {
  scenarioFileList.replaceChildren();
  if (!files.length) {
    const empty = document.createElement("div");
    empty.className = "scenario-file-empty";
    empty.textContent = "No scenarios";
    scenarioFileList.appendChild(empty);
    return;
  }
  for (const filename of files) {
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = filename;
    button.classList.toggle("is-selected", filename === selectedScenarioFile);
    button.addEventListener("click", () => {
      selectedScenarioFile = filename;
      scenarioFileNameInput.value = filename;
      renderScenarioFileList(files);
    });
    button.addEventListener("dblclick", () => {
      selectedScenarioFile = filename;
      scenarioFileNameInput.value = filename;
      confirmScenarioFileDialog().catch((error) => window.alert(error.message));
    });
    scenarioFileList.appendChild(button);
  }
}

async function openScenarioFileDialog(mode) {
  scenarioFileMode = mode;
  selectedScenarioFile = "";
  scenarioFileTitle.textContent = mode === "save" ? "Save Scenario" : "Load Scenario";
  scenarioFileConfirm.textContent = mode === "save" ? "Save" : "Load";
  scenarioFileNameField.hidden = mode !== "save";
  scenarioFileNameInput.value = defaultMapFilename();
  scenarioFileList.replaceChildren();
  const loading = document.createElement("div");
  loading.className = "scenario-file-empty";
  loading.textContent = "Loading...";
  scenarioFileList.appendChild(loading);
  if (scenarioFileDialog.showModal) {
    scenarioFileDialog.showModal();
  } else {
    scenarioFileDialog.setAttribute("open", "");
  }
  try {
    renderScenarioFileList(await listScenarioFiles());
  } catch (error) {
    if (mode === "save" && window.showSaveFilePicker) {
      scenarioFileDialog.close();
      const handle = await window.showSaveFilePicker({
        suggestedName: defaultMapFilename(),
        types: mapFilePickerTypes(),
      });
      const writable = await handle.createWritable();
      await writable.write(mapBlob());
      await writable.close();
      return;
    }
    if (mode === "save") {
      scenarioFileDialog.close();
      downloadMapFallback();
      return;
    }
    scenarioFileDialog.close();
    loadFileInput.click();
  }
}

async function confirmScenarioFileDialog() {
  if (scenarioFileMode === "save") {
    const filename = scenarioFileNameInput.value.trim() || defaultMapFilename();
    await scenarioApi("/api/scenarios/save", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name: filename, scenario: scenarioSnapshot() }),
    });
    scenarioFileDialog.close();
    return;
  }
  const filename = selectedScenarioFile || scenarioFileNameInput.value.trim();
  if (!filename) {
    return;
  }
  await loadScenarioByName(filename);
  scenarioFileDialog.close();
}

async function loadMapText(text) {
  try {
    if (appInitialization) {
      await appInitialization;
    }
    resetScenarioEditorSyncQueue();
    currentMap = normalizeLoadedMap(JSON.parse(text));
    selectedHexCoord = null;
    terrainUndo = new Map();
    clearUndoHistory();
    ensureGameMeta();
    syncEngineProjectionMeta(currentMap, "load_map_text");
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
    const directoryHandle = await scenarioDirectoryHandle("read");
    const [handle] = await window.showOpenFilePicker({
      id: "steppe-scenario",
      startIn: directoryHandle || "documents",
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

scenarioModeButton.addEventListener("click", enterScenarioMode);
playModeButton.addEventListener("click", () => {
  enterPlayMode().catch((error) => window.alert(error.message));
});
generateButton.addEventListener("click", generateMap);
blankMapButton.addEventListener("click", () => createBlankMap());
for (const button of scenarioToolButtons) {
  button.addEventListener("click", () => setScenarioTool(button.dataset.scenarioActivate));
}
for (const button of scenarioCollapseButtons) {
  button.addEventListener("click", (event) => {
    event.stopPropagation();
    toggleScenarioRegion(button.dataset.scenarioToggle);
  });
}
for (const button of unitToolButtons) {
  button.addEventListener("click", () => setUnitTool(button.dataset.unitTool));
}
for (const button of terrainEditModeButtons) {
  button.addEventListener("click", () => setTerrainEditMode(button.dataset.terrainEditMode));
}
for (const button of edgeFeatureButtons) {
  button.addEventListener("click", () => setEditorEdgeFeature(button.dataset.edgeFeature));
}
for (const button of ownershipToolButtons) {
  button.addEventListener("click", () => setOwnershipTool(button.dataset.ownershipTool));
}
if (ownershipFactionSelect) {
  ownershipFactionSelect.addEventListener("change", () => {
    const owner = Number.parseInt(ownershipFactionSelect.value, 10);
    selectedOwnershipOwner = Number.isInteger(owner) ? owner : factions.neutral.owner;
    setScenarioTool("ownership");
    syncOwnershipEditorControls();
    drawMap();
  });
}
if (addAiGroupButton) {
  addAiGroupButton.addEventListener("click", addAiGroup);
}
if (scenarioNameInput) {
  scenarioNameInput.addEventListener("input", updateScenarioNameFromInput);
}
if (foodConsumptionEnabledInput) {
  foodConsumptionEnabledInput.addEventListener("input", updateFoodConsumptionSettingFromInput);
  foodConsumptionEnabledInput.addEventListener("change", updateFoodConsumptionSettingFromInput);
}
for (const row of scenarioFactionRows) {
  for (const input of row.querySelectorAll("input")) {
    input.addEventListener("change", updateScenarioFactionsFromControls);
  }
}
editorEdgeFeatureSelect.addEventListener("change", () => {
  syncEditorControls();
  if (currentMap) {
    requestAnimationFrame(drawMap);
  }
});
editorUnitTypeSelect.addEventListener("change", syncEditorControls);
editorUnitSideSelect.addEventListener("change", () => {
  refreshEditorUnitTypeOptions();
  syncEditorControls();
});
unitNameInput.addEventListener("change", updateEditorUnitName);
unitTypeInput.addEventListener("change", updateEditorUnitType);
unitHpInput.addEventListener("change", updateEditorUnitHp);
unitReadinessInput.addEventListener("change", updateEditorUnitReadiness);
unitStanceInput.addEventListener("change", updateEditorUnitStance);
if (unitInspectorInfoToggle) {
  unitInspectorInfoToggle.addEventListener("click", () => {
    unitInspectorStatsVisible = !unitInspectorStatsVisible;
    syncUnitInspectorView();
  });
}
unitPopulationInput.addEventListener("change", () => updateEditorUnitResource("population", unitPopulationInput));
unitHorsesInput.addEventListener("change", () => updateEditorUnitResource("horses", unitHorsesInput));
saveButton.addEventListener("click", saveCurrentMap);
loadButton.addEventListener("click", chooseMapFile);
loadFileInput.addEventListener("change", () => loadMapFile(loadFileInput.files[0]));
scenarioFileCancel.addEventListener("click", () => scenarioFileDialog.close());
scenarioFileForm.addEventListener("submit", (event) => {
  event.preventDefault();
  confirmScenarioFileDialog().catch((error) => window.alert(error.message));
});
undoButton.addEventListener("click", () => {
  undoLastAction().catch((error) => window.alert(error.message));
});
if (endTurnButton) {
  endTurnButton.addEventListener("click", advanceTurn);
}
controlEndTurnButton.addEventListener("click", advanceTurn);
statusEndTurnButton.addEventListener("click", advanceTurn);
if (executeAiGroupButton) {
  executeAiGroupButton.addEventListener("click", executeNextAiGroupTurn);
}
if (prevUnitButton) {
  prevUnitButton.addEventListener("click", () => {
    cycleActiveFactionUnit(-1).catch((error) => window.alert(error.message));
  });
}
if (nextUnitButton) {
  nextUnitButton.addEventListener("click", () => {
    cycleActiveFactionUnit(1).catch((error) => window.alert(error.message));
  });
}
if (strategicAiToggleButton) {
  strategicAiToggleButton.addEventListener("click", toggleStrategicAiPanel);
}
if (strategicAiSelectNoneButton) {
  strategicAiSelectNoneButton.addEventListener("click", clearStrategicAiSelection);
}
if (mapAiToggleButton) {
  mapAiToggleButton.addEventListener("click", toggleStrategicAiPanel);
}
if (diplomacyToggleButton) {
  diplomacyToggleButton.addEventListener("click", toggleDiplomacySurface);
}
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
if (detailsToggleButton) {
  detailsToggleButton.addEventListener("click", toggleScenarioDetailsPanel);
}
if (scenarioResizeHandle) {
  scenarioResizeHandle.addEventListener("pointerdown", startScenarioPanelResize);
  scenarioResizeHandle.addEventListener("dblclick", resetScenarioPanelHeight);
  scenarioResizeHandle.addEventListener("keydown", (event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();
      nudgeScenarioPanelHeight(event.shiftKey ? -32 : -12);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();
      nudgeScenarioPanelHeight(event.shiftKey ? 32 : 12);
    } else if (event.key === "Home") {
      event.preventDefault();
      applyScenarioPanelHeight(minScenarioPanelHeight);
    } else if (event.key === "End") {
      event.preventDefault();
      applyScenarioPanelHeight(maxScenarioPanelHeight());
    } else if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      resetScenarioPanelHeight();
    }
  });
}
pastureViewButton.addEventListener("click", () => {
  pastureViewEnabled = !pastureViewEnabled;
  syncPastureViewButton();
  drawMap();
});
if (territoryViewButton) {
  territoryViewButton.addEventListener("click", () => {
    territoryViewEnabled = !territoryViewEnabled;
    syncTerritoryViewButton();
    drawMap();
  });
}
if (unitsViewButton) {
  unitsViewButton.addEventListener("click", () => {
    unitsViewEnabled = !unitsViewEnabled;
    syncUnitsViewButton();
    drawMap();
  });
}
if (coordinatesViewButton) {
  coordinatesViewButton.addEventListener("click", () => {
    coordinateLabelsEnabled = !coordinateLabelsEnabled;
    syncCoordinateViewButton();
    drawMap();
  });
}

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
    if (appMode === "play") {
      await showContextMenu(event);
    }
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
    if (hex && await raidSelectedHex(hex)) {
      event.preventDefault();
      return;
    }
    if (hex && await moveSelectedUnitTo(hex)) {
      event.preventDefault();
      return;
    }
    if (hex) {
      event.preventDefault();
      selectHex(hex);
      return;
    }
  }
  if ((terrainEditingActive() || ownershipEditingActive()) && event.button === 0) {
    event.preventDefault();
    isPainting = true;
    paintStrokeKeys = new Set();
    paintUndoRecorded = false;
    if (ownershipEditingActive()) {
      paintOwnershipAtPointer(event);
    } else {
      paintAtPointer(event);
    }
    mapPanel.setPointerCapture(event.pointerId);
    return;
  }
  if (aiMemberPickActive() && event.button === 0) {
    event.preventDefault();
    handleAiMemberPickClick(event);
    return;
  }
  if (aiTargetPickActive() && event.button === 0) {
    event.preventDefault();
    handleAiTargetPickClick(event);
    return;
  }
  if (unitPlaceActive() && event.button === 0) {
    event.preventDefault();
    placeEditorUnitAtPointer(event);
    return;
  }
  if (unitEditActive() && event.button === 0) {
    event.preventDefault();
    handleUnitEditClick(event);
    return;
  }
  if (appMode === "scenario" && event.button === 0) {
    const hex = findNearestHex(point);
    if (hex) {
      event.preventDefault();
      selectHex(hex);
      return;
    }
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
    if (ownershipEditingActive()) {
      paintOwnershipAtPointer(event);
    } else {
      paintAtPointer(event);
    }
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
  if (appMode === "play" || appMode === "scenario") {
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
  applyScenarioPanelHeight(scenarioPanelHeight, false);
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
loadScenarioPanelHeight();
syncModeControls();
loadBitmapUnitSprites();
appInitialization = initializeApp();
