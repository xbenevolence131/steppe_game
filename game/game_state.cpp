#include "game_state.h"

#include <algorithm>

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
    state.crossings = generated.crossings;
    state.clans = {
        {neutral_owner, "Neutral"},
        {persian_owner, "Persian"},
        {chinese_owner, "Chinese"},
    };

    return state;
}

GameState generate_game_state(const GenerateArgs& args) {
    return game_state_from_generated_map(generate_map(args));
}

} // namespace steppe::game
