# Steppe Terrain Generator

Prototype for an authoritative C++ terrain-generation engine with a raw Node.js web proxy.

The current generator intentionally produces a plain beige grassland map. Hexes are labeled with 1-based `q,r` coordinates where `q` is the horizontal index and `r` is the vertical index from the top-left.

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
