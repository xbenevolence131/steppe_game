#include "game_state.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <deque>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <initializer_list>
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

int hex_distance(const Coord& first, const Coord& second) {
    auto to_cube = [](const Coord& coord) {
        const int col = coord.q - 1;
        const int row = coord.r - 1;
        const int x = col;
        const int z = row - (col - (col & 1)) / 2;
        const int y = -x - z;
        return std::array<int, 3>{x, y, z};
    };
    const std::array<int, 3> a = to_cube(first);
    const std::array<int, 3> b = to_cube(second);
    return std::max({
        std::abs(a[0] - b[0]),
        std::abs(a[1] - b[1]),
        std::abs(a[2] - b[2]),
    });
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
        case UnitKind::Herd: return "herd";
        case UnitKind::HorseArcher: return "horse_archer";
        case UnitKind::ChineseCavalry: return "chinese_cavalry";
        case UnitKind::MongolLancer: return "mongol_lancer";
        case UnitKind::ChineseMilitia: return "chinese_militia";
        case UnitKind::Infantry: return "infantry";
        case UnitKind::PersianInfantry: return "persian_infantry";
        case UnitKind::PersianCavalry: return "persian_cavalry";
        case UnitKind::Horde: return "horde";
    }
    return "horse_archer";
}

bool try_unit_kind_from_string(const std::string& kind, UnitKind& out) {
    if (kind == "herd") {
        out = UnitKind::Herd;
        return true;
    }
    if (kind == "horse_archer" || kind == "cavalry") {
        out = UnitKind::HorseArcher;
        return true;
    }
    if (kind == "chinese_cavalry") {
        out = UnitKind::ChineseCavalry;
        return true;
    }
    if (kind == "mongol_lancer") {
        out = UnitKind::MongolLancer;
        return true;
    }
    if (kind == "chinese_militia") {
        out = UnitKind::ChineseMilitia;
        return true;
    }
    if (kind == "infantry") {
        out = UnitKind::Infantry;
        return true;
    }
    if (kind == "persian_infantry") {
        out = UnitKind::PersianInfantry;
        return true;
    }
    if (kind == "persian_cavalry") {
        out = UnitKind::PersianCavalry;
        return true;
    }
    if (kind == "horde") {
        out = UnitKind::Horde;
        return true;
    }
    return false;
}

UnitKind unit_kind_from_string(const std::string& kind) {
    UnitKind parsed = UnitKind::HorseArcher;
    if (try_unit_kind_from_string(kind, parsed)) {
        return parsed;
    }
    return UnitKind::HorseArcher;
}

const char* unit_stance_to_string(UnitStance stance) {
    switch (stance) {
        case UnitStance::Default: return "default";
        case UnitStance::FeignedRetreat: return "feigned_retreat";
        case UnitStance::Defensive: return "defensive";
    }
    return "default";
}

UnitStance unit_stance_from_string(const std::string& stance) {
    if (stance == "feigned_retreat") return UnitStance::FeignedRetreat;
    if (stance == "defensive") return UnitStance::Defensive;
    return UnitStance::Default;
}

std::string trim_copy(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

std::vector<std::string> split_simple(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream input(text);
    std::string part;
    while (std::getline(input, part, delimiter)) {
        parts.push_back(trim_copy(part));
    }
    if (!text.empty() && text.back() == delimiter) {
        parts.push_back("");
    }
    return parts;
}

std::string strip_csv_comment(const std::string& line) {
    const std::size_t comment = line.find('#');
    return comment == std::string::npos ? line : line.substr(0, comment);
}

int parse_int_field(const std::string& value, const std::string& column, int line_number) {
    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return parsed;
    } catch (...) {
        throw std::runtime_error("invalid integer in unit_types.csv line "
            + std::to_string(line_number) + " column " + column + ": " + value);
    }
}

bool parse_bool_field(const std::string& value, const std::string& column, int line_number) {
    if (value == "true" || value == "1") {
        return true;
    }
    if (value == "false" || value == "0") {
        return false;
    }
    throw std::runtime_error("invalid boolean in unit_types.csv line "
        + std::to_string(line_number) + " column " + column + ": " + value);
}

std::vector<UnitStance> parse_legal_stances_field(const std::string& value, int line_number) {
    std::vector<UnitStance> stances;
    for (const std::string& stance_key : split_simple(value, ';')) {
        if (stance_key.empty()) {
            continue;
        }
        const UnitStance stance = unit_stance_from_string(stance_key);
        if (unit_stance_to_string(stance) != stance_key) {
            throw std::runtime_error("invalid stance in unit_types.csv line "
                + std::to_string(line_number) + ": " + stance_key);
        }
        if (std::find(stances.begin(), stances.end(), stance) == stances.end()) {
            stances.push_back(stance);
        }
    }
    if (stances.empty()) {
        throw std::runtime_error("unit_types.csv line " + std::to_string(line_number)
            + " must define at least one legal stance");
    }
    return stances;
}

FactionState faction_for_owner(OwnerId owner) {
    if (owner == mongol_owner) {
        return {mongol_owner, "mongol", "Mongol", "#d6a21a", 4, 0, true, false};
    }
    if (owner == chinese_owner) {
        return {chinese_owner, "chinese", "Chinese", "#c93632", 4, 0, true, false};
    }
    if (owner == persian_owner) {
        return {persian_owner, "persian", "Persian", "#1f4fa3", 4, 0, true, false};
    }
    return {neutral_owner, "neutral", "Neutral", "#777777", 0, 0, false, false};
}

OwnerId owner_from_faction_key(const std::string& key, OwnerId fallback = neutral_owner) {
    if (key == "mongol") {
        return mongol_owner;
    }
    if (key == "chinese") {
        return chinese_owner;
    }
    if (key == "persian") {
        return persian_owner;
    }
    if (key == "neutral") {
        return neutral_owner;
    }
    return fallback;
}

const char* ai_directive_kind_to_string(AiDirectiveKind kind) {
    switch (kind) {
        case AiDirectiveKind::DefendHex: return "defend_hex";
        case AiDirectiveKind::HuntHorde: return "hunt_horde";
        case AiDirectiveKind::CaptureHex: return "capture_hex";
        case AiDirectiveKind::Hunt: return "hunt";
    }
    return "hunt";
}

AiDirectiveKind ai_directive_kind_from_string(const std::string& value) {
    if (value == "defend_hex") {
        return AiDirectiveKind::DefendHex;
    }
    if (value == "hunt_horde") {
        return AiDirectiveKind::HuntHorde;
    }
    if (value == "capture_hex") {
        return AiDirectiveKind::CaptureHex;
    }
    return AiDirectiveKind::Hunt;
}

struct UnitTypeTable {
    std::vector<UnitKind> order;
    std::map<UnitKind, UnitDefaults> defaults;
};

std::vector<UnitKind> required_unit_kinds() {
    return {
        UnitKind::Herd,
        UnitKind::HorseArcher,
        UnitKind::ChineseCavalry,
        UnitKind::MongolLancer,
        UnitKind::ChineseMilitia,
        UnitKind::Infantry,
        UnitKind::PersianInfantry,
        UnitKind::PersianCavalry,
        UnitKind::Horde,
    };
}

std::vector<std::string> unit_type_csv_candidates() {
    std::vector<std::string> candidates;
    if (const char* env_path = std::getenv("STEPPE_UNIT_TYPES_CSV")) {
        if (*env_path != '\0') {
            candidates.push_back(env_path);
        }
    }
    candidates.push_back("data/unit_types.csv");
    candidates.push_back("../data/unit_types.csv");
    candidates.push_back("../../data/unit_types.csv");
    return candidates;
}

std::string unit_type_csv_path() {
    for (const std::string& candidate : unit_type_csv_candidates()) {
        std::ifstream input(candidate);
        if (input.good()) {
            return candidate;
        }
    }
    throw std::runtime_error("could not find authoritative unit type data file data/unit_types.csv");
}

UnitDefaults parse_unit_type_row(const std::vector<std::string>& cells, int line_number) {
    if (cells.size() != 13) {
        throw std::runtime_error("unit_types.csv line " + std::to_string(line_number)
            + " must have 13 columns");
    }
    UnitKind kind = UnitKind::HorseArcher;
    if (!try_unit_kind_from_string(cells[0], kind) || cells[0] == "cavalry") {
        throw std::runtime_error("unknown unit kind in unit_types.csv line "
            + std::to_string(line_number) + ": " + cells[0]);
    }

    UnitDefaults defaults;
    defaults.kind = kind;
    defaults.hp = parse_int_field(cells[1], "hp", line_number);
    defaults.attack = parse_int_field(cells[2], "attack", line_number);
    defaults.defense = parse_int_field(cells[3], "defense", line_number);
    defaults.readiness_damage = parse_int_field(cells[4], "readiness_damage", line_number);
    defaults.readiness = parse_int_field(cells[5], "readiness", line_number);
    defaults.move = parse_int_field(cells[6], "move", line_number);
    defaults.projects_zoc = parse_bool_field(cells[7], "projects_zoc", line_number);
    defaults.respects_zoc = parse_bool_field(cells[8], "respects_zoc", line_number);
    defaults.stance = unit_stance_from_string(cells[9]);
    if (unit_stance_to_string(defaults.stance) != cells[9]) {
        throw std::runtime_error("invalid default stance in unit_types.csv line "
            + std::to_string(line_number) + ": " + cells[9]);
    }
    defaults.legal_stances = parse_legal_stances_field(cells[10], line_number);
    defaults.population = parse_int_field(cells[11], "population", line_number);
    defaults.horses = parse_int_field(cells[12], "horses", line_number);

    if (defaults.hp < 1 || defaults.defense < 1 || defaults.attack < 0
        || defaults.readiness_damage < 0 || defaults.readiness < 1 || defaults.move < 0
        || defaults.population < 0 || defaults.horses < 0) {
        throw std::runtime_error("invalid negative or zero unit stat in unit_types.csv line "
            + std::to_string(line_number));
    }
    if (std::find(defaults.legal_stances.begin(), defaults.legal_stances.end(), defaults.stance)
        == defaults.legal_stances.end()) {
        throw std::runtime_error("default stance must be listed in legal stances in unit_types.csv line "
            + std::to_string(line_number));
    }
    return defaults;
}

UnitTypeTable parse_unit_type_table(const std::string& path) {
    std::ifstream input(path);
    if (!input.good()) {
        throw std::runtime_error("could not open unit type data file: " + path);
    }

    const std::vector<std::string> expected_header = {
        "kind",
        "hp",
        "attack",
        "defense",
        "readiness_damage",
        "readiness",
        "move",
        "projects_zoc",
        "respects_zoc",
        "default_stance",
        "legal_stances",
        "population",
        "horses",
    };

    UnitTypeTable table;
    std::string line;
    bool saw_header = false;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const std::string clean = trim_copy(strip_csv_comment(line));
        if (clean.empty()) {
            continue;
        }
        const std::vector<std::string> cells = split_simple(clean, ',');
        if (!saw_header) {
            if (cells != expected_header) {
                throw std::runtime_error("unit_types.csv header does not match expected schema");
            }
            saw_header = true;
            continue;
        }

        UnitDefaults defaults = parse_unit_type_row(cells, line_number);
        if (table.defaults.find(defaults.kind) != table.defaults.end()) {
            throw std::runtime_error("duplicate unit kind in unit_types.csv: "
                + std::string(unit_kind_to_string(defaults.kind)));
        }
        table.order.push_back(defaults.kind);
        table.defaults[defaults.kind] = std::move(defaults);
    }

    if (!saw_header) {
        throw std::runtime_error("unit_types.csv is empty");
    }
    for (const UnitKind kind : required_unit_kinds()) {
        if (table.defaults.find(kind) == table.defaults.end()) {
            throw std::runtime_error("unit_types.csv is missing unit kind: "
                + std::string(unit_kind_to_string(kind)));
        }
    }
    return table;
}

const UnitTypeTable& unit_type_table() {
    static const UnitTypeTable table = parse_unit_type_table(unit_type_csv_path());
    return table;
}

const UnitDefaults& unit_type_defaults(UnitKind kind) {
    const UnitTypeTable& table = unit_type_table();
    const auto found = table.defaults.find(kind);
    if (found == table.defaults.end()) {
        throw std::runtime_error("unit type data missing kind: " + std::string(unit_kind_to_string(kind)));
    }
    return found->second;
}

int default_hp(UnitKind kind) {
    return unit_type_defaults(kind).hp;
}

int default_move(UnitKind kind) {
    return unit_type_defaults(kind).move;
}

int default_attack(UnitKind kind) {
    return unit_type_defaults(kind).attack;
}

int default_defense(UnitKind kind) {
    return unit_type_defaults(kind).defense;
}

int default_readiness_damage(UnitKind kind) {
    return unit_type_defaults(kind).readiness_damage;
}

int default_readiness(UnitKind kind) {
    return unit_type_defaults(kind).readiness;
}

bool default_projects_zoc(UnitKind kind) {
    return unit_type_defaults(kind).projects_zoc;
}

bool default_respects_zoc(UnitKind kind) {
    return unit_type_defaults(kind).respects_zoc;
}

UnitStance default_stance(UnitKind kind) {
    return unit_type_defaults(kind).stance;
}

std::vector<UnitStance> legal_stances(UnitKind kind) {
    return unit_type_defaults(kind).legal_stances;
}

bool legal_stance(UnitKind kind, UnitStance stance) {
    const std::vector<UnitStance> stances = legal_stances(kind);
    return std::find(stances.begin(), stances.end(), stance) != stances.end();
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
constexpr int create_mongol_lancers_population_cost = 1;
constexpr int create_mongol_lancers_metal_cost = 2;
constexpr int create_mongol_lancers_horses_cost = 3;
constexpr int horde_unit_training_turns = 3;
constexpr int minimum_combat_readiness_percent = 25;
constexpr int full_move_readiness_cost = 8;
constexpr int max_move_readiness_cost = 8;
constexpr int attack_readiness_cost = 10;
constexpr int turn_readiness_recovery = 25;
constexpr int flanked_defense_percent = 75;
constexpr int defender_retreat_condition_threshold = 50;
constexpr int feigned_retreat_pursuit_readiness_penalty = 15;
constexpr int feigned_retreat_defender_hp_damage = 1;
constexpr int feigned_retreat_defender_readiness_penalty = 10;
constexpr int blocked_retreat_readiness_penalty = 15;
constexpr int pasture_capacity_grassland = 100;
constexpr int pasture_consumption_per_horse = 8;
constexpr int starvation_horse_loss_turn_threshold = 3;
constexpr int starvation_horse_loss_percent = 25;

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
            return 8;
        case Terrain::Hill:
        case Terrain::Forest:
            return 12;
        case Terrain::Urban:
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

bool uses_mounted_road_cost(UnitKind kind) {
    return kind == UnitKind::HorseArcher
        || kind == UnitKind::ChineseCavalry
        || kind == UnitKind::MongolLancer
        || kind == UnitKind::PersianCavalry;
}

bool is_cavalry_unit(UnitKind kind) {
    return uses_mounted_road_cost(kind);
}

int road_modified_movement_cost(UnitKind kind, int terrain_cost) {
    if (terrain_cost == blocked_movement_cost()) {
        return terrain_cost;
    }
    if (uses_mounted_road_cost(kind)) {
        return ((terrain_cost + 1) / 2) + 1;
    }
    return (terrain_cost + 1) / 2;
}

int movement_cost(const GameState& state, const Unit& unit, const Coord& from, const GameHex& to_hex) {
    const int terrain_cost = terrain_movement_cost(to_hex.terrain);
    if (road_connects(state, from, to_hex.coord)) {
        return road_modified_movement_cost(unit.kind, terrain_cost);
    }
    return terrain_cost;
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

using Json = nlohmann::json;

int int_field(const Json& json, const char* key, int fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number_integer()) {
        return fallback;
    }
    return found->get<int>();
}

double number_field(const Json& json, const char* key, double fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number()) {
        return fallback;
    }
    return found->get<double>();
}

bool bool_field(const Json& json, const char* key, bool fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_boolean()) {
        return fallback;
    }
    return found->get<bool>();
}

std::uint32_t uint_field(const Json& json, const char* key, std::uint32_t fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number_integer()) {
        return fallback;
    }
    const int value = found->get<int>();
    return value < 0 ? fallback : static_cast<std::uint32_t>(value);
}

std::string string_field(const Json& json, const char* key, const std::string& fallback = "") {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_string()) {
        return fallback;
    }
    return found->get<std::string>();
}

Coord coord_from_json(const Json& json) {
    return {int_field(json, "q", 0), int_field(json, "r", 0)};
}

std::vector<std::string> string_array_field(const Json& json, const char* key) {
    std::vector<std::string> values;
    const auto found = json.find(key);
    if (found == json.end() || !found->is_array()) {
        return values;
    }
    for (const Json& item : *found) {
        if (item.is_string()) {
            values.push_back(item.get<std::string>());
        }
    }
    return values;
}

std::vector<int> int_array_field(const Json& json, const char* key) {
    std::vector<int> values;
    const auto found = json.find(key);
    if (found == json.end() || !found->is_array()) {
        return values;
    }
    for (const Json& item : *found) {
        if (item.is_number_integer()) {
            values.push_back(item.get<int>());
        }
    }
    return values;
}

std::vector<OwnerId> parse_turn_order(const Json& array_json) {
    std::vector<OwnerId> owners;
    if (!array_json.is_array()) {
        return owners;
    }
    for (const Json& item : array_json) {
        if (item.is_number_integer()) {
            owners.push_back(item.get<OwnerId>());
        }
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

const char* unit_kind_key(UnitKind kind) {
    return unit_kind_to_string(kind);
}

void load_unit_types() {
    (void)unit_type_table();
}

const char* unit_stance_key(UnitStance stance) {
    return unit_stance_to_string(stance);
}

UnitStance unit_stance_from_key(const std::string& stance) {
    return unit_stance_from_string(stance);
}

std::vector<UnitKind> unit_kinds() {
    return unit_type_table().order;
}

UnitDefaults unit_defaults(UnitKind kind) {
    return unit_type_defaults(kind);
}

bool unit_kind_available_to_owner(UnitKind kind, OwnerId owner) {
    if (owner == mongol_owner) {
        return kind == UnitKind::HorseArcher
            || kind == UnitKind::MongolLancer
            || kind == UnitKind::Herd
            || kind == UnitKind::Horde;
    }
    if (owner == chinese_owner) {
        return kind == UnitKind::ChineseMilitia
            || kind == UnitKind::Infantry
            || kind == UnitKind::ChineseCavalry;
    }
    if (owner == persian_owner) {
        return kind == UnitKind::PersianInfantry
            || kind == UnitKind::PersianCavalry;
    }
    return false;
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
            return {pasture_capacity_grassland, pasture_capacity_grassland, 0};
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
    state.factions = {
        faction_for_owner(neutral_owner),
        faction_for_owner(mongol_owner),
        faction_for_owner(persian_owner),
        faction_for_owner(chinese_owner),
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
    state.factions = {
        faction_for_owner(mongol_owner),
        faction_for_owner(chinese_owner),
        faction_for_owner(persian_owner),
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

    auto set_sandbox_terrain = [&](Coord coord, Terrain terrain, const std::string& label) {
        if (coord.q < 1 || coord.q > state.width || coord.r < 1 || coord.r > state.height) {
            return;
        }
        GameHex& hex = state.hexes[static_cast<std::size_t>((coord.r - 1) * state.width + (coord.q - 1))];
        hex.terrain = terrain;
        hex.tags = terrain == Terrain::Grassland ? tag_mask(HexTag::BaseSteppe) : 0;
        hex.pasture = initial_pasture_for_terrain(terrain);
        hex.source_labels = label.empty() ? std::vector<std::string>{} : std::vector<std::string>{label};
    };
    const std::vector<Coord> sandbox_forests = {
        {5, 4}, {6, 4}, {5, 5},
        {10, 9}, {11, 9}, {12, 9}, {11, 10},
        {18, 14}, {19, 14}, {20, 15},
    };
    const std::vector<Coord> sandbox_hills = {
        {4, 8}, {5, 8}, {6, 8},
        {11, 5}, {12, 5}, {12, 6},
        {24, 12}, {25, 12}, {25, 13},
    };
    const std::vector<Coord> sandbox_mountains = {
        {13, 2}, {14, 2}, {13, 3}, {14, 3}, {15, 3},
        {29, 18}, {30, 18}, {30, 19}, {31, 19},
    };
    const std::vector<Coord> sandbox_marshes = {
        {16, 7}, {17, 7}, {17, 8},
        {34, 21}, {35, 21},
    };
    for (const Coord coord : sandbox_forests) {
        set_sandbox_terrain(coord, Terrain::Forest, "sandbox_forest");
    }
    for (const Coord coord : sandbox_hills) {
        set_sandbox_terrain(coord, Terrain::Hill, "sandbox_hill");
    }
    for (const Coord coord : sandbox_mountains) {
        set_sandbox_terrain(coord, Terrain::Mountain, "sandbox_mountain");
    }
    for (const Coord coord : sandbox_marshes) {
        set_sandbox_terrain(coord, Terrain::Marsh, "sandbox_marsh");
    }

    auto add_sandbox_road = [&](int id, std::vector<Coord> path) {
        path.erase(std::remove_if(path.begin(), path.end(), [&](const Coord& coord) {
            return coord.q < 1 || coord.q > state.width || coord.r < 1 || coord.r > state.height;
        }), path.end());
        if (path.size() < 2) {
            return;
        }
        Road road;
        road.id = id;
        road.feature = "sandbox_road";
        road.path = std::move(path);
        state.roads.push_back(std::move(road));
    };
    add_sandbox_road(1, {{3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}, {8, 6}, {9, 6}, {10, 6}, {11, 6}, {12, 6}});
    add_sandbox_road(2, {{5, 4}, {6, 4}, {7, 4}, {8, 4}, {9, 4}, {10, 4}, {11, 5}, {12, 5}});
    add_sandbox_road(3, {{22, 11}, {23, 11}, {24, 12}, {25, 12}, {26, 13}, {27, 13}, {28, 14}});

    auto make_unit = [](int id, OwnerId owner, UnitKind kind, Coord coord) {
        Unit unit;
        unit.id = id;
        unit.owner = owner;
        unit.kind = kind;
        unit.stance = default_stance(unit.kind);
        unit.coord = coord;
        unit.hp = default_hp(unit.kind);
        unit.max_hp = unit.hp;
        unit.attack = default_attack(unit.kind);
        unit.defense = default_defense(unit.kind);
        unit.readiness_damage = default_readiness_damage(unit.kind);
        unit.max_readiness = default_readiness(unit.kind);
        unit.readiness = unit.max_readiness;
        unit.scaled_move = to_scaled_move(default_move(unit.kind));
        unit.remaining_scaled_move = unit.scaled_move;
        const UnitDefaults defaults = unit_defaults(unit.kind);
        unit.population = defaults.population;
        unit.horses = defaults.horses;
        unit.projects_zoc = default_projects_zoc(kind);
        unit.respects_zoc = default_respects_zoc(kind);
        return unit;
    };

    state.units = {
        make_unit(1, mongol_owner, UnitKind::HorseArcher, {3, 5}),
        make_unit(2, mongol_owner, UnitKind::MongolLancer, {3, 7}),
        make_unit(3, mongol_owner, UnitKind::Horde, {3, 6}),
        make_unit(4, mongol_owner, UnitKind::Herd, {2, 6}),
        make_unit(5, chinese_owner, UnitKind::ChineseCavalry, {6, 5}),
        make_unit(6, chinese_owner, UnitKind::Infantry, {8, 7}),
        make_unit(7, chinese_owner, UnitKind::Infantry, {8, 6}),
        make_unit(8, chinese_owner, UnitKind::ChineseCavalry, {9, 6}),
        make_unit(9, chinese_owner, UnitKind::ChineseMilitia, {7, 7}),
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

GameHex* find_hex(GameState& state, const Coord& coord) {
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

FactionState* find_faction(GameState& state, OwnerId owner) {
    const auto found = std::find_if(state.factions.begin(), state.factions.end(), [&](const FactionState& faction) {
        return faction.id == owner;
    });
    return found == state.factions.end() ? nullptr : &*found;
}

const FactionState* find_faction(const GameState& state, OwnerId owner) {
    const auto found = std::find_if(state.factions.begin(), state.factions.end(), [&](const FactionState& faction) {
        return faction.id == owner;
    });
    return found == state.factions.end() ? nullptr : &*found;
}

int faction_metal(const GameState& state, OwnerId owner) {
    const FactionState* faction = find_faction(state, owner);
    return faction == nullptr ? 0 : faction->metal;
}

bool faction_ai_controlled(const GameState& state, OwnerId owner) {
    const FactionState* faction = find_faction(state, owner);
    return faction != nullptr && faction->enabled && faction->ai_controlled;
}

int terrain_defense_percent(Terrain terrain) {
    switch (terrain) {
        case Terrain::Hill:
        case Terrain::Forest:
            return 125;
        case Terrain::Mountain:
        case Terrain::Urban:
            return 150;
        case Terrain::Marsh:
            return 115;
        case Terrain::Desert:
            return 90;
        case Terrain::Grassland:
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

int unit_terrain_defense_percent_at(const GameState& state, const Unit& unit) {
    if (is_cavalry_unit(unit.kind)) {
        return 100;
    }
    return terrain_defense_percent_at(state, unit.coord);
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

int percent_of(int value, int maximum) {
    if (maximum <= 0) {
        return 0;
    }
    return std::max(0, std::min(100, (std::max(0, value) * 100 + maximum / 2) / maximum));
}

int combined_condition_percent(int hp, int max_hp, int readiness, int max_readiness) {
    return (percent_of(hp, max_hp) * percent_of(readiness, max_readiness) + 50) / 100;
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
    const int proportional_cost = (full_move_readiness_cost * scaled_cost + unit.scaled_move - 1) / unit.scaled_move;
    return std::max(1, std::min(max_move_readiness_cost, proportional_cost));
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

int effective_defense_score(const GameState& state, const Unit& defender, int flanking_defense_percent = 100) {
    const int defense_strength = std::max(1, defender.defense);
    if (defense_strength <= 0) {
        return 0;
    }
    const int terrain_adjusted = std::max(
        1,
        (defense_strength * unit_terrain_defense_percent_at(state, defender) + 99) / 100
    );
    return std::max(1, (terrain_adjusted * std::max(0, flanking_defense_percent) + 50) / 100);
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
        return {4, 0, "none"};
    }
    if (index == -4) {
        return {3, 0, "none"};
    }
    if (index == -3) {
        return {3, 0, "none"};
    }
    if (index == -2) {
        return {2, 0, "none"};
    }
    if (index == -1) {
        return {2, 1, "none"};
    }
    if (index == 0) {
        return {1, 1, "none"};
    }
    if (index == 1) {
        return {1, 2, "none"};
    }
    if (index == 2) {
        return {0, 2, "none"};
    }
    if (index == 3) {
        return {0, 3, "none"};
    }
    if (index == 4) {
        return {0, 3, "none"};
    }
    return {0, 4, "none"};
}

int scaled_damage_for_remaining(int base_damage, int current, int maximum) {
    if (base_damage <= 0 || current <= 0 || maximum <= 0) {
        return 0;
    }
    return std::max(1, (base_damage * current + maximum - 1) / maximum);
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
    if (retreat_option == "defender") {
        return "Defender retreats";
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
        if (hex == nullptr || movement_cost(state, source, source.coord, *hex) == blocked_movement_cost()) {
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

void mark_enemy_contact(GameState& state, int unit_id) {
    const auto found = std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id == unit_id;
    });
    if (found == state.units.end()) {
        return;
    }
    bool contacted = false;
    for (Unit& unit : state.units) {
        if (unit.id == found->id || unit.owner == found->owner || !adjacent(unit.coord, found->coord)) {
            continue;
        }
        unit.contacted_enemy_this_turn = true;
        contacted = true;
    }
    found->contacted_enemy_this_turn = found->contacted_enemy_this_turn || contacted;
}

bool defender_flanked_by_separated_unit(const GameState& state, const Unit& attacker, const Unit& defender) {
    return std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id != attacker.id
            && unit.id != defender.id
            && unit.owner == attacker.owner
            && unit.hp > 0
            && can_attack(unit.kind)
            && unit.attack > 0
            && adjacent(unit.coord, defender.coord)
            && !adjacent(unit.coord, attacker.coord);
    }) != state.units.end();
}

bool can_pursue_into_defender_hex(const GameState& state, const Unit& attacker, const Unit& defender) {
    const GameHex* hex = find_hex(state, defender.coord);
    return hex != nullptr && movement_cost(state, attacker, attacker.coord, *hex) != blocked_movement_cost();
}

bool open_terrain_for_feigned_retreat(Terrain terrain) {
    return terrain == Terrain::Grassland || terrain == Terrain::Desert;
}

bool defender_on_feigned_retreat_terrain(const GameState& state, const Unit& defender) {
    const GameHex* hex = find_hex(state, defender.coord);
    return hex != nullptr && open_terrain_for_feigned_retreat(hex->terrain);
}

bool find_ordinary_retreat_hex(const GameState& state, const Unit& retreating, const Unit& opposing, Coord& retreat_to) {
    struct Candidate {
        Coord coord;
        int distance = 0;
    };
    std::vector<Candidate> preferred;
    std::vector<Candidate> fallback;
    const int current_distance = hex_distance(retreating.coord, opposing.coord);
    for (int direction = 0; direction < 6; ++direction) {
        const Coord candidate = neighbor_in_direction(retreating.coord, direction);
        if (!in_bounds(state, candidate) || occupied_by_other_unit(state, candidate, retreating.id)) {
            continue;
        }
        const GameHex* hex = find_hex(state, candidate);
        if (hex == nullptr || movement_cost(state, retreating, retreating.coord, *hex) == blocked_movement_cost()) {
            continue;
        }
        if (retreating.respects_zoc && enemy_zoc_at(state, candidate, retreating)) {
            continue;
        }
        const int distance = hex_distance(candidate, opposing.coord);
        if (distance > current_distance) {
            preferred.push_back({candidate, distance});
        } else if (distance == current_distance) {
            fallback.push_back({candidate, distance});
        }
    }
    std::vector<Candidate>& candidates = preferred.empty() ? fallback : preferred;
    if (candidates.empty()) {
        return false;
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& first, const Candidate& second) {
        if (first.distance != second.distance) {
            return first.distance > second.distance;
        }
        return coord_less(first.coord, second.coord);
    });
    retreat_to = candidates.front().coord;
    return true;
}

bool find_feigned_retreat_hex(const GameState& state, const Unit& attacker, const Unit& defender, Coord& retreat_to) {
    std::vector<Coord> preferred;
    std::vector<Coord> fallback;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord candidate = neighbor_in_direction(defender.coord, direction);
        if (!in_bounds(state, candidate) || occupied_by_other_unit(state, candidate, defender.id)) {
            continue;
        }
        const GameHex* hex = find_hex(state, candidate);
        if (hex == nullptr || movement_cost(state, defender, defender.coord, *hex) == blocked_movement_cost()) {
            continue;
        }
        if (coord_equal(candidate, attacker.coord) || adjacent(candidate, attacker.coord)) {
            continue;
        }
        if (enemy_zoc_at(state, candidate, defender)) {
            fallback.push_back(candidate);
        } else {
            preferred.push_back(candidate);
        }
    }
    std::vector<Coord>& candidates = preferred.empty() ? fallback : preferred;
    if (candidates.empty()) {
        return false;
    }
    std::sort(candidates.begin(), candidates.end(), coord_less);
    retreat_to = candidates.front();
    return true;
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
            const int step_cost = movement_cost(state, *unit, current, *hex);
            if (step_cost == blocked_movement_cost()) {
                continue;
            }
            const int next_cost = current_cost + step_cost;
            const bool first_step = current_cost == 0 && coord_equal(current, unit->coord);
            const bool affordable = next_cost <= unit->remaining_scaled_move;
            const bool minimum_step = first_step && !unit->moved_this_turn;
            if (!affordable && !minimum_step) {
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
            if (affordable && (!unit->respects_zoc || !enemy_zoc_at(state, next, *unit))) {
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

CombatantPreview combatant_preview(const GameState& state, const Unit& unit, int flanking_defense_percent = 100) {
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
    preview.base_readiness_damage = unit.readiness_damage;
    preview.hp_percent = hp_percent(unit);
    preview.readiness = clamped_readiness(unit);
    preview.max_readiness = std::max(1, unit.max_readiness);
    preview.readiness_percent = readiness_percent(unit);
    preview.terrain_defense_percent = unit_terrain_defense_percent_at(state, unit);
    preview.flanking_defense_percent = flanking_defense_percent;
    preview.effective_attack = effective_attack_score(unit);
    preview.effective_defense = effective_defense_score(state, unit, flanking_defense_percent);
    preview.result_hp = unit.hp;
    preview.result_readiness = clamped_readiness(unit);
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
    preview.defender_flanked = defender_flanked_by_separated_unit(state, *attacker, *defender);
    preview.flanking_defense_percent = preview.defender_flanked ? flanked_defense_percent : 100;
    preview.defender = combatant_preview(state, *defender, preview.flanking_defense_percent);

    preview.base_differential = preview.attacker.effective_attack - preview.defender.effective_defense;
    const double hp_multiplier = hp_ratio(*attacker, *defender);
    const double readiness_multiplier = readiness_ratio_for_combat(*attacker, *defender);
    const double condition_multiplier = hp_multiplier * readiness_multiplier;
    preview.hp_ratio_percent = ratio_percent(hp_multiplier);
    preview.readiness_ratio_percent = ratio_percent(readiness_multiplier);
    preview.condition_ratio_percent = ratio_percent(condition_multiplier);
    preview.crt_index = std::max(
        -6,
        std::min(6, static_cast<int>(std::lround(preview.base_differential * condition_multiplier)))
    );
    if (is_cavalry_unit(attacker->kind) && defender->kind == UnitKind::Herd) {
        preview.special_resolution = "horse_stealing";
        preview.defender_retaliates = false;
        preview.readiness_impact = "No readiness damage";
        preview.retreat_impact = "Herd changes control";
        preview.attacker.damage_taken = 1;
        preview.attacker.result_hp = std::max(0, attacker->hp - 1);
        preview.attacker.destroyed = preview.attacker.result_hp <= 0;
        preview.attacker.result_readiness = clamped_readiness(*attacker);
        preview.defender.result_hp = defender->hp;
        preview.defender.result_readiness = clamped_readiness(*defender);
        return preview;
    }
    Coord retreat_to;
    if (defender->kind == UnitKind::HorseArcher
        && defender->stance == UnitStance::FeignedRetreat
        && attacker->kind != UnitKind::HorseArcher
        && defender_on_feigned_retreat_terrain(state, *defender)
        && can_pursue_into_defender_hex(state, *attacker, *defender)
        && find_feigned_retreat_hex(state, *attacker, *defender, retreat_to)) {
        preview.special_resolution = "feigned_retreat";
        preview.defender_retaliates = false;
        preview.retreat_option = "defender";
        preview.readiness_impact = "Pursuit costs attacker readiness";
        preview.retreat_impact = "Defender retreats; attacker pursues";
        preview.defender_retreat_to = retreat_to;
        preview.attacker_pursuit_to = defender->coord;
        preview.pursuit_readiness_penalty = feigned_retreat_pursuit_readiness_penalty;
        preview.attacker.damage_dealt = feigned_retreat_defender_hp_damage;
        preview.attacker.readiness_damage_taken = feigned_retreat_pursuit_readiness_penalty;
        preview.attacker.result_readiness = std::max(
            0,
            clamped_readiness(*attacker) - feigned_retreat_pursuit_readiness_penalty
        );
        preview.defender.damage_taken = feigned_retreat_defender_hp_damage;
        preview.defender.readiness_damage_taken = feigned_retreat_defender_readiness_penalty;
        preview.defender.result_hp = std::max(0, defender->hp - feigned_retreat_defender_hp_damage);
        preview.defender.result_readiness = std::max(
            0,
            clamped_readiness(*defender) - feigned_retreat_defender_readiness_penalty
        );
        preview.defender.destroyed = preview.defender.result_hp <= 0;
        return preview;
    }
    const CrtOutcome outcome = crt_outcome(preview.crt_index);
    preview.retreat_option = "none";
    preview.readiness_impact = readiness_impact_text(preview.readiness_ratio_percent);
    preview.retreat_impact = retreat_impact_text(preview.retreat_option);

    const int defender_hp_damage = scaled_damage_for_remaining(outcome.defender_damage, defender->hp, defender->max_hp);
    const int attacker_hp_damage = scaled_damage_for_remaining(outcome.attacker_damage, attacker->hp, attacker->max_hp);
    const int defender_readiness_damage = scaled_damage_for_remaining(attacker->readiness_damage, clamped_readiness(*defender), defender->max_readiness);
    const int attacker_readiness_damage = scaled_damage_for_remaining(attack_readiness_cost, clamped_readiness(*attacker), attacker->max_readiness);

    preview.attacker.damage_dealt = defender_hp_damage;
    preview.defender.damage_taken = defender_hp_damage;
    preview.defender.result_hp = std::max(0, defender->hp - defender_hp_damage);
    preview.attacker.readiness_damage_dealt = defender_readiness_damage;
    preview.defender.readiness_damage_taken = defender_readiness_damage;
    preview.defender.result_readiness = std::max(0, clamped_readiness(*defender) - defender_readiness_damage);
    preview.defender.destroyed = preview.defender.result_hp <= 0;

    preview.defender_retaliates = !preview.defender.destroyed && attacker_hp_damage > 0;
    preview.defender.damage_dealt = attacker_hp_damage;
    preview.attacker.damage_taken = attacker_hp_damage;
    preview.attacker.readiness_damage_taken = attacker_readiness_damage;
    preview.attacker.result_hp = std::max(0, attacker->hp - attacker_hp_damage);
    preview.attacker.result_readiness = std::max(0, clamped_readiness(*attacker) - attacker_readiness_damage);
    preview.attacker.destroyed = preview.attacker.result_hp <= 0;

    const int defender_result_condition = combined_condition_percent(
        preview.defender.result_hp,
        defender->max_hp,
        preview.defender.result_readiness,
        defender->max_readiness);
    if (!preview.defender.destroyed
        && defender_hp_damage > 0
        && defender_result_condition <= defender_retreat_condition_threshold) {
        preview.retreat_option = "defender";
        preview.retreat_impact = retreat_impact_text(preview.retreat_option);
    }

    if (preview.retreat_option == "defender" && !preview.defender.destroyed) {
        if (find_ordinary_retreat_hex(state, *defender, *attacker, preview.defender_retreat_to)) {
            preview.retreat_impact = "Defender retreats";
        } else {
            preview.retreat_blocked = true;
            preview.blocked_retreat_readiness_penalty = blocked_retreat_readiness_penalty;
            preview.retreat_impact = "Defender retreat blocked";
            preview.defender.readiness_damage_taken += blocked_retreat_readiness_penalty;
            preview.defender.result_readiness = std::max(0, preview.defender.result_readiness - blocked_retreat_readiness_penalty);
        }
    }
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

bool set_unit_stance(GameState& state, int unit_id, UnitStance stance) {
    Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->owner != active_faction(state) || !legal_stance(unit->kind, stance)) {
        return false;
    }
    unit->stance = stance;
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

void cancel_horde_production(Unit& unit) {
    if (unit.kind != UnitKind::Horde || !unit.production_active) {
        return;
    }
    unit.production_active = false;
    unit.production_turns_remaining = 0;
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
    cancel_horde_production(*unit);
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
    mark_enemy_contact(state, unit_id);
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
    cancel_horde_production(*defender);

    if (preview.special_resolution == "feigned_retreat") {
        attacker->contacted_enemy_this_turn = true;
        defender->contacted_enemy_this_turn = true;
        defender->hp = preview.defender.result_hp;
        defender->readiness = preview.defender.result_readiness;
        defender->coord = preview.defender_retreat_to;
        defender->remaining_scaled_move = 0;
        defender->move_done = true;
        defender->moved_this_turn = true;
        attacker->coord = preview.attacker_pursuit_to;
        cancel_horde_production(*attacker);
        attacker->readiness = preview.attacker.result_readiness;
        attacker->remaining_scaled_move = 0;
        attacker->move_done = true;
        attacker->combat_done = true;
        mark_enemy_contact(state, attacker_id);
        mark_enemy_contact(state, defender_id);
        state.units.erase(std::remove_if(state.units.begin(), state.units.end(), [](const Unit& unit) {
            return unit.hp <= 0;
        }), state.units.end());
        state.selected_unit_id = attacker_id;
        refresh_legal_actions(state);
        return true;
    }

    if (preview.special_resolution == "horse_stealing") {
        const OwnerId captured_owner = attacker->owner;
        attacker->hp = preview.attacker.result_hp;
        attacker->contacted_enemy_this_turn = true;
        attacker->remaining_scaled_move = 0;
        attacker->move_done = true;
        attacker->combat_done = true;
        defender->owner = captured_owner;
        defender->contacted_enemy_this_turn = true;
        defender->remaining_scaled_move = 0;
        defender->move_done = true;
        defender->moved_this_turn = true;
        state.units.erase(std::remove_if(state.units.begin(), state.units.end(), [](const Unit& unit) {
            return unit.hp <= 0;
        }), state.units.end());
        state.selected_unit_id = find_unit(state, attacker_id) == nullptr ? 0 : attacker_id;
        refresh_legal_actions(state);
        return true;
    }

    defender->hp = preview.defender.result_hp;
    attacker->hp = preview.attacker.result_hp;
    defender->readiness = preview.defender.result_readiness;
    attacker->readiness = preview.attacker.result_readiness;
    attacker->contacted_enemy_this_turn = true;
    defender->contacted_enemy_this_turn = true;
    if (!preview.retreat_blocked && preview.retreat_option == "defender" && defender->hp > 0) {
        defender->coord = preview.defender_retreat_to;
        defender->remaining_scaled_move = 0;
        defender->move_done = true;
        defender->moved_this_turn = true;
    }
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
    herd.stance = default_stance(herd.kind);
    herd.coord = destination;
    herd.hp = default_hp(herd.kind);
    herd.max_hp = herd.hp;
    herd.attack = default_attack(herd.kind);
    herd.defense = default_defense(herd.kind);
    herd.readiness_damage = default_readiness_damage(herd.kind);
    herd.max_readiness = default_readiness(herd.kind);
    herd.readiness = herd.max_readiness;
    herd.scaled_move = to_scaled_move(default_move(herd.kind));
    herd.remaining_scaled_move = herd.scaled_move;
    herd.horses = horses;
    herd.projects_zoc = default_projects_zoc(herd.kind);
    herd.respects_zoc = default_respects_zoc(herd.kind);
    const int horde_id = horde->id;
    const int herd_id = herd.id;
    state.units.push_back(herd);
    mark_enemy_contact(state, horde_id);
    mark_enemy_contact(state, herd_id);
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

CreateUnitOptions create_unit_options(const GameState& state, int unit_id, UnitKind kind);
bool create_unit_from_horde(GameState& state, int unit_id, UnitKind kind, Coord destination);

CreateHorseArchersOptions create_horse_archers_options(const GameState& state, int unit_id) {
    return create_unit_options(state, unit_id, UnitKind::HorseArcher);
}

bool create_horse_archers(GameState& state, int unit_id, Coord destination) {
    return create_unit_from_horde(state, unit_id, UnitKind::HorseArcher, destination);
}

CreateUnitOptions create_mongol_lancers_options(const GameState& state, int unit_id) {
    return create_unit_options(state, unit_id, UnitKind::MongolLancer);
}

bool create_mongol_lancers(GameState& state, int unit_id, Coord destination) {
    return create_unit_from_horde(state, unit_id, UnitKind::MongolLancer, destination);
}

CreateUnitOptions create_unit_options(const GameState& state, int unit_id, UnitKind kind) {
    CreateUnitOptions options;
    options.kind = kind;
    options.unit_id = unit_id;
    if (kind == UnitKind::MongolLancer) {
        options.population_cost = create_mongol_lancers_population_cost;
        options.metal_cost = create_mongol_lancers_metal_cost;
        options.horses_cost = create_mongol_lancers_horses_cost;
    } else {
        options.population_cost = create_horse_archers_population_cost;
        options.metal_cost = create_horse_archers_metal_cost;
        options.horses_cost = create_horse_archers_horses_cost;
    }
    const Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr
        || horde->kind != UnitKind::Horde
        || horde->owner != active_faction(state)
        || (kind == UnitKind::MongolLancer && horde->owner != mongol_owner)
        || (kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_owner(kind, horde->owner)
        || horde->production_active
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)) {
        return options;
    }

    return options;
}

bool create_unit_from_horde(GameState& state, int unit_id, UnitKind kind, Coord destination) {
    (void) destination;
    CreateUnitOptions options = create_unit_options(state, unit_id, kind);
    Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr
        || horde->kind != UnitKind::Horde
        || horde->production_active
        || horde->owner != active_faction(state)
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)
        || (kind == UnitKind::MongolLancer && horde->owner != mongol_owner)
        || (kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_owner(kind, horde->owner)) {
        return false;
    }
    horde->production_active = true;
    horde->production_kind = kind;
    horde->production_turns_remaining = horde_unit_training_turns;
    horde->remaining_scaled_move = 0;
    horde->move_done = true;
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

bool complete_horde_production(GameState& state, Unit& horde) {
    if (!horde.production_active || horde.kind != UnitKind::Horde) {
        return false;
    }
    const UnitKind kind = horde.production_kind;
    CreateUnitOptions options;
    options.kind = kind;
    options.unit_id = horde.id;
    if (kind == UnitKind::MongolLancer) {
        options.population_cost = create_mongol_lancers_population_cost;
        options.metal_cost = create_mongol_lancers_metal_cost;
        options.horses_cost = create_mongol_lancers_horses_cost;
    } else {
        options.population_cost = create_horse_archers_population_cost;
        options.metal_cost = create_horse_archers_metal_cost;
        options.horses_cost = create_horse_archers_horses_cost;
    }
    options.deployable_hexes = adjacent_deployable_hexes(state, horde);
    FactionState* faction = find_faction(state, horde.owner);
    if ((kind == UnitKind::MongolLancer && horde.owner != mongol_owner)
        || (kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_owner(kind, horde.owner)
        || horde.population < options.population_cost
        || horde.horses < options.horses_cost
        || faction == nullptr
        || faction->metal < options.metal_cost
        || options.deployable_hexes.empty()) {
        horde.production_active = false;
        horde.production_turns_remaining = 0;
        return false;
    }

    const Coord destination = options.deployable_hexes.front();
    horde.population -= options.population_cost;
    horde.horses -= options.horses_cost;
    faction->metal -= options.metal_cost;
    Unit created;
    created.id = next_unit_id(state);
    created.owner = horde.owner;
    created.kind = kind;
    created.stance = default_stance(created.kind);
    created.coord = destination;
    created.hp = default_hp(created.kind);
    created.max_hp = created.hp;
    created.attack = default_attack(created.kind);
    created.defense = default_defense(created.kind);
    created.readiness_damage = default_readiness_damage(created.kind);
    created.max_readiness = default_readiness(created.kind);
    created.readiness = created.max_readiness;
    created.scaled_move = to_scaled_move(default_move(created.kind));
    created.remaining_scaled_move = created.scaled_move;
    created.projects_zoc = default_projects_zoc(created.kind);
    created.respects_zoc = default_respects_zoc(created.kind);
    const int horde_id = horde.id;
    const int created_id = created.id;
    horde.production_active = false;
    horde.production_turns_remaining = 0;
    state.units.push_back(created);
    mark_enemy_contact(state, horde_id);
    mark_enemy_contact(state, created_id);
    refresh_legal_actions(state);
    return true;
}

void start_faction_turn(GameState& state, OwnerId owner) {
    std::vector<int> ready_horde_ids;
    for (Unit& unit : state.units) {
        if (unit.owner == owner) {
            if (unit.kind == UnitKind::Horde && unit.production_active) {
                unit.production_turns_remaining = std::max(0, unit.production_turns_remaining - 1);
                if (unit.production_turns_remaining == 0) {
                    ready_horde_ids.push_back(unit.id);
                }
            }
            const bool currently_in_contact = enemy_unit_adjacent_to(state, unit);
            if (!unit.moved_this_turn
                && !unit.combat_done
                && !unit.contacted_enemy_this_turn
                && !currently_in_contact) {
                recover_readiness(unit, turn_readiness_recovery);
            }
            unit.remaining_scaled_move = unit.scaled_move;
            unit.move_done = false;
            unit.moved_this_turn = false;
            unit.combat_done = false;
            unit.contacted_enemy_this_turn = currently_in_contact;
        }
    }
    for (const int horde_id : ready_horde_ids) {
        Unit* horde = find_unit(state, horde_id);
        if (horde != nullptr && horde->owner == owner) {
            complete_horde_production(state, *horde);
        }
    }
}

std::vector<int> ai_unit_turn_order(const GameState& state, OwnerId owner) {
    std::vector<int> ids;
    for (const Unit& unit : state.units) {
        if (unit.owner == owner && unit.hp > 0 && can_attack(unit.kind)) {
            ids.push_back(unit.id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

const Unit* nearest_enemy_unit_from_coord(
    const GameState& state,
    OwnerId owner,
    Coord origin,
    OwnerId target_owner = neutral_owner,
    bool horde_only = false
) {
    const Unit* nearest = nullptr;
    int nearest_distance = std::numeric_limits<int>::max();
    for (const Unit& candidate : state.units) {
        if (candidate.owner == owner
            || candidate.hp <= 0
            || (target_owner != neutral_owner && candidate.owner != target_owner)
            || (horde_only && candidate.kind != UnitKind::Horde)) {
            continue;
        }
        const int distance = hex_distance(origin, candidate.coord);
        if (distance < nearest_distance
            || (distance == nearest_distance && nearest != nullptr && candidate.id < nearest->id)
            || (distance == nearest_distance && nearest == nullptr)) {
            nearest = &candidate;
            nearest_distance = distance;
        }
    }
    return nearest;
}

const Unit* nearest_enemy_unit(const GameState& state, const Unit& unit) {
    return nearest_enemy_unit_from_coord(state, unit.owner, unit.coord);
}

bool ai_attack_allowed_by_directive(const Unit& defender, const AiDirective& directive) {
    if (directive.kind == AiDirectiveKind::HuntHorde) {
        return defender.kind == UnitKind::Horde
            && (directive.target_owner == neutral_owner || defender.owner == directive.target_owner);
    }
    return true;
}

bool ai_attack_best_adjacent_enemy(GameState& state, int attacker_id, const AiDirective& directive) {
    const std::vector<AttackableUnit> attacks = attackable_units(state, attacker_id);
    if (attacks.empty()) {
        return false;
    }
    const Unit* attacker = find_unit(state, attacker_id);
    if (attacker == nullptr) {
        return false;
    }
    int best_defender_id = attacks.front().unit_id;
    int best_score = std::numeric_limits<int>::max();
    for (const AttackableUnit& attack : attacks) {
        const Unit* defender = find_unit(state, attack.unit_id);
        if (defender == nullptr) {
            continue;
        }
        if (!ai_attack_allowed_by_directive(*defender, directive)) {
            continue;
        }
        const int score = defender->hp * 100
            + effective_defense_score(state, *defender) * 10
            + hex_distance(attacker->coord, defender->coord);
        if (score < best_score || (score == best_score && defender->id < best_defender_id)) {
            best_score = score;
            best_defender_id = defender->id;
        }
    }
    if (best_score == std::numeric_limits<int>::max()) {
        return false;
    }
    return attack_unit(state, attacker_id, best_defender_id);
}

bool ai_move_toward_target(GameState& state, int unit_id, Coord target) {
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->move_done || unit->remaining_scaled_move <= 0) {
        return false;
    }
    const int current_distance = hex_distance(unit->coord, target);
    const std::vector<ReachableHex> reachable = reachable_hexes(state, unit_id);
    if (reachable.empty()) {
        return false;
    }

    const ReachableHex* best = nullptr;
    int best_distance = current_distance;
    for (const ReachableHex& candidate : reachable) {
        const int distance = hex_distance(candidate.coord, target);
        if (best == nullptr
            || distance < best_distance
            || (distance == best_distance && candidate.scaled_cost < best->scaled_cost)
            || (distance == best_distance
                && candidate.scaled_cost == best->scaled_cost
                && coord_less(candidate.coord, best->coord))) {
            best = &candidate;
            best_distance = distance;
        }
    }
    if (best == nullptr || best_distance >= current_distance) {
        return false;
    }
    return move_unit(state, unit_id, best->coord);
}

AiDirective default_hunt_directive() {
    AiDirective directive;
    directive.kind = AiDirectiveKind::Hunt;
    return directive;
}

const Unit* ai_target_for_directive(const GameState& state, const Unit& unit, const AiDirective& directive) {
    switch (directive.kind) {
        case AiDirectiveKind::HuntHorde:
            return nearest_enemy_unit_from_coord(state, unit.owner, unit.coord, directive.target_owner, true);
        case AiDirectiveKind::DefendHex:
            return nearest_enemy_unit_from_coord(state, unit.owner, directive.target);
        case AiDirectiveKind::CaptureHex:
            return nearest_enemy_unit_from_coord(state, unit.owner, directive.target);
        case AiDirectiveKind::Hunt:
            return nearest_enemy_unit(state, unit);
    }
    return nearest_enemy_unit(state, unit);
}

void execute_ai_unit_directive(GameState& state, int unit_id, const AiDirective& directive) {
    if (ai_attack_best_adjacent_enemy(state, unit_id, directive)) {
        return;
    }
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr) {
        return;
    }

    if (directive.kind == AiDirectiveKind::DefendHex
        && hex_distance(unit->coord, directive.target) > 1) {
        ai_move_toward_target(state, unit_id, directive.target);
        ai_attack_best_adjacent_enemy(state, unit_id, directive);
        return;
    }

    const Unit* target = ai_target_for_directive(state, *unit, directive);
    if (directive.kind == AiDirectiveKind::DefendHex) {
        if (target == nullptr || hex_distance(target->coord, directive.target) > 2) {
            return;
        }
    }
    if (target == nullptr) {
        if (directive.kind == AiDirectiveKind::CaptureHex) {
            ai_move_toward_target(state, unit_id, directive.target);
            ai_attack_best_adjacent_enemy(state, unit_id, directive);
        }
        return;
    }
    if (ai_move_toward_target(state, unit_id, target->coord)) {
        ai_attack_best_adjacent_enemy(state, unit_id, directive);
    }
}

void execute_ai_units(GameState& state, const std::vector<int>& unit_ids, const AiDirective& directive) {
    for (const int unit_id : unit_ids) {
        const Unit* unit = find_unit(state, unit_id);
        if (unit == nullptr || !can_attack(unit->kind) || unit->hp <= 0) {
            continue;
        }
        execute_ai_unit_directive(state, unit_id, directive);
    }
}

void execute_ai_turn(GameState& state, OwnerId owner) {
    std::vector<int> assigned_unit_ids;
    for (const AiGroup& group : state.ai_groups) {
        if (group.owner != owner) {
            continue;
        }
        std::vector<int> group_units = group.unit_ids;
        std::sort(group_units.begin(), group_units.end());
        execute_ai_units(state, group_units, group.directive);
        assigned_unit_ids.insert(assigned_unit_ids.end(), group_units.begin(), group_units.end());
    }

    std::sort(assigned_unit_ids.begin(), assigned_unit_ids.end());
    assigned_unit_ids.erase(std::unique(assigned_unit_ids.begin(), assigned_unit_ids.end()), assigned_unit_ids.end());
    std::vector<int> unassigned_unit_ids;
    for (const int unit_id : ai_unit_turn_order(state, owner)) {
        if (!std::binary_search(assigned_unit_ids.begin(), assigned_unit_ids.end(), unit_id)) {
            unassigned_unit_ids.push_back(unit_id);
        }
    }
    execute_ai_units(state, unassigned_unit_ids, default_hunt_directive());
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
}

bool grazes_pasture(const Unit& unit) {
    return (unit.kind == UnitKind::Horde || unit.kind == UnitKind::Herd)
        && unit.hp > 0
        && unit.horses > 0;
}

std::vector<GameHex*> grazing_hexes_for_unit(GameState& state, const Unit& unit) {
    std::vector<GameHex*> hexes;
    GameHex* origin = find_hex(state, unit.coord);
    if (origin != nullptr && origin->pasture.capacity > 0) {
        hexes.push_back(origin);
    }
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = neighbor_in_direction(unit.coord, direction);
        if (!in_bounds(state, neighbor)) {
            continue;
        }
        GameHex* hex = find_hex(state, neighbor);
        if (hex != nullptr && hex->pasture.capacity > 0) {
            hexes.push_back(hex);
        }
    }
    std::sort(hexes.begin(), hexes.end(), [](const GameHex* first, const GameHex* second) {
        return coord_less(first->coord, second->coord);
    });
    return hexes;
}

void apply_grazing_for_faction(GameState& state, OwnerId owner) {
    for (Unit& unit : state.units) {
        if (unit.owner != owner || !grazes_pasture(unit)) {
            continue;
        }
        std::vector<GameHex*> pasture_hexes = grazing_hexes_for_unit(state, unit);
        const int demand = unit.horses * pasture_consumption_per_horse;
        int consumed = 0;
        if (!pasture_hexes.empty()) {
            const int base = demand / static_cast<int>(pasture_hexes.size());
            int remainder = demand % static_cast<int>(pasture_hexes.size());
            for (GameHex* hex : pasture_hexes) {
                const int consumption = base + (remainder > 0 ? 1 : 0);
                if (remainder > 0) {
                    --remainder;
                }
                const int actual_consumption = std::min(hex->pasture.remaining, consumption);
                hex->pasture.remaining = std::max(0, hex->pasture.remaining - actual_consumption);
                consumed += actual_consumption;
            }
        }

        const int shortfall = std::max(0, demand - consumed);
        if (shortfall == 0) {
            unit.starvation_turns = 0;
            continue;
        }

        unit.starvation_turns += 1;
        if (unit.starvation_turns < starvation_horse_loss_turn_threshold) {
            continue;
        }
        const int numerator = unit.horses * shortfall * starvation_horse_loss_percent;
        const int denominator = std::max(1, demand * 100);
        const int horse_loss = std::min(
            unit.horses,
            std::max(1, (numerator + denominator - 1) / denominator)
        );
        unit.horses = std::max(0, unit.horses - horse_loss);
        if (unit.horses == 0) {
            unit.starvation_turns = 0;
        }
    }
}

void advance_to_next_faction(GameState& state) {
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }
    state.active_faction_index = (state.active_faction_index + 1) % static_cast<int>(state.turn_order.size());
    if (state.active_faction_index == 0) {
        state.round += 1;
    }
}

void end_turn(GameState& state) {
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }

    apply_grazing_for_faction(state, active_faction(state));
    const int faction_count = static_cast<int>(state.turn_order.size());
    for (int steps = 0; steps < faction_count; ++steps) {
        advance_to_next_faction(state);
        const OwnerId owner = active_faction(state);
        start_faction_turn(state, owner);
        if (faction_ai_controlled(state, owner)) {
            execute_ai_turn(state, owner);
            apply_grazing_for_faction(state, owner);
            continue;
        }
        break;
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

void print_pasture_json(const PastureState& pasture, std::ostream& out) {
    out << "{\"capacity\":" << pasture.capacity
        << ",\"remaining\":" << pasture.remaining
        << ",\"recoveryTurn\":" << pasture.recovery_turn
        << "}";
}

void print_unit_json(const Unit& unit, std::ostream& out) {
    const FactionState faction = faction_for_owner(unit.owner);
    out << "{\"id\":" << unit.id
        << ",\"owner\":" << unit.owner
        << ",\"faction\":\"" << faction.key << "\""
        << ",\"kind\":\"" << unit_kind_to_string(unit.kind) << "\""
        << ",\"stance\":\"" << unit_stance_to_string(unit.stance) << "\""
        << ",\"q\":" << unit.coord.q
        << ",\"r\":" << unit.coord.r
        << ",\"hp\":" << unit.hp
        << ",\"maxHp\":" << unit.max_hp
        << ",\"attack\":" << unit.attack
        << ",\"defense\":" << unit.defense
        << ",\"readinessDamage\":" << unit.readiness_damage
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
            << ",\"horses\":" << unit.horses
            << ",\"starvationTurns\":" << unit.starvation_turns
            << ",\"productionState\":\"" << (unit.production_active ? "building" : "idle") << "\""
            << ",\"productionKind\":\"" << unit_kind_to_string(unit.production_kind) << "\""
            << ",\"productionTurnsRemaining\":" << unit.production_turns_remaining;
    } else if (unit.kind == UnitKind::Herd) {
        out << ",\"horses\":" << unit.horses
            << ",\"starvationTurns\":" << unit.starvation_turns;
    }
    out << ",\"moveDone\":" << (unit.move_done ? "true" : "false")
        << ",\"movedThisTurn\":" << (unit.moved_this_turn ? "true" : "false")
        << ",\"combatDone\":" << (unit.combat_done ? "true" : "false")
        << ",\"contactedEnemyThisTurn\":" << (unit.contacted_enemy_this_turn ? "true" : "false")
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

void print_ai_groups_json(const std::vector<AiGroup>& groups, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const AiGroup& group = groups[i];
        out << "{\"id\":" << group.id
            << ",\"owner\":" << group.owner
            << ",\"name\":\"" << escape_json(group.name) << "\""
            << ",\"unitIds\":[";
        for (std::size_t unit_index = 0; unit_index < group.unit_ids.size(); ++unit_index) {
            if (unit_index > 0) {
                out << ",";
            }
            out << group.unit_ids[unit_index];
        }
        out << "],\"directive\":{\"type\":\"" << ai_directive_kind_to_string(group.directive.kind) << "\"";
        if (group.directive.kind == AiDirectiveKind::DefendHex || group.directive.kind == AiDirectiveKind::CaptureHex) {
            out << ",\"target\":";
            print_coord_json(group.directive.target, out);
        }
        if (group.directive.kind == AiDirectiveKind::HuntHorde) {
            const FactionState faction = faction_for_owner(group.directive.target_owner);
            out << ",\"faction\":\"" << escape_json(faction.key) << "\""
                << ",\"owner\":" << group.directive.target_owner;
        }
        out << "}}";
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
    out << "],\"factions\":[";
    for (std::size_t i = 0; i < state.factions.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const FactionState& faction = state.factions[i];
        out << "{\"id\":" << faction.id
            << ",\"key\":\"" << escape_json(faction.key) << "\""
            << ",\"name\":\"" << escape_json(faction.name) << "\""
            << ",\"color\":\"" << escape_json(faction.color) << "\""
            << ",\"metal\":" << faction.metal
            << ",\"treasure\":" << faction.treasure
            << ",\"enabled\":" << (faction.enabled ? "true" : "false")
            << ",\"ai\":" << (faction.ai_controlled ? "true" : "false")
            << "}";
    }
    out << "],\"aiGroups\":";
    print_ai_groups_json(state.ai_groups, out);
    out << "}";
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
        out << ",\"pasture\":";
        print_pasture_json(hex.pasture, out);
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
    const FactionState faction = faction_for_owner(preview.owner);
    out << "{\"unitId\":" << preview.unit_id
        << ",\"owner\":" << preview.owner
        << ",\"faction\":\"" << faction.key << "\""
        << ",\"kind\":\"" << unit_kind_to_string(preview.kind) << "\""
        << ",\"q\":" << preview.coord.q
        << ",\"r\":" << preview.coord.r
        << ",\"terrain\":\"" << terrain_to_string(preview.terrain) << "\""
        << ",\"hp\":" << preview.hp
        << ",\"maxHp\":" << preview.max_hp
        << ",\"baseAttack\":" << preview.base_attack
        << ",\"baseDefense\":" << preview.base_defense
        << ",\"baseReadinessDamage\":" << preview.base_readiness_damage
        << ",\"hpPercent\":" << preview.hp_percent
        << ",\"readiness\":" << preview.readiness
        << ",\"maxReadiness\":" << preview.max_readiness
        << ",\"readinessPercent\":" << preview.readiness_percent
        << ",\"terrainDefensePercent\":" << preview.terrain_defense_percent
        << ",\"flankingDefensePercent\":" << preview.flanking_defense_percent
        << ",\"effectiveAttack\":" << preview.effective_attack
        << ",\"effectiveDefense\":" << preview.effective_defense
        << ",\"damageDealt\":" << preview.damage_dealt
        << ",\"damageTaken\":" << preview.damage_taken
        << ",\"readinessDamageDealt\":" << preview.readiness_damage_dealt
        << ",\"readinessDamageTaken\":" << preview.readiness_damage_taken
        << ",\"resultHp\":" << preview.result_hp
        << ",\"resultReadiness\":" << preview.result_readiness
        << ",\"destroyed\":" << (preview.destroyed ? "true" : "false")
        << "}";
}

void print_combat_preview_json(const CombatPreview& preview, std::ostream& out) {
    out << "{\"valid\":" << (preview.valid ? "true" : "false")
        << ",\"defenderRetaliates\":" << (preview.defender_retaliates ? "true" : "false")
        << ",\"defenderFlanked\":" << (preview.defender_flanked ? "true" : "false")
        << ",\"flankingDefensePercent\":" << preview.flanking_defense_percent
        << ",\"baseDifferential\":" << preview.base_differential
        << ",\"hpRatioPercent\":" << preview.hp_ratio_percent
        << ",\"readinessRatioPercent\":" << preview.readiness_ratio_percent
        << ",\"conditionRatioPercent\":" << preview.condition_ratio_percent
        << ",\"crtIndex\":" << preview.crt_index
        << ",\"retreatOption\":\"" << escape_json(preview.retreat_option) << "\""
        << ",\"readinessImpact\":\"" << escape_json(preview.readiness_impact) << "\""
        << ",\"retreatImpact\":\"" << escape_json(preview.retreat_impact) << "\""
        << ",\"specialResolution\":\"" << escape_json(preview.special_resolution) << "\""
        << ",\"retreatBlocked\":" << (preview.retreat_blocked ? "true" : "false")
        << ",\"blockedRetreatReadinessPenalty\":" << preview.blocked_retreat_readiness_penalty
        << ",\"pursuitReadinessPenalty\":" << preview.pursuit_readiness_penalty
        << ",\"attackerRetreatTo\":";
    print_coord_json(preview.attacker_retreat_to, out);
    out << ",\"defenderRetreatTo\":";
    print_coord_json(preview.defender_retreat_to, out);
    out << ",\"attackerPursuitTo\":";
    print_coord_json(preview.attacker_pursuit_to, out);
    out << ",\"attacker\":";
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

void print_create_unit_options_json(const CreateUnitOptions& options, std::ostream& out) {
    out << "{\"unitId\":" << options.unit_id
        << ",\"kind\":\"" << unit_kind_to_string(options.kind) << "\""
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

void print_create_horse_archers_options_json(const CreateHorseArchersOptions& options, std::ostream& out) {
    print_create_unit_options_json(options, out);
}

void print_create_mongol_lancers_options_json(const CreateUnitOptions& options, std::ostream& out) {
    print_create_unit_options_json(options, out);
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
        out << ",\"pasture\":";
        print_pasture_json(hex.pasture, out);
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
    Json root;
    try {
        root = Json::parse(json);
    } catch (const Json::parse_error& error) {
        throw std::runtime_error(std::string("invalid game state JSON: ") + error.what());
    }
    if (!root.is_object()) {
        throw std::runtime_error("game state JSON must be an object");
    }

    GameState state;
    state.width = int_field(root, "width", 0);
    state.height = int_field(root, "height", 0);
    state.seed = uint_field(root, "seed", 0);

    const Json empty_object = Json::object();
    const Json empty_array = Json::array();
    const Json& game_json = root.contains("game") && root["game"].is_object() ? root["game"] : empty_object;
    const Json& game_meta = game_json.empty() ? root : game_json;
    state.round = int_field(game_meta, "round", 1);
    state.active_faction_index = int_field(game_meta, "activeFactionIndex", 0);
    state.selected_unit_id = int_field(game_meta, "selectedUnitId", 0);
    state.turn_order = parse_turn_order(game_meta.contains("turnOrder") ? game_meta["turnOrder"] : empty_array);
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }
    state.factions = {
        faction_for_owner(mongol_owner),
        faction_for_owner(chinese_owner),
        faction_for_owner(persian_owner),
    };
    const Json& faction_items = game_json.contains("factions") && game_json["factions"].is_array()
        ? game_json["factions"]
        : (root.contains("factions") && root["factions"].is_array() ? root["factions"] : empty_array);
    if (!faction_items.empty()) {
        state.factions.clear();
        for (const Json& faction_json : faction_items) {
            if (!faction_json.is_object()) {
                continue;
            }
            FactionState faction = faction_for_owner(int_field(faction_json, "id", neutral_owner));
            faction.key = string_field(faction_json, "key", faction.key);
            faction.name = string_field(faction_json, "name", faction.name);
            faction.color = string_field(faction_json, "color", faction.color);
            faction.metal = std::max(0, int_field(faction_json, "metal", 0));
            faction.treasure = std::max(0, int_field(faction_json, "treasure", 0));
            faction.enabled = bool_field(faction_json, "enabled", true);
            faction.ai_controlled = bool_field(faction_json, "ai", false);
            state.factions.push_back(std::move(faction));
        }
    }

    const Json& ai_group_items = game_json.contains("aiGroups") && game_json["aiGroups"].is_array()
        ? game_json["aiGroups"]
        : empty_array;
    for (const Json& group_json : ai_group_items) {
        if (!group_json.is_object()) {
            continue;
        }
        AiGroup group;
        group.id = int_field(group_json, "id", 0);
        group.owner = int_field(group_json, "owner", neutral_owner);
        group.name = string_field(group_json, "name", "");
        group.unit_ids = int_array_field(group_json, "unitIds");
        const Json& directive_json = group_json.contains("directive") && group_json["directive"].is_object()
            ? group_json["directive"]
            : empty_object;
        group.directive.kind = ai_directive_kind_from_string(string_field(directive_json, "type", "hunt"));
        if (directive_json.contains("target") && directive_json["target"].is_object()) {
            group.directive.target = coord_from_json(directive_json["target"]);
        }
        group.directive.target_owner = int_field(
            directive_json,
            "owner",
            owner_from_faction_key(string_field(directive_json, "faction", ""), neutral_owner)
        );
        state.ai_groups.push_back(std::move(group));
    }

    const Json& hex_items = root.contains("hexes") && root["hexes"].is_array() ? root["hexes"] : empty_array;
    for (const Json& hex_json : hex_items) {
        if (!hex_json.is_object()) {
            continue;
        }
        GameHex hex;
        hex.coord = coord_from_json(hex_json);
        hex.terrain = terrain_from_string(string_field(hex_json, "terrain", "none"));
        hex.tags = hex.terrain == Terrain::Grassland ? tag_mask(HexTag::BaseSteppe) : 0;
        hex.pasture = initial_pasture_for_terrain(hex.terrain);
        if (hex_json.contains("pasture") && hex_json["pasture"].is_object()) {
            const Json& pasture_json = hex_json["pasture"];
            hex.pasture.capacity = std::max(0, std::min(100, int_field(pasture_json, "capacity", hex.pasture.capacity)));
            hex.pasture.remaining = std::max(
                0,
                std::min(hex.pasture.capacity, int_field(pasture_json, "remaining", hex.pasture.remaining))
            );
            hex.pasture.recovery_turn = std::max(0, int_field(pasture_json, "recoveryTurn", hex.pasture.recovery_turn));
        }
        hex.source_labels = string_array_field(hex_json, "labels");
        if (!hex.source_labels.empty()) {
            hex.tags = tags_from_labels(hex.source_labels);
        } else if (hex.terrain == Terrain::Grassland) {
            hex.source_labels = {"base_steppe"};
        }
        state.hexes.push_back(std::move(hex));
    }

    const Json& road_items = root.contains("roads") && root["roads"].is_array() ? root["roads"] : empty_array;
    for (const Json& road_json : road_items) {
        if (!road_json.is_object()) {
            continue;
        }
        Road road;
        road.id = int_field(road_json, "id", 0);
        road.feature = string_field(road_json, "feature", "");
        const Json& path_items = road_json.contains("path") && road_json["path"].is_array() ? road_json["path"] : empty_array;
        for (const Json& coord_json : path_items) {
            if (coord_json.is_object()) {
                road.path.push_back(coord_from_json(coord_json));
            }
        }
        state.roads.push_back(std::move(road));
    }

    const Json& wall_items = root.contains("walls") && root["walls"].is_array() ? root["walls"] : empty_array;
    for (const Json& wall_json : wall_items) {
        if (!wall_json.is_object()) {
            continue;
        }
        Wall wall;
        wall.id = int_field(wall_json, "id", 0);
        wall.feature = string_field(wall_json, "feature", "");
        const Json& edge_items = wall_json.contains("edge_path") && wall_json["edge_path"].is_array() ? wall_json["edge_path"] : empty_array;
        for (const Json& edge_json : edge_items) {
            if (!edge_json.is_object()) {
                continue;
            }
            wall.edge_path.push_back({
                edge_json.contains("a") && edge_json["a"].is_object() ? coord_from_json(edge_json["a"]) : Coord{0, 0},
                edge_json.contains("b") && edge_json["b"].is_object() ? coord_from_json(edge_json["b"]) : Coord{0, 0}
            });
        }
        state.walls.push_back(std::move(wall));
    }

    const Json& gate_items = root.contains("wall_gates") && root["wall_gates"].is_array() ? root["wall_gates"] : empty_array;
    for (const Json& gate_json : gate_items) {
        if (!gate_json.is_object()) {
            continue;
        }
        WallGate gate;
        gate.id = int_field(gate_json, "id", 0);
        gate.kind = string_field(gate_json, "kind", "");
        const Json& edge_json = gate_json.contains("edge") && gate_json["edge"].is_object() ? gate_json["edge"] : empty_object;
        gate.edge = {
            edge_json.contains("a") && edge_json["a"].is_object() ? coord_from_json(edge_json["a"]) : Coord{0, 0},
            edge_json.contains("b") && edge_json["b"].is_object() ? coord_from_json(edge_json["b"]) : Coord{0, 0}
        };
        state.wall_gates.push_back(gate);
    }

    const Json& unit_items = root.contains("units") && root["units"].is_array() ? root["units"] : empty_array;
    for (const Json& unit_json : unit_items) {
        if (!unit_json.is_object()) {
            continue;
        }
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
        unit.stance = unit_stance_from_string(string_field(unit_json, "stance", unit_stance_to_string(default_stance(unit.kind))));
        if (!legal_stance(unit.kind, unit.stance)) {
            unit.stance = default_stance(unit.kind);
        }
        const UnitDefaults defaults = unit_defaults(unit.kind);
        unit.coord = coord_from_json(unit_json);
        unit.hp = int_field(unit_json, "hp", defaults.hp);
        unit.max_hp = int_field(unit_json, "maxHp", unit.hp);
        unit.attack = std::max(0, int_field(unit_json, "attack", defaults.attack));
        unit.defense = std::max(1, int_field(unit_json, "defense", defaults.defense));
        unit.readiness_damage = std::max(0, int_field(unit_json, "readinessDamage", defaults.readiness_damage));
        unit.max_readiness = std::max(1, int_field(unit_json, "maxReadiness", defaults.readiness));
        unit.readiness = std::max(0, std::min(int_field(unit_json, "readiness", unit.max_readiness), unit.max_readiness));
        const int default_scaled_move = to_scaled_move(defaults.move);
        unit.scaled_move = int_field(
            unit_json,
            "scaledMove",
            to_scaled_move(number_field(unit_json, "refMove", number_field(unit_json, "move", defaults.move)))
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
        unit.population = std::max(0, int_field(unit_json, "population", defaults.population));
        unit.horses = std::max(0, int_field(unit_json, "horses", defaults.horses));
        unit.starvation_turns = std::max(0, int_field(unit_json, "starvationTurns", 0));
        unit.production_active = string_field(unit_json, "productionState", "idle") == "building";
        unit.production_kind = unit_kind_from_string(string_field(unit_json, "productionKind", unit_kind_to_string(UnitKind::HorseArcher)));
        if (unit.production_kind != UnitKind::HorseArcher && unit.production_kind != UnitKind::MongolLancer) {
            unit.production_kind = UnitKind::HorseArcher;
        }
        unit.production_turns_remaining = std::max(0, int_field(unit_json, "productionTurnsRemaining", unit.production_active ? horde_unit_training_turns : 0));
        if (unit.kind != UnitKind::Horde || !unit.production_active) {
            unit.production_active = false;
            unit.production_turns_remaining = 0;
        }
        unit.move_done = bool_field(unit_json, "moveDone", false);
        unit.moved_this_turn = bool_field(unit_json, "movedThisTurn", false);
        unit.combat_done = bool_field(unit_json, "combatDone", false);
        unit.contacted_enemy_this_turn = bool_field(unit_json, "contactedEnemyThisTurn", false);
        unit.projects_zoc = bool_field(unit_json, "projectsZoc", defaults.projects_zoc);
        unit.respects_zoc = bool_field(unit_json, "respectsZoc", defaults.respects_zoc);
        state.units.push_back(unit);
    }

    if (state.width <= 0 || state.height <= 0 || state.hexes.empty()) {
        throw std::runtime_error("game state JSON is missing width, height, or hexes");
    }
    refresh_legal_actions(state);
    return state;
}

} // namespace steppe::game
