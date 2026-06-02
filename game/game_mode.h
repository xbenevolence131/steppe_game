#pragma once

namespace steppe::game {

struct GameState;

enum class GameModeKind {
    MapGeneration,
    ScenarioEditor,
    StrategicGame,
    DebugOverlay,
};

struct InputEvent {
    int kind = 0;
};

struct FrameContext {
    double delta_seconds = 0.0;
};

struct UiContext {
    bool debug_overlay_enabled = false;
};

class GameMode {
public:
    virtual ~GameMode() = default;

    virtual GameModeKind kind() const = 0;
    virtual void enter(GameState&) {}
    virtual void exit(GameState&) {}
    virtual void handle_input(GameState&, const InputEvent&) = 0;
    virtual void update(GameState&, const FrameContext&) = 0;
    virtual void build_ui(GameState&, UiContext&) = 0;
};

} // namespace steppe::game
