#include "game_state.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <cstdlib>
#include <fstream>
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
        case UnitKind::ChineseCavalry: return "chinese_cavalry";
        case UnitKind::MongolLancer: return "mongol_lancer";
        case UnitKind::ChineseMilitia: return "chinese_militia";
        case UnitKind::Infantry: return "infantry";
        case UnitKind::Horde: return "horde";
    }
    return "horse_archer";
}

bool try_unit_kind_from_string(const std::string& kind, UnitKind& out) {
    if (kind == "camp") {
        out = UnitKind::Camp;
        return true;
    }
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

struct UnitTypeTable {
    std::vector<UnitKind> order;
    std::map<UnitKind, UnitDefaults> defaults;
};

std::vector<UnitKind> required_unit_kinds() {
    return {
        UnitKind::Camp,
        UnitKind::Herd,
        UnitKind::HorseArcher,
        UnitKind::ChineseCavalry,
        UnitKind::MongolLancer,
        UnitKind::ChineseMilitia,
        UnitKind::Infantry,
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
    if (cells.size() != 14) {
        throw std::runtime_error("unit_types.csv line " + std::to_string(line_number)
            + " must have 14 columns");
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
    defaults.metal = parse_int_field(cells[12], "metal", line_number);
    defaults.horses = parse_int_field(cells[13], "horses", line_number);

    if (defaults.hp < 1 || defaults.defense < 1 || defaults.attack < 0
        || defaults.readiness_damage < 0 || defaults.readiness < 1 || defaults.move < 0
        || defaults.population < 0 || defaults.metal < 0 || defaults.horses < 0) {
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
        "metal",
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
constexpr int minimum_combat_readiness_percent = 25;
constexpr int full_move_readiness_cost = 8;
constexpr int max_move_readiness_cost = 8;
constexpr int attack_readiness_cost = 10;
constexpr int turn_readiness_recovery = 25;
constexpr int flanked_defense_percent = 75;
constexpr int feigned_retreat_pursuit_readiness_penalty = 15;

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
        return 5;
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
        unit.metal = defaults.metal;
        unit.horses = defaults.horses;
        unit.projects_zoc = default_projects_zoc(kind);
        unit.respects_zoc = default_respects_zoc(kind);
        return unit;
    };

    state.units = {
        make_unit(1, mongol_owner, UnitKind::HorseArcher, {3, 5}),
        make_unit(2, mongol_owner, UnitKind::Infantry, {3, 7}),
        make_unit(3, mongol_owner, UnitKind::Horde, {3, 6}),
        make_unit(4, mongol_owner, UnitKind::Herd, {2, 6}),
        make_unit(5, chinese_owner, UnitKind::ChineseCavalry, {6, 5}),
        make_unit(6, chinese_owner, UnitKind::Infantry, {8, 7}),
        make_unit(7, chinese_owner, UnitKind::Horde, {8, 6}),
        make_unit(8, chinese_owner, UnitKind::Herd, {9, 6}),
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
        (defense_strength * terrain_defense_percent_at(state, defender.coord) + 99) / 100
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
        return {4, 0, "attacker"};
    }
    if (index == -4) {
        return {3, 0, "attacker"};
    }
    if (index == -3) {
        return {3, 0, "attacker"};
    }
    if (index == -2) {
        return {2, 0, "attacker"};
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
        return {0, 2, "defender"};
    }
    if (index == 3) {
        return {0, 3, "defender"};
    }
    if (index == 4) {
        return {0, 3, "defender"};
    }
    return {0, 4, "defender"};
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
    return hex != nullptr && movement_cost(state, attacker.coord, *hex) != blocked_movement_cost();
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
        if (hex == nullptr || movement_cost(state, defender.coord, *hex) == blocked_movement_cost()) {
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
            const int step_cost = movement_cost(state, current, *hex);
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
    preview.terrain_defense_percent = terrain_defense_percent_at(state, unit.coord);
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
    Coord retreat_to;
    if (defender->kind == UnitKind::HorseArcher
        && defender->stance == UnitStance::FeignedRetreat
        && attacker->kind != UnitKind::HorseArcher
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
        preview.attacker.readiness_damage_taken = feigned_retreat_pursuit_readiness_penalty;
        preview.attacker.result_readiness = std::max(
            0,
            clamped_readiness(*attacker) - feigned_retreat_pursuit_readiness_penalty
        );
        return preview;
    }
    const CrtOutcome outcome = crt_outcome(preview.crt_index);
    preview.retreat_option = outcome.retreat_option;
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

    if (preview.special_resolution == "feigned_retreat") {
        attacker->contacted_enemy_this_turn = true;
        defender->contacted_enemy_this_turn = true;
        defender->coord = preview.defender_retreat_to;
        defender->remaining_scaled_move = 0;
        defender->move_done = true;
        defender->moved_this_turn = true;
        attacker->coord = preview.attacker_pursuit_to;
        attacker->readiness = preview.attacker.result_readiness;
        attacker->remaining_scaled_move = 0;
        attacker->move_done = true;
        attacker->combat_done = true;
        mark_enemy_contact(state, attacker_id);
        mark_enemy_contact(state, defender_id);
        state.selected_unit_id = attacker_id;
        refresh_legal_actions(state);
        return true;
    }

    defender->hp = preview.defender.result_hp;
    attacker->hp = preview.attacker.result_hp;
    defender->readiness = preview.defender.result_readiness;
    attacker->readiness = preview.attacker.result_readiness;
    attacker->contacted_enemy_this_turn = true;
    defender->contacted_enemy_this_turn = true;
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
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)
        || horde->population < options.population_cost
        || horde->metal < options.metal_cost
        || horde->horses < options.horses_cost) {
        return options;
    }

    options.deployable_hexes = adjacent_deployable_hexes(state, *horde);
    return options;
}

bool create_unit_from_horde(GameState& state, int unit_id, UnitKind kind, Coord destination) {
    CreateUnitOptions options = create_unit_options(state, unit_id, kind);
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
    horde->population -= options.population_cost;
    horde->metal -= options.metal_cost;
    horde->horses -= options.horses_cost;
    horde->remaining_scaled_move = 0;
    horde->move_done = true;

    Unit created;
    created.id = next_unit_id(state);
    created.owner = horde->owner;
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
    const int horde_id = horde->id;
    const int created_id = created.id;
    state.units.push_back(created);
    mark_enemy_contact(state, horde_id);
    mark_enemy_contact(state, created_id);
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
            << ",\"metal\":" << unit.metal
            << ",\"horses\":" << unit.horses;
    } else if (unit.kind == UnitKind::Herd) {
        out << ",\"horses\":" << unit.horses;
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
        << ",\"pursuitReadinessPenalty\":" << preview.pursuit_readiness_penalty
        << ",\"defenderRetreatTo\":";
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
        unit.stance = unit_stance_from_string(string_field(unit_json, "stance", unit_stance_to_string(default_stance(unit.kind))));
        if (!legal_stance(unit.kind, unit.stance)) {
            unit.stance = default_stance(unit.kind);
        }
        const UnitDefaults defaults = unit_defaults(unit.kind);
        unit.coord = {int_field(unit_json, "q", 0), int_field(unit_json, "r", 0)};
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
        unit.metal = std::max(0, int_field(unit_json, "metal", defaults.metal));
        unit.horses = std::max(0, int_field(unit_json, "horses", defaults.horses));
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
