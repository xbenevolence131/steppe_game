#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace steppe {

struct GenerateArgs {
    int width = 120;
    int height = 80;
    int river_count = 4;
    int lake_count = 4;
    int lake_size = 6;
    double meander_forward = 8.0;
    double meander_forward_jitter = 4.0;
    double meander_lateral = 7.0;
    double meander_lateral_jitter = 4.0;
    double meander_strength = 1.0;
    double meander_reach = 2.0;
    double river_slant_strength = 10.0;
    double valley_thickness = 2.0;
    int forest_blob_count = 4;
    double forest_blob_radius = 4.0;
    int meander_timeout = 28;
    std::uint32_t seed = 1;
};

struct Coord {
    int q = 0;
    int r = 0;
};

enum class Terrain {
    None,
    Grassland,
    Farmland,
    Lake,
    Hill,
    Mountain,
    Forest,
    Marsh,
    Desert,
    Urban,
};

struct Hex {
    Coord coord;
    Terrain terrain = Terrain::None;
    std::vector<std::string> labels;
};

struct VertexKey {
    long long x = 0;
    long long y = 0;
};

struct RiverEdge {
    Coord a;
    Coord b;
};

struct RiverSegment {
    int id = 0;
    Coord from;
    Coord to;
    std::string kind;
    std::vector<RiverEdge> edge_path;
};

struct LakeRiverConnection {
    int id = 0;
    int river_component = 0;
    int terminal_index = 0;
    VertexKey vertex;
    std::vector<Coord> lake_hexes;
    RiverEdge river_edge;
};

struct LakeNetworkOverlay {
    bool placed = false;
    int template_id = 0;
    Coord anchor;
    std::vector<Coord> lake_hexes;
    std::vector<RiverEdge> edges;
};

struct Town {
    Coord coord;
    std::string feature;
    std::vector<std::string> labels;
};

struct Road {
    int id = 0;
    std::string feature;
    std::vector<Coord> path;
};

struct Crossing {
    int id = 0;
    std::string kind;
    RiverEdge edge;
};

struct Wall {
    int id = 0;
    std::string feature;
    std::vector<RiverEdge> edge_path;
};

struct WallGate {
    int id = 0;
    std::string kind;
    RiverEdge edge;
};

struct GeneratedMapMetadata {
    std::string generator = "prototype-steppe-blob";
    std::vector<Terrain> terrain_types;
    std::string hex_label_model = "final-semantic-labels.v2";
    std::string lake_river_connection_model = "river-terminal-lake-vertex.v1";
    std::string crossing_model = "bridges-and-spaced-fords.v1";
    LakeNetworkOverlay chinese_lake_network;
};

struct GeneratedMap {
    std::string schema = "steppe-terrain.v1";
    std::uint32_t seed = 1;
    int width = 0;
    int height = 0;
    std::vector<Hex> hexes;
    std::vector<Town> towns;
    std::vector<Coord> river_sources;
    std::vector<Coord> river_destinations;
    std::vector<Coord> merge_points;
    std::vector<RiverSegment> river_segments;
    std::vector<RiverEdge> edges;
    std::vector<LakeRiverConnection> lake_river_connections;
    std::vector<Road> roads;
    std::vector<Wall> walls;
    std::vector<WallGate> wall_gates;
    std::vector<Crossing> crossings;
    GeneratedMapMetadata metadata;
};

GeneratedMap generate_map(const GenerateArgs& args);
void print_generated_map_json(const GeneratedMap& map);

} // namespace steppe
