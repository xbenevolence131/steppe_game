# Steppe Terrain Generator

Prototype for an authoritative C++ terrain-generation engine with a raw Node.js web proxy.

The current generator produces a single east-west-ish grassland steppe blob surrounded by `none` terrain. Hexes are labeled with 1-based `q,r` coordinates where `q` is the horizontal index and `r` is the vertical index from the top-left.
Each Generate click sends a new seed to the engine, producing a different corridor while keeping all terrain decisions inside C++.
Rivers are generated as edge-based paths on the hex vertex graph. Configurable northern sources are spaced across the map, grow mostly southward, use alternating lateral steering targets while crossing steppe, and record merge points when they join existing river paths.

## Layout

- `engine/` contains the C++ engine CLI.
- `proxy/` contains the raw Node.js HTTP server.
- `public/` contains the browser UI.

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
