const { test, expect } = require("@playwright/test");
const { execFileSync, spawnSync } = require("child_process");
const path = require("path");

test.describe.configure({ mode: "serial" });

async function openPlayMode(page) {
  await page.goto("/");
  await page.getByRole("button", { name: "Play" }).click();
  await expect(page.locator(".unit-roster-item")).toHaveCount(8);
}

function steppeEnginePath() {
  return path.join(__dirname, "..", "build", process.platform === "win32" ? "steppe_engine.exe" : "steppe_engine");
}

function runEngineJson(args, input) {
  return JSON.parse(execFileSync(steppeEnginePath(), args, {
    cwd: path.join(__dirname, ".."),
    input: JSON.stringify(input),
    encoding: "utf8",
  }));
}

function runEngineOutputJson(args) {
  return JSON.parse(execFileSync(steppeEnginePath(), args, {
    cwd: path.join(__dirname, ".."),
    encoding: "utf8",
  }));
}

function corridorGameState() {
  const hexes = [];
  for (let r = 1; r <= 4; r += 1) {
    for (let q = 1; q <= 5; q += 1) {
      const corridor = r === 2 && q >= 2 && q <= 4;
      hexes.push({ q, r, terrain: corridor ? "grassland" : "mountain", labels: [] });
    }
  }
  return {
    width: 5,
    height: 4,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: "horse_archer", q: 2, r: 2, hp: 5, maxHp: 5, remainingScaledMove: 24 },
      { id: 2, owner: 1, faction: "mongol", kind: "herd", q: 3, r: 2, hp: 1, maxHp: 1, remainingScaledMove: 24 },
    ],
    game: {
      round: 1,
      activeFactionIndex: 0,
      selectedUnitId: 1,
      turnOrder: [1],
    },
  };
}

test("movement can pass through friendly units without stacking", async ({ isMobile }) => {
  test.skip(isMobile, "engine rule is covered once on desktop");

  const reachable = runEngineJson(["game-reachable", "--unit", "1"], corridorGameState()).reachable;
  expect(reachable.some((hex) => hex.q === 3 && hex.r === 2)).toBe(false);
  expect(reachable.some((hex) => hex.q === 4 && hex.r === 2)).toBe(true);

  const moveToFriendly = spawnSync(steppeEnginePath(), ["game-move", "--unit", "1", "--q", "3", "--r", "2"], {
    cwd: path.join(__dirname, ".."),
    input: JSON.stringify(corridorGameState()),
    encoding: "utf8",
  });
  expect(moveToFriendly.status).not.toBe(0);
  expect(JSON.parse(moveToFriendly.stdout).ok).toBe(false);
});

test("great wall avoids the southwest corner on southern access seeds", async ({ isMobile }) => {
  test.skip(isMobile, "engine generation invariant is covered once on desktop");

  for (const seed of ["3385919878", "55555"]) {
    const map = runEngineOutputJson(["generate", "--width", "120", "--height", "80", "--seed", seed]);
    const terrain = new Map(map.hexes.map((hex) => [`${hex.q},${hex.r}`, hex.terrain]));
    const wall = map.walls.find((candidate) => candidate.feature === "great_wall");
    const firstEdge = wall.edge_path[0];
    const firstEdgeTerrain = [firstEdge.a, firstEdge.b].map((coord) => terrain.get(`${coord.q},${coord.r}`));
    const wallRows = wall.edge_path.flatMap((edge) => [edge.a.r, edge.b.r]);
    const wallQs = wall.edge_path.flatMap((edge) => [edge.a.q, edge.b.q]);
    const wallSouthwestEdges = wall.edge_path.filter((edge) => (
      Math.min(edge.a.q, edge.b.q) <= 6 && Math.max(edge.a.r, edge.b.r) >= 72
    ));
    const chineseRows = map.towns
      .filter((town) => town.feature === "chinese_town")
      .map((town) => town.r);

    expect(wallSouthwestEdges).toHaveLength(0);
    expect(firstEdgeTerrain).toContain("mountain");
    expect(Math.min(firstEdge.a.q, firstEdge.b.q)).toBeGreaterThanOrEqual(40);
    expect(Math.max(firstEdge.a.q, firstEdge.b.q)).toBeLessThanOrEqual(80);
    expect(Math.min(...wallQs)).toBeGreaterThan(1);
    expect(Math.max(...wallRows)).toBeLessThanOrEqual(72);
    expect(Math.max(...chineseRows)).toBeLessThanOrEqual(66);
  }
});

test("scenario controls sit above the shared map", async ({ page }) => {
  await page.goto("/");
  await page.getByRole("button", { name: "Scenario Edit" }).click();

  await expect(page.locator("#scenario-controls")).toBeVisible();
  await expect(page.locator("#play-controls")).toBeHidden();
  await expect(page.locator(".map-section")).toBeVisible();

  const layout = await page.evaluate(() => {
    const topbar = document.querySelector("#scenario-controls").getBoundingClientRect();
    const map = document.querySelector(".map-section").getBoundingClientRect();
    return {
      topbarBottom: topbar.bottom,
      mapTop: map.top,
      mapWidth: map.width,
      viewportWidth: window.innerWidth,
      scrollWidth: document.documentElement.scrollWidth,
    };
  });

  expect(layout.mapTop).toBeGreaterThanOrEqual(layout.topbarBottom - 1);
  expect(layout.mapWidth).toBeGreaterThan(layout.viewportWidth * 0.95);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});

test("generated great wall contains Chinese towns from outside", async ({ page, isMobile }) => {
  test.skip(!isMobile, "data invariant is covered once on mobile project");

  await page.goto("/");

  const result = await page.evaluate(async () => {
    const response = await fetch("/api/generate", {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ width: 120, height: 80, seed: 55555 }),
    });
    const map = await response.json();
    const coordKey = (coord) => `${coord.q},${coord.r}`;
    const edgeKey = (edge) => {
      const first = edge.a;
      const second = edge.b;
      const before = first.r < second.r || (first.r === second.r && first.q <= second.q);
      const a = before ? first : second;
      const b = before ? second : first;
      return `${coordKey(a)}|${coordKey(b)}`;
    };
    const neighbor = (coord, direction) => {
      const shiftedDown = ((coord.q - 1) % 2) === 1;
      const neighbors = [
        { q: coord.q + 1, r: coord.r },
        shiftedDown ? { q: coord.q + 1, r: coord.r + 1 } : { q: coord.q + 1, r: coord.r - 1 },
        { q: coord.q, r: coord.r - 1 },
        { q: coord.q - 1, r: coord.r },
        shiftedDown ? { q: coord.q - 1, r: coord.r + 1 } : { q: coord.q - 1, r: coord.r - 1 },
        { q: coord.q, r: coord.r + 1 },
      ];
      return neighbors[direction];
    };

    const wall = map.walls && map.walls[0];
    const wallEdges = new Set((wall ? wall.edge_path : []).map(edgeKey));
    const leftWallRows = (wall ? wall.edge_path : [])
      .flatMap((edge) => [edge.a, edge.b].filter((coord) => coord.q === 1).map((coord) => coord.r));
    const maxLeftSeedRow = leftWallRows.length > 0 ? Math.min(...leftWallRows) : map.height;
    const terrain = new Map(map.hexes.map((hex) => [coordKey(hex), hex.terrain]));
    const chineseTowns = new Set(map.towns
      .filter((town) => town.feature === "chinese_town")
      .map(coordKey));
    const blocked = (coord) => {
      const value = terrain.get(coordKey(coord));
      return value === "mountain" || value === "lake";
    };
    const frontier = [];
    const visited = new Set();
    const add = (coord) => {
      const key = coordKey(coord);
      if (coord.q < 1 || coord.q > map.width || coord.r < 1 || coord.r > map.height || blocked(coord) || visited.has(key)) {
        return;
      }
      visited.add(key);
      frontier.push(coord);
    };

    for (let q = 1; q <= map.width; q += 1) {
      add({ q, r: 1 });
    }
    for (let r = 1; r <= maxLeftSeedRow; r += 1) {
      add({ q: 1, r });
    }

    const leaked = [];
    while (frontier.length > 0) {
      const current = frontier.shift();
      if (chineseTowns.has(coordKey(current))) {
        leaked.push(coordKey(current));
      }
      for (let direction = 0; direction < 6; direction += 1) {
        const next = neighbor(current, direction);
        if (next.q < 1 || next.q > map.width || next.r < 1 || next.r > map.height || blocked(next) || visited.has(coordKey(next))) {
          continue;
        }
        if (wallEdges.has(edgeKey({ a: current, b: next }))) {
          continue;
        }
        add(next);
      }
    }

    return {
      wallCount: map.walls.length,
      wallEdges: wall ? wall.edge_path.length : 0,
      wallGates: Array.isArray(map.wall_gates) ? map.wall_gates.length : 0,
      persianRoads: map.roads.filter((road) => road.feature === "persian_town").length,
      chineseRoads: map.roads.filter((road) => road.feature === "chinese_town").length,
      leaked,
    };
  });

  expect(result.wallCount).toBe(1);
  expect(result.wallEdges).toBeGreaterThan(0);
  expect(result.wallGates).toBeGreaterThanOrEqual(5);
  expect(result.persianRoads).toBeGreaterThanOrEqual(4);
  expect(result.chineseRoads).toBeGreaterThanOrEqual(6);
  expect(result.leaked).toEqual([]);
});

test("play sidebar lists deployed units and bottom panel inspects selection", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop-only bottom panel assertion");

  await openPlayMode(page);

  await page.evaluate(() => {
    for (const unit of currentMap.units) {
      if (unit.kind !== "horde") {
        continue;
      }
      if (unit.faction === "mongol") {
        unit.population = 11;
        unit.horses = 5;
        unit.metal = 2;
      } else if (unit.faction === "chinese") {
        unit.population = 7;
        unit.horses = 3;
        unit.metal = 4;
      }
    }
    syncPlayControls();
  });
  await expect(page.locator("#faction-status-bar")).toBeVisible();
  await expect(page.locator("#faction-status-name")).toHaveText("Mongol");
  await expect(page.locator("#faction-population-total")).toHaveText("11");
  await expect(page.locator("#faction-horses-total")).toHaveText("5");
  await expect(page.locator("#faction-metal-total")).toHaveText("2");

  const rosterItems = page.locator(".unit-roster-item");
  await expect(rosterItems.first()).toContainText("Mongol Horse Archers");
  await expect(rosterItems.first()).toContainText("Horse Archers - HP 10/10");

  await rosterItems.first().click();

  await expect(page.locator(".sidebar-selection-readout")).toHaveText("Mongol Horse Archers");
  await expect(page.locator("#unit-name")).toHaveText("Mongol Horse Archers");
  await expect(page.locator("#unit-hp")).toHaveText("10/10");
  await expect(page.locator("#unit-move")).toHaveText("4/4");
  await expect(page.locator("#unit-resources")).toBeHidden();
  await expect(page.locator("#play-details-bar")).toBeVisible();
  await expect(page.locator("#herd-count")).toHaveText("1");
  await expect(page.locator("#status-active-faction-name")).toHaveText("Mongol");
  await expect(page.locator("#status-end-turn-button")).toBeVisible();

  await page.locator("#status-end-turn-button").click();
  await expect(page.locator("#status-active-faction-name")).toHaveText("Chinese");
  await expect(page.locator("#turn-status-readout")).toHaveText("Chinese turn");
  await expect(page.locator("#faction-status-name")).toHaveText("Chinese");
  await expect(page.locator("#faction-population-total")).toHaveText("4");
  await expect(page.locator("#faction-horses-total")).toHaveText("12");
  await expect(page.locator("#faction-metal-total")).toHaveText("4");

  const layout = await page.evaluate(() => {
    const sidebar = document.querySelector("#play-controls").getBoundingClientRect();
    const map = document.querySelector(".map-section").getBoundingClientRect();
    const inspector = document.querySelector(".unit-inspector").getBoundingClientRect();
    return {
      sidebarRight: sidebar.right,
      mapLeft: map.left,
      inspectorHeight: inspector.height,
      scrollWidth: document.documentElement.scrollWidth,
      viewportWidth: window.innerWidth,
    };
  });

  expect(layout.mapLeft).toBeGreaterThanOrEqual(layout.sidebarRight - 1);
  expect(layout.inspectorHeight).toBeGreaterThan(0);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});

test("horde inspector shows resource counters", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop-only bottom panel assertion");

  await openPlayMode(page);

  await page.locator(".unit-roster-item").filter({ hasText: "Mongol Horde" }).click();
  await expect(page.locator("#unit-name")).toHaveText("Mongol Horde");
  await expect(page.locator("#unit-resources")).toBeVisible();
  await expect(page.locator("#unit-population")).toHaveText("4");
  await expect(page.locator("#unit-metal")).toHaveText("4");
  await expect(page.locator("#unit-horses")).toHaveText("12");
});

test("detach herd creates a herd from horde horses", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for detach assertions");

  await openPlayMode(page);

  const hordePoint = await page.evaluate(() => {
    const unit = currentMap.units.find((candidate) => candidate.kind === "horde" && candidate.faction === "mongol");
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(unit);
    return {
      unitId: unit.id,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(hordePoint.x, hordePoint.y, { button: "right" });
  await expect(page.locator("#context-menu [data-action='detach-herd']")).toHaveText("Detach herd");
  await page.locator("#context-menu [data-action='detach-herd']").click();
  await expect(page.locator("#detach-herd-popover")).toBeVisible();
  await page.locator("#detach-herd-horses").fill("3");
  await page.locator("#detach-herd-form").press("Enter");

  await expect.poll(() => page.evaluate(() => detachHerdPlacement && detachHerdPlacement.deployableHexes.length)).toBeGreaterThan(0);
  const deployPoint = await page.evaluate(() => {
    const hex = detachHerdPlacement.deployableHexes[0];
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(hex);
    return {
      q: hex.q,
      r: hex.r,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(deployPoint.x, deployPoint.y);
  await expect.poll(() => page.evaluate((unitId) => {
    const horde = currentMap.units.find((unit) => unit.id === unitId);
    const detached = currentMap.units.find((unit) => unit.kind === "herd" && unit.horses === 3);
    return {
      hordeHorses: horde.horses,
      detachedAt: detached ? `${detached.q},${detached.r}` : "",
      statusHorses: document.querySelector("#faction-horses-total").textContent,
    };
  }, hordePoint.unitId)).toEqual({
    hordeHorses: 9,
    detachedAt: `${deployPoint.q},${deployPoint.r}`,
    statusHorses: "9",
  });
});

test("create horse archers spends horde resources and deploys adjacent unit", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for deployment assertions");

  await openPlayMode(page);

  const hordePoint = await page.evaluate(() => {
    const unit = currentMap.units.find((candidate) => candidate.kind === "horde" && candidate.faction === "mongol");
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(unit);
    return {
      unitId: unit.id,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(hordePoint.x, hordePoint.y, { button: "right" });
  await expect(page.locator("#context-menu [data-action='create-horse-archers']")).toHaveText("Create Horse Archers");
  await page.locator("#context-menu [data-action='create-horse-archers']").click();

  await expect.poll(() => page.evaluate(() => (
    createHorseArchersPlacement && createHorseArchersPlacement.deployableHexes.length
  ))).toBeGreaterThan(0);
  const deployPoint = await page.evaluate(() => {
    const hex = createHorseArchersPlacement.deployableHexes[0];
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(hex);
    return {
      q: hex.q,
      r: hex.r,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(deployPoint.x, deployPoint.y);
  await expect.poll(() => page.evaluate(({ unitId, q, r }) => {
    const horde = currentMap.units.find((unit) => unit.id === unitId);
    const created = currentMap.units.find((unit) => (
      unit.kind === "horse_archer" && unit.q === q && unit.r === r
    ));
    return {
      population: horde.population,
      metal: horde.metal,
      horses: horde.horses,
      createdKind: created ? created.kind : "",
      createdMove: created ? created.move : 0,
      statusPopulation: document.querySelector("#faction-population-total").textContent,
      statusHorses: document.querySelector("#faction-horses-total").textContent,
      statusMetal: document.querySelector("#faction-metal-total").textContent,
    };
  }, { unitId: hordePoint.unitId, q: deployPoint.q, r: deployPoint.r }), {
    message: "horde resources should be spent and horse archers should deploy",
  }).toEqual({
    population: 3,
    metal: 3,
    horses: 9,
    createdKind: "horse_archer",
    createdMove: 4,
    statusPopulation: "3",
    statusHorses: "9",
    statusMetal: "3",
  });
});

test("play context menu exposes unit actions", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for context menu assertions");

  await openPlayMode(page);

  const unitPoint = async (predicateSource) => page.evaluate((source) => {
    const predicate = Function("unit", `return ${source};`);
    const unit = currentMap.units.find((candidate) => predicate(candidate));
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(unit);
    return {
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  }, predicateSource);

  let point = await unitPoint("unit.kind === 'herd' && unit.faction === 'mongol'");
  await page.mouse.click(point.x, point.y, { button: "right" });
  await expect(page.locator("#context-menu")).toBeVisible();
  await expect(page.locator("#context-menu [data-action='select-unit']")).toHaveText("Select");
  await page.locator("#context-menu [data-action='select-unit']").click();
  await expect(page.locator("#unit-name")).not.toHaveText("None");

  const movePoint = await page.evaluate(() => {
    const selectedId = currentMap.game.selectedUnitId;
    const unit = currentMap.units.find((candidate) => candidate.id === selectedId);
    const move = currentMap.game.legalMoves[0];
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(move);
    return {
      selectedId,
      original: `${unit.q},${unit.r}`,
      q: move.q,
      r: move.r,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });
  await page.mouse.click(movePoint.x, movePoint.y, { button: "right" });
  await expect(page.locator("#context-menu [data-action='move-here']")).toHaveText("Move here");
  await page.locator("#context-menu [data-action='move-here']").click();
  await expect.poll(() => page.evaluate((selectedId) => {
    const unit = currentMap.units.find((candidate) => candidate.id === selectedId);
    return `${unit.q},${unit.r}`;
  }, movePoint.selectedId)).not.toBe(movePoint.original);
});

test("mobile play mode keeps the map usable below the unit roster", async ({ page, isMobile }) => {
  test.skip(!isMobile, "mobile-only layout assertion");

  await openPlayMode(page);

  await expect(page.locator("#faction-status-bar")).toBeVisible();
  await expect(page.locator("#play-details-bar")).toBeHidden();
  await expect(page.locator(".unit-roster-item")).toHaveCount(8);

  const layout = await page.evaluate(() => {
    const sidebar = document.querySelector("#play-controls").getBoundingClientRect();
    const panel = document.querySelector("#map-panel").getBoundingClientRect();
    return {
      sidebarBottom: sidebar.bottom,
      panelTop: panel.top,
      panelHeight: panel.height,
      scrollWidth: document.documentElement.scrollWidth,
      viewportWidth: window.innerWidth,
    };
  });

  expect(layout.panelTop).toBeGreaterThanOrEqual(layout.sidebarBottom - 1);
  expect(layout.panelHeight).toBeGreaterThan(200);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});

test("scenario editor modes toggle terrain edges and units", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for editor assertions");

  await page.goto("/");
  await page.getByRole("button", { name: "Scenario Edit" }).click();
  await page.getByRole("button", { name: "Blank Map" }).click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();

  const hexPoint = async (coord) => page.evaluate(({ q, r }) => {
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter({ q, r });
    return {
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  }, coord);

  const edgePoint = async (first, second) => page.evaluate(({ first, second }) => {
    const panel = mapPanel.getBoundingClientRect();
    const boundary = edgeBoundaryPoints(canonicalEdge(first, second));
    const center = hexCenter(first);
    const boundaryMidpoint = {
      x: (boundary[0][0] + boundary[1][0]) / 2,
      y: (boundary[0][1] + boundary[1][1]) / 2,
    };
    const target = {
      x: boundaryMidpoint.x * 0.75 + center.x * 0.25,
      y: boundaryMidpoint.y * 0.75 + center.y * 0.25,
    };
    return {
      x: panel.left + viewport.offsetX + target.x * viewport.scale,
      y: panel.top + viewport.offsetY + target.y * viewport.scale,
    };
  }, { first, second });

  await expect(page.locator("#editor-mode")).toHaveValue("terrain");
  await expect(page.locator("#terrain-palette")).toBeVisible();
  await expect(page.locator("#editor-edge-feature")).toBeHidden();
  await expect(page.locator("#editor-unit-type")).toBeHidden();

  await page.locator("#editor-mode").selectOption("edges");
  await expect(page.locator("#terrain-palette")).toBeHidden();
  await expect(page.locator("#editor-edge-feature")).toBeVisible();

  await page.locator("#editor-edge-feature").selectOption("road");
  let point = await edgePoint({ q: 2, r: 2 }, { q: 3, r: 2 });
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.roads.length)).toBe(1);
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.roads.length)).toBe(0);

  await page.locator("#editor-edge-feature").selectOption("river");
  point = await edgePoint({ q: 2, r: 2 }, { q: 3, r: 2 });
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.edges.length)).toBe(1);
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.edges.length)).toBe(0);

  await page.locator("#editor-mode").selectOption("units");
  await expect(page.locator("#editor-unit-type")).toBeVisible();
  await expect(page.locator("#editor-unit-side")).toBeVisible();
  await page.locator("#editor-unit-type").selectOption("horde");
  await page.locator("#editor-unit-side").selectOption("chinese");
  point = await hexPoint({ q: 3, r: 3 });
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.units.length)).toBe(1);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].kind)).toBe("horde");
  await expect.poll(() => page.evaluate(() => currentMap.units[0].faction)).toBe("chinese");
  await expect.poll(() => page.evaluate(() => currentMap.units[0].move)).toBe(3);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].projectsZoc)).toBe(true);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].respectsZoc)).toBe(true);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].population)).toBe(0);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].metal)).toBe(0);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].horses)).toBe(0);
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.units.length)).toBe(0);

  await page.locator("#editor-unit-type").selectOption("herd");
  point = await hexPoint({ q: 4, r: 3 });
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.units.length)).toBe(1);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].kind)).toBe("herd");
  await expect.poll(() => page.evaluate(() => currentMap.units[0].move)).toBe(3);
});
