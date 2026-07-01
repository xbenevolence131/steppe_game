#include "game_state.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <deque>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>
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

struct GameTime {
    int year = 1180;
    int week_of_year = 1;
    int month = 1;
    int week_of_month = 1;
    const char* month_name = "jan";
    const char* season = "spring";
};

GameTime game_time_for_round(int round) {
    constexpr int start_year = 1180;
    constexpr int weeks_per_month = 4;
    constexpr int months_per_year = 12;
    constexpr int weeks_per_year = weeks_per_month * months_per_year;
    constexpr std::array<const char*, months_per_year> month_names = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec",
    };

    const int normalized_round = std::max(1, round);
    const int week_index = normalized_round - 1;
    const int week_of_year = (week_index % weeks_per_year) + 1;
    const int month = ((week_of_year - 1) / weeks_per_month) + 1;

    GameTime time;
    time.year = start_year + (week_index / weeks_per_year);
    time.week_of_year = week_of_year;
    time.month = month;
    time.week_of_month = ((week_of_year - 1) % weeks_per_month) + 1;
    time.month_name = month_names[month - 1];
    if (month == 12 || month <= 2) {
        time.season = "winter";
    } else if (month <= 5) {
        time.season = "spring";
    } else if (month <= 8) {
        time.season = "summer";
    } else {
        time.season = "fall";
    }
    return time;
}

void print_game_time_json(int round, std::ostream& out) {
    const GameTime time = game_time_for_round(round);
    out << "\"time\":{"
        << "\"year\":" << time.year
        << ",\"weekOfYear\":" << time.week_of_year
        << ",\"month\":" << time.month
        << ",\"monthName\":\"" << time.month_name << "\""
        << ",\"weekOfMonth\":" << time.week_of_month
        << ",\"season\":\"" << time.season << "\""
        << "}";
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
        case Terrain::Farmland: return "farmland";
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
    if (terrain == "farmland") return Terrain::Farmland;
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
        case UnitKind::JurchenInfantry: return "jurchen_infantry";
        case UnitKind::JurchenCavalry: return "jurchen_cavalry";
        case UnitKind::ForestWarband: return "forest_warband";
        case UnitKind::ForestRaiders: return "forest_raiders";
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
    if (kind == "jurchen_infantry") {
        out = UnitKind::JurchenInfantry;
        return true;
    }
    if (kind == "jurchen_cavalry") {
        out = UnitKind::JurchenCavalry;
        return true;
    }
    if (kind == "forest_warband") {
        out = UnitKind::ForestWarband;
        return true;
    }
    if (kind == "forest_raiders") {
        out = UnitKind::ForestRaiders;
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
        return {mongol_owner, "mongol", "Mongol", "steppe_nomad", "#d6a21a", 4, 0, 20, 0, true, false};
    }
    if (owner == chinese_owner) {
        return {chinese_owner, "chinese", "Chinese", "chinese", "#c93632", 4, 0, 20, 0, true, false};
    }
    if (owner == persian_owner) {
        return {persian_owner, "persian", "Persian", "persian", "#1f4fa3", 4, 0, 20, 0, true, false};
    }
    if (owner == jurchen_owner) {
        return {jurchen_owner, "jurchen", "Jurchen", "jurchen", "#1f6f68", 4, 0, 20, 0, true, true};
    }
    if (owner == forest_nomad_owner) {
        return {forest_nomad_owner, "forest_nomad", "Forest Nomads", "forest_nomad", "#2f6b35", 2, 0, 20, 0, true, true};
    }
    return {neutral_owner, "neutral", "Neutral", "neutral", "#777777", 0, 0, 0, 0, false, false};
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
    if (key == "jurchen") {
        return jurchen_owner;
    }
    if (key == "forest_nomad") {
        return forest_nomad_owner;
    }
    if (key == "neutral") {
        return neutral_owner;
    }
    return fallback;
}

const char* ai_directive_kind_to_string(AiDirectiveKind kind) {
    switch (kind) {
        case AiDirectiveKind::Inactive: return "inactive";
        case AiDirectiveKind::HoldHex: return "hold_hex";
        case AiDirectiveKind::DefendHex: return "defend_hex";
        case AiDirectiveKind::HuntHorde: return "hunt_horde";
        case AiDirectiveKind::CaptureHex: return "capture_hex";
        case AiDirectiveKind::Hunt: return "hunt";
    }
    return "hunt";
}

AiDirectiveKind ai_directive_kind_from_string(const std::string& value) {
    if (value == "inactive") {
        return AiDirectiveKind::Inactive;
    }
    if (value == "hold_hex") {
        return AiDirectiveKind::HoldHex;
    }
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

int clamp_disposition(int disposition) {
    return std::max(0, std::min(100, disposition));
}

int clamp_fear(int fear) {
    return std::max(0, std::min(100, fear));
}

bool hostile_disposition(int disposition) {
    return clamp_disposition(disposition) <= 35;
}

bool hostile_relationship(int disposition, int fear) {
    return hostile_disposition(disposition) && clamp_fear(fear) < 75;
}

bool raiding_disposition(int disposition) {
    return clamp_disposition(disposition) <= 15;
}

bool raiding_relationship(int disposition, int fear) {
    return raiding_disposition(disposition) && clamp_fear(fear) <= 45;
}

bool field_army_disposition(int disposition) {
    return clamp_disposition(disposition) <= 30;
}

bool field_army_relationship(int disposition, int fear) {
    return field_army_disposition(disposition) && clamp_fear(fear) < 75;
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
        UnitKind::JurchenInfantry,
        UnitKind::JurchenCavalry,
        UnitKind::ForestWarband,
        UnitKind::ForestRaiders,
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
    defaults.horses = parse_int_field(cells[12], "horses", line_number);
    defaults.livestock = parse_int_field(cells[13], "livestock", line_number);

    if (defaults.hp < 1 || defaults.defense < 1 || defaults.attack < 0
        || defaults.readiness_damage < 0 || defaults.readiness < 1 || defaults.move < 0
        || defaults.population < 0 || defaults.horses < 0 || defaults.livestock < 0) {
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
        "livestock",
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
constexpr int full_move_readiness_cost = 5;
constexpr int max_move_readiness_cost = 5;
constexpr int attack_readiness_cost = 10;
constexpr int turn_readiness_recovery = 15;
constexpr int food_consumption_per_population = 1;
constexpr int farmland_food_per_round = 2;
constexpr int faction_hunger_readiness_threshold = 3;
constexpr int faction_hunger_starvation_threshold = 10;
constexpr int faction_hunger_starvation_reset = 7;
constexpr int faction_hunger_readiness_penalty = 10;
constexpr int flanked_defense_percent = 75;
constexpr int defender_retreat_condition_threshold = 50;
constexpr int feigned_retreat_pursuit_readiness_penalty = 15;
constexpr int feigned_retreat_defender_hp_damage = 1;
constexpr int feigned_retreat_defender_readiness_penalty = 10;
constexpr int blocked_retreat_readiness_penalty = 15;
constexpr int unbridged_river_crossing_readiness_cost = 15;
constexpr double pasture_capacity_grassland = 100.0;
constexpr double pasture_consumption_per_horse = 15.0;
constexpr double pasture_consumption_per_livestock = 5.0;
constexpr double pasture_recovery_per_round = pasture_capacity_grassland / 16.0;
constexpr int pasture_grazing_radius = 2;
constexpr int horde_horse_capacity = 20;
constexpr int livestock_unit_capacity = 100;
constexpr int livestock_butcher_food_per_head = 10;
constexpr int starvation_horse_loss_turn_threshold = 3;
constexpr int starvation_animal_loss_percent = 25;

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
        case Terrain::Farmland:
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

RiverEdge canonical_edge(const Coord& first, const Coord& second) {
    return coord_less(second, first) ? RiverEdge{second, first} : RiverEdge{first, second};
}

bool edge_equal(const RiverEdge& first, const RiverEdge& second) {
    return coord_equal(first.a, second.a) && coord_equal(first.b, second.b);
}

bool edge_less(const RiverEdge& first, const RiverEdge& second) {
    if (!coord_equal(first.a, second.a)) {
        return coord_less(first.a, second.a);
    }
    return coord_less(first.b, second.b);
}

bool river_edge_between(const GameState& state, const Coord& first, const Coord& second) {
    const RiverEdge edge = canonical_edge(first, second);
    return std::find_if(state.rivers.begin(), state.rivers.end(), [&](const RiverEdge& river) {
        return edge_equal(canonical_edge(river.a, river.b), edge);
    }) != state.rivers.end();
}

bool bridge_crossing_between(const GameState& state, const Coord& first, const Coord& second) {
    const RiverEdge edge = canonical_edge(first, second);
    return std::find_if(state.crossings.begin(), state.crossings.end(), [&](const Crossing& crossing) {
        return crossing.kind == "bridge" && edge_equal(canonical_edge(crossing.edge.a, crossing.edge.b), edge);
    }) != state.crossings.end();
}

bool unbridged_river_crossing_between(const GameState& state, const Coord& first, const Coord& second) {
    return river_edge_between(state, first, second) && !bridge_crossing_between(state, first, second);
}

bool uses_mounted_road_cost(UnitKind kind) {
    return kind == UnitKind::HorseArcher
        || kind == UnitKind::ChineseCavalry
        || kind == UnitKind::MongolLancer
        || kind == UnitKind::PersianCavalry
        || kind == UnitKind::JurchenCavalry
        || kind == UnitKind::ForestRaiders;
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
    if (unbridged_river_crossing_between(state, from, to_hex.coord)) {
        return blocked_movement_cost();
    }
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

std::string decimal_2(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
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

AiDirective ai_directive_from_json(const Json& directive_json) {
    AiDirective directive;
    directive.kind = ai_directive_kind_from_string(string_field(directive_json, "type", "hunt"));
    if (directive_json.contains("target") && directive_json["target"].is_object()) {
        directive.target = coord_from_json(directive_json["target"]);
    }
    directive.target_owner = int_field(
        directive_json,
        "owner",
        owner_from_faction_key(string_field(directive_json, "faction", ""), neutral_owner)
    );
    return directive;
}

std::string normalize_ai_group_role(const std::string& role) {
    if (role == "garrison"
        || role == "field_army"
        || role == "raiding_force"
        || role == "reserve"
        || role == "manual_directive") {
        return role;
    }
    return "manual_directive";
}

std::vector<AiGroup> ai_groups_from_json(const Json& ai_group_items) {
    std::vector<AiGroup> groups;
    if (!ai_group_items.is_array()) {
        return groups;
    }
    for (const Json& group_json : ai_group_items) {
        if (!group_json.is_object()) {
            continue;
        }
        AiGroup group;
        group.id = int_field(group_json, "id", 0);
        group.owner = int_field(group_json, "owner", neutral_owner);
        group.name = string_field(group_json, "name", "");
        group.role = normalize_ai_group_role(string_field(group_json, "role", "manual_directive"));
        group.generated = bool_field(group_json, "generated", false);
        group.unit_ids = int_array_field(group_json, "unitIds");
        const Json empty_object = Json::object();
        const Json& directive_json = group_json.contains("directive") && group_json["directive"].is_object()
            ? group_json["directive"]
            : empty_object;
        group.directive = ai_directive_from_json(directive_json);
        groups.push_back(std::move(group));
    }
    return groups;
}

int diplomacy_key(OwnerId owner, OwnerId target) {
    return owner * 1000 + target;
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
    return unit_kind_available_to_archetype(kind, faction_for_owner(owner).archetype);
}

bool unit_kind_available_to_archetype(UnitKind kind, const std::string& archetype) {
    if (archetype == "steppe_nomad") {
        return kind == UnitKind::HorseArcher
            || kind == UnitKind::MongolLancer
            || kind == UnitKind::Herd
            || kind == UnitKind::Horde;
    }
    if (archetype == "chinese") {
        return kind == UnitKind::ChineseMilitia
            || kind == UnitKind::Infantry
            || kind == UnitKind::ChineseCavalry;
    }
    if (archetype == "persian") {
        return kind == UnitKind::PersianInfantry
            || kind == UnitKind::PersianCavalry;
    }
    if (archetype == "jurchen") {
        return kind == UnitKind::JurchenInfantry
            || kind == UnitKind::JurchenCavalry;
    }
    if (archetype == "forest_nomad") {
        return kind == UnitKind::ForestWarband
            || kind == UnitKind::ForestRaiders
            || kind == UnitKind::Herd;
    }
    return false;
}

void normalize_diplomacy(GameState& state) {
    std::map<int, DiplomaticRelationship> existing_relationships;
    for (const DiplomaticRelationship& relationship : state.diplomacy) {
        if (relationship.owner == neutral_owner || relationship.target == neutral_owner || relationship.owner == relationship.target) {
            continue;
        }
        DiplomaticRelationship normalized = relationship;
        normalized.disposition = clamp_disposition(normalized.disposition);
        normalized.fear = clamp_fear(normalized.fear);
        existing_relationships[diplomacy_key(relationship.owner, relationship.target)] = normalized;
    }

    std::vector<OwnerId> owners;
    for (OwnerId owner : state.turn_order) {
        const FactionState* faction = nullptr;
        const auto found = std::find_if(state.factions.begin(), state.factions.end(), [&](const FactionState& candidate) {
            return candidate.id == owner;
        });
        if (found != state.factions.end()) {
            faction = &*found;
        }
        if (owner == neutral_owner || (faction != nullptr && !faction->enabled)) {
            continue;
        }
        if (std::find(owners.begin(), owners.end(), owner) == owners.end()) {
            owners.push_back(owner);
        }
    }
    if (owners.empty()) {
        for (const FactionState& faction : state.factions) {
            if (faction.id != neutral_owner && faction.enabled) {
                owners.push_back(faction.id);
            }
        }
    }

    std::vector<DiplomaticRelationship> normalized;
    for (OwnerId owner : owners) {
        for (OwnerId target : owners) {
            if (owner == target) {
                continue;
            }
            const int key = diplomacy_key(owner, target);
            const auto found = existing_relationships.find(key);
            const auto reverse_found = existing_relationships.find(diplomacy_key(target, owner));
            DiplomaticRelationship relationship;
            relationship.owner = owner;
            relationship.target = target;
            relationship.disposition = found == existing_relationships.end() ? 25 : found->second.disposition;
            relationship.fear = found == existing_relationships.end() ? 25 : found->second.fear;
            if (found != existing_relationships.end()) {
                relationship.at_war = found->second.at_war;
            } else if (reverse_found != existing_relationships.end()) {
                relationship.at_war = reverse_found->second.at_war;
            } else {
                relationship.at_war = true;
            }
            normalized.push_back(relationship);
        }
    }
    state.diplomacy = std::move(normalized);
}

int disposition_toward(const GameState& state, OwnerId owner, OwnerId target) {
    if (owner == target || owner == neutral_owner || target == neutral_owner) {
        return 50;
    }
    const auto found = std::find_if(state.diplomacy.begin(), state.diplomacy.end(), [&](const DiplomaticRelationship& relationship) {
        return relationship.owner == owner && relationship.target == target;
    });
    return found == state.diplomacy.end() ? 25 : clamp_disposition(found->disposition);
}

int fear_toward(const GameState& state, OwnerId owner, OwnerId target) {
    if (owner == target || owner == neutral_owner || target == neutral_owner) {
        return 0;
    }
    const auto found = std::find_if(state.diplomacy.begin(), state.diplomacy.end(), [&](const DiplomaticRelationship& relationship) {
        return relationship.owner == owner && relationship.target == target;
    });
    return found == state.diplomacy.end() ? 25 : clamp_fear(found->fear);
}

bool relationship_at_war_toward(const GameState& state, OwnerId owner, OwnerId target) {
    if (owner == target || owner == neutral_owner || target == neutral_owner) {
        return false;
    }
    const auto found = std::find_if(state.diplomacy.begin(), state.diplomacy.end(), [&](const DiplomaticRelationship& relationship) {
        return relationship.owner == owner && relationship.target == target;
    });
    return found == state.diplomacy.end() ? true : found->at_war;
}

bool at_war(const GameState& state, OwnerId first, OwnerId second) {
    if (first == second || first == neutral_owner || second == neutral_owner) {
        return false;
    }
    return relationship_at_war_toward(state, first, second)
        || relationship_at_war_toward(state, second, first);
}

bool hostile_units(const GameState& state, const Unit& first, const Unit& second) {
    return first.owner != second.owner && at_war(state, first.owner, second.owner);
}

bool settled_owner(OwnerId owner) {
    return owner == chinese_owner || owner == persian_owner || owner == jurchen_owner;
}

bool settled_unit(const Unit& unit) {
    return settled_owner(unit.owner);
}

bool territory_claimable_terrain(Terrain terrain) {
    switch (terrain) {
        case Terrain::Grassland:
        case Terrain::Farmland:
        case Terrain::Desert:
        case Terrain::Hill:
        case Terrain::Forest:
        case Terrain::Marsh:
        case Terrain::Urban:
            return true;
        case Terrain::None:
        case Terrain::Lake:
        case Terrain::Mountain:
            return false;
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

PastureState initial_pasture_for_terrain(Terrain terrain) {
    switch (terrain) {
        case Terrain::Grassland:
            return {pasture_capacity_grassland, pasture_capacity_grassland};
        default:
            return {};
    }
}

void seed_initial_territory(GameState& state);

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
        game_hex.name = "None";
        game_hex.tags = tags_from_labels(hex.labels);
        game_hex.pasture = initial_pasture_for_terrain(hex.terrain);
        game_hex.source_labels = hex.labels;
        if (has_tag(game_hex.tags, HexTag::PersianTown)) {
            game_hex.owner = persian_owner;
        } else if (has_tag(game_hex.tags, HexTag::ChineseTown)) {
            game_hex.owner = chinese_owner;
        }
        game_hex.supply_source = settled_owner(game_hex.owner) && has_tag(game_hex.tags, HexTag::FixedFeatureTown);
        state.hexes.push_back(std::move(game_hex));
    }

    int next_settlement_id = 1;
    for (const Town& town : generated.towns) {
        Settlement settlement;
        settlement.id = next_settlement_id++;
        settlement.coord = town.coord;
        settlement.kind = settlement_kind_from_town(town);
        settlement.source_feature = town.feature;
        settlement.source_labels = town.labels;
        state.settlements.push_back(std::move(settlement));
    }
    seed_initial_territory(state);

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
        faction_for_owner(jurchen_owner),
        faction_for_owner(forest_nomad_owner),
    };
    state.turn_order = {mongol_owner, chinese_owner};
    normalize_diplomacy(state);

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
        faction_for_owner(jurchen_owner),
        faction_for_owner(forest_nomad_owner),
    };

    const OwnerId default_order[] = {mongol_owner, chinese_owner, jurchen_owner, forest_nomad_owner, persian_owner};
    faction_count = std::max(1, std::min(faction_count, 5));
    state.turn_order.assign(default_order, default_order + faction_count);
    normalize_diplomacy(state);

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
        unit.livestock = defaults.livestock;
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

OwnerId territory_owner_for_unit(const Unit& unit) {
    return settled_unit(unit) ? unit.owner : neutral_owner;
}

void apply_territory_ownership(GameState& state, const Unit& unit, const Coord& coord) {
    GameHex* hex = find_hex(state, coord);
    if (hex == nullptr || !territory_claimable_terrain(hex->terrain)) {
        return;
    }
    hex->owner = territory_owner_for_unit(unit);
}

void apply_territory_ownership(GameState& state, const Unit& unit, const std::vector<Coord>& path) {
    for (const Coord& coord : path) {
        apply_territory_ownership(state, unit, coord);
    }
}

std::set<Coord, decltype(coord_less)*> supplied_hex_coords(const GameState& state) {
    std::set<Coord, decltype(coord_less)*> supplied(coord_less);
    std::set<Coord, decltype(coord_less)*> visited(coord_less);

    for (const GameHex& start : state.hexes) {
        if (visited.find(start.coord) != visited.end()
            || start.owner == neutral_owner
            || !territory_claimable_terrain(start.terrain)) {
            continue;
        }

        std::vector<Coord> component;
        std::deque<Coord> queue;
        bool has_supply_source = false;
        visited.insert(start.coord);
        queue.push_back(start.coord);

        while (!queue.empty()) {
            const Coord current = queue.front();
            queue.pop_front();
            const GameHex* hex = find_hex(state, current);
            if (hex == nullptr) {
                continue;
            }
            component.push_back(current);
            has_supply_source = has_supply_source || hex->supply_source;

            for (int direction = 0; direction < 6; ++direction) {
                const Coord next = neighbor_in_direction(current, direction);
                if (!in_bounds(state, next) || visited.find(next) != visited.end()) {
                    continue;
                }
                const GameHex* neighbor = find_hex(state, next);
                if (neighbor == nullptr
                    || neighbor->owner != start.owner
                    || !territory_claimable_terrain(neighbor->terrain)) {
                    continue;
                }
                visited.insert(next);
                queue.push_back(next);
            }
        }

        if (has_supply_source) {
            supplied.insert(component.begin(), component.end());
        }
    }

    return supplied;
}

bool wall_side_blocked_hex(const GameHex& hex) {
    return hex.terrain == Terrain::Mountain || hex.terrain == Terrain::Lake;
}

std::set<RiverEdge, decltype(edge_less)*> great_wall_edge_set(const GameState& state) {
    std::set<RiverEdge, decltype(edge_less)*> wall_edges(edge_less);
    for (const Wall& wall : state.walls) {
        if (!wall.feature.empty() && wall.feature != "great_wall") {
            continue;
        }
        for (const RiverEdge& edge : wall.edge_path) {
            wall_edges.insert(canonical_edge(edge.a, edge.b));
        }
    }
    return wall_edges;
}

std::set<Coord, decltype(coord_less)*> great_wall_outside_hexes(const GameState& state) {
    std::set<Coord, decltype(coord_less)*> outside(coord_less);
    const auto wall_edges = great_wall_edge_set(state);
    if (wall_edges.empty()) {
        return outside;
    }

    int max_left_seed_row = state.height;
    bool wall_touches_left_edge = false;
    for (const RiverEdge& edge : wall_edges) {
        const auto consider = [&](const Coord& coord) {
            if (coord.q != 1) {
                return;
            }
            wall_touches_left_edge = true;
            max_left_seed_row = std::min(max_left_seed_row, coord.r);
        };
        consider(edge.a);
        consider(edge.b);
    }
    if (!wall_touches_left_edge) {
        max_left_seed_row = state.height;
    }

    std::deque<Coord> frontier;
    const auto add = [&](const Coord& coord) {
        if (!in_bounds(state, coord) || outside.find(coord) != outside.end()) {
            return;
        }
        const GameHex* hex = find_hex(state, coord);
        if (hex == nullptr || wall_side_blocked_hex(*hex)) {
            return;
        }
        outside.insert(coord);
        frontier.push_back(coord);
    };

    for (int q = 1; q <= state.width; ++q) {
        add({q, 1});
    }
    for (int r = 1; r <= max_left_seed_row; ++r) {
        add({1, r});
    }

    while (!frontier.empty()) {
        const Coord current = frontier.front();
        frontier.pop_front();
        for (int direction = 0; direction < 6; ++direction) {
            const Coord next = neighbor_in_direction(current, direction);
            if (!in_bounds(state, next) || outside.find(next) != outside.end()) {
                continue;
            }
            if (wall_edges.find(canonical_edge(current, next)) != wall_edges.end()) {
                continue;
            }
            const GameHex* hex = find_hex(state, next);
            if (hex == nullptr || wall_side_blocked_hex(*hex)) {
                continue;
            }
            outside.insert(next);
            frontier.push_back(next);
        }
    }
    return outside;
}

const Unit* unit_at(const GameState& state, const Coord& coord);
bool enemy_zoc_at(const GameState& state, const Coord& coord, const Unit& moving_unit);

OwnerId settlement_hex_owner(const GameState& state, const Settlement& settlement) {
    const GameHex* hex = find_hex(state, settlement.coord);
    return hex != nullptr && territory_claimable_terrain(hex->terrain) ? hex->owner : neutral_owner;
}

void seed_initial_territory(GameState& state) {
    constexpr int initial_territory_radius = 12;

    struct Frontier {
        Coord coord;
        OwnerId owner = neutral_owner;
        int distance = 0;
    };

    std::vector<Frontier> sources;
    for (const Settlement& settlement : state.settlements) {
        const OwnerId owner = settlement_hex_owner(state, settlement);
        if (!settled_owner(owner)) {
            continue;
        }
        sources.push_back({settlement.coord, owner, 0});
    }
    std::sort(sources.begin(), sources.end(), [](const Frontier& first, const Frontier& second) {
        if (first.owner != second.owner) {
            return first.owner < second.owner;
        }
        return coord_less(first.coord, second.coord);
    });

    std::map<Coord, int, decltype(coord_less)*> best_distances(coord_less);
    std::map<Coord, OwnerId, decltype(coord_less)*> owners(coord_less);
    std::deque<Frontier> queue;
    for (const Frontier& source : sources) {
        const auto existing = best_distances.find(source.coord);
        if (existing != best_distances.end() && existing->second == 0 && owners[source.coord] <= source.owner) {
            continue;
        }
        best_distances[source.coord] = 0;
        owners[source.coord] = source.owner;
        queue.push_back({source.coord, source.owner, 0});
    }

    while (!queue.empty()) {
        const Frontier current = queue.front();
        queue.pop_front();
        const auto current_owner = owners.find(current.coord);
        const auto current_distance = best_distances.find(current.coord);
        if (current_owner == owners.end()
            || current_distance == best_distances.end()
            || current_owner->second != current.owner
            || current_distance->second != current.distance) {
            continue;
        }
        if (current.distance >= initial_territory_radius) {
            continue;
        }

        for (int direction = 0; direction < 6; ++direction) {
            const Coord next = neighbor_in_direction(current.coord, direction);
            if (!in_bounds(state, next)) {
                continue;
            }
            const GameHex* hex = find_hex(state, next);
            if (hex == nullptr || !territory_claimable_terrain(hex->terrain)) {
                continue;
            }
            const int next_distance = current.distance + 1;
            const auto existing_distance = best_distances.find(next);
            const auto existing_owner = owners.find(next);
            if (existing_distance != best_distances.end()
                && (existing_distance->second < next_distance
                    || (existing_distance->second == next_distance
                        && existing_owner != owners.end()
                        && existing_owner->second <= current.owner))) {
                continue;
            }
            best_distances[next] = next_distance;
            owners[next] = current.owner;
            queue.push_back({next, current.owner, next_distance});
        }
    }

    for (GameHex& hex : state.hexes) {
        const auto found = owners.find(hex.coord);
        if (found != owners.end()) {
            hex.owner = found->second;
        }
    }
}

std::vector<Coord> movement_path_to(const GameState& state, const Unit& unit, Coord destination) {
    std::map<Coord, int, decltype(coord_less)*> best_costs(coord_less);
    std::map<Coord, Coord, decltype(coord_less)*> previous(coord_less);
    std::deque<Coord> queue;
    best_costs[unit.coord] = 0;
    queue.push_back(unit.coord);

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
            const bool occupied_by_other = occupant != nullptr && occupant->id != unit.id;
            if (occupied_by_other && hostile_units(state, unit, *occupant)) {
                continue;
            }
            const GameHex* hex = find_hex(state, next);
            if (hex == nullptr) {
                continue;
            }
            const bool unbridged_river_crossing = unbridged_river_crossing_between(state, current, next);
            const bool first_step = current_cost == 0 && coord_equal(current, unit.coord);
            if (unbridged_river_crossing
                && (!first_step || unit.moved_this_turn || unit.remaining_scaled_move != unit.scaled_move)) {
                continue;
            }
            int step_cost = movement_cost(state, unit, current, *hex);
            if (unbridged_river_crossing && terrain_movement_cost(hex->terrain) != blocked_movement_cost()) {
                step_cost = unit.remaining_scaled_move;
            }
            if (step_cost == blocked_movement_cost()) {
                continue;
            }
            const int next_cost = current_cost + step_cost;
            const bool affordable = next_cost <= unit.remaining_scaled_move;
            const bool minimum_step = first_step && !unit.moved_this_turn;
            if (!affordable && !minimum_step) {
                continue;
            }
            const auto existing = best_costs.find(next);
            if (existing != best_costs.end() && existing->second <= next_cost) {
                continue;
            }
            best_costs[next] = next_cost;
            previous[next] = current;
            if (!unbridged_river_crossing
                && affordable
                && (!unit.respects_zoc || !enemy_zoc_at(state, next, unit))) {
                queue.push_back(next);
            }
        }
    }

    if (best_costs.find(destination) == best_costs.end()) {
        return {};
    }
    std::vector<Coord> path;
    Coord cursor = destination;
    path.push_back(cursor);
    while (!coord_equal(cursor, unit.coord)) {
        const auto found = previous.find(cursor);
        if (found == previous.end()) {
            return {};
        }
        cursor = found->second;
        path.push_back(cursor);
    }
    std::reverse(path.begin(), path.end());
    return path;
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

int faction_population(const GameState& state, OwnerId owner) {
    int population = 0;
    for (const Unit& unit : state.units) {
        if (unit.owner == owner && unit.hp > 0) {
            population += std::max(0, unit.population);
        }
    }
    return population;
}

std::uint32_t mix_starvation_seed(std::uint32_t seed, int round, OwnerId owner, int hunger) {
    std::uint32_t value = seed ^ 0x9e3779b9u;
    value ^= static_cast<std::uint32_t>(round + 0x7f4a7c15u) + (value << 6) + (value >> 2);
    value ^= static_cast<std::uint32_t>(owner * 2654435761u) + (value << 6) + (value >> 2);
    value ^= static_cast<std::uint32_t>(hunger * 2246822519u) + (value << 6) + (value >> 2);
    return value;
}

void reduce_random_horde_population(GameState& state, OwnerId owner, int hunger) {
    std::vector<Unit*> candidates;
    for (Unit& unit : state.units) {
        if (unit.owner == owner && unit.kind == UnitKind::Horde && unit.hp > 0 && unit.population > 0) {
            candidates.push_back(&unit);
        }
    }
    if (candidates.empty()) {
        return;
    }
    std::sort(candidates.begin(), candidates.end(), [](const Unit* first, const Unit* second) {
        return first->id < second->id;
    });
    const std::uint32_t roll = mix_starvation_seed(
        static_cast<std::uint32_t>(state.seed),
        state.round,
        owner,
        hunger
    );
    Unit* selected = candidates[roll % candidates.size()];
    selected->population = std::max(0, selected->population - 1);
}

int faction_farmland_food(const GameState& state, OwnerId owner) {
    int farmland_count = 0;
    for (const GameHex& hex : state.hexes) {
        if (hex.owner == owner && hex.terrain == Terrain::Farmland) {
            farmland_count += 1;
        }
    }
    return farmland_count * farmland_food_per_round;
}

void apply_faction_food_for_round(GameState& state) {
    if (!state.food_consumption_enabled) {
        return;
    }

    for (FactionState& faction : state.factions) {
        if (!faction.enabled || faction.id == neutral_owner) {
            continue;
        }

        faction.food = std::max(0, faction.food) + faction_farmland_food(state, faction.id);

        const int required_food = faction_population(state, faction.id) * food_consumption_per_population;
        if (required_food <= 0) {
            faction.hunger = 0;
            continue;
        }

        const int available_food = faction.food;
        const int consumed_food = std::min(available_food, required_food);
        faction.food = available_food - consumed_food;
        const int food_shortage = required_food - consumed_food;

        if (food_shortage <= 0) {
            faction.hunger = 0;
        } else {
            faction.hunger += food_shortage;
        }

        if (faction.hunger >= faction_hunger_starvation_threshold) {
            reduce_random_horde_population(state, faction.id, faction.hunger);
            faction.hunger = faction_hunger_starvation_reset;
        }

    }
}

void apply_faction_hunger_readiness_effects(GameState& state) {
    for (const FactionState& faction : state.factions) {
        if (!faction.enabled || faction.id == neutral_owner || faction.hunger < faction_hunger_readiness_threshold) {
            continue;
        }
        for (Unit& unit : state.units) {
            if (unit.owner == faction.id && unit.hp > 0) {
                unit.readiness = std::max(0, unit.readiness - faction_hunger_readiness_penalty);
            }
        }
    }
}

bool faction_ai_controlled(const GameState& state, OwnerId owner) {
    const FactionState* faction = find_faction(state, owner);
    return faction != nullptr && faction->enabled && faction->ai_controlled;
}

bool unit_kind_available_to_faction(const GameState& state, UnitKind kind, OwnerId owner) {
    const FactionState* faction = find_faction(state, owner);
    if (faction != nullptr) {
        return unit_kind_available_to_archetype(kind, faction->archetype);
    }
    return unit_kind_available_to_owner(kind, owner);
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
        case Terrain::Farmland:
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

int minimum_retreat_readiness_damage(int base_damage) {
    return base_damage <= 0 ? 0 : std::max(1, (base_damage + 1) / 2);
}

int ordinary_retreat_readiness_cost(int base_damage, int readiness_damage) {
    return std::max(0, minimum_retreat_readiness_damage(base_damage) - readiness_damage);
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
    state.legal_raids.clear();
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
    state.legal_raids = raidable_hexes(state, state.selected_unit_id);
}

bool enemy_zoc_at(const GameState& state, const Coord& coord, const Unit& moving_unit) {
    return std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id != moving_unit.id
            && hostile_units(state, moving_unit, unit)
            && unit.projects_zoc
            && adjacent(unit.coord, coord);
    }) != state.units.end();
}

bool enemy_unit_adjacent_to(const GameState& state, const Unit& source) {
    return std::find_if(state.units.begin(), state.units.end(), [&](const Unit& unit) {
        return unit.id != source.id
            && hostile_units(state, source, unit)
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
        if (unit.id == found->id || !hostile_units(state, *found, unit) || !adjacent(unit.coord, found->coord)) {
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
    return terrain == Terrain::Grassland || terrain == Terrain::Farmland || terrain == Terrain::Desert;
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
        if (river_edge_between(state, retreating.coord, candidate)) {
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

bool ordinary_retreat_blocked_by_river(const GameState& state, const Unit& retreating, const Unit& opposing) {
    const int current_distance = hex_distance(retreating.coord, opposing.coord);
    for (int direction = 0; direction < 6; ++direction) {
        const Coord candidate = neighbor_in_direction(retreating.coord, direction);
        if (!in_bounds(state, candidate) || occupied_by_other_unit(state, candidate, retreating.id)) {
            continue;
        }
        if (!river_edge_between(state, retreating.coord, candidate)) {
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
        if (distance >= current_distance) {
            return true;
        }
    }
    return false;
}

bool find_feigned_retreat_hex(const GameState& state, const Unit& attacker, const Unit& defender, Coord& retreat_to) {
    std::vector<Coord> preferred;
    std::vector<Coord> fallback;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord candidate = neighbor_in_direction(defender.coord, direction);
        if (!in_bounds(state, candidate) || occupied_by_other_unit(state, candidate, defender.id)) {
            continue;
        }
        if (river_edge_between(state, defender.coord, candidate)) {
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
            const bool occupied_by_other = occupant != nullptr && occupant->id != unit->id;
            if (occupied_by_other && hostile_units(state, *unit, *occupant)) {
                continue;
            }
            const bool friendly_occupied = occupied_by_other;
            const GameHex* hex = find_hex(state, next);
            if (hex == nullptr) {
                continue;
            }
            const bool unbridged_river_crossing = unbridged_river_crossing_between(state, current, next);
            const bool first_step = current_cost == 0 && coord_equal(current, unit->coord);
            if (unbridged_river_crossing
                && (!first_step || unit->moved_this_turn || unit->remaining_scaled_move != unit->scaled_move)) {
                continue;
            }
            int step_cost = movement_cost(state, *unit, current, *hex);
            if (unbridged_river_crossing && terrain_movement_cost(hex->terrain) != blocked_movement_cost()) {
                step_cost = unit->remaining_scaled_move;
            }
            if (step_cost == blocked_movement_cost()) {
                continue;
            }
            const int next_cost = current_cost + step_cost;
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
            if (!unbridged_river_crossing
                && affordable
                && (!unit->respects_zoc || !enemy_zoc_at(state, next, *unit))) {
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
            || !hostile_units(state, *attacker, defender)
            || defender.hp <= 0
            || !adjacent(attacker->coord, defender.coord)
            || unbridged_river_crossing_between(state, attacker->coord, defender.coord)) {
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

bool legal_raid_target(const GameState& state, const Unit& unit, const GameHex& hex) {
    if (hex.terrain != Terrain::Farmland
        || hex.owner == unit.owner
        || (hex.owner != neutral_owner && !at_war(state, unit.owner, hex.owner))
        || (!coord_equal(unit.coord, hex.coord) && !adjacent(unit.coord, hex.coord))
        || (adjacent(unit.coord, hex.coord) && unbridged_river_crossing_between(state, unit.coord, hex.coord))) {
        return false;
    }
    const Unit* occupant = unit_at(state, hex.coord);
    return occupant == nullptr || occupant->id == unit.id || !hostile_units(state, unit, *occupant);
}

std::vector<Coord> raidable_hexes(const GameState& state, int unit_id) {
    std::vector<Coord> raids;
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr
        || unit->owner != active_faction(state)
        || unit->combat_done
        || !can_attack(unit->kind)
        || unit->attack <= 0) {
        return raids;
    }

    for (const GameHex& hex : state.hexes) {
        if (legal_raid_target(state, *unit, hex)) {
            raids.push_back(hex.coord);
        }
    }
    std::sort(raids.begin(), raids.end(), coord_less);
    return raids;
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
        && hostile_units(state, *attacker, *defender)
        && defender->hp > 0
        && adjacent(attacker->coord, defender->coord)
        && !unbridged_river_crossing_between(state, attacker->coord, defender->coord);
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
    const int defender_readiness_damage = scaled_damage_for_remaining(
        attacker->readiness_damage,
        clamped_readiness(*defender),
        defender->max_readiness
    );
    const int attacker_readiness_damage = scaled_damage_for_remaining(
        attack_readiness_cost,
        clamped_readiness(*attacker),
        attacker->max_readiness
    );

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
            const int retreat_readiness_cost = ordinary_retreat_readiness_cost(
                attacker->readiness_damage,
                preview.defender.readiness_damage_taken
            );
            preview.defender.readiness_damage_taken += retreat_readiness_cost;
            preview.defender.result_readiness = std::max(
                0,
                preview.defender.result_readiness - retreat_readiness_cost
            );
        } else {
            preview.retreat_blocked = true;
            preview.blocked_retreat_readiness_penalty = blocked_retreat_readiness_penalty;
            preview.retreat_impact = "Defender retreat blocked";
            preview.defender.readiness_damage_taken += blocked_retreat_readiness_penalty;
            preview.defender.result_readiness = std::max(0, preview.defender.result_readiness - blocked_retreat_readiness_penalty);
            if (!ordinary_retreat_blocked_by_river(state, *defender, *attacker)) {
                const int capped_damage = std::min(
                    preview.defender.readiness_damage_taken,
                    std::max(0, clamped_readiness(*defender) / 2)
                );
                preview.defender.readiness_damage_taken = capped_damage;
                preview.defender.result_readiness = std::max(0, clamped_readiness(*defender) - capped_damage);
            }
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
        state.legal_raids.clear();
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
    std::vector<Coord> movement_path = movement_path_to(state, *unit, destination);
    if (movement_path.empty()) {
        movement_path = {unit->coord, destination};
    }
    const bool unbridged_river_crossing = unbridged_river_crossing_between(state, unit->coord, destination);
    cancel_horde_production(*unit);
    apply_territory_ownership(state, *unit, movement_path);
    unit->coord = destination;
    unit->moved_this_turn = true;
    reduce_readiness(
        *unit,
        unbridged_river_crossing
            ? unbridged_river_crossing_readiness_cost
            : move_readiness_cost(*unit, found->scaled_cost)
    );
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
        apply_territory_ownership(state, *attacker, {attacker->coord, preview.attacker_pursuit_to});
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

bool raid_hex(GameState& state, int unit_id, Coord target) {
    Unit* unit = find_unit(state, unit_id);
    GameHex* hex = find_hex(state, target);
    if (unit == nullptr
        || hex == nullptr
        || unit->owner != active_faction(state)
        || unit->combat_done
        || !can_attack(unit->kind)
        || unit->attack <= 0
        || !legal_raid_target(state, *unit, *hex)) {
        return false;
    }

    const OwnerId raider_owner = unit->owner;
    const OwnerId victim_owner = hex->owner;
    FactionState* raider = find_faction(state, raider_owner);
    FactionState* victim = find_faction(state, victim_owner);
    if (raider != nullptr) {
        raider->treasure += 1;
    }
    if (victim != nullptr) {
        victim->food = std::max(0, victim->food - 2);
    }

    hex->terrain = Terrain::Grassland;
    hex->owner = neutral_owner;
    hex->supply_source = false;
    hex->tags = tag_mask(HexTag::BaseSteppe);
    hex->source_labels = {"base_steppe"};
    hex->pasture = initial_pasture_for_terrain(hex->terrain);

    cancel_horde_production(*unit);
    unit->combat_done = true;
    unit->contacted_enemy_this_turn = true;
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

bool butcher_livestock(GameState& state, int unit_id, int livestock) {
    Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr
        || (unit->kind != UnitKind::Horde && unit->kind != UnitKind::Herd)
        || unit->owner != active_faction(state)
        || unit->move_done
        || unit->moved_this_turn
        || enemy_unit_adjacent_to(state, *unit)
        || livestock <= 0
        || livestock > unit->livestock) {
        return false;
    }

    FactionState* faction = find_faction(state, unit->owner);
    if (faction == nullptr) {
        return false;
    }

    unit->livestock -= livestock;
    faction->food += livestock * livestock_butcher_food_per_head;
    unit->remaining_scaled_move = 0;
    unit->move_done = true;
    state.selected_unit_id = unit_id;
    refresh_legal_actions(state);
    return true;
}

DetachHerdOptions detach_herd_options(const GameState& state, int unit_id, int horses, int livestock) {
    DetachHerdOptions options;
    options.unit_id = unit_id;
    options.horses = horses;
    options.livestock = livestock;
    const Unit* horde = find_unit(state, unit_id);
    if (horde == nullptr
        || horde->kind != UnitKind::Horde
        || horde->owner != active_faction(state)
        || horde->move_done
        || horde->moved_this_turn
        || enemy_unit_adjacent_to(state, *horde)
        || horses < 0
        || livestock < 0
        || (horses <= 0 && livestock <= 0)
        || horses > horde->horses
        || livestock > horde->livestock) {
        return options;
    }

    options.deployable_hexes = adjacent_deployable_hexes(state, *horde);
    return options;
}

bool detach_herd(GameState& state, int unit_id, int horses, int livestock, Coord destination) {
    DetachHerdOptions options = detach_herd_options(state, unit_id, horses, livestock);
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
    horde->livestock -= livestock;
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
    herd.livestock = livestock;
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
        || (kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_faction(state, kind, horde->owner)
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
        || (kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_faction(state, kind, horde->owner)) {
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
    if ((kind != UnitKind::HorseArcher && kind != UnitKind::MongolLancer)
        || !unit_kind_available_to_faction(state, kind, horde.owner)
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
    const FactionState* faction = find_faction(state, owner);
    const bool hunger_blocks_recovery = faction != nullptr
        && faction->hunger >= faction_hunger_readiness_threshold;
    for (Unit& unit : state.units) {
        if (unit.owner == owner) {
            if (unit.kind == UnitKind::Horde && unit.production_active) {
                unit.production_turns_remaining = std::max(0, unit.production_turns_remaining - 1);
                if (unit.production_turns_remaining == 0) {
                    ready_horde_ids.push_back(unit.id);
                }
            }
            const bool currently_in_contact = enemy_unit_adjacent_to(state, unit);
            if (!hunger_blocks_recovery
                && !unit.moved_this_turn
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
            || !at_war(state, owner, candidate.owner)
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

bool ai_attack_best_adjacent_enemy(
    GameState& state,
    int attacker_id,
    const AiDirective& directive,
    AiAnimationStep* animation_step = nullptr
) {
    const std::vector<AttackableUnit> attacks = attackable_units(state, attacker_id);
    if (attacks.empty()) {
        return false;
    }
    const Unit* attacker = find_unit(state, attacker_id);
    if (attacker == nullptr) {
        return false;
    }
    bool adjacent_directive_horde = false;
    if (directive.kind == AiDirectiveKind::HuntHorde) {
        for (const AttackableUnit& attack : attacks) {
            const Unit* defender = find_unit(state, attack.unit_id);
            if (defender != nullptr
                && defender->kind == UnitKind::Horde
                && (directive.target_owner == neutral_owner || defender->owner == directive.target_owner)) {
                adjacent_directive_horde = true;
                break;
            }
        }
        if (!adjacent_directive_horde) {
            return false;
        }
    }
    int best_defender_id = attacks.front().unit_id;
    int best_score = std::numeric_limits<int>::max();
    for (const AttackableUnit& attack : attacks) {
        const Unit* defender = find_unit(state, attack.unit_id);
        if (defender == nullptr) {
            continue;
        }
        const bool directive_horde =
            directive.kind == AiDirectiveKind::HuntHorde
            && defender->kind == UnitKind::Horde
            && (directive.target_owner == neutral_owner || defender->owner == directive.target_owner);
        if (directive.kind == AiDirectiveKind::HuntHorde
            && adjacent_directive_horde
            && !directive_horde) {
            continue;
        }
        const int score = defender->hp * 100
            + effective_defense_score(state, *defender) * 10
            + hex_distance(attacker->coord, defender->coord)
            + (directive.kind == AiDirectiveKind::HuntHorde && !directive_horde ? 500 : 0);
        if (score < best_score || (score == best_score && defender->id < best_defender_id)) {
            best_score = score;
            best_defender_id = defender->id;
        }
    }
    if (best_score == std::numeric_limits<int>::max()) {
        return false;
    }
    const CombatPreview preview = combat_preview(state, attacker_id, best_defender_id);
    if (animation_step != nullptr) {
        const Unit* attacker_before_attack = find_unit(state, attacker_id);
        const Unit* defender = find_unit(state, best_defender_id);
        if (attacker_before_attack != nullptr && defender != nullptr) {
            animation_step->to = attacker_before_attack->coord;
            animation_step->attacks.push_back(defender->coord);

            AiAnimationStep::AttackEvent event;
            event.target = defender->coord;
            event.defender_id = defender->id;
            event.defender_from = defender->coord;
            event.defender_to = defender->coord;
            event.attacker_to = attacker_before_attack->coord;
            if (preview.valid && !preview.defender.destroyed) {
                if (!preview.retreat_blocked && preview.retreat_option == "defender") {
                    event.defender_to = preview.defender_retreat_to;
                    event.defender_moved = !coord_equal(event.defender_from, event.defender_to);
                }
                if (preview.special_resolution == "feigned_retreat") {
                    event.attacker_to = preview.attacker_pursuit_to;
                    event.attacker_moved = !coord_equal(attacker_before_attack->coord, event.attacker_to);
                }
            }
            animation_step->attack_events.push_back(event);
        }
    }
    return attack_unit(state, attacker_id, best_defender_id);
}

std::vector<AiPathCandidateTrace> ranked_ai_path_candidates(const GameState& state, const Unit& unit, Coord target) {
    std::vector<AiPathCandidateTrace> candidates;
    const int current_distance = hex_distance(unit.coord, target);
    const std::vector<ReachableHex> reachable = reachable_hexes(state, unit.id);
    for (const ReachableHex& reachable_hex : reachable) {
        AiPathCandidateTrace candidate;
        candidate.coord = reachable_hex.coord;
        candidate.scaled_cost = reachable_hex.scaled_cost;
        candidate.distance_before = current_distance;
        candidate.distance_after = hex_distance(reachable_hex.coord, target);
        candidate.distance_gain = current_distance - candidate.distance_after;
        candidate.score = candidate.distance_gain * 100 - candidate.distance_after * 10 - candidate.scaled_cost;
        candidate.path = movement_path_to(state, unit, reachable_hex.coord);
        candidates.push_back(std::move(candidate));
    }
    std::sort(candidates.begin(), candidates.end(), [](const AiPathCandidateTrace& first, const AiPathCandidateTrace& second) {
        if (first.distance_after != second.distance_after) {
            return first.distance_after < second.distance_after;
        }
        if (first.scaled_cost != second.scaled_cost) {
            return first.scaled_cost < second.scaled_cost;
        }
        return coord_less(first.coord, second.coord);
    });
    if (!candidates.empty() && candidates.front().distance_after < current_distance) {
        candidates.front().selected = true;
    }
    return candidates;
}

int settled_defense_position_score(const GameState& state, const Unit& unit, Coord coord, Coord target) {
    const int objective_distance = hex_distance(coord, target);
    const Unit* threat = nearest_enemy_unit_from_coord(state, unit.owner, target);
    const int threat_distance = threat == nullptr ? 8 : hex_distance(coord, threat->coord);
    const GameHex* hex = find_hex(state, coord);
    int score = 1000
        - objective_distance * 140
        + std::max(0, 6 - objective_distance) * 25
        + (terrain_defense_percent_at(state, coord) - 100) * (is_cavalry_unit(unit.kind) ? 1 : 3)
        + readiness_percent(unit);
    if (coord_equal(coord, target)) {
        score += 360;
    } else if (objective_distance == 1) {
        score += 130;
    }
    if (hex != nullptr && hex->terrain == Terrain::Urban) {
        score += 160;
    }
    if (hex != nullptr && hex->owner == unit.owner) {
        score += 45;
    }
    if (threat != nullptr) {
        score += std::max(0, 7 - threat_distance) * (is_cavalry_unit(unit.kind) ? 35 : 20);
        if (threat_distance == 1 && unit.readiness < 55) {
            score -= 140;
        }
    }
    return score;
}

std::vector<AiPathCandidateTrace> ranked_settled_defense_candidates(
    const GameState& state,
    const Unit& unit,
    Coord target
) {
    std::vector<AiPathCandidateTrace> candidates;
    const int current_distance = hex_distance(unit.coord, target);
    const std::vector<ReachableHex> reachable = reachable_hexes(state, unit.id);
    for (const ReachableHex& reachable_hex : reachable) {
        AiPathCandidateTrace candidate;
        candidate.coord = reachable_hex.coord;
        candidate.scaled_cost = reachable_hex.scaled_cost;
        candidate.distance_before = current_distance;
        candidate.distance_after = hex_distance(reachable_hex.coord, target);
        candidate.distance_gain = current_distance - candidate.distance_after;
        candidate.score = settled_defense_position_score(state, unit, reachable_hex.coord, target)
            - reachable_hex.scaled_cost;
        candidate.path = movement_path_to(state, unit, reachable_hex.coord);
        candidates.push_back(std::move(candidate));
    }
    std::sort(candidates.begin(), candidates.end(), [](const AiPathCandidateTrace& first, const AiPathCandidateTrace& second) {
        if (first.score != second.score) {
            return first.score > second.score;
        }
        if (first.distance_after != second.distance_after) {
            return first.distance_after < second.distance_after;
        }
        if (first.scaled_cost != second.scaled_cost) {
            return first.scaled_cost < second.scaled_cost;
        }
        return coord_less(first.coord, second.coord);
    });
    const int current_score = settled_defense_position_score(state, unit, unit.coord, target);
    if (!candidates.empty() && candidates.front().score > current_score) {
        candidates.front().selected = true;
    }
    return candidates;
}

void copy_top_ai_path_candidates(
    const std::vector<AiPathCandidateTrace>& candidates,
    AiUnitDecisionTrace* trace,
    std::size_t limit = 5
) {
    if (trace == nullptr) {
        return;
    }
    trace->path_candidates.clear();
    const std::size_t count = std::min(limit, candidates.size());
    trace->path_candidates.insert(trace->path_candidates.end(), candidates.begin(), candidates.begin() + count);
}

bool ai_move_toward_target(GameState& state, int unit_id, Coord target, AiUnitDecisionTrace* trace = nullptr) {
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->move_done || unit->remaining_scaled_move <= 0) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = unit == nullptr ? "unit_not_found" : "no_movement_available";
        }
        return false;
    }
    const int current_distance = hex_distance(unit->coord, target);
    std::vector<AiPathCandidateTrace> candidates = ranked_ai_path_candidates(state, *unit, target);
    copy_top_ai_path_candidates(candidates, trace);
    if (candidates.empty()) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = "no_reachable_hexes";
        }
        return false;
    }

    const auto selected = std::find_if(candidates.begin(), candidates.end(), [](const AiPathCandidateTrace& candidate) {
        return candidate.selected;
    });
    if (selected == candidates.end()) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = "no_distance_improvement";
        }
        return false;
    }
    const bool moved = move_unit(state, unit_id, selected->coord);
    if (trace != nullptr) {
        trace->action = moved ? "move" : "idle";
        trace->reason = moved ? "distance_improved" : "move_rejected";
    }
    return moved;
}

bool ai_move_to_settled_defense_position(
    GameState& state,
    int unit_id,
    Coord target,
    AiUnitDecisionTrace* trace = nullptr
) {
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->move_done || unit->remaining_scaled_move <= 0) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = unit == nullptr ? "unit_not_found" : "no_movement_available";
        }
        return false;
    }
    std::vector<AiPathCandidateTrace> candidates = ranked_settled_defense_candidates(state, *unit, target);
    copy_top_ai_path_candidates(candidates, trace);
    if (candidates.empty()) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = "no_reachable_hexes";
        }
        return false;
    }
    const auto selected = std::find_if(candidates.begin(), candidates.end(), [](const AiPathCandidateTrace& candidate) {
        return candidate.selected;
    });
    if (selected == candidates.end()) {
        if (trace != nullptr) {
            trace->action = "idle";
            trace->reason = "hold_defensive_position";
        }
        return false;
    }
    const bool moved = move_unit(state, unit_id, selected->coord);
    if (trace != nullptr) {
        trace->action = moved ? "move" : "idle";
        trace->reason = moved ? "settled_defense_position" : "move_rejected";
    }
    return moved;
}

void populate_ai_animation_step_snapshot(GameState& state, AiAnimationStep& step) {
    step.hexes = state.hexes;
    const auto supplied = supplied_hex_coords(state);
    step.supplied_hexes.assign(supplied.begin(), supplied.end());
    step.units = state.units;
}

AiDirective default_hunt_directive() {
    AiDirective directive;
    directive.kind = AiDirectiveKind::Hunt;
    return directive;
}

const FactionState* faction_for_state_owner(const GameState& state, OwnerId owner) {
    const FactionState* faction = find_faction(state, owner);
    return faction == nullptr ? nullptr : faction;
}

std::string faction_archetype(const GameState& state, OwnerId owner) {
    const FactionState* faction = faction_for_state_owner(state, owner);
    return faction == nullptr ? faction_for_owner(owner).archetype : faction->archetype;
}

bool first_ai_anchor_coord(const GameState& state, OwnerId owner, Coord& anchor) {
    const std::vector<int> ids = ai_unit_turn_order(state, owner);
    if (ids.empty()) {
        return false;
    }
    const Unit* unit = find_unit(state, ids.front());
    if (unit == nullptr) {
        return false;
    }
    anchor = unit->coord;
    return true;
}

bool nearest_owned_settlement_threat(const GameState& state, OwnerId owner, Coord origin, Coord& target, OwnerId& threat_owner) {
    bool found = false;
    int best_score = std::numeric_limits<int>::min();
    for (const Settlement& settlement : state.settlements) {
        if (settlement_hex_owner(state, settlement) != owner) {
            continue;
        }
        const Unit* threat = nearest_enemy_unit_from_coord(state, owner, settlement.coord);
        if (threat == nullptr) {
            continue;
        }
        const int threat_distance = hex_distance(threat->coord, settlement.coord);
        if (threat_distance > 5) {
            continue;
        }
        const int score = (6 - threat_distance) * 120
            + threat->attack * 12
            + threat->readiness
            - hex_distance(origin, settlement.coord) * 8;
        if (!found || score > best_score || (score == best_score && coord_less(settlement.coord, target))) {
            target = settlement.coord;
            threat_owner = threat->owner;
            best_score = score;
            found = true;
        }
    }
    return found;
}

bool settled_archetype(const std::string& archetype) {
    return archetype == "chinese" || archetype == "persian" || archetype == "jurchen";
}

bool passable_ai_objective_hex(const GameState& state, Coord coord) {
    const GameHex* hex = find_hex(state, coord);
    return hex != nullptr && terrain_movement_cost(hex->terrain) != blocked_movement_cost();
}

int nearest_owned_settlement_distance(const GameState& state, OwnerId owner, Coord coord) {
    int best = std::numeric_limits<int>::max();
    for (const Settlement& settlement : state.settlements) {
        if (settlement_hex_owner(state, settlement) == owner) {
            best = std::min(best, hex_distance(coord, settlement.coord));
        }
    }
    return best;
}

bool wall_gate_hold_coord(const GameState& state, OwnerId owner, const WallGate& gate, Coord& target) {
    std::vector<Coord> candidates;
    if (passable_ai_objective_hex(state, gate.edge.a)) {
        candidates.push_back(gate.edge.a);
    }
    if (passable_ai_objective_hex(state, gate.edge.b)) {
        candidates.push_back(gate.edge.b);
    }
    if (candidates.empty()) {
        return false;
    }

    if (owner == chinese_owner) {
        const auto outside_wall = great_wall_outside_hexes(state);
        if (!outside_wall.empty()) {
            std::vector<Coord> enclosed_candidates;
            for (const Coord& coord : candidates) {
                if (outside_wall.find(coord) == outside_wall.end()) {
                    enclosed_candidates.push_back(coord);
                }
            }
            if (!enclosed_candidates.empty()) {
                candidates = std::move(enclosed_candidates);
            }
        }
    }

    bool found = false;
    int best_score = std::numeric_limits<int>::min();
    for (const Coord& coord : candidates) {
        const GameHex* hex = find_hex(state, coord);
        const int owned_settlement_distance = nearest_owned_settlement_distance(state, owner, coord);
        const bool owned_hex = hex != nullptr && hex->owner == owner;
        if (!owned_hex
            && (owned_settlement_distance == std::numeric_limits<int>::max() || owned_settlement_distance > 8)) {
            continue;
        }
        const int settlement_distance_penalty = owned_settlement_distance == std::numeric_limits<int>::max()
            ? 0
            : owned_settlement_distance * 12;
        const int score = (owned_hex ? 80 : 0)
            - settlement_distance_penalty
            + terrain_defense_percent_at(state, coord);
        if (!found || score > best_score || (score == best_score && coord_less(coord, target))) {
            target = coord;
            best_score = score;
            found = true;
        }
    }
    return found;
}

bool best_threatened_wall_gate(const GameState& state, OwnerId owner, Coord origin, Coord& target, OwnerId& threat_owner) {
    bool found = false;
    int best_score = std::numeric_limits<int>::min();
    for (const WallGate& gate : state.wall_gates) {
        Coord gate_coord;
        if (!wall_gate_hold_coord(state, owner, gate, gate_coord)) {
            continue;
        }
        const Unit* threat = nearest_enemy_unit_from_coord(state, owner, gate_coord);
        if (threat == nullptr) {
            continue;
        }
        const int threat_distance = hex_distance(threat->coord, gate_coord);
        if (threat_distance > 5) {
            continue;
        }
        const int settlement_distance = nearest_owned_settlement_distance(state, owner, gate_coord);
        const int score = 250
            + (6 - threat_distance) * 140
            + threat->attack * 12
            + threat->readiness
            - settlement_distance * 10
            - hex_distance(origin, gate_coord) * 6;
        if (!found || score > best_score || (score == best_score && coord_less(gate_coord, target))) {
            target = gate_coord;
            threat_owner = threat->owner;
            best_score = score;
            found = true;
        }
    }
    return found;
}

bool nearest_capturable_hex(const GameState& state, OwnerId owner, Coord origin, Coord& target, OwnerId& target_owner) {
    bool found = false;
    int best_score = std::numeric_limits<int>::max();
    for (const Settlement& settlement : state.settlements) {
        const OwnerId settlement_owner = settlement_hex_owner(state, settlement);
        if (settlement_owner == owner
            || (settlement_owner != neutral_owner && !at_war(state, owner, settlement_owner))) {
            continue;
        }
        const int score = hex_distance(origin, settlement.coord);
        if (!found || score < best_score || (score == best_score && coord_less(settlement.coord, target))) {
            target = settlement.coord;
            target_owner = settlement_owner;
            best_score = score;
            found = true;
        }
    }
    for (const GameHex& hex : state.hexes) {
        if (hex.terrain != Terrain::Urban
            || hex.owner == owner
            || (hex.owner != neutral_owner && !at_war(state, owner, hex.owner))) {
            continue;
        }
        const int score = hex_distance(origin, hex.coord);
        if (!found || score < best_score || (score == best_score && coord_less(hex.coord, target))) {
            target = hex.coord;
            target_owner = hex.owner;
            best_score = score;
            found = true;
        }
    }
    return found;
}

bool strategic_directive_for_owner(const GameState& state, OwnerId owner, Coord origin, AiDirective& directive) {
    const std::string archetype = faction_archetype(state, owner);
    if (archetype == "steppe_nomad") {
        const Unit* horde = nearest_enemy_unit_from_coord(state, owner, origin, neutral_owner, true);
        if (horde != nullptr) {
            directive.kind = AiDirectiveKind::HuntHorde;
            directive.target_owner = horde->owner;
            return true;
        }
        if (nearest_enemy_unit_from_coord(state, owner, origin) != nullptr) {
            directive = default_hunt_directive();
            return true;
        }
        return false;
    }

    if (settled_archetype(archetype)) {
        Coord target;
        OwnerId target_owner = neutral_owner;
        if (best_threatened_wall_gate(state, owner, origin, target, target_owner)) {
            directive.kind = AiDirectiveKind::DefendHex;
            directive.target = target;
            return true;
        }
        if (nearest_owned_settlement_threat(state, owner, origin, target, target_owner)) {
            directive.kind = AiDirectiveKind::DefendHex;
            directive.target = target;
            return true;
        }
        if (nearest_capturable_hex(state, owner, origin, target, target_owner)) {
            if (target_owner == neutral_owner || !at_war(state, owner, target_owner)) {
                return false;
            }
            directive.kind = AiDirectiveKind::CaptureHex;
            directive.target = target;
            return true;
        }
        if (const Unit* horde = nearest_enemy_unit_from_coord(state, owner, origin, neutral_owner, true)) {
            const int disposition = disposition_toward(state, owner, horde->owner);
            const int fear = fear_toward(state, owner, horde->owner);
            if (raiding_relationship(disposition, fear)) {
                directive.kind = AiDirectiveKind::HuntHorde;
                directive.target_owner = horde->owner;
                return true;
            }
        }
        if (const Unit* enemy = nearest_enemy_unit_from_coord(state, owner, origin)) {
            directive = default_hunt_directive();
            return true;
        }
    }

    if (archetype == "forest_nomad") {
        const Unit* horde = nearest_enemy_unit_from_coord(state, owner, origin, neutral_owner, true);
        if (horde != nullptr) {
            directive.kind = AiDirectiveKind::HuntHorde;
            directive.target_owner = horde->owner;
            return true;
        }
        if (nearest_enemy_unit_from_coord(state, owner, origin) != nullptr) {
            directive = default_hunt_directive();
            return true;
        }
    }

    return false;
}

AiDirective strategic_directive_for_owner(const GameState& state, OwnerId owner) {
    Coord origin{1, 1};
    AiDirective directive = default_hunt_directive();
    if (!first_ai_anchor_coord(state, owner, origin)) {
        return directive;
    }
    strategic_directive_for_owner(state, owner, origin, directive);
    return directive;
}

std::string strategic_group_name(const AiDirective& directive) {
    switch (directive.kind) {
        case AiDirectiveKind::Inactive:
            return "Inactive";
        case AiDirectiveKind::HoldHex:
            return "Hold Hex";
        case AiDirectiveKind::Hunt:
            return "Strategic: Hunt";
        case AiDirectiveKind::DefendHex:
            return "Strategic: Defend";
        case AiDirectiveKind::HuntHorde:
            return "Strategic: Hunt Horde";
        case AiDirectiveKind::CaptureHex:
            return "Strategic: Capture";
    }
    return "Strategic";
}

int generated_ai_group_id(OwnerId owner) {
    return -1000 - owner;
}

int generated_town_garrison_group_id(OwnerId owner, int index) {
    return -100000 - owner * 1000 - index;
}

int generated_field_army_group_id(OwnerId owner, int index) {
    return -200000 - owner * 1000 - index;
}

enum class DefensiveHexKind {
    Town,
    WallGate,
};

struct DefensiveHex {
    Coord coord;
    DefensiveHexKind kind = DefensiveHexKind::Town;
    int priority = 0;
    int desired_garrison = 1;
};

std::string generated_settled_mobile_role(const GameState& state, OwnerId owner) {
    bool has_field_disposition = false;
    for (const DiplomaticRelationship& relationship : state.diplomacy) {
        if (relationship.owner != owner
            || relationship.target == neutral_owner
            || !at_war(state, owner, relationship.target)) {
            continue;
        }
        const int disposition = clamp_disposition(relationship.disposition);
        const int fear = clamp_fear(relationship.fear);
        if (raiding_relationship(disposition, fear)) {
            return "raiding_force";
        }
        if (field_army_relationship(disposition, fear)) {
            has_field_disposition = true;
        }
    }
    return has_field_disposition ? "field_army" : "reserve";
}

std::string generated_settled_mobile_group_name(const std::string& role) {
    if (role == "reserve") {
        return "Reserve";
    }
    if (role == "raiding_force") {
        return "Raiding Force";
    }
    return "Field Army";
}

std::string defensive_hex_group_name(DefensiveHexKind kind) {
    switch (kind) {
        case DefensiveHexKind::Town:
            return "Town Garrison";
        case DefensiveHexKind::WallGate:
            return "Gate Garrison";
    }
    return "Garrison";
}

bool defensive_hex_kind_less(DefensiveHexKind first, DefensiveHexKind second) {
    return static_cast<int>(first) < static_cast<int>(second);
}

void add_owned_defensive_hex(std::vector<DefensiveHex>& objectives, DefensiveHex objective) {
    auto existing = std::find_if(objectives.begin(), objectives.end(), [&](const DefensiveHex& prior) {
        return coord_equal(prior.coord, objective.coord);
    });
    if (existing == objectives.end()) {
        objectives.push_back(objective);
        return;
    }
    if (objective.priority > existing->priority
        || (objective.priority == existing->priority
            && defensive_hex_kind_less(objective.kind, existing->kind))) {
        *existing = objective;
    }
}

std::vector<DefensiveHex> owned_defensive_hexes(const GameState& state, OwnerId owner) {
    std::vector<DefensiveHex> objectives;
    for (const GameHex& hex : state.hexes) {
        if (hex.owner != owner || hex.terrain != Terrain::Urban) {
            continue;
        }
        DefensiveHex objective;
        objective.coord = hex.coord;
        objective.kind = DefensiveHexKind::Town;
        objective.priority = 90 + terrain_defense_percent_at(state, hex.coord);
        objective.desired_garrison = 1;
        add_owned_defensive_hex(objectives, objective);
    }

    for (const WallGate& gate : state.wall_gates) {
        Coord gate_coord;
        if (!wall_gate_hold_coord(state, owner, gate, gate_coord)) {
            continue;
        }
        DefensiveHex objective;
        objective.coord = gate_coord;
        objective.kind = DefensiveHexKind::WallGate;
        objective.priority = 80 + terrain_defense_percent_at(state, gate_coord);
        objective.desired_garrison = 1;
        add_owned_defensive_hex(objectives, objective);
    }

    std::sort(objectives.begin(), objectives.end(), [](const DefensiveHex& first, const DefensiveHex& second) {
        if (first.priority != second.priority) {
            return first.priority > second.priority;
        }
        if (first.kind != second.kind) {
            return defensive_hex_kind_less(first.kind, second.kind);
        }
        return coord_less(first.coord, second.coord);
    });
    return objectives;
}

bool friendly_combat_unit_holding_coord(const GameState& state, OwnerId owner, Coord coord) {
    for (const Unit& unit : state.units) {
        if (unit.owner == owner
            && unit.hp > 0
            && can_attack(unit.kind)
            && coord_equal(unit.coord, coord)) {
            return true;
        }
    }
    return false;
}

bool garrison_infantry_unit(UnitKind kind) {
    switch (kind) {
        case UnitKind::ChineseMilitia:
        case UnitKind::Infantry:
        case UnitKind::PersianInfantry:
        case UnitKind::JurchenInfantry:
        case UnitKind::ForestWarband:
            return true;
        case UnitKind::ChineseCavalry:
        case UnitKind::PersianCavalry:
        case UnitKind::JurchenCavalry:
        case UnitKind::ForestRaiders:
        case UnitKind::HorseArcher:
        case UnitKind::MongolLancer:
        case UnitKind::Herd:
        case UnitKind::Horde:
            return false;
    }
    return false;
}

bool unassigned_unit_holding_coord(const GameState& state, const std::vector<int>& unit_ids, Coord coord) {
    for (const int unit_id : unit_ids) {
        const Unit* unit = find_unit(state, unit_id);
        if (unit != nullptr && coord_equal(unit->coord, coord)) {
            return true;
        }
    }
    return false;
}

bool assign_best_garrison_unit(
    const GameState& state,
    std::vector<int>& available_unit_ids,
    const DefensiveHex& objective,
    int& assigned_unit_id
) {
    bool found = false;
    std::size_t best_index = 0;
    int best_score = std::numeric_limits<int>::max();
    for (std::size_t index = 0; index < available_unit_ids.size(); ++index) {
        const Unit* unit = find_unit(state, available_unit_ids[index]);
        if (unit == nullptr || unit->hp <= 0 || !can_attack(unit->kind) || !garrison_infantry_unit(unit->kind)) {
            continue;
        }
        const int distance = hex_distance(unit->coord, objective.coord);
        const int score = (coord_equal(unit->coord, objective.coord) ? -100000 : 0)
            + distance * 100
            + unit->id;
        if (!found || score < best_score) {
            best_index = index;
            best_score = score;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    assigned_unit_id = available_unit_ids[best_index];
    available_unit_ids.erase(available_unit_ids.begin() + static_cast<std::ptrdiff_t>(best_index));
    return true;
}

bool reserve_defensive_hole_target(const GameState& state, OwnerId owner, Coord origin, Coord& target) {
    bool found = false;
    int best_score = std::numeric_limits<int>::max();

    const auto consider = [&](const DefensiveHex& objective) {
        if (!passable_ai_objective_hex(state, objective.coord)
            || friendly_combat_unit_holding_coord(state, owner, objective.coord)) {
            return;
        }
        const Unit* threat = nearest_enemy_unit_from_coord(state, owner, objective.coord);
        int threat_bonus = 0;
        if (threat != nullptr) {
            const int threat_distance = hex_distance(threat->coord, objective.coord);
            if (threat_distance <= 5) {
                threat_bonus = (6 - threat_distance) * 160 + threat->attack * 12 + threat->readiness;
            }
        }
        const int score = hex_distance(origin, objective.coord) * 16 - threat_bonus - objective.priority;
        if (!found || score < best_score || (score == best_score && coord_less(objective.coord, target))) {
            target = objective.coord;
            best_score = score;
            found = true;
        }
    };

    for (const DefensiveHex& objective : owned_defensive_hexes(state, owner)) {
        consider(objective);
    }
    return found;
}

void refresh_generated_strategic_ai_group(GameState& state, OwnerId owner) {
    state.ai_groups.erase(
        std::remove_if(state.ai_groups.begin(), state.ai_groups.end(), [&](const AiGroup& group) {
            return group.generated && group.owner == owner;
        }),
        state.ai_groups.end()
    );

    std::vector<int> assigned_unit_ids;
    for (const AiGroup& group : state.ai_groups) {
        if (group.owner != owner || group.generated) {
            continue;
        }
        assigned_unit_ids.insert(assigned_unit_ids.end(), group.unit_ids.begin(), group.unit_ids.end());
    }
    std::sort(assigned_unit_ids.begin(), assigned_unit_ids.end());
    assigned_unit_ids.erase(std::unique(assigned_unit_ids.begin(), assigned_unit_ids.end()), assigned_unit_ids.end());

    std::vector<int> unassigned_unit_ids;
    for (const int unit_id : ai_unit_turn_order(state, owner)) {
        if (!std::binary_search(assigned_unit_ids.begin(), assigned_unit_ids.end(), unit_id)) {
            unassigned_unit_ids.push_back(unit_id);
        }
    }
    if (unassigned_unit_ids.empty()) {
        return;
    }

    if (settled_archetype(faction_archetype(state, owner))) {
        int garrison_index = 0;
        std::vector<int> remaining_unit_ids = std::move(unassigned_unit_ids);
        for (const DefensiveHex& objective : owned_defensive_hexes(state, owner)) {
            if (objective.desired_garrison <= 0 || remaining_unit_ids.empty()) {
                break;
            }
            if (!unassigned_unit_holding_coord(state, remaining_unit_ids, objective.coord)
                && friendly_combat_unit_holding_coord(state, owner, objective.coord)) {
                continue;
            }
            int unit_id = 0;
            if (!assign_best_garrison_unit(state, remaining_unit_ids, objective, unit_id)) {
                continue;
            }
            const Unit* unit = find_unit(state, unit_id);
            if (unit == nullptr) {
                continue;
            }

            AiDirective directive;
            directive.kind = coord_equal(unit->coord, objective.coord)
                ? AiDirectiveKind::HoldHex
                : AiDirectiveKind::DefendHex;
            directive.target = objective.coord;

            AiGroup group;
            group.id = generated_town_garrison_group_id(owner, garrison_index++);
            group.owner = owner;
            group.name = defensive_hex_group_name(objective.kind);
            group.role = "garrison";
            group.unit_ids = {unit_id};
            group.directive = directive;
            group.generated = true;
            state.ai_groups.push_back(std::move(group));
        }
        unassigned_unit_ids = std::move(remaining_unit_ids);
        if (unassigned_unit_ids.empty()) {
            return;
        }
    }

    if (settled_archetype(faction_archetype(state, owner))) {
        constexpr std::size_t field_army_size = 4;
        int army_index = 0;
        const std::string mobile_role = generated_settled_mobile_role(state, owner);
        for (std::size_t start = 0; start < unassigned_unit_ids.size(); start += field_army_size) {
            std::vector<int> army_unit_ids;
            const std::size_t end = std::min(start + field_army_size, unassigned_unit_ids.size());
            army_unit_ids.insert(army_unit_ids.end(), unassigned_unit_ids.begin() + start, unassigned_unit_ids.begin() + end);
            const Unit* anchor_unit = find_unit(state, army_unit_ids.front());
            if (anchor_unit == nullptr) {
                continue;
            }
            AiDirective directive;
            directive.kind = AiDirectiveKind::Inactive;
            strategic_directive_for_owner(state, owner, anchor_unit->coord, directive);

            AiGroup group;
            group.id = generated_field_army_group_id(owner, army_index++);
            group.owner = owner;
            group.name = generated_settled_mobile_group_name(mobile_role);
            group.role = mobile_role;
            group.unit_ids = std::move(army_unit_ids);
            group.directive = directive;
            group.generated = true;
            state.ai_groups.push_back(std::move(group));
        }
        return;
    }

    const Unit* anchor_unit = find_unit(state, unassigned_unit_ids.front());
    if (anchor_unit == nullptr) {
        return;
    }
    AiDirective directive;
    if (!strategic_directive_for_owner(state, owner, anchor_unit->coord, directive)) {
        return;
    }

    AiGroup group;
    group.id = generated_ai_group_id(owner);
    group.owner = owner;
    group.name = strategic_group_name(directive);
    group.role = "manual_directive";
    group.unit_ids = std::move(unassigned_unit_ids);
    group.directive = directive;
    group.generated = true;
    state.ai_groups.push_back(std::move(group));
}

const Unit* ai_target_for_directive(const GameState& state, const Unit& unit, const AiDirective& directive) {
    switch (directive.kind) {
        case AiDirectiveKind::Inactive:
        case AiDirectiveKind::HoldHex:
            return nullptr;
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

const Unit* ai_group_anchor_unit(const GameState& state, const AiGroup& group) {
    for (const int unit_id : group.unit_ids) {
        const Unit* unit = find_unit(state, unit_id);
        if (unit != nullptr
            && unit->owner == group.owner
            && unit->hp > 0
            && can_attack(unit->kind)) {
            return unit;
        }
    }
    return nullptr;
}

AiDirective directive_for_ai_group(const GameState& state, const AiGroup& group) {
    const std::string role = normalize_ai_group_role(group.role);
    if (role == "field_army") {
        const Unit* anchor = ai_group_anchor_unit(state, group);
        AiDirective directive;
        if (anchor != nullptr && strategic_directive_for_owner(state, group.owner, anchor->coord, directive)) {
            return directive;
        }
        return group.directive;
    }
    if (role == "raiding_force") {
        const Unit* anchor = ai_group_anchor_unit(state, group);
        if (anchor != nullptr) {
            const Unit* horde = nearest_enemy_unit_from_coord(state, group.owner, anchor->coord, neutral_owner, true);
            if (horde != nullptr) {
                AiDirective directive;
                directive.kind = AiDirectiveKind::HuntHorde;
                directive.target_owner = horde->owner;
                return directive;
            }
            if (nearest_enemy_unit_from_coord(state, group.owner, anchor->coord) != nullptr) {
                return default_hunt_directive();
            }
        }
        return group.directive;
    }
    if (role == "reserve") {
        const Unit* anchor = ai_group_anchor_unit(state, group);
        Coord target;
        if (anchor != nullptr
            && settled_archetype(faction_archetype(state, group.owner))
            && reserve_defensive_hole_target(state, group.owner, anchor->coord, target)) {
            AiDirective directive;
            directive.kind = AiDirectiveKind::DefendHex;
            directive.target = target;
            return directive;
        }
        AiDirective directive;
        directive.kind = AiDirectiveKind::Inactive;
        directive.target = group.directive.target;
        directive.target_owner = group.directive.target_owner;
        return directive;
    }
    if (role == "garrison" && group.directive.kind == AiDirectiveKind::Hunt) {
        AiDirective directive;
        directive.kind = AiDirectiveKind::HoldHex;
        directive.target = group.directive.target;
        return directive;
    }
    return group.directive;
}

bool execute_ai_unit_single_action(
    GameState& state,
    int unit_id,
    const AiDirective& directive,
    AiAnimationStep* animation_step = nullptr,
    AiUnitDecisionTrace* decision_trace = nullptr
) {
    if (decision_trace != nullptr) {
        decision_trace->unit_id = unit_id;
        decision_trace->directive = directive;
        if (const Unit* unit = find_unit(state, unit_id)) {
            decision_trace->owner = unit->owner;
            decision_trace->from = unit->coord;
        }
        const std::vector<AttackableUnit> attacks = attackable_units(state, unit_id);
        for (const AttackableUnit& attack : attacks) {
            decision_trace->attackable_unit_ids.push_back(attack.unit_id);
        }
    }
    if (directive.kind == AiDirectiveKind::Inactive || directive.kind == AiDirectiveKind::HoldHex) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->action = "idle";
            decision_trace->reason = directive.kind == AiDirectiveKind::HoldHex ? "hold_hex_directive" : "inactive_directive";
        }
        return false;
    }
    if (ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step)) {
        if (decision_trace != nullptr) {
            decision_trace->action = "attack";
            decision_trace->reason = "adjacent_enemy";
            if (animation_step != nullptr && !animation_step->attack_events.empty()) {
                const AiAnimationStep::AttackEvent& event = animation_step->attack_events.front();
                decision_trace->target = event.target;
                decision_trace->target_unit_id = event.defender_id;
            }
        }
        return true;
    }

    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr || unit->move_done || unit->remaining_scaled_move <= 0) {
        if (decision_trace != nullptr) {
            decision_trace->action = "idle";
            decision_trace->reason = unit == nullptr ? "unit_not_found" : "no_movement_available";
        }
        return false;
    }

    const bool settled_ai = settled_archetype(faction_archetype(state, unit->owner));
    if (settled_ai
        && directive.kind == AiDirectiveKind::DefendHex
        && readiness_percent(*unit) < 55
        && hex_distance(unit->coord, directive.target) <= 1) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->action = "idle";
            decision_trace->reason = "recover_readiness_on_defense";
        }
        return false;
    }
    if (settled_ai && directive.kind == AiDirectiveKind::DefendHex) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = "settled_defense";
        }
        return ai_move_to_settled_defense_position(state, unit_id, directive.target, decision_trace);
    }

    if (directive.kind == AiDirectiveKind::DefendHex
        && hex_distance(unit->coord, directive.target) > 1) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = "defend_target";
        }
        return ai_move_toward_target(state, unit_id, directive.target, decision_trace);
    }

    const Unit* target = ai_target_for_directive(state, *unit, directive);
    if (directive.kind == AiDirectiveKind::DefendHex) {
        if (target == nullptr || hex_distance(target->coord, directive.target) > 2) {
            if (decision_trace != nullptr) {
                decision_trace->target = directive.target;
                decision_trace->action = "idle";
                decision_trace->reason = target == nullptr ? "no_defense_threat" : "threat_outside_defense_radius";
            }
            return false;
        }
    }
    if (target == nullptr) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = directive.kind == AiDirectiveKind::CaptureHex ? "capture_target" : "no_target";
        }
        return directive.kind == AiDirectiveKind::CaptureHex
            && ai_move_toward_target(state, unit_id, directive.target, decision_trace);
    }
    if (decision_trace != nullptr) {
        decision_trace->target = target->coord;
        decision_trace->target_unit_id = target->id;
        decision_trace->reason = "target_unit";
    }
    return ai_move_toward_target(state, unit_id, target->coord, decision_trace);
}

void execute_ai_unit_directive(
    GameState& state,
    int unit_id,
    const AiDirective& directive,
    AiAnimationStep* animation_step = nullptr,
    AiUnitDecisionTrace* decision_trace = nullptr
) {
    if (decision_trace != nullptr) {
        decision_trace->unit_id = unit_id;
        decision_trace->directive = directive;
        if (const Unit* unit = find_unit(state, unit_id)) {
            decision_trace->owner = unit->owner;
            decision_trace->from = unit->coord;
        }
        const std::vector<AttackableUnit> attacks = attackable_units(state, unit_id);
        for (const AttackableUnit& attack : attacks) {
            decision_trace->attackable_unit_ids.push_back(attack.unit_id);
        }
    }
    if (directive.kind == AiDirectiveKind::Inactive || directive.kind == AiDirectiveKind::HoldHex) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->action = "idle";
            decision_trace->reason = directive.kind == AiDirectiveKind::HoldHex ? "hold_hex_directive" : "inactive_directive";
        }
        return;
    }
    if (ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step)) {
        if (decision_trace != nullptr) {
            decision_trace->action = "attack";
            decision_trace->reason = "adjacent_enemy";
            if (animation_step != nullptr && !animation_step->attack_events.empty()) {
                const AiAnimationStep::AttackEvent& event = animation_step->attack_events.front();
                decision_trace->target = event.target;
                decision_trace->target_unit_id = event.defender_id;
            }
        }
        return;
    }
    const Unit* unit = find_unit(state, unit_id);
    if (unit == nullptr) {
        if (decision_trace != nullptr) {
            decision_trace->action = "idle";
            decision_trace->reason = "unit_not_found";
        }
        return;
    }

    const bool settled_ai = settled_archetype(faction_archetype(state, unit->owner));
    if (settled_ai
        && directive.kind == AiDirectiveKind::DefendHex
        && readiness_percent(*unit) < 55
        && hex_distance(unit->coord, directive.target) <= 1) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->action = "idle";
            decision_trace->reason = "recover_readiness_on_defense";
        }
        return;
    }
    if (settled_ai && directive.kind == AiDirectiveKind::DefendHex) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = "settled_defense";
        }
        ai_move_to_settled_defense_position(state, unit_id, directive.target, decision_trace);
        ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step);
        return;
    }

    if (directive.kind == AiDirectiveKind::DefendHex
        && hex_distance(unit->coord, directive.target) > 1) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = "defend_target";
        }
        ai_move_toward_target(state, unit_id, directive.target, decision_trace);
        ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step);
        return;
    }

    const Unit* target = ai_target_for_directive(state, *unit, directive);
    if (directive.kind == AiDirectiveKind::DefendHex) {
        if (target == nullptr || hex_distance(target->coord, directive.target) > 2) {
            if (decision_trace != nullptr) {
                decision_trace->target = directive.target;
                decision_trace->action = "idle";
                decision_trace->reason = target == nullptr ? "no_defense_threat" : "threat_outside_defense_radius";
            }
            return;
        }
    }
    if (target == nullptr) {
        if (decision_trace != nullptr) {
            decision_trace->target = directive.target;
            decision_trace->reason = directive.kind == AiDirectiveKind::CaptureHex ? "capture_target" : "no_target";
        }
        if (directive.kind == AiDirectiveKind::CaptureHex) {
            ai_move_toward_target(state, unit_id, directive.target, decision_trace);
            ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step);
        }
        return;
    }
    if (decision_trace != nullptr) {
        decision_trace->target = target->coord;
        decision_trace->target_unit_id = target->id;
        decision_trace->reason = "target_unit";
    }
    if (ai_move_toward_target(state, unit_id, target->coord, decision_trace)) {
        ai_attack_best_adjacent_enemy(state, unit_id, directive, animation_step);
    }
}

void execute_ai_units(
    GameState& state,
    const std::vector<int>& unit_ids,
    const AiDirective& directive,
    std::vector<AiAnimationStep>* animation = nullptr,
    std::vector<AiUnitDecisionTrace>* decision_trace = nullptr,
    int group_id = 0
) {
    for (const int unit_id : unit_ids) {
        const Unit* unit = find_unit(state, unit_id);
        if (unit == nullptr || !can_attack(unit->kind) || unit->hp <= 0) {
            if (decision_trace != nullptr) {
                AiUnitDecisionTrace trace;
                trace.unit_id = unit_id;
                trace.group_id = group_id;
                trace.directive = directive;
                trace.action = "skip";
                trace.reason = unit == nullptr ? "unit_not_found" : (!can_attack(unit->kind) ? "non_combat_unit" : "unit_destroyed");
                if (unit != nullptr) {
                    trace.owner = unit->owner;
                    trace.from = unit->coord;
                }
                decision_trace->push_back(std::move(trace));
            }
            continue;
        }
        AiAnimationStep step;
        step.unit_id = unit_id;
        step.from = unit->coord;
        step.to = unit->coord;
        AiUnitDecisionTrace trace;
        trace.group_id = group_id;
        execute_ai_unit_directive(
            state,
            unit_id,
            directive,
            animation == nullptr ? nullptr : &step,
            decision_trace == nullptr ? nullptr : &trace
        );
        if (decision_trace != nullptr) {
            decision_trace->push_back(std::move(trace));
        }
        const Unit* updated = find_unit(state, unit_id);
        if (updated != nullptr && step.attack_events.empty()) {
            step.to = updated->coord;
        }
        if (animation != nullptr
            && (!coord_equal(step.from, step.to) || !step.attacks.empty() || !step.attack_events.empty())) {
            populate_ai_animation_step_snapshot(state, step);
            animation->push_back(std::move(step));
        }
    }
}

struct AiExecutionBatch {
    std::vector<int> unit_ids;
    AiDirective directive;
    int group_id = 0;
};

bool step_ai_turn(GameState& state, AiAnimationStep* animation, std::vector<AiUnitDecisionTrace>* decision_trace) {
    const OwnerId owner = active_faction(state);
    if (!faction_ai_controlled(state, owner)) {
        if (decision_trace != nullptr) {
            AiUnitDecisionTrace trace;
            trace.owner = owner;
            trace.action = "skip";
            trace.reason = "active_faction_not_ai";
            decision_trace->push_back(std::move(trace));
        }
        return false;
    }

    refresh_generated_strategic_ai_group(state, owner);
    std::vector<int> assigned_unit_ids;
    std::vector<AiExecutionBatch> batches;
    for (const AiGroup& group : state.ai_groups) {
        if (group.owner != owner) {
            continue;
        }
        std::vector<int> group_units = group.unit_ids;
        std::sort(group_units.begin(), group_units.end());
        batches.push_back({group_units, directive_for_ai_group(state, group), group.id});
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
    if (!unassigned_unit_ids.empty()) {
        const Unit* anchor_unit = find_unit(state, unassigned_unit_ids.front());
        AiDirective directive;
        if (anchor_unit != nullptr && strategic_directive_for_owner(state, owner, anchor_unit->coord, directive)) {
            batches.push_back({unassigned_unit_ids, directive, 0});
        }
    }

    for (const AiExecutionBatch& batch : batches) {
        for (const int unit_id : batch.unit_ids) {
            const Unit* unit = find_unit(state, unit_id);
            if (unit == nullptr
                || unit->owner != owner
                || !can_attack(unit->kind)
                || unit->hp <= 0
                || (unit->move_done && unit->combat_done)) {
                if (decision_trace != nullptr) {
                    AiUnitDecisionTrace trace;
                    trace.unit_id = unit_id;
                    trace.owner = unit == nullptr ? owner : unit->owner;
                    trace.group_id = batch.group_id;
                    trace.directive = batch.directive;
                    trace.action = "skip";
                    if (unit == nullptr) {
                        trace.reason = "unit_not_found";
                    } else if (unit->owner != owner) {
                        trace.reason = "wrong_owner";
                    } else if (!can_attack(unit->kind)) {
                        trace.reason = "non_combat_unit";
                    } else if (unit->hp <= 0) {
                        trace.reason = "unit_destroyed";
                    } else {
                        trace.reason = "unit_done";
                    }
                    if (unit != nullptr) {
                        trace.from = unit->coord;
                    }
                    decision_trace->push_back(std::move(trace));
                }
                continue;
            }
            AiAnimationStep step;
            step.unit_id = unit_id;
            step.from = unit->coord;
            step.to = unit->coord;
            AiUnitDecisionTrace trace;
            trace.group_id = batch.group_id;
            const bool acted = execute_ai_unit_single_action(
                state,
                unit_id,
                batch.directive,
                animation == nullptr ? nullptr : &step,
                decision_trace == nullptr ? nullptr : &trace
            );
            if (decision_trace != nullptr) {
                decision_trace->push_back(std::move(trace));
            }
            if (!acted) {
                continue;
            }
            const Unit* updated = find_unit(state, unit_id);
            if (updated != nullptr && step.attack_events.empty()) {
                step.to = updated->coord;
            }
            if (animation != nullptr) {
                populate_ai_animation_step_snapshot(state, step);
                *animation = std::move(step);
            }
            state.selected_unit_id = 0;
            state.legal_moves.clear();
            state.legal_attacks.clear();
            state.legal_raids.clear();
            return true;
        }
    }

    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    state.legal_raids.clear();
    return false;
}

void execute_ai_turn(GameState& state, OwnerId owner, std::vector<AiAnimationStep>* animation = nullptr) {
    refresh_generated_strategic_ai_group(state, owner);
    std::vector<int> assigned_unit_ids;
    for (const AiGroup& group : state.ai_groups) {
        if (group.owner != owner) {
            continue;
        }
        std::vector<int> group_units = group.unit_ids;
        std::sort(group_units.begin(), group_units.end());
        execute_ai_units(state, group_units, directive_for_ai_group(state, group), animation, nullptr, group.id);
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
    if (!unassigned_unit_ids.empty()) {
        const Unit* anchor_unit = find_unit(state, unassigned_unit_ids.front());
        AiDirective directive;
        if (anchor_unit != nullptr && strategic_directive_for_owner(state, owner, anchor_unit->coord, directive)) {
            execute_ai_units(state, unassigned_unit_ids, directive, animation);
        }
    }
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    state.legal_raids.clear();
}

int turn_order_index_for_owner(const GameState& state, OwnerId owner) {
    for (std::size_t index = 0; index < state.turn_order.size(); ++index) {
        if (state.turn_order[index] == owner) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool execute_ai_group_turn(
    GameState& state,
    int group_id,
    std::vector<AiAnimationStep>* animation,
    std::vector<AiUnitDecisionTrace>* decision_trace
) {
    const auto found = std::find_if(state.ai_groups.begin(), state.ai_groups.end(), [&](const AiGroup& group) {
        return group.id == group_id;
    });
    if (found == state.ai_groups.end() || found->unit_ids.empty()) {
        return false;
    }

    const int owner_turn_index = turn_order_index_for_owner(state, found->owner);
    if (owner_turn_index < 0) {
        return false;
    }

    const int previous_turn_index = state.active_faction_index;
    state.active_faction_index = owner_turn_index;
    std::vector<int> group_units = found->unit_ids;
    std::sort(group_units.begin(), group_units.end());
    execute_ai_units(state, group_units, directive_for_ai_group(state, *found), animation, decision_trace, found->id);
    state.active_faction_index = previous_turn_index;
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    state.legal_raids.clear();
    return true;
}

bool replace_ai_groups_json(GameState& state, const std::string& ai_groups_json) {
    Json parsed;
    try {
        parsed = Json::parse(ai_groups_json);
    } catch (const Json::parse_error&) {
        return false;
    }
    if (!parsed.is_array()) {
        return false;
    }

    std::vector<AiGroup> groups = ai_groups_from_json(parsed);
    std::set<int> used_ids;
    for (AiGroup& group : groups) {
        if (group.id == 0 || used_ids.find(group.id) != used_ids.end()) {
            return false;
        }
        used_ids.insert(group.id);

        std::vector<int> valid_unit_ids;
        std::set<int> seen_unit_ids;
        for (const int unit_id : group.unit_ids) {
            if (seen_unit_ids.find(unit_id) != seen_unit_ids.end()) {
                continue;
            }
            const Unit* unit = find_unit(state, unit_id);
            if (unit == nullptr || unit->owner != group.owner) {
                continue;
            }
            seen_unit_ids.insert(unit_id);
            valid_unit_ids.push_back(unit_id);
        }
        group.unit_ids = std::move(valid_unit_ids);
    }

    state.ai_groups = std::move(groups);
    return true;
}

bool grazes_pasture(const Unit& unit) {
    return (unit.kind == UnitKind::Horde || unit.kind == UnitKind::Herd)
        && unit.hp > 0
        && (unit.horses > 0 || unit.livestock > 0);
}

std::vector<GameHex*> grazing_hexes_for_unit(GameState& state, const Unit& unit) {
    std::vector<GameHex*> hexes;
    for (GameHex& hex : state.hexes) {
        if (hex.pasture.capacity > 0.0 && hex_distance(unit.coord, hex.coord) <= pasture_grazing_radius) {
            hexes.push_back(&hex);
        }
    }
    std::sort(hexes.begin(), hexes.end(), [](const GameHex* first, const GameHex* second) {
        return coord_less(first->coord, second->coord);
    });
    return hexes;
}

void apply_pasture_for_round(GameState& state) {
    std::vector<Coord> grazed_coords;
    std::vector<Unit*> grazing_units;
    for (Unit& unit : state.units) {
        if (grazes_pasture(unit)) {
            grazing_units.push_back(&unit);
        }
    }
    std::sort(grazing_units.begin(), grazing_units.end(), [](const Unit* first, const Unit* second) {
        return first->id < second->id;
    });

    for (Unit* unit : grazing_units) {
        if (unit == nullptr || !grazes_pasture(*unit)) {
            continue;
        }
        std::vector<GameHex*> pasture_hexes = grazing_hexes_for_unit(state, *unit);
        for (const GameHex* hex : pasture_hexes) {
            grazed_coords.push_back(hex->coord);
        }

        const double demand = unit->horses * pasture_consumption_per_horse
            + unit->livestock * pasture_consumption_per_livestock;
        double consumed = 0.0;
        if (!pasture_hexes.empty()) {
            const double consumption_per_hex = demand / static_cast<double>(pasture_hexes.size());
            for (GameHex* hex : pasture_hexes) {
                const double actual_consumption = std::min(hex->pasture.remaining, consumption_per_hex);
                hex->pasture.remaining = std::max(0.0, hex->pasture.remaining - actual_consumption);
                consumed += actual_consumption;
            }
        }

        const double shortfall = std::max(0.0, demand - consumed);
        if (shortfall <= 0.0001) {
            unit->starvation_turns = 0;
            continue;
        }

        unit->starvation_turns += 1;
        if (unit->starvation_turns < starvation_horse_loss_turn_threshold) {
            continue;
        }
        const double numerator = unit->horses * shortfall * starvation_animal_loss_percent;
        const double denominator = std::max(1.0, demand * 100.0);
        if (unit->horses > 0) {
            const int horse_loss = std::min(
                unit->horses,
                std::max(1, static_cast<int>(std::ceil(numerator / denominator)))
            );
            unit->horses = std::max(0, unit->horses - horse_loss);
        }
        if (unit->livestock > 0) {
            const double livestock_numerator = unit->livestock * shortfall * starvation_animal_loss_percent;
            const int livestock_loss = std::min(
                unit->livestock,
                std::max(1, static_cast<int>(std::ceil(livestock_numerator / denominator)))
            );
            unit->livestock = std::max(0, unit->livestock - livestock_loss);
        }
        if (unit->horses == 0 && unit->livestock == 0) {
            unit->starvation_turns = 0;
        }
    }

    std::sort(grazed_coords.begin(), grazed_coords.end(), coord_less);
    grazed_coords.erase(std::unique(grazed_coords.begin(), grazed_coords.end(), coord_equal), grazed_coords.end());

    for (GameHex& hex : state.hexes) {
        if (hex.pasture.capacity <= 0.0) {
            continue;
        }
        const bool grazed = std::binary_search(grazed_coords.begin(), grazed_coords.end(), hex.coord, coord_less);
        if (!grazed) {
            hex.pasture.remaining = std::min(hex.pasture.capacity, hex.pasture.remaining + pasture_recovery_per_round);
        }
        hex.pasture.remaining = std::max(0.0, std::min(hex.pasture.capacity, hex.pasture.remaining));
    }
}

bool advance_to_next_faction(GameState& state) {
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }
    state.active_faction_index = (state.active_faction_index + 1) % static_cast<int>(state.turn_order.size());
    if (state.active_faction_index == 0) {
        state.round += 1;
        return true;
    }
    return false;
}

void end_turn(GameState& state, std::vector<AiAnimationStep>* /*animation*/) {
    state.selected_unit_id = 0;
    state.legal_moves.clear();
    state.legal_attacks.clear();
    state.legal_raids.clear();
    if (state.turn_order.empty()) {
        state.turn_order = {mongol_owner, chinese_owner};
    }

    const bool completed_round = advance_to_next_faction(state);
    if (completed_round) {
        apply_faction_food_for_round(state);
        apply_pasture_for_round(state);
    }
    start_faction_turn(state, active_faction(state));
    if (completed_round) {
        apply_faction_hunger_readiness_effects(state);
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

void print_river_edges_array_json(const std::vector<RiverEdge>& edges, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < edges.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "{\"a\":";
        print_coord_json(edges[i].a, out);
        out << ",\"b\":";
        print_coord_json(edges[i].b, out);
        out << ",\"river\":true}";
    }
    out << "]";
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
    out << "{\"capacity\":" << decimal_2(pasture.capacity)
        << ",\"remaining\":" << decimal_2(pasture.remaining)
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
            << ",\"livestock\":" << unit.livestock
            << ",\"starvationTurns\":" << unit.starvation_turns
            << ",\"productionState\":\"" << (unit.production_active ? "building" : "idle") << "\""
            << ",\"productionKind\":\"" << unit_kind_to_string(unit.production_kind) << "\""
            << ",\"productionTurnsRemaining\":" << unit.production_turns_remaining;
    } else if (unit.kind == UnitKind::Herd) {
        out << ",\"horses\":" << unit.horses
            << ",\"livestock\":" << unit.livestock
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

void print_animation_hexes_json(
    const std::vector<GameHex>& hexes,
    const std::vector<Coord>& supplied_hexes,
    std::ostream& out
) {
    out << "[";
    for (std::size_t i = 0; i < hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const GameHex& hex = hexes[i];
        const bool supplied = std::binary_search(supplied_hexes.begin(), supplied_hexes.end(), hex.coord, coord_less);
        out << "{\"q\":" << hex.coord.q
            << ",\"r\":" << hex.coord.r
            << ",\"owner\":" << hex.owner
            << ",\"supplySource\":" << (hex.supply_source ? "true" : "false")
            << ",\"supplied\":" << (supplied ? "true" : "false")
            << "}";
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

void print_coord_array_json(const std::vector<Coord>& coords, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < coords.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        print_coord_json(coords[i], out);
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

void print_crossings_array_json(const std::vector<Crossing>& crossings, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < crossings.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const Crossing& crossing = crossings[i];
        out << "{\"id\":" << crossing.id
            << ",\"kind\":\"" << escape_json(crossing.kind) << "\""
            << ",\"edge\":";
        print_edge_json(crossing.edge, out);
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
            << ",\"role\":\"" << escape_json(normalize_ai_group_role(group.role)) << "\""
            << ",\"generated\":" << (group.generated ? "true" : "false")
            << ",\"unitIds\":[";
        for (std::size_t unit_index = 0; unit_index < group.unit_ids.size(); ++unit_index) {
            if (unit_index > 0) {
                out << ",";
            }
            out << group.unit_ids[unit_index];
        }
        out << "],\"directive\":{\"type\":\"" << ai_directive_kind_to_string(group.directive.kind) << "\"";
        if (group.directive.kind == AiDirectiveKind::HoldHex
            || group.directive.kind == AiDirectiveKind::DefendHex
            || group.directive.kind == AiDirectiveKind::CaptureHex) {
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

void print_diplomacy_json(const std::vector<DiplomaticRelationship>& diplomacy, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < diplomacy.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const DiplomaticRelationship& relationship = diplomacy[i];
        const FactionState owner = faction_for_owner(relationship.owner);
        const FactionState target = faction_for_owner(relationship.target);
        out << "{\"owner\":" << relationship.owner
            << ",\"faction\":\"" << escape_json(owner.key) << "\""
            << ",\"target\":" << relationship.target
            << ",\"targetFaction\":\"" << escape_json(target.key) << "\""
            << ",\"status\":\"" << (relationship.at_war ? "war" : "peace") << "\""
            << ",\"disposition\":" << clamp_disposition(relationship.disposition)
            << ",\"fear\":" << clamp_fear(relationship.fear)
            << "}";
    }
    out << "]";
}

void hash_append(std::uint64_t& hash, const std::string& value) {
    for (const unsigned char ch : value) {
        hash ^= ch;
        hash *= 1099511628211ull;
    }
    hash ^= 0xff;
    hash *= 1099511628211ull;
}

void hash_append_int(std::uint64_t& hash, int value) {
    hash_append(hash, std::to_string(value));
}

void hash_append_bool(std::uint64_t& hash, bool value) {
    hash_append(hash, value ? "1" : "0");
}

std::string game_state_hash(const GameState& state) {
    std::uint64_t hash = 1469598103934665603ull;
    hash_append_int(hash, state.width);
    hash_append_int(hash, state.height);
    hash_append_int(hash, static_cast<int>(state.seed));
    hash_append_int(hash, state.round);
    hash_append_int(hash, state.active_faction_index);
    hash_append_int(hash, state.selected_unit_id);
    hash_append_bool(hash, state.food_consumption_enabled);
    for (const OwnerId owner : state.turn_order) {
        hash_append_int(hash, owner);
    }
    for (const ReachableHex& move : state.legal_moves) {
        hash_append_int(hash, move.coord.q);
        hash_append_int(hash, move.coord.r);
        hash_append_int(hash, move.scaled_cost);
    }
    for (const AttackableUnit& attack : state.legal_attacks) {
        hash_append_int(hash, attack.unit_id);
        hash_append_int(hash, attack.coord.q);
        hash_append_int(hash, attack.coord.r);
    }
    for (const Coord& raid : state.legal_raids) {
        hash_append_int(hash, raid.q);
        hash_append_int(hash, raid.r);
    }
    for (const GameHex& hex : state.hexes) {
        hash_append_int(hash, hex.coord.q);
        hash_append_int(hash, hex.coord.r);
        hash_append(hash, terrain_to_string(hex.terrain));
        hash_append(hash, hex.name);
        hash_append_int(hash, hex.owner);
        hash_append_bool(hash, hex.supply_source);
        hash_append(hash, decimal_2(hex.pasture.capacity));
        hash_append(hash, decimal_2(hex.pasture.remaining));
    }
    for (const FactionState& faction : state.factions) {
        hash_append_int(hash, faction.id);
        hash_append(hash, faction.key);
        hash_append_int(hash, faction.metal);
        hash_append_int(hash, faction.treasure);
        hash_append_int(hash, faction.food);
        hash_append_int(hash, faction.hunger);
        hash_append_bool(hash, faction.enabled);
        hash_append_bool(hash, faction.ai_controlled);
    }
    for (const DiplomaticRelationship& relationship : state.diplomacy) {
        hash_append_int(hash, relationship.owner);
        hash_append_int(hash, relationship.target);
        hash_append_bool(hash, relationship.at_war);
        hash_append_int(hash, clamp_disposition(relationship.disposition));
        hash_append_int(hash, clamp_fear(relationship.fear));
    }
    for (const AiGroup& group : state.ai_groups) {
        hash_append_int(hash, group.id);
        hash_append_int(hash, group.owner);
        hash_append(hash, group.name);
        hash_append(hash, normalize_ai_group_role(group.role));
        hash_append_bool(hash, group.generated);
        hash_append(hash, ai_directive_kind_to_string(group.directive.kind));
        hash_append_int(hash, group.directive.target.q);
        hash_append_int(hash, group.directive.target.r);
        hash_append_int(hash, group.directive.target_owner);
        for (const int unit_id : group.unit_ids) {
            hash_append_int(hash, unit_id);
        }
    }
    for (const Unit& unit : state.units) {
        hash_append_int(hash, unit.id);
        hash_append_int(hash, unit.owner);
        hash_append(hash, unit_kind_to_string(unit.kind));
        hash_append(hash, unit_stance_to_string(unit.stance));
        hash_append_int(hash, unit.coord.q);
        hash_append_int(hash, unit.coord.r);
        hash_append_int(hash, unit.hp);
        hash_append_int(hash, unit.readiness);
        hash_append_int(hash, unit.remaining_scaled_move);
        hash_append_int(hash, unit.population);
        hash_append_int(hash, unit.horses);
        hash_append_int(hash, unit.livestock);
        hash_append_int(hash, unit.starvation_turns);
        hash_append_bool(hash, unit.production_active);
        hash_append(hash, unit_kind_to_string(unit.production_kind));
        hash_append_int(hash, unit.production_turns_remaining);
        hash_append_bool(hash, unit.move_done);
        hash_append_bool(hash, unit.moved_this_turn);
        hash_append_bool(hash, unit.combat_done);
        hash_append_bool(hash, unit.contacted_enemy_this_turn);
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

void print_game_meta_json(const GameState& state, std::ostream& out) {
    out << "\"game\":{";
    out << "\"stateVersion\":" << state.state_version << ",";
    out << "\"stateHash\":\"" << game_state_hash(state) << "\",";
    out << "\"round\":" << state.round << ",";
    print_game_time_json(state.round, out);
    out << ",";
    out << "\"activeFactionIndex\":" << state.active_faction_index << ",";
    out << "\"activeOwner\":" << active_faction(state) << ",";
    out << "\"selectedUnitId\":" << state.selected_unit_id << ",";
    out << "\"foodConsumption\":" << (state.food_consumption_enabled ? "true" : "false") << ",";
    out << "\"legalMoves\":";
    print_reachable_array_json(state.legal_moves, out);
    out << ",\"legalAttacks\":";
    print_attackable_array_json(state.legal_attacks, out);
    out << ",\"legalRaids\":";
    print_coord_array_json(state.legal_raids, out);
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
            << ",\"food\":" << faction.food
            << ",\"hunger\":" << faction.hunger
            << ",\"enabled\":" << (faction.enabled ? "true" : "false")
            << ",\"ai\":" << (faction.ai_controlled ? "true" : "false")
            << "}";
    }
    out << "],\"aiGroups\":";
    print_ai_groups_json(state.ai_groups, out);
    out << ",\"diplomacy\":";
    print_diplomacy_json(state.diplomacy, out);
    out << "}";
}

void print_game_state_json(const GameState& state, std::ostream& out) {
    out << "{";
    out << "\"schema\":\"steppe-game.v1\",";
    out << "\"seed\":" << state.seed << ",";
    out << "\"width\":" << state.width << ",";
    out << "\"height\":" << state.height << ",";
    out << "\"hexes\":[";
    const auto supplied_hexes = supplied_hex_coords(state);
    for (std::size_t i = 0; i < state.hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const GameHex& hex = state.hexes[i];
        out << "{\"q\":" << hex.coord.q
            << ",\"r\":" << hex.coord.r
            << ",\"terrain\":\"" << terrain_to_string(hex.terrain) << "\""
            << ",\"name\":\"" << escape_json(hex.name.empty() ? "None" : hex.name) << "\""
            << ",\"owner\":" << hex.owner
            << ",\"supplySource\":" << (hex.supply_source ? "true" : "false")
            << ",\"supplied\":" << (supplied_hexes.find(hex.coord) != supplied_hexes.end() ? "true" : "false")
            << ",\"labels\":";
        print_string_array_json(hex.source_labels, out);
        out << ",\"pasture\":";
        print_pasture_json(hex.pasture, out);
        out << "}";
    }
    out << "],\"towns\":[],\"river_sources\":[],\"river_destinations\":[],\"merge_points\":[],";
    out << "\"river_segments\":[],\"edges\":";
    print_river_edges_array_json(state.rivers, out);
    out << ",\"lake_river_connections\":[],\"roads\":";
    print_roads_array_json(state.roads, out);
    out << ",\"walls\":";
    print_walls_array_json(state.walls, out);
    out << ",\"wall_gates\":";
    print_wall_gates_array_json(state.wall_gates, out);
    out << ",\"crossings\":";
    print_crossings_array_json(state.crossings, out);
    out << ",";
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
        << ",\"livestock\":" << options.livestock
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

void print_ai_animation_json(const std::vector<AiAnimationStep>& animation, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < animation.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const AiAnimationStep& step = animation[i];
        out << "{\"unitId\":" << step.unit_id << ",\"from\":";
        print_coord_json(step.from, out);
        out << ",\"to\":";
        print_coord_json(step.to, out);
        out << ",\"hexes\":";
        print_animation_hexes_json(step.hexes, step.supplied_hexes, out);
        out << ",\"units\":";
        print_units_json(step.units, out);
        out << ",\"attacks\":[";
        for (std::size_t attack_index = 0; attack_index < step.attacks.size(); ++attack_index) {
            if (attack_index > 0) {
                out << ",";
            }
            print_coord_json(step.attacks[attack_index], out);
        }
        out << "],\"attackEvents\":[";
        for (std::size_t event_index = 0; event_index < step.attack_events.size(); ++event_index) {
            if (event_index > 0) {
                out << ",";
            }
            const AiAnimationStep::AttackEvent& event = step.attack_events[event_index];
            out << "{\"target\":";
            print_coord_json(event.target, out);
            out << ",\"defenderId\":" << event.defender_id
                << ",\"defenderFrom\":";
            print_coord_json(event.defender_from, out);
            out << ",\"defenderTo\":";
            print_coord_json(event.defender_to, out);
            out << ",\"defenderMoved\":" << (event.defender_moved ? "true" : "false")
                << ",\"attackerTo\":";
            print_coord_json(event.attacker_to, out);
            out << ",\"attackerMoved\":" << (event.attacker_moved ? "true" : "false")
                << "}";
        }
        out << "]}";
    }
    out << "]";
}

void print_ai_directive_json(const AiDirective& directive, std::ostream& out) {
    out << "{\"type\":\"" << ai_directive_kind_to_string(directive.kind) << "\"";
    if (directive.kind == AiDirectiveKind::HoldHex
        || directive.kind == AiDirectiveKind::DefendHex
        || directive.kind == AiDirectiveKind::CaptureHex) {
        out << ",\"target\":";
        print_coord_json(directive.target, out);
    }
    if (directive.kind == AiDirectiveKind::HuntHorde) {
        const FactionState faction = faction_for_owner(directive.target_owner);
        out << ",\"faction\":\"" << escape_json(faction.key) << "\""
            << ",\"owner\":" << directive.target_owner;
    }
    out << "}";
}

void print_ai_decision_trace_json(const std::vector<AiUnitDecisionTrace>& trace, std::ostream& out) {
    out << "[";
    for (std::size_t i = 0; i < trace.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const AiUnitDecisionTrace& decision = trace[i];
        out << "{\"unitId\":" << decision.unit_id
            << ",\"owner\":" << decision.owner
            << ",\"groupId\":" << decision.group_id
            << ",\"directive\":";
        print_ai_directive_json(decision.directive, out);
        out << ",\"from\":";
        print_coord_json(decision.from, out);
        out << ",\"target\":";
        print_coord_json(decision.target, out);
        out << ",\"targetUnitId\":" << decision.target_unit_id
            << ",\"action\":\"" << escape_json(decision.action) << "\""
            << ",\"reason\":\"" << escape_json(decision.reason) << "\""
            << ",\"attackableUnitIds\":[";
        for (std::size_t attack_index = 0; attack_index < decision.attackable_unit_ids.size(); ++attack_index) {
            if (attack_index > 0) {
                out << ",";
            }
            out << decision.attackable_unit_ids[attack_index];
        }
        out << "],\"pathCandidates\":[";
        for (std::size_t candidate_index = 0; candidate_index < decision.path_candidates.size(); ++candidate_index) {
            if (candidate_index > 0) {
                out << ",";
            }
            const AiPathCandidateTrace& candidate = decision.path_candidates[candidate_index];
            out << "{\"coord\":";
            print_coord_json(candidate.coord, out);
            out << ",\"scaledCost\":" << candidate.scaled_cost
                << ",\"distanceBefore\":" << candidate.distance_before
                << ",\"distanceAfter\":" << candidate.distance_after
                << ",\"distanceGain\":" << candidate.distance_gain
                << ",\"score\":" << candidate.score
                << ",\"selected\":" << (candidate.selected ? "true" : "false")
                << ",\"path\":[";
            for (std::size_t path_index = 0; path_index < candidate.path.size(); ++path_index) {
                if (path_index > 0) {
                    out << ",";
                }
                print_coord_json(candidate.path[path_index], out);
            }
            out << "]}";
        }
        out << "]}";
    }
    out << "]";
}

void print_game_events_json(const GameState& state, const GameState* before, std::ostream& out) {
    out << "[";
    if (before == nullptr) {
        out << "]";
        return;
    }

    bool first_event = true;
    const auto event_prefix = [&]() {
        if (!first_event) {
            out << ",";
        }
        first_event = false;
    };

    if (before->round != state.round || active_faction(*before) != active_faction(state)) {
        event_prefix();
        out << "{\"type\":\"turn_changed\""
            << ",\"fromRound\":" << before->round
            << ",\"toRound\":" << state.round
            << ",\"fromOwner\":" << active_faction(*before)
            << ",\"toOwner\":" << active_faction(state)
            << "}";
    }

    if (before->selected_unit_id != state.selected_unit_id) {
        event_prefix();
        out << "{\"type\":\"unit_selected\""
            << ",\"fromUnitId\":" << before->selected_unit_id
            << ",\"toUnitId\":" << state.selected_unit_id
            << "}";
    }

    std::map<int, Unit> before_units;
    for (const Unit& unit : before->units) {
        before_units[unit.id] = unit;
    }
    std::set<int> after_unit_ids;
    for (const Unit& unit : state.units) {
        after_unit_ids.insert(unit.id);
        const auto found = before_units.find(unit.id);
        if (found == before_units.end()) {
            event_prefix();
            out << "{\"type\":\"unit_created\",\"unitId\":" << unit.id << ",\"to\":";
            print_coord_json(unit.coord, out);
            out << ",\"owner\":" << unit.owner
                << ",\"kind\":\"" << unit_kind_to_string(unit.kind) << "\""
                << "}";
            continue;
        }
        const Unit& prior = found->second;
        if (!coord_equal(prior.coord, unit.coord)) {
            event_prefix();
            out << "{\"type\":\"unit_moved\",\"unitId\":" << unit.id << ",\"from\":";
            print_coord_json(prior.coord, out);
            out << ",\"to\":";
            print_coord_json(unit.coord, out);
            out << "}";
        }
        if (prior.owner != unit.owner) {
            event_prefix();
            out << "{\"type\":\"unit_owner_changed\",\"unitId\":" << unit.id
                << ",\"fromOwner\":" << prior.owner
                << ",\"toOwner\":" << unit.owner
                << "}";
        }
        if (prior.hp != unit.hp
            || prior.readiness != unit.readiness
            || prior.remaining_scaled_move != unit.remaining_scaled_move
            || prior.move_done != unit.move_done
            || prior.combat_done != unit.combat_done
            || prior.moved_this_turn != unit.moved_this_turn) {
            event_prefix();
            out << "{\"type\":\"unit_status_changed\",\"unitId\":" << unit.id
                << ",\"fromHp\":" << prior.hp
                << ",\"toHp\":" << unit.hp
                << ",\"fromReadiness\":" << prior.readiness
                << ",\"toReadiness\":" << unit.readiness
                << ",\"fromRemainingScaledMove\":" << prior.remaining_scaled_move
                << ",\"toRemainingScaledMove\":" << unit.remaining_scaled_move
                << "}";
        }
    }
    for (const Unit& unit : before->units) {
        if (after_unit_ids.find(unit.id) != after_unit_ids.end()) {
            continue;
        }
        event_prefix();
        out << "{\"type\":\"unit_removed\",\"unitId\":" << unit.id << ",\"from\":";
        print_coord_json(unit.coord, out);
        out << ",\"owner\":" << unit.owner
            << ",\"kind\":\"" << unit_kind_to_string(unit.kind) << "\""
            << "}";
    }

    std::map<Coord, GameHex, decltype(coord_less)*> before_hexes(coord_less);
    for (const GameHex& hex : before->hexes) {
        before_hexes[hex.coord] = hex;
    }
    const auto before_supplied = supplied_hex_coords(*before);
    const auto after_supplied = supplied_hex_coords(state);
    for (const GameHex& hex : state.hexes) {
        const auto found = before_hexes.find(hex.coord);
        if (found == before_hexes.end()) {
            continue;
        }
        const GameHex& prior = found->second;
        if (prior.owner != hex.owner) {
            event_prefix();
            out << "{\"type\":\"hex_owner_changed\",\"hex\":";
            print_coord_json(hex.coord, out);
            out << ",\"fromOwner\":" << prior.owner
                << ",\"toOwner\":" << hex.owner
                << "}";
        }
        if (prior.terrain != hex.terrain) {
            event_prefix();
            out << "{\"type\":\"hex_terrain_changed\",\"hex\":";
            print_coord_json(hex.coord, out);
            out << ",\"fromTerrain\":\"" << terrain_to_string(prior.terrain) << "\""
                << ",\"toTerrain\":\"" << terrain_to_string(hex.terrain) << "\""
                << "}";
        }
        if (prior.supply_source != hex.supply_source) {
            event_prefix();
            out << "{\"type\":\"hex_supply_source_changed\",\"hex\":";
            print_coord_json(hex.coord, out);
            out << ",\"from\":" << (prior.supply_source ? "true" : "false")
                << ",\"to\":" << (hex.supply_source ? "true" : "false")
                << "}";
        }
        const bool prior_supplied = before_supplied.find(hex.coord) != before_supplied.end();
        const bool now_supplied = after_supplied.find(hex.coord) != after_supplied.end();
        if (prior_supplied != now_supplied) {
            event_prefix();
            out << "{\"type\":\"hex_supply_changed\",\"hex\":";
            print_coord_json(hex.coord, out);
            out << ",\"from\":" << (prior_supplied ? "true" : "false")
                << ",\"to\":" << (now_supplied ? "true" : "false")
                << "}";
        }
    }

    std::map<OwnerId, FactionState> before_factions;
    for (const FactionState& faction : before->factions) {
        before_factions[faction.id] = faction;
    }
    for (const FactionState& faction : state.factions) {
        const auto found = before_factions.find(faction.id);
        if (found == before_factions.end()) {
            continue;
        }
        const FactionState& prior = found->second;
        if (prior.metal != faction.metal
            || prior.treasure != faction.treasure
            || prior.food != faction.food
            || prior.hunger != faction.hunger) {
            event_prefix();
            out << "{\"type\":\"faction_resources_changed\",\"owner\":" << faction.id
                << ",\"fromMetal\":" << prior.metal
                << ",\"toMetal\":" << faction.metal
                << ",\"fromTreasure\":" << prior.treasure
                << ",\"toTreasure\":" << faction.treasure
                << ",\"fromFood\":" << prior.food
                << ",\"toFood\":" << faction.food
                << ",\"fromHunger\":" << prior.hunger
                << ",\"toHunger\":" << faction.hunger
                << "}";
        }
    }

    out << "]";
}

void print_game_patch_json(
    const GameState& state,
    bool ok,
    std::ostream& out,
    const std::vector<AiAnimationStep>* animation,
    const GameState* before,
    const std::vector<AiUnitDecisionTrace>* decision_trace
) {
    out << "{";
    out << "\"ok\":" << (ok ? "true" : "false") << ",";
    out << "\"schema\":\"steppe-game.v1\",";
    out << "\"seed\":" << state.seed << ",";
    out << "\"width\":" << state.width << ",";
    out << "\"height\":" << state.height << ",";
    out << "\"hexes\":[";
    const auto supplied_hexes = supplied_hex_coords(state);
    for (std::size_t i = 0; i < state.hexes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const GameHex& hex = state.hexes[i];
        out << "{\"q\":" << hex.coord.q
            << ",\"r\":" << hex.coord.r
            << ",\"terrain\":\"" << terrain_to_string(hex.terrain) << "\""
            << ",\"name\":\"" << escape_json(hex.name.empty() ? "None" : hex.name) << "\""
            << ",\"owner\":" << hex.owner
            << ",\"supplySource\":" << (hex.supply_source ? "true" : "false")
            << ",\"supplied\":" << (supplied_hexes.find(hex.coord) != supplied_hexes.end() ? "true" : "false")
            << ",\"labels\":";
        print_string_array_json(hex.source_labels, out);
        out << ",\"pasture\":";
        print_pasture_json(hex.pasture, out);
        out << "}";
    }
    out << "],\"towns\":[],\"river_sources\":[],\"river_destinations\":[],\"merge_points\":[],";
    out << "\"river_segments\":[],\"edges\":";
    print_river_edges_array_json(state.rivers, out);
    out << ",\"lake_river_connections\":[],\"roads\":";
    print_roads_array_json(state.roads, out);
    out << ",\"walls\":";
    print_walls_array_json(state.walls, out);
    out << ",\"wall_gates\":";
    print_wall_gates_array_json(state.wall_gates, out);
    out << ",\"crossings\":";
    print_crossings_array_json(state.crossings, out);
    out << ",";
    out << "\"units\":";
    print_units_json(state.units, out);
    out << ",";
    print_game_meta_json(state, out);
    out << ",\"events\":";
    print_game_events_json(state, before, out);
    if (animation != nullptr) {
        out << ",\"aiAnimation\":";
        print_ai_animation_json(*animation, out);
    }
    if (decision_trace != nullptr) {
        out << ",\"aiDecisionTrace\":";
        print_ai_decision_trace_json(*decision_trace, out);
    }
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
    state.state_version = int_field(game_meta, "stateVersion", int_field(root, "stateVersion", 0));
    state.round = int_field(game_meta, "round", 1);
    state.active_faction_index = int_field(game_meta, "activeFactionIndex", 0);
    state.selected_unit_id = int_field(game_meta, "selectedUnitId", 0);
    state.food_consumption_enabled = bool_field(game_meta, "foodConsumption", true);
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
            faction.food = std::max(0, int_field(faction_json, "food", faction.food));
            faction.hunger = std::max(0, int_field(faction_json, "hunger", faction.hunger));
            faction.enabled = bool_field(faction_json, "enabled", true);
            faction.ai_controlled = bool_field(faction_json, "ai", false);
            state.factions.push_back(std::move(faction));
        }
    }

    const Json& ai_group_items = game_json.contains("aiGroups") && game_json["aiGroups"].is_array()
        ? game_json["aiGroups"]
        : empty_array;
    state.ai_groups = ai_groups_from_json(ai_group_items);

    const Json& diplomacy_items = game_json.contains("diplomacy") && game_json["diplomacy"].is_array()
        ? game_json["diplomacy"]
        : empty_array;
    for (const Json& diplomacy_json : diplomacy_items) {
        if (!diplomacy_json.is_object()) {
            continue;
        }
        DiplomaticRelationship relationship;
        relationship.owner = int_field(
            diplomacy_json,
            "owner",
            owner_from_faction_key(string_field(diplomacy_json, "faction", ""), neutral_owner)
        );
        relationship.target = int_field(
            diplomacy_json,
            "target",
            owner_from_faction_key(string_field(diplomacy_json, "targetFaction", ""), neutral_owner)
        );
        const bool has_explicit_status = diplomacy_json.contains("status") && diplomacy_json["status"].is_string();
        if (diplomacy_json.contains("disposition") && diplomacy_json["disposition"].is_number_integer()) {
            relationship.disposition = clamp_disposition(diplomacy_json["disposition"].get<int>());
        } else {
            int disposition = int_field(diplomacy_json, "affinity", 50);
            const std::string legacy_status = string_field(diplomacy_json, "status", "war") == "peace" ? "peace" : "war";
            if (legacy_status == "war") {
                disposition = std::min(disposition, 25);
            } else {
                disposition = std::max(disposition, 50);
            }
            const std::string legacy_posture = string_field(diplomacy_json, "aiPosture", "balanced");
            if (legacy_posture == "raiding") {
                disposition = std::min(disposition, 10);
            } else if (legacy_posture == "aggressive") {
                disposition = std::min(disposition, 20);
            } else if (legacy_posture == "defensive") {
                disposition = std::min(disposition, 35);
            } else if (legacy_posture == "passive") {
                disposition = std::max(disposition, 60);
            }
            relationship.disposition = clamp_disposition(disposition);
        }
        relationship.fear = clamp_fear(int_field(diplomacy_json, "fear", 25));
        relationship.at_war = has_explicit_status
            ? string_field(diplomacy_json, "status", "war") != "peace"
            : hostile_relationship(relationship.disposition, relationship.fear);
        state.diplomacy.push_back(std::move(relationship));
    }
    normalize_diplomacy(state);

    bool saw_explicit_hex_owner = false;
    bool saw_explicit_supply_source = false;
    const Json& hex_items = root.contains("hexes") && root["hexes"].is_array() ? root["hexes"] : empty_array;
    for (const Json& hex_json : hex_items) {
        if (!hex_json.is_object()) {
            continue;
        }
        GameHex hex;
        hex.coord = coord_from_json(hex_json);
        hex.terrain = terrain_from_string(string_field(hex_json, "terrain", "none"));
        hex.name = string_field(hex_json, "name", "None");
        if (hex.name.empty()) {
            hex.name = "None";
        }
        hex.tags = hex.terrain == Terrain::Grassland ? tag_mask(HexTag::BaseSteppe) : 0;
        hex.pasture = initial_pasture_for_terrain(hex.terrain);
        if (hex_json.contains("pasture") && hex_json["pasture"].is_object()) {
            const Json& pasture_json = hex_json["pasture"];
            hex.pasture.capacity = std::max(
                0.0,
                std::min(pasture_capacity_grassland, number_field(pasture_json, "capacity", hex.pasture.capacity))
            );
            hex.pasture.remaining = std::max(
                0.0,
                std::min(hex.pasture.capacity, number_field(pasture_json, "remaining", hex.pasture.remaining))
            );
        }
        hex.source_labels = string_array_field(hex_json, "labels");
        if (!hex.source_labels.empty()) {
            hex.tags = tags_from_labels(hex.source_labels);
        } else if (hex.terrain == Terrain::Grassland) {
            hex.source_labels = {"base_steppe"};
        }
        if (hex_json.contains("owner") && hex_json["owner"].is_number_integer()) {
            hex.owner = hex_json["owner"].get<int>();
            saw_explicit_hex_owner = true;
        } else if (has_tag(hex.tags, HexTag::PersianTown)) {
            hex.owner = persian_owner;
        } else if (has_tag(hex.tags, HexTag::ChineseTown)) {
            hex.owner = chinese_owner;
        }
        if (hex_json.contains("supplySource") && hex_json["supplySource"].is_boolean()) {
            hex.supply_source = hex_json["supplySource"].get<bool>();
            saw_explicit_supply_source = true;
        } else if (hex_json.contains("supply_source") && hex_json["supply_source"].is_boolean()) {
            hex.supply_source = hex_json["supply_source"].get<bool>();
            saw_explicit_supply_source = true;
        }
        state.hexes.push_back(std::move(hex));
    }

    const Json& town_items = root.contains("towns") && root["towns"].is_array() ? root["towns"] : empty_array;
    int next_settlement_id = 1;
    for (const Json& town_json : town_items) {
        if (!town_json.is_object()) {
            continue;
        }
        Town town;
        town.coord = coord_from_json(town_json);
        town.feature = string_field(town_json, "feature", "");
        town.labels = string_array_field(town_json, "labels");
        Settlement settlement;
        settlement.id = next_settlement_id++;
        settlement.coord = town.coord;
        settlement.kind = settlement_kind_from_town(town);
        settlement.source_feature = town.feature;
        settlement.source_labels = town.labels;
        state.settlements.push_back(std::move(settlement));
    }
    if (state.settlements.empty()) {
        for (const GameHex& hex : state.hexes) {
            Town town;
            town.coord = hex.coord;
            town.labels = hex.source_labels;
            if (has_tag(hex.tags, HexTag::PersianTown)) {
                town.feature = "persian_town";
            } else if (has_tag(hex.tags, HexTag::ChineseTown)) {
                town.feature = "chinese_town";
            } else {
                continue;
            }
            Settlement settlement;
            settlement.id = next_settlement_id++;
            settlement.coord = town.coord;
            settlement.kind = settlement_kind_from_town(town);
            settlement.source_feature = town.feature;
            settlement.source_labels = town.labels;
            state.settlements.push_back(std::move(settlement));
        }
    }
    if (!saw_explicit_hex_owner) {
        seed_initial_territory(state);
    }
    if (!saw_explicit_supply_source) {
        for (const Settlement& settlement : state.settlements) {
            if (!settled_owner(settlement_hex_owner(state, settlement))) {
                continue;
            }
            GameHex* hex = find_hex(state, settlement.coord);
            if (hex != nullptr && territory_claimable_terrain(hex->terrain)) {
                hex->supply_source = true;
            }
        }
    }

    const Json& river_edge_items = root.contains("edges") && root["edges"].is_array() ? root["edges"] : empty_array;
    for (const Json& edge_json : river_edge_items) {
        if (!edge_json.is_object()
            || !edge_json.contains("a")
            || !edge_json["a"].is_object()
            || !edge_json.contains("b")
            || !edge_json["b"].is_object()) {
            continue;
        }
        state.rivers.push_back(canonical_edge(coord_from_json(edge_json["a"]), coord_from_json(edge_json["b"])));
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

    const Json& crossing_items = root.contains("crossings") && root["crossings"].is_array() ? root["crossings"] : empty_array;
    for (const Json& crossing_json : crossing_items) {
        if (!crossing_json.is_object()) {
            continue;
        }
        const Json& edge_json = crossing_json.contains("edge") && crossing_json["edge"].is_object()
            ? crossing_json["edge"]
            : empty_object;
        if (!edge_json.contains("a")
            || !edge_json["a"].is_object()
            || !edge_json.contains("b")
            || !edge_json["b"].is_object()) {
            continue;
        }
        Crossing crossing;
        crossing.id = int_field(crossing_json, "id", 0);
        crossing.kind = string_field(crossing_json, "kind", "ford") == "bridge" ? "bridge" : "ford";
        crossing.edge = canonical_edge(coord_from_json(edge_json["a"]), coord_from_json(edge_json["b"]));
        state.crossings.push_back(std::move(crossing));
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
            } else if (faction == "jurchen") {
                unit.owner = jurchen_owner;
            } else if (faction == "forest_nomad") {
                unit.owner = forest_nomad_owner;
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
        unit.livestock = std::max(0, int_field(unit_json, "livestock", defaults.livestock));
        if (unit.kind == UnitKind::Horde) {
            unit.horses = std::min(unit.horses, horde_horse_capacity);
        }
        if (unit.kind != UnitKind::Horde && unit.kind != UnitKind::Herd) {
            unit.livestock = 0;
        }
        unit.livestock = std::min(unit.livestock, livestock_unit_capacity);
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
