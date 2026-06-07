#pragma once

#include "game_mode.h"
#include "steppe_generator.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace steppe::game {

using OwnerId = int;
constexpr OwnerId neutral_owner = -1;
constexpr OwnerId mongol_owner = 0;
constexpr OwnerId persian_owner = 1;
constexpr OwnerId chinese_owner = 2;

enum class HexTag : std::uint64_t {
    BaseSteppe = 1ull << 0,
    WildTerrain = 1ull << 1,
    Valley = 1ull << 2,
    ForestBlob = 1ull << 3,
    EasternDesert = 1ull << 4,
    DesertRiverCorridor = 1ull << 5,
    DesertRiverMarsh = 1ull << 6,
    DesertRiverForest = 1ull << 7,
    SteppeHill = 1ull << 8,
    SteppeForest = 1ull << 9,
    SteppeMarsh = 1ull << 10,
    LakeBaikal = 1ull << 11,
    CaspianSea = 1ull << 12,
    ChineseLakes = 1ull << 13,
    RandomLakes = 1ull << 14,
    Urban = 1ull << 15,
    FixedFeatureTown = 1ull << 16,
    WaterAdjacentTown = 1ull << 17,
    PersianTown = 1ull << 18,
    ChineseTown = 1ull << 19,
    ChinaAccessTown = 1ull << 20,
    DzungarianGate = 1ull << 21,
    Oasis = 1ull << 22,
};

using HexTagMask = std::uint64_t;

enum class SettlementKind {
    Generic,
    PersianTown,
    ChineseTown,
    ChinaAccessTown,
    DzungarianGate,
    Oasis,
    WaterAdjacentTown,
};

enum class UnitKind {
    Herd,
    HorseArcher,
    ChineseCavalry,
    MongolLancer,
    ChineseMilitia,
    Infantry,
    Horde,
};

enum class UnitStance {
    Default,
    FeignedRetreat,
    Defensive,
};

struct PastureState {
    int capacity = 0;
    int remaining = 0;
    int recovery_turn = 0;
};

struct GameHex {
    Coord coord;
    Terrain terrain = Terrain::None;
    HexTagMask tags = 0;
    OwnerId owner = neutral_owner;
    PastureState pasture;
    std::vector<std::string> source_labels;
};

struct Settlement {
    int id = 0;
    Coord coord;
    SettlementKind kind = SettlementKind::Generic;
    OwnerId owner = neutral_owner;
    std::string source_feature;
    std::vector<std::string> source_labels;
};

struct FactionState {
    OwnerId id = neutral_owner;
    std::string key;
    std::string name;
    std::string color;
    int metal = 0;
    int treasure = 0;
};

struct Unit {
    int id = 0;
    OwnerId owner = neutral_owner;
    UnitKind kind = UnitKind::HorseArcher;
    UnitStance stance = UnitStance::FeignedRetreat;
    Coord coord;
    int hp = 10;
    int max_hp = 10;
    int attack = 0;
    int defense = 1;
    int readiness_damage = 0;
    int readiness = 100;
    int max_readiness = 100;
    int scaled_move = 32;
    int remaining_scaled_move = 32;
    int population = 0;
    int horses = 0;
    bool move_done = false;
    bool moved_this_turn = false;
    bool combat_done = false;
    bool contacted_enemy_this_turn = false;
    bool projects_zoc = false;
    bool respects_zoc = false;
};

struct UnitDefaults {
    UnitKind kind = UnitKind::HorseArcher;
    UnitStance stance = UnitStance::FeignedRetreat;
    std::vector<UnitStance> legal_stances;
    int hp = 1;
    int attack = 0;
    int defense = 1;
    int readiness_damage = 0;
    int readiness = 100;
    int move = 0;
    bool projects_zoc = false;
    bool respects_zoc = false;
    int population = 0;
    int horses = 0;
};

struct ReachableHex {
    Coord coord;
    int scaled_cost = 0;
};

struct AttackableUnit {
    int unit_id = 0;
    Coord coord;
};

struct CombatantPreview {
    int unit_id = 0;
    OwnerId owner = neutral_owner;
    UnitKind kind = UnitKind::HorseArcher;
    Coord coord;
    Terrain terrain = Terrain::None;
    int hp = 0;
    int max_hp = 0;
    int base_attack = 0;
    int base_defense = 0;
    int base_readiness_damage = 0;
    int hp_percent = 0;
    int readiness = 100;
    int max_readiness = 100;
    int readiness_percent = 100;
    int terrain_defense_percent = 100;
    int flanking_defense_percent = 100;
    int effective_attack = 0;
    int effective_defense = 0;
    int damage_dealt = 0;
    int damage_taken = 0;
    int readiness_damage_dealt = 0;
    int readiness_damage_taken = 0;
    int result_hp = 0;
    int result_readiness = 100;
    bool destroyed = false;
};

struct CombatPreview {
    bool valid = false;
    bool defender_retaliates = false;
    bool defender_flanked = false;
    int flanking_defense_percent = 100;
    int base_differential = 0;
    int hp_ratio_percent = 100;
    int readiness_ratio_percent = 100;
    int condition_ratio_percent = 100;
    int crt_index = 0;
    std::string retreat_option = "none";
    std::string readiness_impact = "Even readiness";
    std::string retreat_impact = "No retreat";
    std::string special_resolution = "normal";
    bool retreat_blocked = false;
    int blocked_retreat_readiness_penalty = 0;
    Coord attacker_retreat_to;
    Coord defender_retreat_to;
    Coord attacker_pursuit_to;
    int pursuit_readiness_penalty = 0;
    CombatantPreview attacker;
    CombatantPreview defender;
};

struct DetachHerdOptions {
    int unit_id = 0;
    int horses = 0;
    std::vector<Coord> deployable_hexes;
};

struct CreateUnitOptions {
    UnitKind kind = UnitKind::HorseArcher;
    int unit_id = 0;
    int population_cost = 1;
    int metal_cost = 1;
    int horses_cost = 3;
    std::vector<Coord> deployable_hexes;
};

using CreateHorseArchersOptions = CreateUnitOptions;

struct GameState {
    int width = 0;
    int height = 0;
    std::uint32_t seed = 1;
    int round = 1;
    int active_faction_index = 0;
    int selected_unit_id = 0;
    GameModeKind active_mode = GameModeKind::StrategicGame;
    std::vector<OwnerId> turn_order;
    std::vector<ReachableHex> legal_moves;
    std::vector<AttackableUnit> legal_attacks;

    std::vector<GameHex> hexes;
    std::vector<RiverEdge> rivers;
    std::vector<RiverSegment> river_segments;
    std::vector<LakeRiverConnection> lake_river_connections;
    std::vector<Road> roads;
    std::vector<Wall> walls;
    std::vector<WallGate> wall_gates;
    std::vector<Crossing> crossings;
    std::vector<Settlement> settlements;
    std::vector<FactionState> factions;
    std::vector<Unit> units;
};

HexTagMask tag_mask(HexTag tag);
bool has_tag(HexTagMask mask, HexTag tag);
HexTagMask tags_from_labels(const std::vector<std::string>& labels);

SettlementKind settlement_kind_from_town(const Town& town);
OwnerId owner_from_town(const Town& town);
PastureState initial_pasture_for_terrain(Terrain terrain);

GameState game_state_from_generated_map(const GeneratedMap& generated);
GameState generate_game_state(const GenerateArgs& args);
GameState create_default_play_sandbox(int width = 10, int height = 10, int faction_count = 2);
void load_unit_types();
const char* unit_kind_key(UnitKind kind);
const char* unit_stance_key(UnitStance stance);
UnitStance unit_stance_from_key(const std::string& stance);
std::vector<UnitKind> unit_kinds();
UnitDefaults unit_defaults(UnitKind kind);

OwnerId active_faction(const GameState& state);
std::vector<ReachableHex> reachable_hexes(const GameState& state, int unit_id);
std::vector<AttackableUnit> attackable_units(const GameState& state, int unit_id);
CombatPreview combat_preview(const GameState& state, int attacker_id, int defender_id);
bool select_unit(GameState& state, int unit_id);
bool set_unit_stance(GameState& state, int unit_id, UnitStance stance);
bool move_unit(GameState& state, int unit_id, Coord destination);
bool attack_unit(GameState& state, int attacker_id, int defender_id);
DetachHerdOptions detach_herd_options(const GameState& state, int unit_id, int horses);
bool detach_herd(GameState& state, int unit_id, int horses, Coord destination);
CreateHorseArchersOptions create_horse_archers_options(const GameState& state, int unit_id);
bool create_horse_archers(GameState& state, int unit_id, Coord destination);
CreateUnitOptions create_mongol_lancers_options(const GameState& state, int unit_id);
bool create_mongol_lancers(GameState& state, int unit_id, Coord destination);
void end_turn(GameState& state);

void print_game_state_json(const GameState& state, std::ostream& out);
void print_reachable_json(const std::vector<ReachableHex>& reachable, std::ostream& out);
void print_attackable_json(const std::vector<AttackableUnit>& attackable, std::ostream& out);
void print_combat_preview_json(const CombatPreview& preview, std::ostream& out);
void print_detach_herd_options_json(const DetachHerdOptions& options, std::ostream& out);
void print_create_unit_options_json(const CreateUnitOptions& options, std::ostream& out);
void print_create_horse_archers_options_json(const CreateHorseArchersOptions& options, std::ostream& out);
void print_create_mongol_lancers_options_json(const CreateUnitOptions& options, std::ostream& out);
void print_game_patch_json(const GameState& state, bool ok, std::ostream& out);
GameState parse_game_state_json(const std::string& json);

} // namespace steppe::game
