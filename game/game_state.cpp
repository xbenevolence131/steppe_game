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

bool default_projects_zoc(UnitKind kind) {
    return kind == UnitKind::HorseArcher || kind == UnitKind::Infantry || kind == UnitKind::Horde;
}

bool default_respects_zoc(UnitKind kind) {
    return kind == UnitKind::Infantry || kind == UnitKind::Horde;
}

bool can_attack(UnitKind kind) {
    return kind == UnitKind::HorseArcher || kind == UnitKind::Infantry || kind == UnitKind::Horde;
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
    if (unit == nullptr || unit->owner != active_faction(state)) {
        state.selected_unit_id = 0;
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
    if (attacker == nullptr || attacker->owner != active_faction(state) || attacker->combat_done || !can_attack(attacker->kind)) {
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

bool select_unit(GameState& state, int unit_id) {
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->owner != active_faction(state)) {
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
    Unit* attacker = find_unit(state, attacker_id);
    Unit* defender = find_unit(state, defender_id);
    if (attacker == nullptr
        || defender == nullptr
        || attacker->owner != active_faction(state)
        || attacker->combat_done
        || !can_attack(attacker->kind)
        || attacker->owner == defender->owner
        || !adjacent(attacker->coord, defender->coord)) {
        return false;
    }

    constexpr int attack_damage = 3;
    defender->hp = std::max(0, defender->hp - attack_damage);
    attacker = find_unit(state, attacker_id);
    if (attacker != nullptr) {
        attacker->remaining_scaled_move = 0;
        attacker->move_done = true;
        attacker->combat_done = true;
    }
    state.units.erase(std::remove_if(state.units.begin(), state.units.end(), [](const Unit& unit) {
        return unit.hp <= 0;
    }), state.units.end());
    state.selected_unit_id = attacker_id;
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

    Unit herd;
    herd.id = next_unit_id(state);
    herd.owner = horde->owner;
    herd.kind = UnitKind::Herd;
    herd.coord = destination;
    herd.hp = default_hp(herd.kind);
    herd.max_hp = herd.hp;
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

    Unit horse_archers;
    horse_archers.id = next_unit_id(state);
    horse_archers.owner = horde->owner;
    horse_archers.kind = UnitKind::HorseArcher;
    horse_archers.coord = destination;
    horse_archers.hp = default_hp(horse_archers.kind);
    horse_archers.max_hp = horse_archers.hp;
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
            unit.combat_done = false;
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
