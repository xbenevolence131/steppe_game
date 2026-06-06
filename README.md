# Steppe Terrain Generator

Prototype for an authoritative C++ terrain-generation and single-map Civ-like game-state engine with a raw Node.js web proxy.

The generator builds deterministic hex maps for a turn-based Mongol Civ-like game experiment. Maps are designed around a broad east-west steppe belt with edge-based rivers, lakes, valleys, wild terrain, regional towns, local road meshes, a Silk Road trunk, the Dzungarian pass, eastern desert, and a Great Wall with gates around the Chinese region. Hexes use 1-based `q,r` coordinates where `q` is the horizontal index and `r` is the vertical index from the top-left.

Each Generate click sends a seed and parameters to the C++ engine. Terrain, hydrology, settlements, roads, walls, crossings, and game-facing metadata are generated in C++; the browser renders state and sends commands.

## Current Features

- Coherent steppe-first terrain generation with peripheral hills, forests, mountains, desert, marshes, and lake/river valleys.
- Edge-based rivers, roads, walls, bridges, fords, and Great Wall gates.
- Persian and Chinese town regions with cleaner local road meshes and a Silk Road route biased toward useful central anchors.
- Single strategic map state with units, owners, turn state, selectable units, reachable movement, ZOC flags, integer-scaled movement costs, horde resource counters, horde-to-herd detachment, and horde creation of steppe units such as horse-archers and lancers.
- The current `mongol` side is the first steppe-nomad sandbox faction. Other steppe-nomad factions, for example the Naimans, are expected to use the same steppe unit kinds and horde mechanics rather than separate duplicate unit types.
- Browser scenario editor with `Terrain`, `Edges`, and `Units` modes. Terrain paints hexes, Edges toggles roads/rivers, and Units toggles deployed units by side and type.
- Play UI with an active-faction resource status bar above the shared map, a left unit roster/sidebar, and a bottom unit inspector on desktop.

## Layout

- `engine/` contains the authoritative C++17 generator, JSON serialization, CLI wrapper, and long-running local daemon.
- `game/` contains the game-facing state layer and command handling for the shared strategic map.
- `proxy/` contains the raw Node.js HTTP server that serves the browser and forwards commands to the daemon.
- `public/` contains the browser UI for map inspection, scenario editing, and play controls.
- `tests/` contains Playwright coverage for layout, editor behavior, and generated-map invariants.

## Build

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

If MSVC is installed but not active in the shell, run the commands from a Visual Studio developer PowerShell.

## Run

```powershell
npm start
```

Then open `http://localhost:3000`.
