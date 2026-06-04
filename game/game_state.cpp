#include "game_state.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace steppe::game {

HexTagMask tag_mask(HexTag tag) {
    return static_cast<HexTagMask>(tag);
}

bool has_tag(HexTagMask mask, HexTag tag) {
    return (mask & tag_mask(tag)) != 0;
}

namespace {

void add_tag(HexTagMask& mask, HexTag tag) {
    mask |= tag_mask(tag);
}

bool contains_label(const std::vector<std::string>& labels, const std::string& label) {
    return std::find(labels.begin(), labels.end(), label) != labels.end();
}

void add_tag_for_label(HexTagMask& mask, const std::string& label) {
    if (label == "base_steppe") {
        add_tag(mask, HexTag::BaseSteppe);
    } else if (label == "wild_terrain") {
        add_tag(mask, HexTag::WildTerrain);
    } else if (label == "valley") {
        add_tag(mask, HexTag::Valley);
    } else if (label == "forest_blob") {
        add_tag(mask, HexTag::ForestBlob);
    } else if (label == "eastern_desert") {
        add_tag(mask, HexTag::EasternDesert);
    } else if (label == "desert_river_corridor") {
        add_tag(mask, HexTag::DesertRiverCorridor);
    } else if (label == "desert_river_marsh") {
        add_tag(mask, HexTag::DesertRiverMarsh);
    } else if (label == "desert_river_forest") {
        add_tag(mask, HexTag::DesertRiverForest);
    } else if (label == "steppe_hill") {
        add_tag(mask, HexTag::SteppeHill);
    } else if (label == "steppe_forest") {
        add_tag(mask, HexTag::SteppeForest);
    } else if (label == "steppe_marsh") {
        add_tag(mask, HexTag::SteppeMarsh);
    } else if (label == "lake_baikal") {
        add_tag(mask, HexTag::LakeBaikal);
    } else if (label == "caspian_sea") {
        add_tag(mask, HexTag::CaspianSea);
    } else if (label == "chinese_lakes") {
        add_tag(mask, HexTag::ChineseLakes);
    } else if (label == "random_lakes") {
        add_tag(mask, HexTag::RandomLakes);
    } else if (label == "urban") {
        add_tag(mask, HexTag::Urban);
    } else if (label == "fixed_feature_town") {
        add_tag(mask, HexTag::FixedFeatureTown);
    } else if (label == "water_adjacent_town") {
        add_tag(mask, HexTag::WaterAdjacentTown);
    } else if (label == "persian_town") {
        add_tag(mask, HexTag::PersianTown);
    } else if (label == "chinese_town") {
        add_tag(mask, HexTag::ChineseTown);
    } else if (label == "china_access_town") {
        add_tag(mask, HexTag::ChinaAccessTown);
    } else if (label == "dzungarian_gate") {
        add_tag(mask, HexTag::DzungarianGate);
    } else if (label == "oasis") {
        add_tag(mask, HexTag::Oasis);
    }
}

bool coord_less(const Coord& first, const Coord& second) {
    if (first.q != second.q) {
        return first.q < second.q;
    }
    return first.r < second.r;
}

bool coord_equal(const Coord& first, const Coord& second) {
    return first.q == second.q && first.r == second.r;
}

Coord neighbor_in_direction(const Coord& coord, int direction) {
    const bool shifted_down = (coord.q - 1) % 2 == 1;
    switch (direction) {
        case 0: return {coord.q + 1, coord.r};
        case 1: return shifted_down ? Coord{coord.q + 1, coord.r + 1} : Coord{coord.q + 1, coord.r - 1};
        case 2: return {coord.q, coord.r - 1};
        case 3: return {coord.q - 1, coord.r};
        case 4: return shifted_down ? Coord{coord.q - 1, coord.r + 1} : Coord{coord.q - 1, coord.r - 1};
        case 5: return {coord.q, coord.r + 1};
    }
    return coord;
}

bool in_bounds(const GameState& state, const Coord& coord) {
    return coord.q >= 1 && coord.q <= state.width && coord.r >= 1 && coord.r <= state.height;
}

const char* terrain_to_string(Terrain terrain) {
    switch (terrain) {
        case Terrain::None: return "none";
        case Terrain::Grassland: return "grassland";
        case Terrain::Lake: return "lake";
        case Terrain::Hill: return "hill";
        case Terrain::Mountain: return "mountain";
        case Terrain::Forest: return "forest";
        case Terrain::Marsh: return "marsh";
        case Terrain::Desert: return "desert";
        case Terrain::Urban: return "urban";
    }
    return "none";
}

Terrain terrain_from_string(const std::string& terrain) {
    if (terrain == "grassland") return Terrain::Grassland;
    if (terrain == "lake") return Terrain::Lake;
    if (terrain == "hill") return Terrain::Hill;
    if (terrain == "mountain") return Terrain::Mountain;
    if (terrain == "forest") return Terrain::Forest;
    if (terrain == "marsh") return Terrain::Marsh;
    if (terrain == "desert") return Terrain::Desert;
    if (terrain == "urban") return Terrain::Urban;
    return Terrain::None;
}

const char* unit_kind_to_string(UnitKind kind) {
    switch (kind) {
        case UnitKind::Camp: return "camp";
        case UnitKind::Herd: return "herd";
        case UnitKind::HorseArcher: return "horse_archer";
        case UnitKind::Infantry: return "infantry";
        case UnitKind::Horde: return "horde";
    }
    return "horse_archer";
}

UnitKind unit_kind_from_string(const std::string& kind) {
    if (kind == "camp") return UnitKind::Camp;
    if (kind == "herd") return UnitKind::Herd;
    if (kind == "horse_archer" || kind == "cavalry") return UnitKind::HorseArcher;
    if (kind == "infantry") return UnitKind::Infantry;
    if (kind == "horde") return UnitKind::Horde;
    return UnitKind::HorseArcher;
}

Clan clan_for_owner(OwnerId owner) {
    if (owner == mongol_owner) {
        return {mongol_owner, "mongol", "Mongol", "#2368c4"};
    }
    if (owner == chinese_owner) {
        return {chinese_owner, "chinese", "Chinese", "#c93632"};
    }
    if (owner == persian_owner) {
        return {persian_owner, "persian", "Persian", "#8a4fb0"};
    }
    return {neutral_owner, "neutral", "Neutral", "#777777"};
}

int default_hp(UnitKind kind) {
    if (kind == UnitKind::HorseArcher || kind == UnitKind::Infantry || kind == UnitKind::Horde) {
        return 10;
    }
    return 1;
}

int default_move(UnitKind kind) {
    if (kind == UnitKind::Herd) {
        return 3;
    }
    if (kind == UnitKind::HorseArcher) {
        return 4;
    }
    if (kind == UnitKind::Infantry) {
        return 2;
    }
    if (kind == UnitKind::Horde) {
        return 3;
    }
    return 0;
}

int default_attack(UnitKind kind) {
    if (kind == UnitKind::HorseArcher) {
        return 4;
    }
    if (kind == UnitKind::Infantry) {
        return 3;
    }
    if (kind == UnitKind::Horde) {
        return 2;
    }
    return 0;
}

int default_defense(UnitKind kind) {
    if (kind == UnitKind::HorseArcher) {
        return 3;
    }
    if (kind == UnitKind::Infantry) {
        return 5;
    }
    if (kind == UnitKind::Horde) {
        return 3;
    }
    return 1;
}

bool default_projects_zoc(UnitKind kind) {
    return kind == UnitKind::HorseArcher || kind == UnitKind::Infantry || kind == UnitKind::Horde;
}

bool default_respects_zoc(UnitKind kind) {
    return kind == UnitKind::Infantry || kind == UnitKind::Horde;
}

bool can_attack(UnitKind kind) {
    return default_attack(kind) > 0;
}

int next_unit_id(const GameState& state) {
    int next_id = 1;
    for (const Unit& unit : state.units) {
        next_id = std::max(next_id, unit.id + 1);
    }
    return next_id;
}

constexpr int create_horse_archers_population_cost = 1;
constexpr int create_horse_archers_metal_cost = 1;
constexpr int create_horse_archers_horses_cost = 3;
constexpr int default_max_readiness = 100;
constexpr int minimum_combat_readiness_percent = 25;
constexpr int full_move_readiness_cost = 25;
constexpr int attack_readiness_cost = 30;
constexpr int defense_readiness_cost = 20;
constexpr int turn_readiness_recovery = 25;

constexpr int move_scale = 8;

int to_scaled_move(int ref_move) {
    return ref_move * move_scale;
}

int to_scaled_move(double ref_move) {
    return static_cast<int>(std::lround(ref_move * move_scale));
}

double to_ref_move(int scaled_move) {
    return static_cast<double>(scaled_move) / move_scale;
}

int blocked_movement_cost() {
    return std::numeric_limits<int>::max();
}

int terrain_movement_cost(Terrain terrain) {
    switch (terrain) {
        case Terrain::Grassland:
        case Terrain::Desert:
        case Terrain::Urban:
            return 8;
        case Terrain::Hill:
        case Terrain::Forest:
            return 12;
        case Terrain::Marsh:
        case Terrain::Mountain:
            return 16;
        case Terrain::None:
        case Terrain::Lake:
            return blocked_movement_cost();
    }
    return blocked_movement_cost();
}

bool road_connects(const GameState& state, const Coord& first, const Coord& second) {
    for (const Road& road : state.roads) {
        for (std::size_t i = 1; i < road.path.size(); ++i) {
            if ((coord_equal(road.path[i - 1], first) && coord_equal(road.path[i], second))
                || (coord_equal(road.path[i - 1], second) && coord_equal(road.path[i], first))) {
                return true;
            }
        }
    }
    return false;
}

int movement_cost(const GameState& state, const Coord& from, const GameHex& to_hex) {
    if (road_connects(state, from, to_hex.coord)) {
        return 6;
    }
    return terrain_movement_cost(to_hex.terrain);
}

bool adjacent(const Coord& first, const Coord& second) {
    for (int direction = 0; direction < 6; ++direction) {
        if (coord_equal(neighbor_in_direction(first, direction), second)) {
            return true;
        }
    }
    return false;
}

std::string escape_json(const std::string& text) {
    std::string escaped;
    for (const char ch : text) {
        if (ch == '"' || ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

std::size_t find_matching(const std::string& json, std::size_t open_index, char open, char close) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t i = open_index; i < json.size(); ++i) {
        const char ch = json[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
        } else if (ch == open) {
            ++depth;
        } else if (ch == close) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    throw std::runtime_error("invalid JSON structure");
}

std::string object_field(const std::string& json, const std::string& key) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) {
        return {};
    }
    std::size_t value_start = start + marker.size();
    while (value_start < json.size() && std::isspace(static_cast<unsigned char>(json[value_start]))) {
        ++value_start;
    }
    if (value_start >= json.size()) {
        return {};
    }
    if (json[value_start] == '[') {
        return json.substr(value_start, find_matching(json, value_start, '[', ']') - value_start + 1);
    }
    if (json[value_start] == '{') {
        return json.substr(value_start, find_matching(json, value_start, '{', '}') - value_start + 1);
    }
    if (json[value_start] == '"') {
        std::size_t end = value_start + 1;
        bool escaped = false;
        for (; end < json.size(); ++end) {
            if (escaped) {
                escaped = false;
            } else if (json[end] == '\\') {
                escaped = true;
            } else if (json[end] == '"') {
                break;
            }
        }
        return json.substr(value_start, end - value_start + 1);
    }
    std::size_t end = value_start;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') {
        ++end;
    }
    return json.substr(value_start, end - value_start);
}

int int_field(const std::string& json, const std::string& key, int fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return std::stoi(field);
    } catch (...) {
        return fallback;
    }
}

double number_field(const std::string& json, const std::string& key, double fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return std::stod(field);
    } catch (...) {
        return fallback;
    }
}

bool bool_field(const std::string& json, const std::string& key, bool fallback) {
    const std::string field = object_field(json, key);
    if (field == "true") {
        return true;
    }
    if (field == "false") {
        return false;
    }
    return fallback;
}

std::uint32_t uint_field(const std::string& json, const std::string& key, std::uint32_t fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::uint32_t>(std::stoul(field));
    } catch (...) {
        return fallback;
    }
}

std::string string_field(const std::string& json, const std::string& key, const std::string& fallback = "") {
    const std::string field = object_field(json, key);
    if (field.size() < 2 || field.front() != '"' || field.back() != '"') {
        return fallback;
    }
    return field.substr(1, field.size() - 2);
}

std::vector<std::string> object_array_items(const std::string& array_json) {
    std::vector<std::string> items;
    if (array_json.size() < 2 || array_json.front() != '[') {
        return items;
    }
    for (std::size_t i = 1; i + 1 < array_json.size(); ++i) {
        if (array_json[i] != '{') {
            continue;
        }
        const std::size_t end = find_matching(array_json, i, '{', '}');
        items.push_back(array_json.substr(i, end - i + 1));
        i = end;
    }
    return items;
}

std::vector<OwnerId> parse_turn_order(const std::string& array_json) {
    std::vector<OwnerId> owners;
    std::string number;
    for (const char ch : array_json) {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-') {
            number.push_back(ch);
        } else if (!number.empty()) {
            owners.push_back(std::stoi(number));
            number.clear();
        }
    }
    if (!number.empty()) {
        owners.push_back(std::stoi(number));
    }
    return owners;
}

} // namespace

HexTagMask tags_from_labels(const std::vector<std::string>& labels) {
    HexTagMask mask = 0;
    for (const std::string& label : labels) {
        add_tag_for_label(mask, label);
    }
    return mask;
}

SettlementKind settlement_kind_from_town(const Town& town) {
    if (contains_label(town.labels, "china_access_town")) {
        return SettlementKind::ChinaAccessTown;
    }
    if (town.feature == "persian_town") {
        return SettlementKind::PersianTown;
    }
    if (town.feature == "chinese_town") {
        return SettlementKind::ChineseTown;
    }
    if (town.feature == "dzungarian_gate") {
        return SettlementKind::DzungarianGate;
    }
    if (town.feature == "oasis") {
        return SettlementKind::Oasis;
    }
    if (town.feature == "grassland_water") {
        return SettlementKind::WaterAdjacentTown;
    }
    return SettlementKind::Generic;
}

OwnerId owner_from_town(const Town& town) {
    if (town.feature == "persian_town") {
        return persian_owner;
    }
    if (town.feature == "chinese_town") {
        return chinese_owner;
    }
    return neutral_owner;
}

PastureState initial_pasture_for_terrain(Terrain terrain) {
    switch (terrain) {
        case Terrain::Grassland:
            return {4, 4, 0};
        case Terrain::Marsh:
        case Terrain::Forest:
            return {1, 1, 0};
        default:
            return {};
    }
}

GameState game_state_from_generated_map(const GeneratedMap& generated) {
    GameState state;
    state.width = generated.width;
    state.height = generated.height;
    state.seed = generated.seed;
    state.hexes.reserve(generated.hexes.size());
    state.settlements.reserve(generated.towns.size());

    for (const Hex& hex : generated.hexes) {
        GameHex game_hex;
        game_hex.coord = hex.coord;
        game_hex.terrain = hex.terrain;
        game_hex.tags = tags_from_labels(hex.labels);
        game_hex.pasture = initial_pasture_for_terrain(hex.terrain);
        game_hex.source_labels = hex.labels;
        if (has_tag(game_hex.tags, HexTag::PersianTown)) {
            game_hex.owner = persian_owner;
        } else if (has_tag(game_hex.tags, HexTag::ChineseTown)) {
            game_hex.owner = chinese_owner;
        }
        state.hexes.push_back(std::move(game_hex));
    }

    int next_settlement_id = 1;
    for (const Town& town : generated.towns) {
        Settlement settlement;
        settlement.id = next_settlement_id++;
        settlement.coord = town.coord;
        settlement.kind = settlement_kind_from_town(town);
        settlement.owner = owner_from_town(town);
        settlement.source_feature = town.feature;
        settlement.source_labels = town.labels;
        state.settlements.push_back(std::move(settlement));
    }

    state.rivers = generated.edges;
    state.river_segments = generated.river_segments;
    state.lake_river_connections = generated.lake_river_connections;
    state.roads = generated.roads;
    state.walls = generated.walls;
    state.wall_gates = generated.wall_gates;
    state.crossings = generated.crossings;
    state.clans = {
        clan_for_owner(neutral_owner),
        clan_for_owner(mongol_owner),
        clan_for_owner(persian_owner),
        clan_for_owner(chinese_owner),
    };
    state.turn_order = {mongol_owner, chinese_owner};

    return state;
}

GameState generate_game_state(const GenerateArgs& args) {
    return game_state_from_generated_map(generate_map(args));
}

GameState create_default_play_sandbox(int width, int height, int faction_count) {
    GameState state;
    state.width = std::max(1, width);
    state.height = std::max(1, height);
    state.seed = 0;
    state.round = 1;
    state.active_faction_index = 0;
    state.clans = {
        clan_for_owner(mongol_owner),
        clan_for_owner(chinese_owner),
        clan_for_owner(persian_owner),
    };

    const OwnerId default_order[] = {mongol_owner, chinese_owner, persian_owner};
    faction_count = std::max(1, std::min(faction_count, 3));
    state.turn_order.assign(default_order, default_order + faction_count);

    state.hexes.reserve(static_cast<std::size_t>(state.width * state.height));
    for (int r = 1; r <= state.height; ++r) {
        for (int q = 1; q <= state.width; ++q) {
            GameHex hex;
            hex.coord = {q, r};
            hex.terrain = Terrain::Grassland;
            hex.tags = tag_mask(HexTag::BaseSteppe);
            hex.pasture = initial_pasture_for_terrain(hex.terrain);
            hex.source_labels = {"base_steppe"};
            state.hexes.push_back(std::move(hex));
        }
    }

    auto make_unit = [](int id, OwnerId owner, UnitKind kind, Coord coord) {
        Unit unit;
        unit.id = id;
        unit.owner = owner;
        unit.kind = kind;
        unit.coord = coord;
        unit.hp = default_hp(unit.kind);
        unit.max_hp = unit.hp;
        unit.attack = default_attack(unit.kind);
        unit.defense = default_defense(unit.kind);
        unit.max_readiness = default_max_readiness;
        unit.readiness = unit.max_readiness;
        unit.scaled_move = to_scaled_move(default_move(unit.kind));
        unit.remaining_scaled_move = unit.scaled_move;
        if (unit.kind == UnitKind::Horde) {
            unit.population = 4;
            unit.metal = 4;
            unit.horses = 12;
        }
        unit.projects_zoc = default_projects_zoc(kind);
        unit.respects_zoc = default_respects_zoc(kind);
        return unit;
    };

    state.units = {
        make_unit(1, mongol_owner, UnitKind::HorseArcher, {3, 5}),
        make_unit(2, mongol_owner, UnitKind::Infantry, {3, 7}),
        make_unit(3, mongol_owner, UnitKind::Horde, {3, 6}),
        make_unit(4, mongol_owner, UnitKind::Herd, {2, 6}),
        make_unit(5, chinese_owner, UnitKind::HorseArcher, {8, 5}),
        make_unit(6, chinese_owner, UnitKind::Infantry, {8, 7}),
        make_unit(7, chinese_owner, UnitKind::Horde, {8, 6}),
        make_unit(8, chinese_owner, UnitKind::Herd, {9, 6}),
    };
    return state;
}

OwnerId active_faction(const GameState& state) {
    if (state.turn_order.empty()) {
        return mongol_owner;
    }
    const int index = std::max(0, std::min(state.active_faction_index, static_cast<int>(state.turn_order.size()) - 1));
    return state.turn_order[static_cast<std::size_t>(index)];
}

const GameHex* find_hex(const GameState& state, const Coord& coord) {
    const auto found = std::find_if(state.hexes.begin(), state.hexes.end(), [&](const GameHex& hex) {
        return coord_equal(hex.coord, coord);
    });
    return found == state.hexes.end() ? nullptr : &*found;
}

Unit* find_unit(GameState& state, int unit_id) {
    const auto found = std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id == unit_id;
    });
    return found == state.units.end() ? nullptr : &*found;
}

const Unit* find_unit(const GameState& state, int unit_id) {
    const auto found = std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id == unit_id;
    });
    return found == state.units.end() ? nullptr : &*found;
}

int terrain_defense_percent(Terrain terrain) {
    switch (terrain) {
        case Terrain::Hill:
        case Terrain::Forest:
            return 125;
        case Terrain::Mountain:
            return 150;
        case Terrain::Urban:
            return 115;
        case Terrain::Marsh:
            return 90;
        case Terrain::Grassland:
        case Terrain::Desert:
        case Terrain::None:
        case Terrain::Lake:
            return 100;
    }
    return 100;
}

int terrain_defense_percent_at(const GameState& state, const Coord& coord) {
    const GameHex* hex = find_hex(state, coord);
    return terrain_defense_percent(hex == nullptr ? Terrain::None : hex->terrain);
}

int hp_scaled_strength(int strength, const Unit& unit) {
    if (strength <= 0 || unit.hp <= 0 || unit.max_hp <= 0) {
        return 0;
    }
    return std::max(1, (strength * unit.hp + unit.max_hp - 1) / unit.max_hp);
}

int hp_percent(const Unit& unit) {
    if (unit.hp <= 0 || unit.max_hp <= 0) {
        return 0;
    }
    return std::max(1, std::min(100, (unit.hp * 100 + unit.max_hp / 2) / unit.max_hp));
}

int clamped_readiness(const Unit& unit) {
    const int max_readiness = std::max(1, unit.max_readiness);
    return std::max(0, std::min(unit.readiness, max_readiness));
}

int readiness_percent(const Unit& unit) {
    const int max_readiness = std::max(1, unit.max_readiness);
    return std::max(0, std::min(100, (clamped_readiness(unit) * 100 + max_readiness / 2) / max_readiness));
}

int combat_readiness_percent(const Unit& unit) {
    if (unit.hp <= 0) {
        return 0;
    }
    return std::max(minimum_combat_readiness_percent, readiness_percent(unit));
}

int move_readiness_cost(const Unit& unit, int scaled_cost) {
    if (scaled_cost <= 0 || unit.scaled_move <= 0) {
        return 0;
    }
    return std::max(1, (full_move_readiness_cost * scaled_cost + unit.scaled_move - 1) / unit.scaled_move);
}

void reduce_readiness(Unit& unit, int amount) {
    unit.readiness = std::max(0, clamped_readiness(unit) - std::max(0, amount));
}

void recover_readiness(Unit& unit, int amount) {
    unit.max_readiness = std::max(1, unit.max_readiness);
    unit.readiness = std::min(unit.max_readiness, clamped_readiness(unit) + std::max(0, amount));
}

int effective_attack_score(const Unit& attacker) {
    return std::max(0, attacker.attack);
}

int effective_defense_score(const GameState& state, const Unit& defender) {
    const int defense_strength = std::max(1, defender.defense);
    if (defense_strength <= 0) {
        return 0;
    }
    return std::max(
        1,
        (defense_strength * terrain_defense_percent_at(state, defender.coord) + 99) / 100
    );
}

double hp_ratio(const Unit& attacker, const Unit& defender) {
    if (attacker.hp <= 0 || attacker.max_hp <= 0 || defender.hp <= 0 || defender.max_hp <= 0) {
        return 1.0;
    }
    const double attacker_ratio = static_cast<double>(attacker.hp) / attacker.max_hp;
    const double defender_ratio = static_cast<double>(defender.hp) / defender.max_hp;
    return defender_ratio <= 0.0 ? 1.0 : attacker_ratio / defender_ratio;
}

double readiness_ratio_for_combat(const Unit& attacker, const Unit& defender) {
    const int defender_percent = combat_readiness_percent(defender);
    if (defender_percent <= 0) {
        return 1.0;
    }
    return static_cast<double>(combat_readiness_percent(attacker)) / defender_percent;
}

int ratio_percent(double ratio) {
    return std::max(0, static_cast<int>(std::lround(ratio * 100.0)));
}

struct CrtOutcome {
    int attacker_damage = 0;
    int defender_damage = 0;
    std::string retreat_option = "none";
};

CrtOutcome crt_outcome(int index) {
    if (index <= -5) {
        return {5, 0, "attacker"};
    }
    if (index == -4) {
        return {4, 0, "attacker"};
    }
    if (index == -3) {
        return {4, 1, "attacker"};
    }
    if (index == -2) {
        return {3, 1, "attacker"};
    }
    if (index == -1) {
        return {3, 2, "none"};
    }
    if (index == 0) {
        return {2, 2, "none"};
    }
    if (index == 1) {
        return {2, 3, "none"};
    }
    if (index == 2) {
        return {1, 3, "defender"};
    }
    if (index == 3) {
        return {1, 4, "defender"};
    }
    if (index == 4) {
        return {0, 4, "defender"};
    }
    return {0, 5, "defender"};
}

std::string readiness_impact_text(int readiness_ratio_percent) {
    if (readiness_ratio_percent >= 115) {
        return "Attacker readiness improves CRT";
    }
    if (readiness_ratio_percent <= 85) {
        return "Defender readiness suppresses CRT";
    }
    return "Even readiness";
}

std::string retreat_impact_text(const std::string& retreat_option) {
    if (retreat_option == "attacker") {
        return "Attacker may retreat";
    }
    if (retreat_option == "defender") {
        return "Defender may retreat";
    }
    return "No retreat";
}

const Unit* unit_at(const GameState& state, const Coord& coord) {
    const auto found = std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return coord_equal(unit.coord, coord);
    });
    return found == state.units.end() ? nullptr : &*found;
}

bool occupied_by_other_unit(const GameState& state, const Coord& coord, int moving_unit_id) {
    const Unit* unit = unit_at(state, coord);
    return unit != nullptr && unit->id != moving_unit_id;
}

std::vector<Coord> adjacent_deployable_hexes(const GameState& state, const Unit& source) {
    std::vector<Coord> deployable;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord candidate = neighbor_in_direction(source.coord, direction);
        if (!in_bounds(state, candidate) || occupied_by_other_unit(state, candidate, 0)) {
            continue;
        }
        const GameHex* hex = find_hex(state, candidate);
        if (hex == nullptr || movement_cost(state, source.coord, *hex) == blocked_movement_cost()) {
            continue;
        }
        deployable.push_back(candidate);
    }
    std::sort(deployable.begin(), deployable.end(), coord_less);
    return deployable;
}

void refresh_legal_actions(GameState& state) {
    state.legal_moves.clear();
    state.legal_attacks.clear();
    if (state.selected_unit_id == 0) {
        return;
    }
    const Unit* unit = find_unit(state, state.selected_unit_id);
    if (unit == nullptr) {
        state.selected_unit_id = 0;
        return;
    }
    if (unit->owner != active_faction(state)) {
        return;
    }
    state.legal_moves = reachable_hexes(state, state.selected_unit_id);
    state.legal_attacks = attackable_units(state, state.selected_unit_id);
}

bool enemy_zoc_at(const GameState& state, const Coord& coord, const Unit& moving_unit) {
    return std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id != moving_unit.id
            && unit.owner != moving_unit.owner
            && unit.projects_zoc
            && adjacent(unit.coord, coord);
    }) != state.units.end();
}

bool enemy_unit_adjacent_to(const GameState& state, const Unit& source) {
    return std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id != source.id
            && unit.owner != source.owner
            && adjacent(unit.coord, source.coord);
    }) != state.units.end();
}

std::vector<ReachableHex> reachable_hexes(const GameState& state, int unit_id) {
    std::vector<ReachableHex> reachable;
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->owner != active_faction(state) || unit->move_done || unit->remaining_scaled_move <= 0) {
        return reachable;
    }

    std::map<Coord, int, decltype(coord_less)*> best_costs(coord_less);
    std::deque<Coord> queue;
    best_costs[unit->coord] = 0;
    queue.push_back(unit->coord);

    while (!queue.empty()) {
        const Coord current = queue.front();
        queue.pop_front();
        const int current_cost = best_costs[current];
        for (int direction = 0; direction < 6; ++direction) {
            const Coord next = neighbor_in_direction(current, direction);
            if (!in_bounds(state, next)) {
                continue;
            }
            const Unit* occupant = unit_at(state, next);
            if (occupant != nullptr && occupant->id != unit->id && occupant->owner != unit->owner) {
                continue;
            }
            const bool friendly_occupied = occupant != nullptr && occupant->id != unit->id && occupant->owner == unit->owner;
            const GameHex* hex = find_hex(state, next);
            if (hex == nullptr) {
                continue;
            }
            const int step_cost = movement_cost(state, current, *hex);
            if (step_cost == blocked_movement_cost()) {
                continue;
            }
            const int next_cost = current_cost + step_cost;
            if (next_cost > unit->remaining_scaled_move) {
                continue;
            }
            const auto existing = best_costs.find(next);
            if (existing != best_costs.end() && existing->second <= next_cost) {
                continue;
            }
            best_costs[next] = next_cost;
            if (!friendly_occupied) {
                reachable.push_back({next, next_cost});
            }
            if (!unit->respects_zoc || !enemy_zoc_at(state, next, *unit)) {
                queue.push_back(next);
            }
        }
    }

    std::sort(reachable.begin(), reachable.end(), [](const ReachableHex& first, const ReachableHex& second) {
        if (first.scaled_cost != second.scaled_cost) {
            return first.scaled_cost < second.scaled_cost;
        }
        return coord_less(first.coord, second.coord);
    });
    return reachable;
}

std::vector<AttackableUnit> attackable_units(const GameState& state, int unit_id) {
    std::vector<AttackableUnit> attackable;
    const Unit* attacker = find_unit(state, unit_id);
    if (attacker == nullptr
        || attacker->owner != active_faction(state)
        || attacker->combat_done
        || !can_attack(attacker->kind)
        || attacker->attack <= 0) {
        return attackable;
    }

    for (const Unit& defender : state.units) {
        if (defender.id == attacker->id
            || defender.owner == attacker->owner
            || defender.hp <= 0
            || !adjacent(attacker->coord, defender.coord)) {
            continue;
        }
        attackable.push_back({defender.id, defender.coord});
    }

    std::sort(attackable.begin(), attackable.end(), [](const AttackableUnit& first, const AttackableUnit& second) {
        if (first.unit_id != second.unit_id) {
            return first.unit_id < second.unit_id;
        }
        return coord_less(first.coord, second.coord);
    });
    return attackable;
}

CombatantPreview combatant_preview(const GameState& state, const Unit& unit) {
    CombatantPreview preview;
    preview.unit_id = unit.id;
    preview.owner = unit.owner;
    preview.kind = unit.kind;
    preview.coord = unit.coord;
    const GameHex* hex = find_hex(state, unit.coord);
    preview.terrain = hex == nullptr ? Terrain::None : hex->terrain;
    preview.hp = unit.hp;
    preview.max_hp = unit.max_hp;
    preview.base_attack = unit.attack;
    preview.base_defense = unit.defense;
    preview.hp_percent = hp_percent(unit);
    preview.readiness = clamped_readiness(unit);
    preview.max_readiness = std::max(1, unit.max_readiness);
    preview.readiness_percent = readiness_percent(unit);
    preview.terrain_defense_percent = terrain_defense_percent_at(state, unit.coord);
    preview.effective_attack = effective_attack_score(unit);
    preview.effective_defense = effective_defense_score(state, unit);
    preview.result_hp = unit.hp;
    return preview;
}

bool legal_combat_pair(const GameState& state, const Unit* attacker, const Unit* defender) {
    return attacker != nullptr
        && defender != nullptr
        && attacker->owner == active_faction(state)
        && !attacker->combat_done
        && can_attack(attacker->kind)
        && attacker->attack > 0
        && attacker->owner != defender->owner
        && defender->hp > 0
        && adjacent(attacker->coord, defender->coord);
}

CombatPreview combat_preview(const GameState& state, int attacker_id, int defender_id) {
    CombatPreview preview;
    const Unit* attacker = find_unit(state, attacker_id);
    const Unit* defender = find_unit(state, defender_id);
    if (!legal_combat_pair(state, attacker, defender)) {
        return preview;
    }

    preview.valid = true;
    preview.attacker = combatant_preview(state, *attacker);
    preview.defender = combatant_preview(state, *defender);

    preview.base_differential = preview.attacker.effective_attack - preview.defender.effective_defense;
    const double hp_multiplier = hp_ratio(*attacker, *defender);
    const double readiness_multiplier = readiness_ratio_for_combat(*attacker, *defender);
    preview.hp_ratio_percent = ratio_percent(hp_multiplier);
    preview.readiness_ratio_percent = ratio_percent(readiness_multiplier);
    preview.crt_index = std::max(
        -6,
        std::min(6, static_cast<int>(std::lround(preview.base_differential * hp_multiplier * readiness_multiplier)))
    );
    const CrtOutcome outcome = crt_outcome(preview.crt_index);
    preview.retreat_option = outcome.retreat_option;
    preview.readiness_impact = readiness_impact_text(preview.readiness_ratio_percent);
    preview.retreat_impact = retreat_impact_text(preview.retreat_option);

    preview.attacker.damage_dealt = outcome.defender_damage;
    preview.defender.damage_taken = outcome.defender_damage;
    preview.defender.result_hp = std::max(0, defender->hp - outcome.defender_damage);
    preview.defender.destroyed = preview.defender.result_hp <= 0;

    preview.defender_retaliates = !preview.defender.destroyed && outcome.attacker_damage > 0;
    preview.defender.damage_dealt = outcome.attacker_damage;
    preview.attacker.damage_taken = outcome.attacker_damage;
    preview.attacker.result_hp = std::max(0, attacker->hp - outcome.attacker_damage);
    preview.attacker.destroyed = preview.attacker.result_hp <= 0;
    return preview;
}

bool select_unit(GameState& state, int unit_id) {
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr) {
        state.selected_unit_id = 0;
        state.legal_moves.clear();
        state.legal_attacks.clear();
        return false;
    }
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

bool move_unit(GameState& state, int unit_id, Coord destination) {
    Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->owner != active_faction(state) || unit->move_done) {
        return false;
    }
    const std::vector<ReachableHex> reachable = reachable_hexes(state, unit_id);
    const auto found = std::find_if(reachable.begin(), reachable.end(), [&](const ReachableHex& hex) {
        return coord_equal(hex.coord, destination);
    });
    if (found == reachable.end()) {
        return false;
    }
    unit->coord = destination;
    unit->moved_this_turn = true;
    reduce_readiness(*unit, move_readiness_cost(*unit, found->scaled_cost));
    unit->remaining_scaled_move = std::max(0, unit->remaining_scaled_move - found->scaled_cost);
    if (unit->respects_zoc && enemy_zoc_at(state, destination, *unit)) {
        unit->remaining_scaled_move = 0;
    }
    if (unit->remaining_scaled_move == 0) {
        unit->move_done = true;
    }
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

bool attack_unit(GameState& state, int attacker_id, int defender_id) {
    const CombatPreview preview = combat_preview(state, attacker_id, defender_id);
    if (!preview.valid) {
        return false;
    }

    Unit* attacker = find_unit(state, attacker_id);
    Unit* defender = find_unit(state, defender_id);
    if (attacker == nullptr || defender == nullptr) {
        return false;
    }

    defender->hp = preview.defender.result_hp;
    attacker->hp = preview.attacker.result_hp;
    reduce_readiness(*attacker, attack_readiness_cost);
    reduce_readiness(*defender, defense_readiness_cost);
    attacker = find_unit(state, attacker_id);
    if (attacker != nullptr) {
        attacker->remaining_scaled_move = 0;
        attacker->move_done = true;
        attacker->combat_done = true;
    }
    state.units.erase(std::remove_if(state.units.begin(), state.units.end(), [](const Unit& unit) {
        return unit.hp <= 0;
    }), state.units.end());
    state.selected_unit_id = find_unit(state, attacker_id) == nullptr ? 0 : attacker_id;
    refresh_legal_actions(state);
    return true;
}

DetachHerdOptions detach_herd_options(const GameState& state, int unit_id, int horses) {
    DetachHerdOptions options;
    options.unit_id = unit_id;
    options.horses = horses;
    const Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr
        || horde->kind != UnitKind::Horde
        || horde->owner != active_faction(state)
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)
        || horses <= 0
        || horses > horde->horses) {
        return options;
    }

    options.deployable_hexes = adjacent_deployable_hexes(state, *horde);
    return options;
}

bool detach_herd(GameState& state, int unit_id, int horses, Coord destination) {
    DetachHerdOptions options = detach_herd_options(state, unit_id, horses);
    const bool valid_destination = std::find_if(
        options.deployable_hexes.begin(),
        options.deployable_hexes.end(),
        [&](const Coord& coord) { return coord_equal(coord, destination); }
    ) != options.deployable_hexes.end();
    if (!valid_destination) {
        return false;
    }

    Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr) {
        return false;
    }
    horde->horses -= horses;
    horde->remaining_scaled_move = 0;
    horde->move_done = true;

    Unit herd;
    herd.id = next_unit_id(state);
    herd.owner = horde->owner;
    herd.kind = UnitKind::Herd;
    herd.coord = destination;
    herd.hp = default_hp(herd.kind);
    herd.max_hp = herd.hp;
    herd.attack = default_attack(herd.kind);
    herd.defense = default_defense(herd.kind);
    herd.max_readiness = default_max_readiness;
    herd.readiness = herd.max_readiness;
    herd.scaled_move = to_scaled_move(default_move(herd.kind));
    herd.remaining_scaled_move = herd.scaled_move;
    herd.horses = horses;
    herd.projects_zoc = default_projects_zoc(herd.kind);
    herd.respects_zoc = default_respects_zoc(herd.kind);
    state.units.push_back(herd);
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

CreateHorseArchersOptions create_horse_archers_options(const GameState& state, int unit_id) {
    CreateHorseArchersOptions options;
    options.unit_id = unit_id;
    options.population_cost = create_horse_archers_population_cost;
    options.metal_cost = create_horse_archers_metal_cost;
    options.horses_cost = create_horse_archers_horses_cost;
    const Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr
        || horde->kind != UnitKind::Horde
        || horde->owner != active_faction(state)
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)
        || horde->population < create_horse_archers_population_cost
        || horde->metal < create_horse_archers_metal_cost
        || horde->horses < create_horse_archers_horses_cost) {
        return options;
    }

    options.deployable_hexes = adjacent_deployable_hexes(state, *horde);
    return options;
}

bool create_horse_archers(GameState& state, int unit_id, Coord destination) {
    CreateHorseArchersOptions options = create_horse_archers_options(state, unit_id);
    const bool valid_destination = std::find_if(
        options.deployable_hexes.begin(),
        options.deployable_hexes.end(),
        [&](const Coord& coord) { return coord_equal(coord, destination); }
    ) != options.deployable_hexes.end();
    if (!valid_destination) {
        return false;
    }

    Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr) {
        return false;
    }
    horde->population -= create_horse_archers_population_cost;
    horde->metal -= create_horse_archers_metal_cost;
    horde->horses -= create_horse_archers_horses_cost;
    horde->remaining_scaled_move = 0;
    horde->move_done = true;

    Unit horse_archers;
    horse_archers.id = next_unit_id(state);
    horse_archers.owner = horde->owner;
    horse_archers.kind = UnitKind::HorseArcher;
    horse_archers.coord = destination;
    horse_archers.hp = default_hp(horse_archers.kind);
    horse_archers.max_hp = horse_archers.hp;
    horse_archers.attack = default_attack(horse_archers.kind);
    horse_archers.defense = default_defense(horse_archers.kind);
    horse_archers.max_readiness = default_max_readiness;
    horse_archers.readiness = horse_archers.max_readiness;
    horse_archers.scaled_move = to_scaled_move(default_move(horse_archers.kind));
    horse_archers.remaining_scaled_move = horse_archers.scaled_move;
    horse_archers.projects_zoc = default_projects_zoc(horse_archers.kind);
    horse_archers.respects_zoc = default_respects_zoc(horse_archers.kind);
    state.units.push_back(horse_archers);
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

void end_turn(GameState& state) {
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }
    state.active_faction_index = (state.active_faction_index + 1) % static_cast<int>(state.turn_order.size());
    if (state.active_faction_index == 0) {
        state.round += 1;
    }
    const OwnerId owner = active_faction(state);
    for (Unit& unit : state.units) {
        if (unit.owner == owner) {
            unit.remaining_scaled_move = unit.scaled_move;
            unit.move_done = false;
            unit.moved_this_turn = false;
            unit.combat_done = false;
            recover_readiness(unit, turn_readiness_recovery);
        }
    }
}

void print_coord_json(const Coord& coord, std::ostream& out) {
    out << "{\"q\":" << coord.q << ",\"r\":" << coord.r << "}";
}

void print_edge_json(const RiverEdge& edge, std::ostream& out) {
    out << "{\"a\":";
    print_coord_json(edge.a, out);
    out << ",\"b\":";
    print_coord_json(edge.b, out);
    out << "}";
}

void print_string_array_json(const std::vector<std::string>& values, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "\"" << escape_json(values[i]) << "\"";
    }
    out << "]";
}

void print_unit_json(const Unit& unit, std::ostream& out) {
    const Clan clan = clan_for_owner(unit.owner);
    out << "{\"id\":" << unit.id
        << ",\"owner\":" << unit.owner
        << ",\"faction\":\"" << clan.key << "\""
        << ",\"kind\":\"" << unit_kind_to_string(unit.kind) << "\""
        << ",\"q\":" << unit.coord.q
        << ",\"r\":" << unit.coord.r
        << ",\"hp\":" << unit.hp
        << ",\"maxHp\":" << unit.max_hp
        << ",\"attack\":" << unit.attack
        << ",\"defense\":" << unit.defense
        << ",\"readiness\":" << clamped_readiness(unit)
        << ",\"maxReadiness\":" << std::max(1, unit.max_readiness)
        << ",\"scaledMove\":" << unit.scaled_move
        << ",\"remainingScaledMove\":" << unit.remaining_scaled_move
        << ",\"refMove\":" << to_ref_move(unit.scaled_move)
        << ",\"remainingRefMove\":" << to_ref_move(unit.remaining_scaled_move)
        << ",\"move\":" << to_ref_move(unit.scaled_move)
        << ",\"remainingMove\":" << to_ref_move(unit.remaining_scaled_move);
    if (unit.kind == UnitKind::Horde) {
        out << ",\"population\":" << unit.population
            << ",\"metal\":" << unit.metal
            << ",\"horses\":" << unit.horses;
    } else if (unit.kind == UnitKind::Herd) {
        out << ",\"horses\":" << unit.horses;
    }
    out << ",\"moveDone\":" << (unit.move_done ? "true" : "false")
        << ",\"movedThisTurn\":" << (unit.moved_this_turn ? "true" : "false")
        << ",\"combatDone\":" << (unit.combat_done ? "true" : "false")
        << ",\"projectsZoc\":" << (unit.projects_zoc ? "true" : "false")
        << ",\"respectsZoc\":" << (unit.respects_zoc ? "true" : "false")
        << "}";
}

void print_units_json(const std::vector<Unit>& units, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        print_unit_json(units[i], out);
    }
    out << "]";
}

void print_reachable_array_json(const std::vector<ReachableHex>& reachable, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < reachable.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "{\"q\":" << reachable[i].coord.q
            << ",\"r\":" << reachable[i].coord.r
            << ",\"scaledCost\":" << reachable[i].scaled_cost
            << ",\"refCost\":" << to_ref_move(reachable[i].scaled_cost)
            << ",\"cost\":" << to_ref_move(reachable[i].scaled_cost)
            << "}";
    }
    out << "]";
}

void print_attackable_array_json(const std::vector<AttackableUnit>& attackable, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < attackable.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "{\"unitId\":" << attackable[i].unit_id
            << ",\"q\":" << attackable[i].coord.q
            << ",\"r\":" << attackable[i].coord.r
            << "}";
    }
    out << "]";
}

void print_roads_array_json(const std::vector<Road>& roads, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < roads.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const Road& road = roads[i];
        out << "{\"id\":" << road.id
            << ",\"feature\":\"" << escape_json(road.feature) << "\""
            << ",\"path\":[";
        for (std::size_t path_index = 0; path_index < road.path.size(); ++path_index) {
            if (path_index > 0) {
                out << ",";
            }
            print_coord_json(road.path[path_index], out);
        }
        out << "]}";
    }
    out << "]";
}

void print_walls_array_json(const std::vector<Wall>& walls, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < walls.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const Wall& wall = walls[i];
        out << "{\"id\":" << wall.id
            << ",\"feature\":\"" << escape_json(wall.feature) << "\""
            << ",\"edge_path\":[";
        for (std::size_t edge_index = 0; edge_index < wall.edge_path.size(); ++edge_index) {
            if (edge_index > 0) {
                out << ",";
            }
            print_edge_json(wall.edge_path[edge_index], out);
        }
        out << "]}";
    }
    out << "]";
}

void print_wall_gates_array_json(const std::vector<WallGate>& gates, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < gates.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const WallGate& gate = gates[i];
        out << "{\"id\":" << gate.id
            << ",\"kind\":\"" << escape_json(gate.kind) << "\""
            << ",\"edge\":";
        print_edge_json(gate.edge, out);
        out << "}";
    }
    out << "]";
}

void print_game_meta_json(const GameState& state, std::ostream& out) {
    out << "\"game\":{";
    out << "\"round\":" << state.round << ",";
    out << "\"activeFactionIndex\":" << state.active_faction_index << ",";
    out << "\"activeOwner\":" << active_faction(state) << ",";
    out << "\"selectedUnitId\":" << state.selected_unit_id << ",";
    out << "\"legalMoves\":";
    print_reachable_array_json(state.legal_moves, out);
    out << ",\"legalAttacks\":";
    print_attackable_array_json(state.legal_attacks, out);
    out << ",";
    out << "\"turnOrder\":[";
    for (std::size_t i = 0; i < state.turn_order.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << state.turn_order[i];
    }
    out << "],\"clans\":[";
    for (std::size_t i = 0; i < state.clans.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const Clan& clan = state.clans[i];
        out << "{\"id\":" << clan.id
            << ",\"key\":\"" << escape_json(clan.key) << "\""
            << ",\"name\":\"" << escape_json(clan.name) << "\""
            << ",\"color\":\"" << escape_json(clan.color) << "\"}";
    }
    out << "]}";
}

void print_game_state_json(const GameState& state, std::ostream& out) {
    out << "{";
    out << "\"schema\":\"steppe-game.v1\",";
    out << "\"seed\":" << state.seed << ",";
    out << "\"width\":" << state.width << ",";
    out << "\"height\":" << state.height << ",";
    out << "\"hexes\":[";
    for (std::size_t i = 0; i < state.hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const GameHex& hex = state.hexes[i];
        out << "{\"q\":" << hex.coord.q
            << ",\"r\":" << hex.coord.r
            << ",\"terrain\":\"" << terrain_to_string(hex.terrain) << "\""
            << ",\"labels\":";
        print_string_array_json(hex.source_labels, out);
        out << "}";
    }
    out << "],\"towns\":[],\"river_sources\":[],\"river_destinations\":[],\"merge_points\":[],";
    out << "\"river_segments\":[],\"edges\":[],\"lake_river_connections\":[],\"roads\":";
    print_roads_array_json(state.roads, out);
    out << ",\"walls\":";
    print_walls_array_json(state.walls, out);
    out << ",\"wall_gates\":";
    print_wall_gates_array_json(state.wall_gates, out);
    out << ",\"crossings\":[],";
    out << "\"units\":";
    print_units_json(state.units, out);
    out << ",";
    print_game_meta_json(state, out);
    out << "}\n";
}

void print_reachable_json(const std::vector<ReachableHex>& reachable, std::ostream& out) {
    out << "{\"reachable\":";
    print_reachable_array_json(reachable, out);
    out << "}\n";
}

void print_attackable_json(const std::vector<AttackableUnit>& attackable, std::ostream& out) {
    out << "{\"attackable\":";
    print_attackable_array_json(attackable, out);
    out << "}\n";
}

void print_combatant_preview_json(const CombatantPreview& preview, std::ostream& out) {
    const Clan clan = clan_for_owner(preview.owner);
    out << "{\"unitId\":" << preview.unit_id
        << ",\"owner\":" << preview.owner
        << ",\"faction\":\"" << clan.key << "\""
        << ",\"kind\":\"" << unit_kind_to_string(preview.kind) << "\""
        << ",\"q\":" << preview.coord.q
        << ",\"r\":" << preview.coord.r
        << ",\"terrain\":\"" << terrain_to_string(preview.terrain) << "\""
        << ",\"hp\":" << preview.hp
        << ",\"maxHp\":" << preview.max_hp
        << ",\"baseAttack\":" << preview.base_attack
        << ",\"baseDefense\":" << preview.base_defense
        << ",\"hpPercent\":" << preview.hp_percent
        << ",\"readiness\":" << preview.readiness
        << ",\"maxReadiness\":" << preview.max_readiness
        << ",\"readinessPercent\":" << preview.readiness_percent
        << ",\"terrainDefensePercent\":" << preview.terrain_defense_percent
        << ",\"effectiveAttack\":" << preview.effective_attack
        << ",\"effectiveDefense\":" << preview.effective_defense
        << ",\"damageDealt\":" << preview.damage_dealt
        << ",\"damageTaken\":" << preview.damage_taken
        << ",\"resultHp\":" << preview.result_hp
        << ",\"destroyed\":" << (preview.destroyed ? "true" : "false")
        << "}";
}

void print_combat_preview_json(const CombatPreview& preview, std::ostream& out) {
    out << "{\"valid\":" << (preview.valid ? "true" : "false")
        << ",\"defenderRetaliates\":" << (preview.defender_retaliates ? "true" : "false")
        << ",\"baseDifferential\":" << preview.base_differential
        << ",\"hpRatioPercent\":" << preview.hp_ratio_percent
        << ",\"readinessRatioPercent\":" << preview.readiness_ratio_percent
        << ",\"crtIndex\":" << preview.crt_index
        << ",\"retreatOption\":\"" << escape_json(preview.retreat_option) << "\""
        << ",\"readinessImpact\":\"" << escape_json(preview.readiness_impact) << "\""
        << ",\"retreatImpact\":\"" << escape_json(preview.retreat_impact) << "\""
        << ",\"attacker\":";
    print_combatant_preview_json(preview.attacker, out);
    out << ",\"defender\":";
    print_combatant_preview_json(preview.defender, out);
    out << "}\n";
}

void print_detach_herd_options_json(const DetachHerdOptions& options, std::ostream& out) {
    out << "{\"unitId\":" << options.unit_id
        << ",\"horses\":" << options.horses
        << ",\"deployableHexes\":[";
    for (std::size_t i = 0; i < options.deployable_hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        print_coord_json(options.deployable_hexes[i], out);
    }
    out << "]}\n";
}

void print_create_horse_archers_options_json(const CreateHorseArchersOptions& options, std::ostream& out) {
    out << "{\"unitId\":" << options.unit_id
        << ",\"populationCost\":" << options.population_cost
        << ",\"metalCost\":" << options.metal_cost
        << ",\"horsesCost\":" << options.horses_cost
        << ",\"deployableHexes\":[";
    for (std::size_t i = 0; i < options.deployable_hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        print_coord_json(options.deployable_hexes[i], out);
    }
    out << "]}\n";
}

void print_game_patch_json(const GameState& state, bool ok, std::ostream& out) {
    out << "{";
    out << "\"ok\":" << (ok ? "true" : "false") << ",";
    out << "\"schema\":\"steppe-game.v1\",";
    out << "\"seed\":" << state.seed << ",";
    out << "\"width\":" << state.width << ",";
    out << "\"height\":" << state.height << ",";
    out << "\"hexes\":[";
    for (std::size_t i = 0; i < state.hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const GameHex& hex = state.hexes[i];
        out << "{\"q\":" << hex.coord.q
            << ",\"r\":" << hex.coord.r
            << ",\"terrain\":\"" << terrain_to_string(hex.terrain) << "\""
            << ",\"labels\":";
        print_string_array_json(hex.source_labels, out);
        out << "}";
    }
    out << "],\"towns\":[],\"river_sources\":[],\"river_destinations\":[],\"merge_points\":[],";
    out << "\"river_segments\":[],\"edges\":[],\"lake_river_connections\":[],\"roads\":";
    print_roads_array_json(state.roads, out);
    out << ",\"walls\":";
    print_walls_array_json(state.walls, out);
    out << ",\"wall_gates\":";
    print_wall_gates_array_json(state.wall_gates, out);
    out << ",\"crossings\":[],";
    out << "\"units\":";
    print_units_json(state.units, out);
    out << ",";
    print_game_meta_json(state, out);
    out << "}\n";
}

GameState parse_game_state_json(const std::string& json) {
    GameState state;
    state.width = int_field(json, "width", 0);
    state.height = int_field(json, "height", 0);
    state.seed = uint_field(json, "seed", 0);

    const std::string game_json = object_field(json, "game");
    state.round = int_field(game_json.empty() ? json : game_json, "round", 1);
    state.active_faction_index = int_field(game_json.empty() ? json : game_json, "activeFactionIndex", 0);
    state.selected_unit_id = int_field(game_json.empty() ? json : game_json, "selectedUnitId", 0);
    state.turn_order = parse_turn_order(object_field(game_json.empty() ? json : game_json, "turnOrder"));
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }
    state.clans = {
        clan_for_owner(mongol_owner),
        clan_for_owner(chinese_owner),
        clan_for_owner(persian_owner),
    };

    for (const std::string& hex_json : object_array_items(object_field(json, "hexes"))) {
        GameHex hex;
        hex.coord = {int_field(hex_json, "q", 0), int_field(hex_json, "r", 0)};
        hex.terrain = terrain_from_string(string_field(hex_json, "terrain", "none"));
        hex.tags = hex.terrain == Terrain::Grassland ? tag_mask(HexTag::BaseSteppe) : 0;
        hex.pasture = initial_pasture_for_terrain(hex.terrain);
        if (hex.terrain == Terrain::Grassland) {
            hex.source_labels = {"base_steppe"};
        }
        state.hexes.push_back(std::move(hex));
    }

    for (const std::string& road_json : object_array_items(object_field(json, "roads"))) {
        Road road;
        road.id = int_field(road_json, "id", 0);
        road.feature = string_field(road_json, "feature", "");
        for (const std::string& coord_json : object_array_items(object_field(road_json, "path"))) {
            road.path.push_back({int_field(coord_json, "q", 0), int_field(coord_json, "r", 0)});
        }
        state.roads.push_back(std::move(road));
    }

    for (const std::string& wall_json : object_array_items(object_field(json, "walls"))) {
        Wall wall;
        wall.id = int_field(wall_json, "id", 0);
        wall.feature = string_field(wall_json, "feature", "");
        for (const std::string& edge_json : object_array_items(object_field(wall_json, "edge_path"))) {
            const std::string a_json = object_field(edge_json, "a");
            const std::string b_json = object_field(edge_json, "b");
            wall.edge_path.push_back({
                {int_field(a_json, "q", 0), int_field(a_json, "r", 0)},
                {int_field(b_json, "q", 0), int_field(b_json, "r", 0)}
            });
        }
        state.walls.push_back(std::move(wall));
    }

    for (const std::string& gate_json : object_array_items(object_field(json, "wall_gates"))) {
        WallGate gate;
        gate.id = int_field(gate_json, "id", 0);
        gate.kind = string_field(gate_json, "kind", "");
        const std::string edge_json = object_field(gate_json, "edge");
        const std::string a_json = object_field(edge_json, "a");
        const std::string b_json = object_field(edge_json, "b");
        gate.edge = {
            {int_field(a_json, "q", 0), int_field(a_json, "r", 0)},
            {int_field(b_json, "q", 0), int_field(b_json, "r", 0)}
        };
        state.wall_gates.push_back(gate);
    }

    for (const std::string& unit_json : object_array_items(object_field(json, "units"))) {
        Unit unit;
        unit.id = int_field(unit_json, "id", 0);
        unit.owner = int_field(unit_json, "owner", neutral_owner);
        if (unit.owner == neutral_owner) {
            const std::string faction = string_field(unit_json, "faction", "");
            if (faction == "mongol") {
                unit.owner = mongol_owner;
            } else if (faction == "chinese") {
                unit.owner = chinese_owner;
            } else if (faction == "persian") {
                unit.owner = persian_owner;
            }
        }
        unit.kind = unit_kind_from_string(string_field(unit_json, "kind", "horse_archer"));
        unit.coord = {int_field(unit_json, "q", 0), int_field(unit_json, "r", 0)};
        unit.hp = int_field(unit_json, "hp", default_hp(unit.kind));
        unit.max_hp = int_field(unit_json, "maxHp", unit.hp);
        unit.attack = std::max(0, int_field(unit_json, "attack", default_attack(unit.kind)));
        unit.defense = std::max(1, int_field(unit_json, "defense", default_defense(unit.kind)));
        unit.max_readiness = std::max(1, int_field(unit_json, "maxReadiness", default_max_readiness));
        unit.readiness = std::max(0, std::min(int_field(unit_json, "readiness", unit.max_readiness), unit.max_readiness));
        const int default_scaled_move = to_scaled_move(default_move(unit.kind));
        unit.scaled_move = int_field(
            unit_json,
            "scaledMove",
            to_scaled_move(number_field(unit_json, "refMove", number_field(unit_json, "move", default_move(unit.kind))))
        );
        unit.remaining_scaled_move = int_field(
            unit_json,
            "remainingScaledMove",
            to_scaled_move(number_field(unit_json, "remainingRefMove", number_field(unit_json, "remainingMove", to_ref_move(unit.scaled_move))))
        );
        if (unit.scaled_move <= 0) {
            unit.scaled_move = default_scaled_move;
        }
        unit.remaining_scaled_move = std::max(0, std::min(unit.remaining_scaled_move, unit.scaled_move));
        unit.population = std::max(0, int_field(unit_json, "population", 0));
        unit.metal = std::max(0, int_field(unit_json, "metal", 0));
        unit.horses = std::max(0, int_field(unit_json, "horses", 0));
        unit.move_done = bool_field(unit_json, "moveDone", false);
        unit.moved_this_turn = bool_field(unit_json, "movedThisTurn", false);
        unit.combat_done = bool_field(unit_json, "combatDone", false);
        unit.projects_zoc = bool_field(unit_json, "projectsZoc", default_projects_zoc(unit.kind));
        unit.respects_zoc = bool_field(unit_json, "respectsZoc", default_respects_zoc(unit.kind));
        state.units.push_back(unit);
    }

    if (state.width <= 0 || state.height <= 0 || state.hexes.empty()) {
        throw std::runtime_error("game state JSON is missing width, height, or hexes");
    }
    refresh_legal_actions(state);
    return state;
}

} // namespace steppe::game
