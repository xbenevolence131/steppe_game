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

GenerateArgs parse_generate_args(int argc, char** argv);
void print_generated_map(const GenerateArgs& args);
void print_usage();

} // namespace steppe
