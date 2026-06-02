# Steppe Terrain Generator Agent Notes

This is an experimental procedural territory-generation app for a hex-based turn-based Mongol game. The purpose is to explore random map generation that evokes the Eurasian steppe: a broad east-west plains belt with surrounding and crossing terrain features that can later support game rules.

## Design Intent

- Generate maps that read as a large, mostly east-west steppe/plains mass, not scattered generic noise.
- Future rivers should exist on hex edges, not as terrain painted inside hexes. They should cross or drain through the plains and imply river valleys.
- Mountains, hills, lakes, and forests should usually appear around the periphery or as believable regional interruptions, not as uniform decoration.
- Favor coherent geography over visual variety for its own sake.
- The output should remain useful as turn-based game territory: readable, connected, and inspectable on a hex map.

## Architecture

- `engine/` contains the authoritative C++17 generation engine.
- `proxy/` contains a raw Node.js HTTP server that invokes the engine.
- `public/` contains the browser UI for inspecting generated maps.
- Terrain and hydrology decisions should generally live in C++, with the browser focused on visualization.

## Build And Run

```powershell
cmake -S . -B build -G Ninja
cmake --build build
npm start
```

Open `http://localhost:3000`.

On Windows, if a clean build fails with missing standard-library includes, run the build from a Visual Studio developer PowerShell or through `VsDevCmd.bat`.

## Verification

- At minimum, rebuild the C++ engine after changing `engine/main.cpp`.
- Run deterministic CLI samples with fixed seeds and confirm the output parses as JSON.
- When rivers are reintroduced, check that each segment starts and ends at its declared endpoints, that edge paths are continuous, and that collected river edges are deduplicated.
- If changing proxy behavior, test `POST /api/generate` through the local server.

## Working Notes

- The main east-west steppe blob generation is currently in acceptable shape.
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
- Forest blobs are derived after valleys and before urban assignment. They seed from wild terrain adjacent to grassland, then grow as seeded blobby frontier expansions. `Forest blobs` defaults to `4`, valid range is `0-10`, and `0` disables the blobs; `Forest radius` defaults to `4`, valid range is `0-20`, and `0` also disables them. They can cover grassland and wild terrain but not lakes.
- Steppe texture is derived after valleys and forest blobs and before towns: sparse internal `steppe_hill` and `steppe_forest` terrain can vary otherwise plain `base_steppe`, and `steppe_marsh` can appear only on `base_steppe` adjacent to a lake or river edge.
- Non-steppe, non-valley land is filled as wild terrain instead of left empty: hexes near grassland skew toward forest and hills, while farther hexes increasingly become mountains. A coherent `eastern_desert` basin can replace wild terrain and dry out the southern base-steppe fringe east/southeast of the Dzungarian/pinch pass without consuming valleys, lakes, or forest blobs.
- Generated hexes include a terrain-independent `labels` array for final semantic roles. Each generated hex starts with `base_steppe` or `wild_terrain`. `base_steppe` is removed when a forest blob or desert grows onto that hex. `wild_terrain` is removed when a hex becomes valley, lake, or urban, but wild forest blobs and desert keep `wild_terrain`. Current labels include `base_steppe`, `wild_terrain`, `valley`, `forest_blob`, `eastern_desert`, `steppe_hill`, `steppe_forest`, `steppe_marsh`, `lake_baikal`, `caspian_sea`, `chinese_lakes`, `random_lakes`, `urban`, `fixed_feature_town`, `water_adjacent_town`, `persian_town`, `chinese_town`, `dzungarian_gate`, and `oasis`.
- Regional helpers place special towns: Baikal still uses the generic lake-feature town pass, while `generate_persian_region` handles Persian-region towns and `generate_chinese_region` handles China/SE lake-network towns. Persian towns are labelled `persian_town`: one tries land adjacent to the north-east-most Caspian lake hex, one tries another Caspian shoreline site more than 3 hexes away, one follows the westernmost river source and places a valid town near the vertical midpoint with about 4 rows of seeded jitter, and one tries the western map-edge steppe. Chinese towns are labelled `chinese_town`: two try non-adjacent China/SE lake-network shoreline sites, one follows the easternmost river source to a valid town near the vertical midpoint with about 4 rows of seeded jitter, and one tries the eastern map edge around the lower third with seeded jitter. These towns are emitted in top-level `towns` and carry `urban` plus `fixed_feature_town` labels on the hex.
- A final town pass places `2-4` extra `urban` hexes on remaining grassland adjacent to any river edge or lake. These towns use feature `grassland_water` and carry `water_adjacent_town`.
- Roads route from hex center to hex center and are emitted in top-level `roads` as coordinate paths. The browser draws regional roads by owner color and draws `silk_road` as a thicker gold-brown trunk. Current roads connect Persian towns and Chinese towns in nearest-neighbor regional chains. A `dzungarian_gate` town is placed at the central seeded pass in the pinch, an `oasis` town is placed on a river-adjacent grassland/valley hex near the middle of the map, and the `silk_road` routes from the easternmost Chinese town through the oasis and Dzungarian gate to the westernmost Persian town while avoiding lakes and wild terrain. Road routing uses a generic A* hex pathfinder; a final conservative cleanup pass straightens short zigzagging road chunks without changing road endpoints or allowing illegal terrain.
- Bridges and fords are emitted in top-level `crossings` as river-edge crossings. Crossing generation emits bridges first, then road fords, then extra fords at the end of the sequence. Road crossings over river edges get grey bridges when the crossed road edge is town-adjacent, or white fords otherwise; road crossings that would violate the six-hex ford/crossing spacing are bridged. Extra random fords are attempted at 2-4 per river segment, restricted to steppe/valley river edges, jittered along the river path, and spaced more than 6 hexes from existing crossings.
- Current meander defaults are: `Bend forward=8`, `Forward jitter=4`, `Bend lateral=7`, `Lateral jitter=4`, `Bend strength=1`, `Bend reach=2`, `River slant=10`, `Bend timeout=28`.
- Meander scoring currently uses progress toward the active lateral target, not absolute target distance, so bend strength has a visible effect. The router also heavily penalizes northward moves, lightly penalizes flat side moves, and clamps meander influence so lateral bends do not overpower southward drainage.
- Each river gets an independent seed-derived east or west slant direction, producing subtle SSE/SSW angles without overriding southward drainage.
- Each successful browser/proxy map generation writes `latest-map.json` for inspection. This is generated output and should stay untracked.
- The browser has a `Blank Map` mode for editor/template work. It creates a client-side all-grassland map with the current width and height, no procedural terrain, and no river edges.
- Editor mode supports hex terrain painting and river-edge painting in the browser. The current editor terrain keys are `grassland`, `none`, `lake`, `hill`, `mountain`, `forest`, `marsh`, and `urban`.
- Keep changes scoped to the experiment being explored; avoid broad refactors unless they directly improve generation behavior or observability.
- Preserve deterministic seeded generation. A fixed seed should produce the same map for the same parameters.
