# Steppe Terrain Generator Agent Notes

This is an experimental procedural territory-generation app for a hex-based turn-based Mongol Civ-like game. The purpose is to explore random map generation that evokes the Eurasian steppe: a broad east-west plains belt with surrounding and crossing terrain features that can support game rules on one shared strategic map.

## Design Intent

- Generate maps that read as a large, mostly east-west steppe/plains mass, not scattered generic noise.
- Future rivers should exist on hex edges, not as terrain painted inside hexes. They should cross or drain through the plains and imply river valleys.
- Mountains, hills, lakes, and forests should usually appear around the periphery or as believable regional interruptions, not as uniform decoration.
- Favor coherent geography over visual variety for its own sake.
- The output should remain useful as turn-based game territory: readable, connected, and inspectable on a hex map.

## Architecture

- `engine/steppe_generator.cpp` contains the authoritative C++17 generation implementation, exposed through `engine/steppe_generator.h` and built as the `steppe_generator` static library. `engine/main.cpp` is only the CLI wrapper.
- The library API exposes typed `GeneratedMap` data through `generate_map(const GenerateArgs&)`; JSON output should serialize that map via `print_generated_map_json` instead of interleaving generation and printing.
- `game/` contains the first game-facing state model, built as the `steppe_game` static library. It adapts `steppe::GeneratedMap` into `steppe::game::GameState`, stable hex tag bitmasks, settlements, owners, pasture placeholders, and mode-controller interfaces for the same shared strategic map.
- `data/unit_types.csv` is the authoritative unit-type table. The C++ game layer loads it at startup and uses it for unit HP, attack, defense, readiness damage, readiness, movement, ZOC flags, stance options, and resource counters.
- All gameplay rules must be authored in the C++ engine/game layer. A complete game state should be playable by engine commands and data structures with no dependency on the Node proxy or browser UI.
- `engine/steppe_daemon.cpp` is the long-running local engine daemon. It owns game instances in memory and exposes application-level command envelopes over localhost HTTP.
- `proxy/` contains a raw Node.js HTTP server that serves the browser and forwards application commands to the C++ daemon. It should not spawn the CLI per game action or own gameplay rules.
- `public/` contains the browser UI for inspecting generated maps and controlling play. It should render state and submit commands, not own gameplay rules.
- Terrain and hydrology decisions should generally live in C++, with the browser focused on visualization.

## Build And Run

```powershell
npm run build
npm run launch
```

Open `http://localhost:3000`.

Agents should use the project-local npm entry points instead of constructing direct Visual Studio, CMake, Node proxy, daemon, Playwright, or JSON smoke-test commands:

- `npm run build`
- `npm run lint`
- `npm test`
- `npm run test:movement`
- `npm run test:layout`
- `npm run launch`
- `npm run status`
- `npm run diff:stat`

These wrappers encapsulate Visual Studio environment setup, daemon cleanup, port cleanup, Playwright invocation, and smoke-test JSON.

## Verification

- At minimum, run `npm run build` after changing `engine/`, `game/`, `data/unit_types.csv`, or CMake files.
- Run deterministic generation samples with fixed seeds and confirm the output parses as JSON.
- When changing gameplay command flow, test through the Node proxy to the long-running daemon using `POST /api/command`; do not validate by adding browser-owned rule logic.
- When rivers are reintroduced, check that each segment starts and ends at its declared endpoints, that edge paths are continuous, and that collected river edges are deduplicated.
- If changing proxy behavior, test `POST /api/generate` and a basic `new_game -> select_unit -> move_unit` command flow through the local server.

## Working Notes

- The main east-west steppe blob generation is currently in acceptable shape and intentionally biased toward a broad playable steppe. Recent balance tuning widened the base belt, reduced width volatility, softened the Dzungarian pinch slightly, and delayed the wild-terrain transition into mountains so sampled maps do not become mountain-dominant.
- Base steppe generation reserves a river-source band near one-third map width as a seeded mountain/terrain pinch. River sources are placed into `S+1` horizontal bands while skipping that pinch band; the pinch narrows the steppe smoothly and carves one or two seeded pass lanes through the throat.
- River generation is a fresh baseline pass. Rivers live on hex edges, are routed on the vertex graph, grow generally north-to-south, use tunable steppe-only lateral meander steering, and should preserve clean merge topology.
- River merges track edge ownership. If a head merges into an active downstream head, it terminates as a tributary; if it merges into a dead confluence, that head becomes the downstream trunk and keeps routing south.
- Lake/river connectivity is vertex-based: a river/canal sequence may connect to a lake by starting or terminating on any vertex of a lake hex. Do not test this as river-edge face adjacency. The derived `lake_river_connections` array records `river_terminal_to_lake_vertex` links for gameplay/topology code.
- Current map defaults are `120` width, `80` height, and `4` river sources.
- Lake generation grows small lake hex clusters out of selected river edges. Current defaults are `4` lakes with `Lake size=6`.
- A Baikal analogue is always generated as a large northern lake with slight seeded variation in placement, size, aspect ratio, and orientation.
- A Caspian analogue is always generated as the visible eastern edge of a giant off-map western lake/sea, biased slightly south with a seeded wavy shoreline.
- A China/SE lake-network analogue is placed by selecting one saved lake/canal template, choosing a jittered southeast anchor that prefers existing steppe coverage, and overlaying only the template's lake hexes and river edges. It does not repaint non-lake template cells into grassland.
- Valley terrain is derived after rivers and lakes are assembled: seeded, uneven river/lake fringe bands can convert local `none` hexes to `grassland`, while lakes still override all valley terrain. `Valley thickness` defaults to `2` and can be set to `0` to disable the conversion.
- A hilly Dzungarian pass overlay is derived after the eastern desert footprint and before forest blobs. It marks rugged shoulder hexes around the seeded `dzungarian_gate` target as `hill` with the `dzungarian_pass_hill` label while preserving lakes, towns, and desert. These hill hexes are blocked from later forest-blob growth so the pass remains visibly rugged.
- Forest blobs are derived after roads, the eastern desert footprint, and Dzungarian pass hills, then seed from wild terrain adjacent to grassland and grow as seeded blobby frontier expansions. `Forest blobs` defaults to `4`, valid range is `0-10`, and `0` disables the blobs; `Forest radius` defaults to `4`, valid range is `0-20`, and `0` also disables them. They can cover grassland and wild terrain but not lakes, desert, towns, road paths, or Dzungarian pass hills.
- Steppe texture is derived after valleys and forest blobs and before towns: sparse internal `steppe_hill` and `steppe_forest` terrain can vary otherwise plain `base_steppe`, and `steppe_marsh` can appear only on `base_steppe` adjacent to a lake or river edge.
- Non-steppe, non-valley land is filled as wild terrain instead of left empty: hexes near grassland skew toward forest and hills, while farther hexes increasingly become mountains. A coherent `eastern_desert` basin can replace wild terrain and dry out the southern base-steppe fringe east/southeast of the Dzungarian/pinch pass without consuming valleys, lakes, towns, or forest blobs. Roads are overlays and do not protect underlying terrain from becoming desert. Rivers near desert carve a `desert_river_corridor` out of nearby desert hexes, mostly as grassland with infrequent forest; any generated marsh must be directly adjacent to a river edge or lake hex.
- Generated hexes include a terrain-independent `labels` array for final semantic roles. Each generated hex starts with `base_steppe` or `wild_terrain`. `base_steppe` is removed when a forest blob or desert grows onto that hex. `wild_terrain` is removed when a hex becomes valley, lake, urban, or desert river corridor, but wild forest blobs and desert keep `wild_terrain`. Current labels include `base_steppe`, `wild_terrain`, `valley`, `forest_blob`, `eastern_desert`, `dzungarian_pass_hill`, `desert_river_corridor`, `desert_river_marsh`, `desert_river_forest`, `steppe_hill`, `steppe_forest`, `steppe_marsh`, `lake_baikal`, `caspian_sea`, `chinese_lakes`, `random_lakes`, `urban`, `fixed_feature_town`, `water_adjacent_town`, `persian_town`, `chinese_town`, `china_access_town`, `dzungarian_gate`, and `oasis`.
- Game code should consume `steppe::game::GameState` from `game_state_from_generated_map` or `generate_game_state`, not raw JSON. Modes should operate on shared `GameState`; switching modes should not regenerate or discard state unless explicitly requested.
- Movement uses integer scaled costs to avoid fractional pathfinding drift. The scale factor is `8`: `scaled_move` is authoritative, `refMove = scaledMove / 8`, and legacy JSON `move` fields are compatibility aliases. Grassland and desert cost `8`; hills and forests cost `12`; urban, marsh, and mountains cost `16`; lakes and `none` are blocked. Road-connected steps modify the destination terrain cost by unit class: mounted combat units use `ceil(base / 2) + 1`, while other units use `ceil(base / 2)`.
- The `mongol` faction is the current playable steppe-nomad sandbox faction, not the only intended steppe-nomad people. Future steppe-nomad factions such as the Naimans should reuse the same steppe unit kinds and horde resource actions as the Mongols unless a specific future rule says otherwise. Do not create duplicate Naiman-specific unit kinds just to represent the same horse-archers, lancers, herds, or hordes under another steppe faction.
- Current combat-capable unit kinds include horse-archers, Chinese cavalry, Chinese militia, Mongol/steppe lancers, infantry, and horde. The current A/D, readiness damage, movement, ZOC, stance, and resource defaults are defined in `data/unit_types.csv`, not hardcoded in the browser.
- Normal combat retreat is a deterministic CRT result. The retreating unit tries to move to an adjacent passable, unoccupied hex that preserves or increases distance from the opponent, respecting enemy ZOC when that unit respects ZOC. If CRT retreat is blocked, the unit stays in place and takes an additional `15` readiness damage. Feigned retreat remains a separate horse-archer stance behavior.
- Faction-level resources live under `game.factions`. Current global faction resources are `metal` and `treasure`; unit-local resource counters represent physical assets carried by a unit.
- Scenario factions carry `enabled` and `ai` flags. Enabled factions define the turn order; AI-controlled factions are advanced through automatically by `end_turn` until the next human-controlled faction. The current tactical AI is a minimal greedy executor that attacks adjacent enemies or advances combat units toward the nearest enemy, with full directive parsing still pending.
- Each horde carries non-negative integer resource counters: `population` and `horses`. These serialize on units and are shown in the unit inspector only when a horde is selected. Default play sandbox hordes currently start with `population=4` and `horses=12`.
- Horde resource actions are `detach_herd` and `create_horse_archers`. Active-faction hordes with `horses > 0` may detach horses into a new adjacent herd. Active-faction hordes with at least `population=1` and `horses=3`, plus enough active-faction `metal`, may create adjacent horse-archers or lancers. The browser may collect inputs and draw deployment highlights, but C++ returns legal deploy hexes and revalidates amount, destination, ownership, occupancy, terrain, global faction resources, and costs before mutating state.
- Herd is the first movable resource unit. It has move `3`, does not project or respect ZOC, and is not currently combat-capable.
- Regional helpers place special towns: Baikal still uses the generic lake-feature town pass, while `generate_persian_region` handles Persian-region towns and `generate_chinese_region` handles China/SE lake-network towns. Persian towns are labelled `persian_town`: one tries land adjacent to the north-east-most Caspian lake hex, one tries another Caspian shoreline site more than 3 hexes away, one follows the westernmost river source and places a valid town near the vertical midpoint with about 4 rows of seeded jitter, and one tries the western map-edge steppe. Chinese towns are labelled `chinese_town`: two try non-adjacent China/SE lake-network shoreline sites, one follows the easternmost river source to a valid town near the vertical midpoint with about 4 rows of seeded jitter, one adds a `china_access_town` near the easternmost river destination, and one tries the eastern map edge around the lower third with seeded jitter. These towns are emitted in top-level `towns` and carry `urban` plus `fixed_feature_town` labels on the hex.
- A final town pass places `2-4` extra `urban` hexes on remaining grassland adjacent to any river edge or lake. These towns use feature `grassland_water` and carry `water_adjacent_town`.
- Roads route from hex center to hex center and are emitted in top-level `roads` as coordinate paths. The browser draws regional roads by owner color and draws `silk_road` as a thicker gold-brown trunk. Persian and Chinese regional roads now build clean local meshes: a trunk connects towns along the region's primary axis, then a small number of secondary links may close larger loops, while links that would create tiny triangle clutter are rejected. A `dzungarian_gate` town is placed at the central seeded pass in the pinch, and an `oasis` town is placed on a river-adjacent grassland/valley hex near the middle of the map. The `silk_road` routes from a Persian anchor through the Dzungarian gate and oasis to a Chinese anchor while avoiding lakes and wild terrain. Silk Road anchors are biased toward the east/west edge of their region, but if another town is only a few columns less extreme and much closer to the Dzungarian/oasis corridor row, the more central-row town is chosen to avoid unnatural southern detours. Road routing uses a generic A* hex pathfinder; a final conservative cleanup pass straightens short zigzagging road chunks without changing road endpoints or allowing illegal terrain.
- The Great Wall is emitted in top-level `walls` as continuous hex-edge paths and is generated after final terrain as a last overlay. Its strategic anchors are based on the Chinese-town envelope; the wall route is built on the edge/vertex graph so every segment shares a vertex with the next. Endpoints extend outward to a mountain edge or map edge, with hard map-edge fallback retries when mountain termini do not contain China. A flood-fill validation treats wall edges, mountains, and lakes as barriers and rejects routes where any `chinese_town` is reachable from the north/west outside side.
- Great Wall gates are emitted in top-level `wall_gates`. Road/wall intersections become `silk_gate` or `road_gate`, and supplemental spaced `gate` entries are added along the wall so China is not sealed behind only one or two crossings. The browser renders gate markers on the wall, and `GameState` preserves `walls` and `wall_gates` for later movement and border rules.
- Bridges and fords are emitted in top-level `crossings` as river-edge crossings. Crossing generation emits bridges first, then road fords, then extra fords at the end of the sequence. Road crossings over river edges get grey bridges when the crossed road edge is town-adjacent, or white fords otherwise; road crossings that would violate the six-hex ford/crossing spacing are bridged. Extra random fords are attempted at 2-4 per river segment, restricted to steppe/valley river edges, jittered along the river path, and spaced more than 6 hexes from existing crossings.
- Current meander defaults are: `Bend forward=8`, `Forward jitter=4`, `Bend lateral=7`, `Lateral jitter=4`, `Bend strength=1`, `Bend reach=2`, `River slant=10`, `Bend timeout=28`.
- Meander scoring currently uses progress toward the active lateral target, not absolute target distance, so bend strength has a visible effect. The router also heavily penalizes northward moves, lightly penalizes flat side moves, and clamps meander influence so lateral bends do not overpower southward drainage.
- Each river gets an independent seed-derived east or west slant direction, producing subtle SSE/SSW angles without overriding southward drainage.
- Each successful browser/proxy map generation writes `latest-map.json` for inspection. This is generated output and should stay untracked.
- The browser has a `Blank Map` mode for editor/template work. It creates a client-side all-grassland map with the current width and height, no procedural terrain, and no river edges.
- Editor mode supports separate `Terrain`, `Edges`, and `Units` modes in the browser. Terrain mode paints hex terrain. Edges mode toggles river or road edge features. Units mode toggles selected unit type and side on a hex. The current editor terrain keys are `grassland`, `none`, `lake`, `hill`, `mountain`, `forest`, `marsh`, and `urban`.
- The browser layout keeps scenario-edit controls above the shared map/workbench area. In play mode, the left sidebar is reserved for turn controls, faction counts, selected-unit summary, and a compact roster of deployed units; the detailed unit inspector lives in the bottom details panel on desktop.
- This is currently a PC/desktop-focused project. Do not spend time optimizing mobile layout or chasing mobile-only layout failures unless the user explicitly asks for mobile support.
- In play mode, a resource status bar sits above the sidebar/map workbench and shows active-faction horde `population` and `horses` plus global faction `metal` and `treasure`.
- Playwright layout tests cover the scenario-control placement, play sidebar/unit inspector behavior, mobile play layout, editor mode toggles, and a deterministic generated Great Wall containment invariant. The Playwright config currently runs with `workers: 1` so the heavier generation invariant does not interfere with browser-layout assertions.
- Keep changes scoped to the experiment being explored; avoid broad refactors unless they directly improve generation behavior or observability.
- Preserve deterministic seeded generation. A fixed seed should produce the same map for the same parameters.
