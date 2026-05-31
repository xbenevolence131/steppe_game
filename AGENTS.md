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
- River generation is a fresh baseline pass. Rivers live on hex edges, are routed on the vertex graph, grow generally north-to-south, use tunable steppe-only lateral meander steering, and should preserve clean merge topology.
- Current map defaults are `120` width, `80` height, and `4` river sources.
- Current meander defaults are: `Bend forward=8`, `Forward jitter=4`, `Bend lateral=7`, `Lateral jitter=4`, `Bend strength=1`, `Bend reach=2`, `Bend timeout=28`.
- Meander scoring currently uses progress toward the active lateral target, not absolute target distance, so bend strength has a visible effect. The router also heavily penalizes northward moves, lightly penalizes flat side moves, and clamps meander influence so lateral bends do not overpower southward drainage.
- Each successful browser/proxy map generation writes `latest-map.json` for inspection. This is generated output and should stay untracked.
- Keep changes scoped to the experiment being explored; avoid broad refactors unless they directly improve generation behavior or observability.
- Preserve deterministic seeded generation. A fixed seed should produce the same map for the same parameters.
