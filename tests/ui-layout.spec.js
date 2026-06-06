const { test, expect } = require("@playwright/test");
const { execFileSync, spawnSync } = require("child_process");
const path = require("path");

test.describe.configure({ mode: "serial" });

async function openPlayMode(page) {
  await page.goto("/");
  await page.getByRole("button", { name: "Play" }).click();
  await expect(page.locator(".unit-roster-item")).toHaveCount(9);
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

function openMovementGameState() {
  const hexes = [];
  for (let r = 1; r <= 2; r += 1) {
    for (let q = 1; q <= 5; q += 1) {
      hexes.push({ q, r, terrain: "grassland", labels: [] });
    }
  }
  return {
    width: 5,
    height: 2,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: "horse_archer", q: 1, r: 1, hp: 10, maxHp: 10, remainingScaledMove: 32 },
    ],
    game: {
      round: 1,
      activeFactionIndex: 0,
      selectedUnitId: 1,
      turnOrder: [1],
    },
  };
}

function readinessRecoveryGameState(adjacent = false) {
  const hexes = [];
  for (let r = 1; r <= 2; r += 1) {
    for (let q = 1; q <= 5; q += 1) {
      hexes.push({ q, r, terrain: "grassland", labels: [] });
    }
  }
  return {
    width: 5,
    height: 2,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: "horse_archer", q: 2, r: 1, hp: 10, maxHp: 10, readiness: 50, remainingScaledMove: 32 },
      { id: 2, owner: 2, faction: "chinese", kind: "infantry", q: adjacent ? 3 : 5, r: 1, hp: 10, maxHp: 10, readiness: 50, remainingScaledMove: 16 },
    ],
    game: {
      round: 1,
      activeFactionIndex: 1,
      selectedUnitId: 0,
      turnOrder: [1, 2],
    },
  };
}

function combatGameState(defenderTerrain = "grassland") {
  const hexes = [];
  for (let r = 1; r <= 2; r += 1) {
    for (let q = 1; q <= 3; q += 1) {
      hexes.push({ q, r, terrain: q === 2 && r === 1 ? defenderTerrain : "grassland", labels: [] });
    }
  }
  return {
    width: 3,
    height: 2,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: "horse_archer", q: 1, r: 1, hp: 10, maxHp: 10, remainingScaledMove: 32 },
      { id: 2, owner: 2, faction: "chinese", kind: "horse_archer", q: 2, r: 1, hp: 10, maxHp: 10, remainingScaledMove: 32 },
    ],
    game: {
      round: 1,
      activeFactionIndex: 0,
      selectedUnitId: 1,
      turnOrder: [1, 2],
    },
  };
}

function flankingCombatGameState(flankerCoord) {
  const hexes = [];
  for (let r = 1; r <= 3; r += 1) {
    for (let q = 1; q <= 3; q += 1) {
      hexes.push({ q, r, terrain: "grassland", labels: [] });
    }
  }
  return {
    width: 3,
    height: 3,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: "horse_archer", q: 1, r: 1, hp: 10, maxHp: 10, remainingScaledMove: 32 },
      { id: 2, owner: 2, faction: "chinese", kind: "horse_archer", q: 2, r: 1, hp: 10, maxHp: 10, remainingScaledMove: 32 },
      { id: 3, owner: 1, faction: "mongol", kind: "infantry", q: flankerCoord.q, r: flankerCoord.r, hp: 10, maxHp: 10, remainingScaledMove: 16 },
    ],
    game: {
      round: 1,
      activeFactionIndex: 0,
      selectedUnitId: 1,
      turnOrder: [1, 2],
    },
  };
}

function feignedRetreatGameState(defenderStance = "feigned_retreat", attackerKind = "infantry") {
  const hexes = [];
  for (let r = 1; r <= 3; r += 1) {
    for (let q = 1; q <= 4; q += 1) {
      hexes.push({ q, r, terrain: "grassland", labels: [] });
    }
  }
  return {
    width: 4,
    height: 3,
    seed: 0,
    hexes,
    units: [
      { id: 1, owner: 1, faction: "mongol", kind: attackerKind, q: 1, r: 2, hp: 10, maxHp: 10, remainingScaledMove: 16 },
      {
        id: 2,
        owner: 2,
        faction: "chinese",
        kind: "horse_archer",
        stance: defenderStance,
        q: 2,
        r: 2,
        hp: 10,
        maxHp: 10,
        remainingScaledMove: 32,
      },
    ],
    game: {
      round: 1,
      activeFactionIndex: 0,
      selectedUnitId: 1,
      turnOrder: [1, 2],
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

test("default sandbox includes Chinese militia stats", async ({ isMobile }) => {
  test.skip(isMobile, "engine unit defaults are covered once on desktop");

  const state = runEngineOutputJson(["game-new"]);
  const militia = state.units.find((unit) => unit.kind === "chinese_militia");
  expect(militia).toEqual(expect.objectContaining({
    faction: "chinese",
    attack: 2,
    defense: 2,
    move: 2,
    projectsZoc: true,
    respectsZoc: true,
  }));
});

test("combat uses unit stats terrain defense and retaliation", async ({ isMobile }) => {
  test.skip(isMobile, "engine combat rule is covered once on desktop");

  const preview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], combatGameState("grassland"));
  expect(preview.valid).toBe(true);
  expect(preview.defenderRetaliates).toBe(true);
  expect(preview.attacker.effectiveAttack).toBe(4);
  expect(preview.baseDifferential).toBe(1);
  expect(preview.hpRatioPercent).toBe(100);
  expect(preview.readinessRatioPercent).toBe(100);
  expect(preview.conditionRatioPercent).toBe(100);
  expect(preview.crtIndex).toBe(1);
  expect(preview.specialResolution).toBe("normal");
  expect(preview.retreatOption).toBe("none");
  expect(preview.readinessImpact).toBe("Even readiness");
  expect(preview.retreatImpact).toBe("No retreat");
  expect(preview.attacker.damageDealt).toBe(2);
  expect(preview.attacker.damageTaken).toBe(1);
  expect(preview.attacker.readinessDamageDealt).toBe(25);
  expect(preview.attacker.readinessDamageTaken).toBe(10);
  expect(preview.attacker.resultHp).toBe(9);
  expect(preview.attacker.resultReadiness).toBe(90);
  expect(preview.attacker.readiness).toBe(100);
  expect(preview.attacker.readinessPercent).toBe(100);
  expect(preview.attacker.baseReadinessDamage).toBe(25);
  expect(preview.defender.effectiveDefense).toBe(3);
  expect(preview.defender.readinessDamageDealt).toBe(0);
  expect(preview.defender.readinessDamageTaken).toBe(25);
  expect(preview.defender.damageTaken).toBe(2);
  expect(preview.defender.resultHp).toBe(8);
  expect(preview.defender.resultReadiness).toBe(75);

  const grass = runEngineJson(["game-attack", "--attacker", "1", "--defender", "2"], combatGameState("grassland"));
  const grassAttacker = grass.units.find((unit) => unit.id === 1);
  const grassDefender = grass.units.find((unit) => unit.id === 2);
  expect(grass.ok).toBe(true);
  expect(grassAttacker.hp).toBe(9);
  expect(grassAttacker.remainingScaledMove).toBe(0);
  expect(grassAttacker.moveDone).toBe(true);
  expect(grassAttacker.combatDone).toBe(true);
  expect(grassAttacker.readiness).toBe(90);
  expect(grassDefender.hp).toBe(8);
  expect(grassDefender.readiness).toBe(75);

  const recovered = runEngineJson(["game-end-turn"], grass);
  const recoveredDefender = recovered.units.find((unit) => unit.id === 2);
  expect(recoveredDefender.readiness).toBe(75);

  const hill = runEngineJson(["game-attack", "--attacker", "1", "--defender", "2"], combatGameState("hill"));
  const hillPreview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], combatGameState("hill"));
  const hillAttacker = hill.units.find((unit) => unit.id === 1);
  const hillDefender = hill.units.find((unit) => unit.id === 2);
  expect(hillPreview.defender.terrainDefensePercent).toBe(125);
  expect(hillPreview.defender.effectiveDefense).toBe(4);
  expect(hillPreview.baseDifferential).toBe(0);
  expect(hillPreview.crtIndex).toBe(0);
  expect(hillPreview.defender.damageTaken).toBe(1);
  expect(hill.ok).toBe(true);
  expect(hillAttacker.hp).toBe(9);
  expect(hillDefender.hp).toBe(9);

  const infantryState = combatGameState("grassland");
  infantryState.units[0].kind = "infantry";
  infantryState.units[1].stance = "defensive";
  const infantryPreview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], infantryState);
  expect(infantryPreview.attacker.baseReadinessDamage).toBe(10);
  expect(infantryPreview.attacker.readinessDamageDealt).toBe(10);
  expect(infantryPreview.defender.resultReadiness).toBe(90);

  const hordeState = combatGameState("grassland");
  hordeState.units[0].kind = "horde";
  hordeState.units[1].stance = "defensive";
  const hordePreview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], hordeState);
  expect(hordePreview.attacker.baseReadinessDamage).toBe(0);
  expect(hordePreview.attacker.readinessDamageDealt).toBe(0);
  expect(hordePreview.defender.resultReadiness).toBe(100);
});

test("horse archers default to feigned retreat when attacked by non horse archers", async ({ isMobile }) => {
  test.skip(isMobile, "engine combat rule is covered once on desktop");

  const preview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], feignedRetreatGameState());
  expect(preview.valid).toBe(true);
  expect(preview.specialResolution).toBe("feigned_retreat");
  expect(preview.defenderRetaliates).toBe(false);
  expect(preview.pursuitReadinessPenalty).toBe(15);
  expect(preview.attacker.readinessDamageTaken).toBe(15);
  expect(preview.attacker.resultReadiness).toBe(85);
  expect(preview.attacker.damageDealt).toBe(0);
  expect(preview.defender.damageTaken).toBe(0);
  expect(preview.attackerPursuitTo).toEqual({ q: 2, r: 2 });
  expect(preview.defenderRetreatTo).toEqual({ q: 2, r: 3 });

  const resolved = runEngineJson(["game-attack", "--attacker", "1", "--defender", "2"], feignedRetreatGameState());
  const attacker = resolved.units.find((unit) => unit.id === 1);
  const defender = resolved.units.find((unit) => unit.id === 2);
  expect(resolved.ok).toBe(true);
  expect(attacker).toEqual(expect.objectContaining({ q: 2, r: 2, hp: 10, readiness: 85, combatDone: true }));
  expect(defender).toEqual(expect.objectContaining({ q: 2, r: 3, hp: 10, readiness: 100 }));

  const defensivePreview = runEngineJson(
    ["game-combat-preview", "--attacker", "1", "--defender", "2"],
    feignedRetreatGameState("defensive")
  );
  expect(defensivePreview.specialResolution).toBe("normal");
  expect(defensivePreview.defender.damageTaken).toBeGreaterThan(0);

  const horseArcherAttackerPreview = runEngineJson(
    ["game-combat-preview", "--attacker", "1", "--defender", "2"],
    feignedRetreatGameState("feigned_retreat", "horse_archer")
  );
  expect(horseArcherAttackerPreview.specialResolution).toBe("normal");
});

test("combat flanking requires separated support", async ({ isMobile }) => {
  test.skip(isMobile, "engine combat rule is covered once on desktop");

  const separated = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], flankingCombatGameState({ q: 3, r: 1 }));
  expect(separated.valid).toBe(true);
  expect(separated.defenderFlanked).toBe(true);
  expect(separated.flankingDefensePercent).toBe(75);
  expect(separated.defender.flankingDefensePercent).toBe(75);
  expect(separated.defender.effectiveDefense).toBe(2);
  expect(separated.baseDifferential).toBe(2);

  const adjacentToAttacker = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], flankingCombatGameState({ q: 1, r: 2 }));
  expect(adjacentToAttacker.valid).toBe(true);
  expect(adjacentToAttacker.defenderFlanked).toBe(false);
  expect(adjacentToAttacker.flankingDefensePercent).toBe(100);
  expect(adjacentToAttacker.defender.effectiveDefense).toBe(3);
  expect(adjacentToAttacker.baseDifferential).toBe(1);
});

test("combat damage tapers against worn down targets", async ({ isMobile }) => {
  test.skip(isMobile, "engine combat rule is covered once on desktop");

  const wornState = combatGameState("grassland");
  wornState.units[1].hp = 4;
  wornState.units[1].readiness = 40;

  const preview = runEngineJson(["game-combat-preview", "--attacker", "1", "--defender", "2"], wornState);
  expect(preview.valid).toBe(true);
  expect(preview.hpRatioPercent).toBe(250);
  expect(preview.readinessRatioPercent).toBe(250);
  expect(preview.conditionRatioPercent).toBe(625);
  expect(preview.crtIndex).toBe(6);
  expect(preview.retreatOption).toBe("defender");
  expect(preview.defender.damageTaken).toBe(2);
  expect(preview.defender.resultHp).toBe(2);
  expect(preview.defender.readinessDamageTaken).toBe(10);
  expect(preview.defender.resultReadiness).toBe(30);

  const resolved = runEngineJson(["game-attack", "--attacker", "1", "--defender", "2"], wornState);
  const defender = resolved.units.find((unit) => unit.id === 2);
  expect(resolved.ok).toBe(true);
  expect(defender.hp).toBe(2);
  expect(defender.readiness).toBe(30);
});

test("movement spends readiness in proportion to movement cost", async ({ isMobile }) => {
  test.skip(isMobile, "engine readiness rule is covered once on desktop");

  const moved = runEngineJson(["game-move", "--unit", "1", "--q", "4", "--r", "2"], corridorGameState());
  const unit = moved.units.find((candidate) => candidate.id === 1);
  expect(moved.ok).toBe(true);
  expect(unit.readiness).toBe(96);

  const fullMove = runEngineJson(["game-move", "--unit", "1", "--q", "5", "--r", "1"], openMovementGameState());
  const fullMoveUnit = fullMove.units.find((candidate) => candidate.id === 1);
  expect(fullMove.ok).toBe(true);
  expect(fullMoveUnit.readiness).toBe(92);
});

test("readiness recovers only after quiet non-contact turns", async ({ isMobile }) => {
  test.skip(isMobile, "engine readiness rule is covered once on desktop");

  const quiet = runEngineJson(["game-end-turn"], readinessRecoveryGameState(false));
  const quietUnit = quiet.units.find((candidate) => candidate.id === 1);
  expect(quietUnit.readiness).toBe(75);
  expect(quietUnit.contactedEnemyThisTurn).toBe(false);

  const adjacent = runEngineJson(["game-end-turn"], readinessRecoveryGameState(true));
  const adjacentUnit = adjacent.units.find((candidate) => candidate.id === 1);
  expect(adjacentUnit.readiness).toBe(50);
  expect(adjacentUnit.contactedEnemyThisTurn).toBe(true);

  const movedState = openMovementGameState();
  movedState.units[0].readiness = 50;
  const moved = runEngineJson(["game-move", "--unit", "1", "--q", "2", "--r", "1"], movedState);
  const movedUnit = moved.units.find((candidate) => candidate.id === 1);
  const afterMovedTurn = runEngineJson(["game-end-turn"], moved);
  const afterMovedTurnUnit = afterMovedTurn.units.find((candidate) => candidate.id === 1);
  expect(afterMovedTurnUnit.readiness).toBe(movedUnit.readiness);
});

test("inactive units can be selected for inspection without legal actions", async ({ isMobile }) => {
  test.skip(isMobile, "engine selection rule is covered once on desktop");

  const selected = runEngineJson(["game-select", "--unit", "2"], combatGameState());
  expect(selected.ok).toBe(true);
  expect(selected.game.selectedUnitId).toBe(2);
  expect(selected.game.legalMoves).toEqual([]);
  expect(selected.game.legalAttacks).toEqual([]);
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
    const coordKey = (coord) => `${coord.q},${coord.r}`;
    const edgeKey = (edge) => {
      const before = edge.a.r < edge.b.r || (edge.a.r === edge.b.r && edge.a.q <= edge.b.q);
      const a = before ? edge.a : edge.b;
      const b = before ? edge.b : edge.a;
      return `${coordKey(a)}|${coordKey(b)}`;
    };
    const neighbor = (coord, direction) => {
      const shiftedDown = ((coord.q - 1) % 2) === 1;
      return [
        { q: coord.q + 1, r: coord.r },
        shiftedDown ? { q: coord.q + 1, r: coord.r + 1 } : { q: coord.q + 1, r: coord.r - 1 },
        { q: coord.q, r: coord.r - 1 },
        { q: coord.q - 1, r: coord.r },
        shiftedDown ? { q: coord.q - 1, r: coord.r + 1 } : { q: coord.q - 1, r: coord.r - 1 },
        { q: coord.q, r: coord.r + 1 },
      ][direction];
    };
    const wallEdges = new Set(wall.edge_path.map(edgeKey));
    const leftWallRows = wall.edge_path
      .flatMap((edge) => [edge.a, edge.b].filter((coord) => coord.q === 1).map((coord) => coord.r));
    const maxLeftSeedRow = leftWallRows.length > 0 ? Math.min(...leftWallRows) : map.height;
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
    while (frontier.length > 0) {
      const current = frontier.shift();
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
    const passableHexes = map.hexes.filter((hex) => !blocked(hex)).length;
    const enclosedPassablePercent = ((passableHexes - visited.size) / passableHexes) * 100;
    const leakedChineseTowns = map.towns
      .filter((town) => town.feature === "chinese_town" && visited.has(coordKey(town)))
      .map(coordKey);

    expect(wallSouthwestEdges).toHaveLength(0);
    expect(firstEdgeTerrain.includes("mountain") || firstEdge.a.r === map.height || firstEdge.b.r === map.height).toBe(true);
    expect(Math.min(firstEdge.a.q, firstEdge.b.q)).toBeGreaterThanOrEqual(40);
    expect(Math.max(firstEdge.a.q, firstEdge.b.q)).toBeLessThanOrEqual(80);
    expect(Math.min(...wallQs)).toBeGreaterThan(1);
    expect(Math.max(...chineseRows)).toBeLessThanOrEqual(66);
    expect(leakedChineseTowns).toEqual([]);
    expect(enclosedPassablePercent).toBeLessThanOrEqual(30);
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

test("clicking inactive units inspects without enabling actions", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for map selection assertions");

  await openPlayMode(page);

  const point = await page.evaluate(() => {
    const unit = currentMap.units.find((candidate) => candidate.kind === "chinese_cavalry" && candidate.faction === "chinese");
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(unit);
    return {
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(point.x, point.y);
  await expect(page.locator("#unit-name")).toHaveText("Chinese Cavalry");
  await expect(page.locator(".sidebar-selection-readout")).toHaveText("Chinese Cavalry");
  await expect.poll(() => page.evaluate(() => ({
    selectedUnitId: currentMap.game.selectedUnitId,
    legalMoves: currentMap.game.legalMoves.length,
    legalAttacks: currentMap.game.legalAttacks.length,
  }))).toEqual({
    selectedUnitId: 5,
    legalMoves: 0,
    legalAttacks: 0,
  });
});

test("clicking attackable enemies resolves combat", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for map combat assertions");

  await openPlayMode(page);

  const result = await page.evaluate(async () => {
    const horde = currentMap.units.find((unit) => unit.kind === "horde" && unit.faction === "mongol");
    applyGamePatch(await postAppCommand({ type: "end_turn" }));
    const enemy = currentMap.units.find((unit) => unit.faction === "chinese" && unit.kind === "chinese_cavalry");
    applyGamePatch(await postAppCommand({ type: "select_unit", unitId: enemy.id }));
    const hordeNeighbors = Array.from({ length: 6 }, (_, direction) => neighborInDirection(horde, direction));
    const adjacentHex = currentMap.game.legalMoves.find((hex) => (
      hordeNeighbors.some((neighbor) => neighbor.q === hex.q && neighbor.r === hex.r)
    ));
    applyGamePatch(await postAppCommand({
      type: "move_unit",
      unitId: enemy.id,
      to: { q: adjacentHex.q, r: adjacentHex.r },
    }));
    applyGamePatch(await postAppCommand({ type: "end_turn" }));
    applyGamePatch(await postAppCommand({ type: "select_unit", unitId: horde.id }));
    const movedEnemy = currentMap.units.find((unit) => unit.id === enemy.id);
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(movedEnemy);
    return {
      hordeId: horde.id,
      enemyId: enemy.id,
      enemyHp: movedEnemy.hp,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  await page.mouse.click(result.x, result.y);
  await expect.poll(() => page.evaluate(({ hordeId, enemyId }) => {
    const horde = currentMap.units.find((unit) => unit.id === hordeId);
    const enemy = currentMap.units.find((unit) => unit.id === enemyId);
    return {
      selectedUnitId: currentMap.game.selectedUnitId,
      hordeCombatDone: horde.combatDone,
      enemyHp: enemy ? enemy.hp : 0,
    };
  }, result)).toEqual({
    selectedUnitId: result.hordeId,
    hordeCombatDone: true,
    enemyHp: result.enemyHp - 1,
  });
});

test("undo reverts play movement through the engine", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop pointer geometry is simpler for undo assertions");

  await openPlayMode(page);
  await expect(page.locator("#undo-button")).toBeDisabled();

  const moved = await page.evaluate(async () => {
    const unit = currentMap.units.find((candidate) => candidate.kind === "horse_archer" && candidate.faction === "mongol");
    await selectUnit(unit);
    const move = currentMap.game.legalMoves[0];
    const before = { q: unit.q, r: unit.r };
    await moveSelectedUnitTo(move);
    return {
      unitId: unit.id,
      before,
      after: { q: move.q, r: move.r },
    };
  });

  await expect(page.locator("#undo-button")).toBeEnabled();
  await expect.poll(() => page.evaluate((unitId) => {
    const unit = currentMap.units.find((candidate) => candidate.id === unitId);
    return { q: unit.q, r: unit.r };
  }, moved.unitId)).toEqual(moved.after);

  await page.locator("#undo-button").click();
  await expect.poll(() => page.evaluate((unitId) => {
    const unit = currentMap.units.find((candidate) => candidate.id === unitId);
    return { q: unit.q, r: unit.r };
  }, moved.unitId)).toEqual(moved.before);
  await expect(page.locator("#undo-button")).toBeDisabled();
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
  await expect(page.locator("#unit-attack")).toHaveText("4");
  await expect(page.locator("#unit-defense")).toHaveText("3");
  await expect(page.locator("#unit-readiness")).toHaveText("100/100");
  await expect(page.locator("#unit-move")).toHaveText("4/4");
  await expect(page.evaluate(() => [
    unitHpReadinessColor({ readiness: 100, maxReadiness: 100 }),
    unitHpReadinessColor({ readiness: 65, maxReadiness: 100 }),
    unitHpReadinessColor({ readiness: 64, maxReadiness: 100 }),
    unitHpReadinessColor({ readiness: 30, maxReadiness: 100 }),
    unitHpReadinessColor({ readiness: 29, maxReadiness: 100 }),
  ])).resolves.toEqual(["#237a3b", "#237a3b", "#a97800", "#a97800", "#c93632"]);
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

test("unit counters use sprite glyph zoom bands", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop canvas setup is enough for sprite asset coverage");

  await openPlayMode(page);
  await expect.poll(() => page.evaluate(() => unitSpriteSheetReady)).toBe(true);
  await expect(page.evaluate(() => {
    const kinds = ["infantry", "horde", "herd", "horse_archer", "chinese_cavalry", "chinese_militia", "mongol_lancer", "camp"];
    const medium = unitSpriteZoomLevels.find((level) => level.key === "medium");
    return {
      levels: unitSpriteZoomLevels.map((level) => level.key),
      initialIndex: viewport.zoomLevelIndex,
      initialScale: viewport.scale,
      counterAreaRatio: (unitCounterMetrics().width * unitCounterMetrics().height)
        / ((3 * Math.sqrt(3) / 2) * geometry.size * geometry.size),
      sprites: kinds.every((kind) => Boolean(tintedUnitSprite(kind, "#2368c4", medium))),
    };
  })).resolves.toEqual(expect.objectContaining({
    levels: ["small", "medium", "large"],
    initialIndex: 0,
    initialScale: expect.any(Number),
    counterAreaRatio: expect.any(Number),
    sprites: true,
  }));
  const counterAreaRatio = await page.evaluate(() => (
    (unitCounterMetrics().width * unitCounterMetrics().height)
      / ((3 * Math.sqrt(3) / 2) * geometry.size * geometry.size)
  ));
  expect(counterAreaRatio).toBeGreaterThan(0.45);
  expect(counterAreaRatio).toBeLessThan(0.53);
  const initialScale = await page.evaluate(() => viewport.scale);
  const initialCounterScreenWidth = await page.evaluate(() => unitCounterMetrics().width * viewport.scale);
  await page.locator("#zoom-in-button").click();
  await expect(page.evaluate(() => ({
    index: viewport.zoomLevelIndex,
    level: currentUnitSpriteLevel().key,
    counterScreenWidth: unitCounterMetrics().width * viewport.scale,
    scale: viewport.scale,
  }))).resolves.toEqual(expect.objectContaining({ index: 1, level: "medium" }));
  const mediumScale = await page.evaluate(() => viewport.scale);
  const mediumCounterScreenWidth = await page.evaluate(() => unitCounterMetrics().width * viewport.scale);
  expect(mediumScale).toBeGreaterThan(initialScale);
  expect(mediumCounterScreenWidth).toBeGreaterThan(initialCounterScreenWidth);
  await page.locator("#zoom-in-button").click();
  await expect(page.evaluate(() => ({
    index: viewport.zoomLevelIndex,
    level: currentUnitSpriteLevel().key,
    counterScreenWidth: unitCounterMetrics().width * viewport.scale,
    scale: viewport.scale,
  }))).resolves.toEqual(expect.objectContaining({ index: 2, level: "large" }));
  const largeScale = await page.evaluate(() => viewport.scale);
  expect(largeScale).toBeGreaterThan(mediumScale);
  expect(largeScale / initialScale).toBeLessThanOrEqual(2.01);
  await page.locator("#zoom-in-button").click();
  await expect(page.evaluate(() => viewport.zoomLevelIndex)).resolves.toBe(2);
  const coverageScales = await page.evaluate(() => {
    currentMap = { width: 120, height: 80 };
    updateGeometry(currentMap);
    viewport.fitScale = Math.min(viewport.width / geometry.width, viewport.height / geometry.height);
    const mediumTarget = worldSizeForHexSpan(40, 20);
    const largeTarget = worldSizeForHexSpan(20, 10);
    const mediumLevel = unitSpriteZoomLevels.find((level) => level.key === "medium");
    const largeLevel = unitSpriteZoomLevels.find((level) => level.key === "large");
    return {
      fitDelta: Math.abs(zoomScaleForLevel(0) - viewport.fitScale),
      mediumDelta: Math.abs(zoomScaleForLevel(1) - Math.max(
        viewport.fitScale,
        viewport.fitScale * mediumLevel.minFitMultiplier,
        Math.min(viewport.width / mediumTarget.width, viewport.height / mediumTarget.height)
      )),
      largeDelta: Math.abs(zoomScaleForLevel(2) - Math.max(
        viewport.fitScale,
        viewport.fitScale * largeLevel.minFitMultiplier,
        Math.min(viewport.width / largeTarget.width, viewport.height / largeTarget.height)
      )),
    };
  });
  expect(coverageScales.fitDelta).toBeLessThan(0.000001);
  expect(coverageScales.mediumDelta).toBeLessThan(0.000001);
  expect(coverageScales.largeDelta).toBeLessThan(0.000001);
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
    createUnitPlacement && createUnitPlacement.deployableHexes.length
  ))).toBeGreaterThan(0);
  const deployPoint = await page.evaluate(() => {
    const hex = createUnitPlacement.deployableHexes[0];
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
      moveDone: horde.moveDone,
      remainingScaledMove: horde.remainingScaledMove,
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
    moveDone: true,
    remainingScaledMove: 0,
    createdKind: "horse_archer",
    createdMove: 4,
    statusPopulation: "3",
    statusHorses: "9",
    statusMetal: "3",
  });
});

test("create mongol lancers spends horde resources and deploys adjacent unit", async ({ page, isMobile }) => {
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
  await expect(page.locator("#context-menu [data-action='create-mongol-lancers']")).toHaveText("Create Mongol Lancers");
  await page.locator("#context-menu [data-action='create-mongol-lancers']").click();

  await expect.poll(() => page.evaluate(() => (
    createUnitPlacement && createUnitPlacement.kind === "mongol_lancer" && createUnitPlacement.deployableHexes.length
  ))).toBeGreaterThan(0);
  const deployPoint = await page.evaluate(() => {
    const hex = createUnitPlacement.deployableHexes[0];
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
      unit.kind === "mongol_lancer" && unit.q === q && unit.r === r
    ));
    return {
      population: horde.population,
      metal: horde.metal,
      horses: horde.horses,
      moveDone: horde.moveDone,
      createdKind: created ? created.kind : "",
      createdAttack: created ? created.attack : 0,
      createdDefense: created ? created.defense : 0,
      createdMove: created ? created.move : 0,
      statusPopulation: document.querySelector("#faction-population-total").textContent,
      statusHorses: document.querySelector("#faction-horses-total").textContent,
      statusMetal: document.querySelector("#faction-metal-total").textContent,
    };
  }, { unitId: hordePoint.unitId, q: deployPoint.q, r: deployPoint.r }), {
    message: "horde resources should be spent and mongol lancers should deploy",
  }).toEqual({
    population: 3,
    metal: 2,
    horses: 9,
    moveDone: true,
    createdKind: "mongol_lancer",
    createdAttack: 5,
    createdDefense: 3,
    createdMove: 4,
    statusPopulation: "3",
    statusHorses: "9",
    statusMetal: "2",
  });
});

test("horde resource actions are unavailable after movement", async ({ page, isMobile }) => {
  test.skip(isMobile, "daemon rule is covered once on desktop");

  await openPlayMode(page);

  const result = await page.evaluate(async () => {
    const horde = currentMap.units.find((unit) => unit.kind === "horde" && unit.faction === "mongol");
    applyGamePatch(await postAppCommand({ type: "select_unit", unitId: horde.id }));
    const move = currentMap.game.legalMoves[0];
    applyGamePatch(await postAppCommand({
      type: "move_unit",
      unitId: horde.id,
      to: { q: move.q, r: move.r },
    }));
    const movedHorde = currentMap.units.find((unit) => unit.id === horde.id);
    const createOptions = await postAppCommand({ type: "create_horse_archers_options", unitId: horde.id });
    const lancerOptions = await postAppCommand({ type: "create_mongol_lancers_options", unitId: horde.id });
    const detachOptions = await postAppCommand({ type: "detach_herd_options", unitId: horde.id, horses: 1 });
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(movedHorde);
    return {
      moveDone: movedHorde.moveDone,
      movedThisTurn: movedHorde.movedThisTurn,
      createDeployable: createOptions.deployableHexes.length,
      lancerDeployable: lancerOptions.deployableHexes.length,
      detachDeployable: detachOptions.deployableHexes.length,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
    };
  });

  expect(result.moveDone).toBe(false);
  expect(result.movedThisTurn).toBe(true);
  expect(result.createDeployable).toBe(0);
  expect(result.lancerDeployable).toBe(0);
  expect(result.detachDeployable).toBe(0);
  await page.mouse.click(result.x, result.y, { button: "right" });
  await expect(page.locator("#context-menu [data-action='detach-herd']")).toHaveCount(0);
  await expect(page.locator("#context-menu [data-action='create-horse-archers']")).toHaveCount(0);
  await expect(page.locator("#context-menu [data-action='create-mongol-lancers']")).toHaveCount(0);
});

test("horde resource actions are unavailable next to enemies", async ({ page, isMobile }) => {
  test.skip(isMobile, "daemon rule is covered once on desktop");

  await openPlayMode(page);

  const result = await page.evaluate(async () => {
    const horde = currentMap.units.find((unit) => unit.kind === "horde" && unit.faction === "mongol");
    applyGamePatch(await postAppCommand({ type: "end_turn" }));
    const enemy = currentMap.units.find((unit) => unit.faction === "chinese" && unit.kind === "chinese_cavalry");
    applyGamePatch(await postAppCommand({ type: "select_unit", unitId: enemy.id }));
    const hordeNeighbors = Array.from({ length: 6 }, (_, direction) => neighborInDirection(horde, direction));
    const adjacentHex = currentMap.game.legalMoves.find((hex) => (
      hordeNeighbors.some((neighbor) => neighbor.q === hex.q && neighbor.r === hex.r)
    ));
    applyGamePatch(await postAppCommand({
      type: "move_unit",
      unitId: enemy.id,
      to: { q: adjacentHex.q, r: adjacentHex.r },
    }));
    applyGamePatch(await postAppCommand({ type: "end_turn" }));
    applyGamePatch(await postAppCommand({ type: "select_unit", unitId: horde.id }));
    const createOptions = await postAppCommand({ type: "create_horse_archers_options", unitId: horde.id });
    const lancerOptions = await postAppCommand({ type: "create_mongol_lancers_options", unitId: horde.id });
    const detachOptions = await postAppCommand({ type: "detach_herd_options", unitId: horde.id, horses: 1 });
    const movedEnemy = currentMap.units.find((unit) => unit.id === enemy.id);
    const activeHorde = currentMap.units.find((unit) => unit.id === horde.id);
    const panel = mapPanel.getBoundingClientRect();
    const center = hexCenter(activeHorde);
    const enemyCenter = hexCenter(movedEnemy);
    return {
      enemyAdjacent: hordeNeighbors.some((neighbor) => neighbor.q === movedEnemy.q && neighbor.r === movedEnemy.r),
      attackable: currentMap.game.legalAttacks.some((attack) => attack.unitId === enemy.id),
      createDeployable: createOptions.deployableHexes.length,
      lancerDeployable: lancerOptions.deployableHexes.length,
      detachDeployable: detachOptions.deployableHexes.length,
      x: panel.left + viewport.offsetX + center.x * viewport.scale,
      y: panel.top + viewport.offsetY + center.y * viewport.scale,
      enemyX: panel.left + viewport.offsetX + enemyCenter.x * viewport.scale,
      enemyY: panel.top + viewport.offsetY + enemyCenter.y * viewport.scale,
    };
  });

  expect(result.enemyAdjacent).toBe(true);
  expect(result.attackable).toBe(true);
  expect(result.createDeployable).toBe(0);
  expect(result.lancerDeployable).toBe(0);
  expect(result.detachDeployable).toBe(0);
  await page.mouse.move(result.enemyX, result.enemyY);
  await expect(page.locator("#combat-preview")).toBeVisible();
  await expect(page.locator("#combat-preview")).toContainText("Combat Preview");
  await expect(page.locator("#combat-preview")).toContainText("Attacker");
  await expect(page.locator("#combat-preview")).toContainText("Defender");
  await expect(page.locator("#combat-preview")).toContainText("Readiness");
  await expect(page.locator("#combat-preview")).toContainText("RDY factor");
  await expect(page.locator("#combat-preview")).toContainText("HP/RDY factor");
  await expect(page.locator("#combat-preview")).toContainText("Readiness impact");
  await expect(page.locator("#combat-preview")).toContainText("Retreat impact");
  await expect(page.locator("#combat-preview")).toContainText("CRT");
  await expect(page.locator("#combat-preview")).toContainText("Retreat");
  await expect(page.locator("#combat-preview")).toContainText("No");
  await expect(page.locator("#combat-preview")).toContainText("Result");
  await expect(page.locator("#combat-preview")).toContainText("RDY damage");
  await expect(page.locator("#combat-preview")).toContainText("Result RDY");
  await page.mouse.click(result.x, result.y, { button: "right" });
  await expect(page.locator("#context-menu [data-action='detach-herd']")).toHaveCount(0);
  await expect(page.locator("#context-menu [data-action='create-horse-archers']")).toHaveCount(0);
  await expect(page.locator("#context-menu [data-action='create-mongol-lancers']")).toHaveCount(0);
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
  await expect(page.locator(".unit-roster-item")).toHaveCount(9);

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
  await expect(page.locator("#undo-button")).toBeEnabled();
  await page.locator("#undo-button").click();
  await expect.poll(() => page.evaluate(() => currentMap.roads.length)).toBe(0);
  await expect(page.locator("#undo-button")).toBeDisabled();

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
  await expect.poll(() => page.evaluate(() => currentMap.units[0].population)).toBe(4);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].metal)).toBe(4);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].horses)).toBe(12);
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.units.length)).toBe(0);

  await page.locator("#editor-unit-type").selectOption("herd");
  point = await hexPoint({ q: 4, r: 3 });
  await page.mouse.click(point.x, point.y);
  await expect.poll(() => page.evaluate(() => currentMap.units.length)).toBe(1);
  await expect.poll(() => page.evaluate(() => currentMap.units[0].kind)).toBe("herd");
  await expect.poll(() => page.evaluate(() => currentMap.units[0].move)).toBe(3);
});
