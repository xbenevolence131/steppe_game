#include "steppe_generator.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace steppe {

struct BoundaryEdge {
    RiverEdge edge;
    VertexKey first;
    VertexKey second;
};

struct RiverNetwork {
    std::vector<Coord> sources;
    std::vector<Coord> destinations;
    std::vector<Coord> merge_points;
    std::vector<RiverSegment> segments;
    std::vector<RiverEdge> edges;
};

struct LakeCandidate {
    RiverEdge edge;
    double score = 0.0;
};

struct LakeNetworkTemplate {
    int id = 0;
    int width = 0;
    int height = 0;
    Coord anchor;
    std::vector<Coord> lake_hexes;
    std::vector<RiverEdge> edges;
};

struct RiverHead {
    int id = 0;
    Coord source;
    int current_edge = -1;
    VertexKey current_vertex;
    std::vector<RiverEdge> edge_path;
    std::set<VertexKey> visited_vertices;
    bool active = true;
    bool merged = false;
    Coord endpoint;
    bool has_meander_target = false;
    int next_meander_side = 1;
    double meander_target_x = 0.0;
    double meander_target_y = 0.0;
    int meander_steps = 0;
    int slant_direction = 1;
};

int parse_positive_int(const std::string& value, const std::string& name) {
    std::size_t parsed = 0;
    int result = 0;

    try {
        result = std::stoi(value, &parsed, 10);
    } catch (const std::exception&) {
        throw std::runtime_error(name + " must be a positive integer");
    }

    if (parsed != value.size() || result <= 0) {
        throw std::runtime_error(name + " must be a positive integer");
    }

    return result;
}

int parse_non_negative_int(const std::string& value, const std::string& name) {
    std::size_t parsed = 0;
    int result = 0;

    try {
        result = std::stoi(value, &parsed, 10);
    } catch (const std::exception&) {
        throw std::runtime_error(name + " must be a non-negative integer");
    }

    if (parsed != value.size() || result < 0) {
        throw std::runtime_error(name + " must be a non-negative integer");
    }

    return result;
}

double parse_non_negative_double(const std::string& value, const std::string& name) {
    std::size_t parsed = 0;
    double result = 0.0;

    try {
        result = std::stod(value, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error(name + " must be a non-negative number");
    }

    if (parsed != value.size() || result < 0.0) {
        throw std::runtime_error(name + " must be a non-negative number");
    }

    return result;
}

std::uint32_t parse_seed(const std::string& value) {
    std::size_t parsed = 0;
    unsigned long result = 0;

    try {
        result = std::stoul(value, &parsed, 10);
    } catch (const std::exception&) {
        throw std::runtime_error("seed must be an unsigned integer");
    }

    if (parsed != value.size() || result > 0xffffffffUL) {
        throw std::runtime_error("seed must be an unsigned integer");
    }

    return static_cast<std::uint32_t>(result);
}

GenerateArgs parse_generate_args(int argc, char** argv) {
    GenerateArgs args;

    for (int i = 2; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--width" && i + 1 < argc) {
            args.width = parse_positive_int(argv[++i], "width");
        } else if (key == "--height" && i + 1 < argc) {
            args.height = parse_positive_int(argv[++i], "height");
        } else if (key == "--seed" && i + 1 < argc) {
            args.seed = parse_seed(argv[++i]);
        } else if ((key == "--rivers" || key == "--river-sources") && i + 1 < argc) {
            args.river_count = parse_non_negative_int(argv[++i], "rivers");
        } else if (key == "--lakes" && i + 1 < argc) {
            args.lake_count = parse_non_negative_int(argv[++i], "lakes");
        } else if (key == "--lake-size" && i + 1 < argc) {
            args.lake_size = parse_positive_int(argv[++i], "lake size");
        } else if (key == "--meander-forward" && i + 1 < argc) {
            args.meander_forward = parse_non_negative_double(argv[++i], "meander forward");
        } else if (key == "--meander-forward-jitter" && i + 1 < argc) {
            args.meander_forward_jitter = parse_non_negative_double(argv[++i], "meander forward jitter");
        } else if (key == "--meander-lateral" && i + 1 < argc) {
            args.meander_lateral = parse_non_negative_double(argv[++i], "meander lateral");
        } else if (key == "--meander-lateral-jitter" && i + 1 < argc) {
            args.meander_lateral_jitter = parse_non_negative_double(argv[++i], "meander lateral jitter");
        } else if (key == "--meander-strength" && i + 1 < argc) {
            args.meander_strength = parse_non_negative_double(argv[++i], "meander strength");
        } else if (key == "--meander-reach" && i + 1 < argc) {
            args.meander_reach = parse_non_negative_double(argv[++i], "meander reach");
        } else if ((key == "--river-slant" || key == "--river-slant-strength") && i + 1 < argc) {
            args.river_slant_strength = parse_non_negative_double(argv[++i], "river slant strength");
        } else if (key == "--valley-thickness" && i + 1 < argc) {
            args.valley_thickness = parse_non_negative_double(argv[++i], "valley thickness");
        } else if ((key == "--forest-blobs" || key == "--forest-blob-count") && i + 1 < argc) {
            args.forest_blob_count = parse_non_negative_int(argv[++i], "forest blobs");
        } else if (key == "--forest-blob-radius" && i + 1 < argc) {
            args.forest_blob_radius = parse_non_negative_double(argv[++i], "forest blob radius");
        } else if (key == "--meander-timeout" && i + 1 < argc) {
            args.meander_timeout = parse_positive_int(argv[++i], "meander timeout");
        } else {
            throw std::runtime_error("unknown or incomplete argument: " + key);
        }
    }

    if (args.width > 120 || args.height > 80) {
        throw std::runtime_error("width and height are capped at 120x80 for the prototype");
    }
    if (args.river_count > 20) {
        throw std::runtime_error("rivers are capped at 20 for the prototype");
    }
    if (args.lake_count > 20 || args.lake_size > 40) {
        throw std::runtime_error("lake values exceed prototype limits");
    }
    if (args.meander_forward > 40.0 || args.meander_forward_jitter > 40.0 || args.meander_lateral > 40.0 || args.meander_lateral_jitter > 40.0) {
        throw std::runtime_error("meander distances are capped at 40 for the prototype");
    }
    if (args.meander_strength > 10.0 || args.meander_reach > 40.0 || args.river_slant_strength > 10.0 || args.valley_thickness > 5.0 || args.forest_blob_count > 10 || args.forest_blob_radius > 20.0 || args.meander_timeout > 200) {
        throw std::runtime_error("meander tuning values exceed prototype limits");
    }

    return args;
}

double unit_noise(std::uint32_t seed, std::uint32_t channel) {
    std::uint32_t value = seed + channel * 0x9e3779b9U;
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return static_cast<double>(value) / static_cast<double>(0xffffffffU);
}

double signed_noise(std::uint32_t seed, std::uint32_t channel) {
    return unit_noise(seed, channel) * 2.0 - 1.0;
}

int steppe_pinch_band_count(const GenerateArgs& args) {
    return std::max(2, args.river_count + 1);
}

int steppe_pinch_band_index(const GenerateArgs& args) {
    const int band_count = steppe_pinch_band_count(args);
    int best_band = 0;
    double best_distance = std::numeric_limits<double>::max();
    for (int band = 0; band < band_count; ++band) {
        const double center = (static_cast<double>(band) + 0.5) / static_cast<double>(band_count);
        const double distance = std::abs(center - 0.30);
        if (distance < best_distance) {
            best_distance = distance;
            best_band = band;
        }
    }
    return best_band;
}

double steppe_pinch_center_x(const GenerateArgs& args) {
    const int band_count = steppe_pinch_band_count(args);
    return (static_cast<double>(steppe_pinch_band_index(args)) + 0.5) / static_cast<double>(band_count);
}

double seeded_steppe_pinch_center_x(const GenerateArgs& args) {
    return steppe_pinch_center_x(args) + signed_noise(args.seed, 69000) * 0.018;
}

double steppe_center_r_for_x(const GenerateArgs& args, double x) {
    const double map_midline = (static_cast<double>(args.height) + 1.0) * 0.5;
    const double angle_degrees = signed_noise(args.seed, 1) * 15.0;
    const double angle_radians = angle_degrees * 3.14159265358979323846 / 180.0;
    const double total_row_rise = std::tan(angle_radians) * 0.8660254037844386 * static_cast<double>(args.width - 1);
    const double wobble_phase = unit_noise(args.seed, 2) * 2.0 * 3.14159265358979323846;
    const double wobble_frequency = 1.0 + unit_noise(args.seed, 3) * 1.5;
    const double wobble = std::sin(x * wobble_frequency * 2.0 * 3.14159265358979323846 + wobble_phase)
        * std::max(0.35, args.height * 0.07);
    const double center_offset = signed_noise(args.seed, 4) * std::max(0.0, args.height * 0.08);
    return map_midline + center_offset + (x - 0.5) * total_row_rise + wobble;
}

double steppe_average_half_width(const GenerateArgs& args) {
    return std::max(1.0, args.height * (0.18 + unit_noise(args.seed, 5) * 0.1));
}

int steppe_pinch_pass_count(const GenerateArgs& args) {
    return unit_noise(args.seed, 69001) < 0.58 ? 1 : 2;
}

double steppe_pinch_pass_center_r(
    const GenerateArgs& args,
    int pass,
    double x,
    double center_r,
    double average_half_width
) {
    const int pass_count = steppe_pinch_pass_count(args);
    double offset = signed_noise(args.seed, static_cast<std::uint32_t>(69010 + pass * 17))
        * std::max(1.0, average_half_width * 0.34);
    if (pass_count == 2) {
        offset += (pass == 0 ? -1.0 : 1.0) * std::max(1.0, average_half_width * 0.18);
    }
    return center_r + offset
        + std::sin((x * 5.0 + unit_noise(args.seed, static_cast<std::uint32_t>(69020 + pass)) * 2.0)
            * 3.14159265358979323846) * 1.1;
}

bool in_steppe_pinch_pass(
    const GenerateArgs& args,
    int q,
    int r,
    double x,
    double center_r,
    double average_half_width,
    double pinch_influence
) {
    if (pinch_influence < 0.28) {
        return false;
    }

    const int pass_count = steppe_pinch_pass_count(args);
    for (int pass = 0; pass < pass_count; ++pass) {
        const double pass_center = steppe_pinch_pass_center_r(args, pass, x, center_r, average_half_width);
        const double pass_half_width = 1.15 + unit_noise(args.seed, static_cast<std::uint32_t>(69030 + pass)) * 1.2;
        if (std::abs(static_cast<double>(r) - pass_center) <= pass_half_width) {
            return true;
        }
    }

    return false;
}

bool is_steppe_hex(int q, int r, const GenerateArgs& args) {
    if (args.height <= 2) {
        return true;
    }

    const double x = args.width == 1
        ? 0.5
        : static_cast<double>(q - 1) / static_cast<double>(args.width - 1);
    const double center_r = steppe_center_r_for_x(args, x);
    const double average_half_width = steppe_average_half_width(args);
    const double width_phase = unit_noise(args.seed, 6) * 2.0 * 3.14159265358979323846;
    const double width_variation = std::sin((x * 2.0 + 0.25) * 3.14159265358979323846 + width_phase)
        * average_half_width * 0.22;
    const double half_width = average_half_width + width_variation;
    const double pinch_center_x = seeded_steppe_pinch_center_x(args);
    const double pinch_sigma = 0.052 + unit_noise(args.seed, 69002) * 0.018;
    const double pinch_dx = x - pinch_center_x;
    const double pinch_influence = std::exp(-(pinch_dx * pinch_dx) / (2.0 * pinch_sigma * pinch_sigma));
    const double pinch_strength = 0.76 + unit_noise(args.seed, 69003) * 0.13;
    const double pinched_half_width = std::max(1.1, half_width * (1.0 - pinch_influence * pinch_strength));
    const bool pass = in_steppe_pinch_pass(args, q, r, x, center_r, average_half_width, pinch_influence);

    return std::abs(static_cast<double>(r) - center_r) <= half_width
        && (std::abs(static_cast<double>(r) - center_r) <= pinched_half_width || pass);
}

Coord dzungarian_gate_target(const GenerateArgs& args) {
    const double x = std::max(0.0, std::min(1.0, seeded_steppe_pinch_center_x(args)));
    const int q = std::max(1, std::min(args.width, static_cast<int>(std::round(x * static_cast<double>(args.width - 1))) + 1));
    const double center_r = steppe_center_r_for_x(args, x);
    const double average_half_width = steppe_average_half_width(args);
    int best_pass = 0;
    double best_distance = std::numeric_limits<double>::max();
    for (int pass = 0; pass < steppe_pinch_pass_count(args); ++pass) {
        const double pass_center = steppe_pinch_pass_center_r(args, pass, x, center_r, average_half_width);
        const double distance = std::abs(pass_center - static_cast<double>(args.height) * 0.5);
        if (distance < best_distance) {
            best_distance = distance;
            best_pass = pass;
        }
    }
    const int r = std::max(1, std::min(args.height, static_cast<int>(std::round(steppe_pinch_pass_center_r(args, best_pass, x, center_r, average_half_width)))));
    return {q, r};
}

bool coord_equal(const Coord& first, const Coord& second) {
    return first.q == second.q && first.r == second.r;
}

bool coord_less(const Coord& first, const Coord& second) {
    return first.r == second.r ? first.q < second.q : first.r < second.r;
}

bool vertex_equal(const VertexKey& first, const VertexKey& second) {
    return first.x == second.x && first.y == second.y;
}

bool operator<(const VertexKey& first, const VertexKey& second) {
    return first.y == second.y ? first.x < second.x : first.y < second.y;
}

bool edge_less(const RiverEdge& first, const RiverEdge& second) {
    if (!coord_equal(first.a, second.a)) {
        return coord_less(first.a, second.a);
    }
    return coord_less(first.b, second.b);
}

bool edge_equal(const RiverEdge& first, const RiverEdge& second) {
    return coord_equal(first.a, second.a) && coord_equal(first.b, second.b);
}

bool in_bounds(const Coord& coord, const GenerateArgs& args) {
    return coord.q >= 1 && coord.q <= args.width && coord.r >= 1 && coord.r <= args.height;
}

Coord neighbor_in_direction(const Coord& coord, int direction) {
    const bool shifted_down = ((coord.q - 1) % 2) == 1;
    switch (direction) {
        case 0: return {coord.q + 1, coord.r};
        case 1: return shifted_down ? Coord{coord.q + 1, coord.r + 1} : Coord{coord.q + 1, coord.r - 1};
        case 2: return {coord.q, coord.r - 1};
        case 3: return {coord.q - 1, coord.r};
        case 4: return shifted_down ? Coord{coord.q - 1, coord.r + 1} : Coord{coord.q - 1, coord.r - 1};
        case 5: return {coord.q, coord.r + 1};
        default: return coord;
    }
}

RiverEdge canonical_edge(const Coord& first, const Coord& second) {
    return coord_less(second, first) ? RiverEdge{second, first} : RiverEdge{first, second};
}

bool edge_touches_coord(const RiverEdge& edge, const Coord& coord) {
    return coord_equal(edge.a, coord) || coord_equal(edge.b, coord);
}

bool edge_touches_steppe(const RiverEdge& edge, const GenerateArgs& args) {
    return is_steppe_hex(edge.a.q, edge.a.r, args) || is_steppe_hex(edge.b.q, edge.b.r, args);
}

bool contains_edge(const std::vector<RiverEdge>& edges, const RiverEdge& edge) {
    for (const RiverEdge& existing : edges) {
        if (edge_equal(existing, edge)) {
            return true;
        }
    }
    return false;
}

std::vector<VertexKey> hex_corners(const Coord& coord) {
    constexpr double scale = 1000000.0;
    constexpr double pi = 3.14159265358979323846;
    const int col = coord.q - 1;
    const int row = coord.r - 1;
    const double center_x = static_cast<double>(col) * 1.5;
    const double center_y = std::sqrt(3.0) * (static_cast<double>(row) + (col % 2) * 0.5);
    std::vector<VertexKey> corners;

    for (int i = 0; i < 6; ++i) {
        const double angle = pi / 180.0 * static_cast<double>(60 * i);
        corners.push_back({
            static_cast<long long>(std::llround((center_x + std::cos(angle)) * scale)),
            static_cast<long long>(std::llround((center_y + std::sin(angle)) * scale)),
        });
    }

    return corners;
}

BoundaryEdge make_boundary_edge(const RiverEdge& edge) {
    const std::vector<VertexKey> first_corners = hex_corners(edge.a);
    const std::vector<VertexKey> second_corners = hex_corners(edge.b);
    std::vector<VertexKey> shared;

    for (const VertexKey& first : first_corners) {
        for (const VertexKey& second : second_corners) {
            if (vertex_equal(first, second)) {
                shared.push_back(first);
            }
        }
    }

    if (shared.size() != 2) {
        throw std::runtime_error(
            "could not resolve shared edge vertices for edge "
            + std::to_string(edge.a.q) + "," + std::to_string(edge.a.r)
            + " to " + std::to_string(edge.b.q) + "," + std::to_string(edge.b.r)
        );
    }

    if (shared[1] < shared[0]) {
        std::swap(shared[0], shared[1]);
    }

    return {edge, shared[0], shared[1]};
}

double vertex_x(const VertexKey& vertex) {
    return static_cast<double>(vertex.x) / 1000000.0;
}

double vertex_y(const VertexKey& vertex) {
    return static_cast<double>(vertex.y) / 1000000.0;
}

VertexKey other_vertex(const BoundaryEdge& edge, const VertexKey& vertex) {
    return vertex_equal(edge.first, vertex) ? edge.second : edge.first;
}

Coord representative_coord_for_vertex(const VertexKey& vertex, const std::vector<BoundaryEdge>& boundaries) {
    Coord best{1, 1};
    double best_distance = std::numeric_limits<double>::max();

    for (const BoundaryEdge& boundary : boundaries) {
        if (!vertex_equal(boundary.first, vertex) && !vertex_equal(boundary.second, vertex)) {
            continue;
        }

        const Coord candidates[] = {boundary.edge.a, boundary.edge.b};
        for (const Coord& coord : candidates) {
            const int col = coord.q - 1;
            const int row = coord.r - 1;
            const double center_x = static_cast<double>(col) * 1.5;
            const double center_y = std::sqrt(3.0) * (static_cast<double>(row) + (col % 2) * 0.5);
            const double dx = center_x - vertex_x(vertex);
            const double dy = center_y - vertex_y(vertex);
            const double distance = dx * dx + dy * dy;
            if (distance < best_distance) {
                best_distance = distance;
                best = coord;
            }
        }
    }

    return best;
}

Coord representative_coord_for_edge_vertex(const VertexKey& vertex, const BoundaryEdge& boundary) {
    Coord best = boundary.edge.a;
    double best_distance = std::numeric_limits<double>::max();
    const Coord candidates[] = {boundary.edge.a, boundary.edge.b};

    for (const Coord& coord : candidates) {
        const int col = coord.q - 1;
        const int row = coord.r - 1;
        const double center_x = static_cast<double>(col) * 1.5;
        const double center_y = std::sqrt(3.0) * (static_cast<double>(row) + (col % 2) * 0.5);
        const double dx = center_x - vertex_x(vertex);
        const double dy = center_y - vertex_y(vertex);
        const double distance = dx * dx + dy * dy;
        if (distance < best_distance) {
            best_distance = distance;
            best = coord;
        }
    }

    return best;
}

std::vector<RiverEdge> all_map_edges(const GenerateArgs& args) {
    std::vector<RiverEdge> edges;
    edges.reserve(static_cast<std::size_t>(args.width * args.height * 3));
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord hex{q, r};
            const int directions[] = {0, 1, 5};
            for (const int direction : directions) {
                const Coord neighbor = neighbor_in_direction(hex, direction);
                if (!in_bounds(neighbor, args)) {
                    continue;
                }
                edges.push_back(canonical_edge(hex, neighbor));
            }
        }
    }
    return edges;
}

std::vector<Coord> generate_river_sources(const GenerateArgs& args) {
    std::vector<Coord> sources;
    if (args.river_count == 0) {
        return sources;
    }

    const int band_count = steppe_pinch_band_count(args);
    const int pinch_band = steppe_pinch_band_index(args);
    const double slot_width = static_cast<double>(args.width) / static_cast<double>(band_count);
    const int jitter_limit = std::max(0, static_cast<int>(std::floor(slot_width * 0.25)));
    for (int i = 0; i < args.river_count; ++i) {
        int band = i;
        if (band >= pinch_band) {
            ++band;
        }
        const double slot_center = (static_cast<double>(band) + 0.5) * slot_width + 1.0;
        const int jitter = jitter_limit == 0
            ? 0
            : static_cast<int>(std::floor(unit_noise(args.seed, 1000U + static_cast<std::uint32_t>(i)) * (jitter_limit * 2 + 1))) - jitter_limit;
        int q = std::max(1, std::min(args.width, static_cast<int>(std::round(slot_center)) + jitter));
        while (q <= args.width && std::find_if(sources.begin(), sources.end(), [q](const Coord& coord) { return coord.q == q; }) != sources.end()) {
            ++q;
        }
        if (q > args.width) {
            q = std::max(1, std::min(args.width, static_cast<int>(std::round(slot_center))));
        }
        sources.push_back({q, 1});
    }

    std::sort(sources.begin(), sources.end(), coord_less);
    return sources;
}

std::optional<int> start_edge_for_source(
    const Coord& source,
    const std::vector<RiverEdge>& edges,
    const std::vector<BoundaryEdge>& boundaries
) {
    double best_score = std::numeric_limits<double>::max();
    std::optional<int> best;

    for (std::size_t i = 0; i < edges.size(); ++i) {
        if (!edge_touches_coord(edges[i], source)) {
            continue;
        }

        const double midpoint_y = (vertex_y(boundaries[i].first) + vertex_y(boundaries[i].second)) * 0.5;
        const double span_y = std::abs(vertex_y(boundaries[i].first) - vertex_y(boundaries[i].second));
        const double score = midpoint_y - span_y * 0.35;
        if (score < best_score) {
            best_score = score;
            best = static_cast<int>(i);
        }
    }

    return best;
}

void add_unique_edge(std::vector<RiverEdge>& edges, const RiverEdge& edge) {
    if (!contains_edge(edges, edge)) {
        edges.push_back(edge);
    }
}

RiverNetwork generate_river_network(const GenerateArgs& args) {
    RiverNetwork network;
    network.sources = generate_river_sources(args);
    if (network.sources.empty()) {
        return network;
    }

    const std::vector<RiverEdge> edges = all_map_edges(args);
    std::vector<BoundaryEdge> boundaries;
    boundaries.reserve(edges.size());
    for (const RiverEdge& edge : edges) {
        boundaries.push_back(make_boundary_edge(edge));
    }

    std::map<VertexKey, std::vector<int>> edges_by_vertex;
    for (std::size_t i = 0; i < boundaries.size(); ++i) {
        edges_by_vertex[boundaries[i].first].push_back(static_cast<int>(i));
        edges_by_vertex[boundaries[i].second].push_back(static_cast<int>(i));
    }

    std::set<RiverEdge, decltype(edge_less)*> occupied_edges(edge_less);
    std::map<RiverEdge, int, decltype(edge_less)*> occupied_edge_owners(edge_less);
    std::map<VertexKey, int> occupied_vertices;
    std::vector<RiverHead> heads;

    int next_id = 1;
    for (const Coord& source : network.sources) {
        const std::optional<int> start_edge = start_edge_for_source(source, edges, boundaries);
        if (!start_edge.has_value()) {
            continue;
        }

        const BoundaryEdge& boundary = boundaries[*start_edge];
        const VertexKey upstream = vertex_y(boundary.first) <= vertex_y(boundary.second) ? boundary.first : boundary.second;
        const VertexKey downstream = vertex_equal(upstream, boundary.first) ? boundary.second : boundary.first;

        RiverHead head;
        head.id = next_id++;
        head.source = source;
        head.slant_direction = unit_noise(args.seed, static_cast<std::uint32_t>(head.id * 37000 + 7)) < 0.5 ? -1 : 1;
        head.current_edge = *start_edge;
        head.current_vertex = downstream;
        head.edge_path.push_back(edges[*start_edge]);
        head.visited_vertices.insert(upstream);
        head.visited_vertices.insert(downstream);
        occupied_edges.insert(edges[*start_edge]);
        occupied_edge_owners[edges[*start_edge]] = head.id;
        occupied_vertices[upstream] = head.id;
        occupied_vertices[downstream] = head.id;
        heads.push_back(head);
    }

    const double bottom_y = std::sqrt(3.0) * static_cast<double>(std::max(1, args.height - 1));
    const double max_x = 1.5 * static_cast<double>(std::max(1, args.width - 1));
    const int max_steps = std::max(32, args.width * args.height);
    const int minimum_steps_before_merge = std::max(6, args.height / 12);
    const auto reset_meander_target = [&](RiverHead& head, int step) {
        const double forward_rows = std::max(1.0, args.meander_forward + signed_noise(
            args.seed,
            static_cast<std::uint32_t>(head.id * 31000 + step * 19 + 1)
        ) * args.meander_forward_jitter);
        const double lateral_cols = std::max(0.0, args.meander_lateral + signed_noise(
            args.seed,
            static_cast<std::uint32_t>(head.id * 31000 + step * 19 + 2)
        ) * args.meander_lateral_jitter);
        const double target_x = vertex_x(head.current_vertex) + static_cast<double>(head.next_meander_side) * lateral_cols * 1.5;
        const double target_y = vertex_y(head.current_vertex) + forward_rows * std::sqrt(3.0);

        head.meander_target_x = std::max(1.5, std::min(max_x - 1.5, target_x));
        head.meander_target_y = std::min(bottom_y, target_y);
        head.next_meander_side *= -1;
        head.has_meander_target = true;
        head.meander_steps = 0;
    };
    const auto head_is_active = [&](int id) {
        return std::find_if(heads.begin(), heads.end(), [id](const RiverHead& candidate) {
            return candidate.id == id && candidate.active;
        }) != heads.end();
    };

    for (int step = 0; step < max_steps; ++step) {
        bool any_active = false;

        for (RiverHead& head : heads) {
            if (!head.active) {
                continue;
            }
            any_active = true;

            if (vertex_y(head.current_vertex) >= bottom_y) {
                head.active = false;
                head.endpoint = representative_coord_for_edge_vertex(head.current_vertex, boundaries[head.current_edge]);
                network.destinations.push_back(head.endpoint);
                continue;
            }

            const auto found = edges_by_vertex.find(head.current_vertex);
            if (found == edges_by_vertex.end()) {
                head.active = false;
                head.endpoint = representative_coord_for_edge_vertex(head.current_vertex, boundaries[head.current_edge]);
                network.destinations.push_back(head.endpoint);
                continue;
            }

            int best_edge = -1;
            VertexKey best_next;
            double best_score = std::numeric_limits<double>::max();
            const bool can_merge = static_cast<int>(head.edge_path.size()) >= minimum_steps_before_merge;
            const bool meander_active = edge_touches_steppe(edges[head.current_edge], args);
            if (meander_active) {
                const double target_dx = vertex_x(head.current_vertex) - head.meander_target_x;
                const double target_dy = vertex_y(head.current_vertex) - head.meander_target_y;
                const double target_distance = std::sqrt(target_dx * target_dx + target_dy * target_dy);
                if (!head.has_meander_target || target_distance < args.meander_reach || head.meander_steps > args.meander_timeout) {
                    reset_meander_target(head, step);
                }
            } else {
                head.has_meander_target = false;
                head.meander_steps = 0;
            }

            for (const int candidate : found->second) {
                if (candidate == head.current_edge) {
                    continue;
                }

                const BoundaryEdge& boundary = boundaries[candidate];
                const VertexKey next_vertex = other_vertex(boundary, head.current_vertex);
                if (head.visited_vertices.find(next_vertex) != head.visited_vertices.end()) {
                    continue;
                }

                const auto occupied_edge_owner = occupied_edge_owners.find(edges[candidate]);
                const bool occupied_by_other_edge = occupied_edge_owner != occupied_edge_owners.end()
                    && occupied_edge_owner->second != head.id;
                const auto occupied_vertex = occupied_vertices.find(next_vertex);
                const bool occupied_by_other_vertex = occupied_vertex != occupied_vertices.end() && occupied_vertex->second != head.id;
                if ((occupied_by_other_edge || occupied_by_other_vertex) && !can_merge) {
                    continue;
                }

                const double dy = vertex_y(next_vertex) - vertex_y(head.current_vertex);
                const double dx = vertex_x(next_vertex) - vertex_x(head.current_vertex);
                const double side_distance = std::min(vertex_x(next_vertex), max_x - vertex_x(next_vertex));
                const double south_score = dy > 0.0
                    ? -dy * 3.0
                    : (dy == 0.0 ? 6.0 : 80.0 + std::abs(dy) * 30.0);
                const double side_score = vertex_y(next_vertex) < bottom_y * 0.9
                    ? (side_distance < 1.0 ? 90.0 : (side_distance < 2.0 ? 35.0 : 0.0))
                    : 0.0;
                const double current_meander_dx = vertex_x(head.current_vertex) - head.meander_target_x;
                const double current_meander_dy = vertex_y(head.current_vertex) - head.meander_target_y;
                const double current_meander_distance = std::sqrt(
                    current_meander_dx * current_meander_dx + current_meander_dy * current_meander_dy
                );
                const double next_meander_dx = vertex_x(next_vertex) - head.meander_target_x;
                const double next_meander_dy = vertex_y(next_vertex) - head.meander_target_y;
                const double next_meander_distance = std::sqrt(next_meander_dx * next_meander_dx + next_meander_dy * next_meander_dy);
                const double raw_meander_score = meander_active && head.has_meander_target
                    ? (next_meander_distance - current_meander_distance) * args.meander_strength * 8.0
                    : 0.0;
                const double meander_score = std::clamp(raw_meander_score, -10.0, 10.0);
                const double raw_slant_score = -dx * static_cast<double>(head.slant_direction) * args.river_slant_strength;
                const double slant_score = std::clamp(raw_slant_score, -2.0, 2.0);
                const double merge_score = (occupied_by_other_edge || occupied_by_other_vertex) ? -30.0 : 0.0;
                const double noise = unit_noise(
                    args.seed,
                    static_cast<std::uint32_t>(head.id * 20000 + step * 131 + candidate * 17)
                ) * 14.0;
                const double score = south_score + side_score + slant_score + meander_score + merge_score + noise;

                if (score < best_score) {
                    best_score = score;
                    best_edge = candidate;
                    best_next = next_vertex;
                }
            }

            if (best_edge < 0) {
                head.active = false;
                head.endpoint = representative_coord_for_edge_vertex(head.current_vertex, boundaries[head.current_edge]);
                network.destinations.push_back(head.endpoint);
                continue;
            }

            const auto occupied_vertex = occupied_vertices.find(best_next);
            const bool merges = (occupied_edges.find(edges[best_edge]) != occupied_edges.end())
                || (occupied_vertex != occupied_vertices.end() && occupied_vertex->second != head.id);

            if (!merges) {
                head.current_edge = best_edge;
                head.current_vertex = best_next;
                head.edge_path.push_back(edges[best_edge]);
                head.visited_vertices.insert(best_next);
                if (meander_active && head.has_meander_target) {
                    ++head.meander_steps;
                }
                occupied_edges.insert(edges[best_edge]);
                occupied_edge_owners[edges[best_edge]] = head.id;
                occupied_vertices[best_next] = head.id;
                add_unique_edge(network.edges, edges[best_edge]);
                continue;
            }

            const auto best_edge_owner = occupied_edge_owners.find(edges[best_edge]);
            const int merge_owner = occupied_vertex != occupied_vertices.end() && occupied_vertex->second != head.id
                ? occupied_vertex->second
                : (best_edge_owner != occupied_edge_owners.end() && best_edge_owner->second != head.id ? best_edge_owner->second : 0);
            const bool merge_owner_active = merge_owner != 0 && head_is_active(merge_owner);

            head.edge_path.push_back(edges[best_edge]);
            head.current_edge = best_edge;
            head.current_vertex = best_next;
            head.endpoint = representative_coord_for_edge_vertex(best_next, boundaries[best_edge]);
            network.merge_points.push_back(head.endpoint);
            occupied_edges.insert(edges[best_edge]);
            add_unique_edge(network.edges, edges[best_edge]);
            if (merge_owner_active) {
                head.active = false;
                head.merged = true;
                continue;
            }

            head.visited_vertices.insert(best_next);
            if (meander_active && head.has_meander_target) {
                ++head.meander_steps;
            }
            occupied_edge_owners[edges[best_edge]] = head.id;
            occupied_vertices[best_next] = head.id;
        }

        if (!any_active) {
            break;
        }
    }

    for (RiverHead& head : heads) {
        if (head.active) {
            head.active = false;
            head.endpoint = representative_coord_for_edge_vertex(head.current_vertex, boundaries[head.current_edge]);
            network.destinations.push_back(head.endpoint);
        }

        for (const RiverEdge& edge : head.edge_path) {
            add_unique_edge(network.edges, edge);
        }

        network.segments.push_back({
            head.id,
            head.source,
            head.endpoint,
            head.merged ? "source_to_merge" : "source_to_destination",
            head.edge_path,
        });
    }

    std::sort(network.destinations.begin(), network.destinations.end(), coord_less);
    network.destinations.erase(std::unique(network.destinations.begin(), network.destinations.end(), coord_equal), network.destinations.end());
    std::sort(network.merge_points.begin(), network.merge_points.end(), coord_less);
    network.merge_points.erase(std::unique(network.merge_points.begin(), network.merge_points.end(), coord_equal), network.merge_points.end());
    std::sort(network.edges.begin(), network.edges.end(), edge_less);
    return network;
}

int rough_coord_distance(const Coord& first, const Coord& second) {
    return std::abs(first.q - second.q) + std::abs(first.r - second.r);
}

bool contains_coord(const std::vector<Coord>& coords, const Coord& coord) {
    return std::find_if(coords.begin(), coords.end(), [&coord](const Coord& existing) {
        return coord_equal(existing, coord);
    }) != coords.end();
}

std::set<Coord, decltype(coord_less)*> generate_baikal_hexes(const GenerateArgs& args) {
    std::set<Coord, decltype(coord_less)*> lake_hexes(coord_less);
    if (args.width < 20 || args.height < 20) {
        return lake_hexes;
    }

    const double center_q = static_cast<double>(args.width) * (0.68 + signed_noise(args.seed, 71001) * 0.035);
    const double center_r = static_cast<double>(args.height) * (0.19 + signed_noise(args.seed, 71002) * 0.035);
    const double major_radius = static_cast<double>(args.width) * (0.075 + unit_noise(args.seed, 71003) * 0.025);
    const double minor_radius = static_cast<double>(args.height) * (0.028 + unit_noise(args.seed, 71004) * 0.018);
    const double angle = (-25.0 + signed_noise(args.seed, 71005) * 18.0) * 3.14159265358979323846 / 180.0;
    const double cos_angle = std::cos(angle);
    const double sin_angle = std::sin(angle);

    const int min_q = std::max(1, static_cast<int>(std::floor(center_q - major_radius - 3.0)));
    const int max_q = std::min(args.width, static_cast<int>(std::ceil(center_q + major_radius + 3.0)));
    const int min_r = std::max(1, static_cast<int>(std::floor(center_r - major_radius - 3.0)));
    const int max_r = std::min(args.height, static_cast<int>(std::ceil(center_r + major_radius + 3.0)));

    for (int r = min_r; r <= max_r; ++r) {
        for (int q = min_q; q <= max_q; ++q) {
            const double dx = static_cast<double>(q) - center_q;
            const double dy = (static_cast<double>(r) - center_r) * 1.35;
            const double along = dx * cos_angle + dy * sin_angle;
            const double across = -dx * sin_angle + dy * cos_angle;
            const double shoreline = 1.0 + signed_noise(
                args.seed,
                static_cast<std::uint32_t>(71050 + q * 97 + r * 131)
            ) * 0.16;
            const double normalized = (along * along) / (major_radius * major_radius)
                + (across * across) / (minor_radius * minor_radius);
            if (normalized <= shoreline) {
                lake_hexes.insert({q, r});
            }
        }
    }

    return lake_hexes;
}

std::set<Coord, decltype(coord_less)*> generate_caspian_hexes(const GenerateArgs& args) {
    std::set<Coord, decltype(coord_less)*> lake_hexes(coord_less);
    if (args.width < 20 || args.height < 20) {
        return lake_hexes;
    }

    std::vector<int> west_steppe_rows;
    const int steppe_probe_max_q = std::min(args.width, std::max(8, args.width / 7));
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= steppe_probe_max_q; ++q) {
            if (is_steppe_hex(q, r, args)) {
                west_steppe_rows.push_back(r);
                break;
            }
        }
    }

    double steppe_anchor_r = static_cast<double>(args.height) * 0.72;
    if (!west_steppe_rows.empty()) {
        const int min_steppe_r = *std::min_element(west_steppe_rows.begin(), west_steppe_rows.end());
        const int max_steppe_r = *std::max_element(west_steppe_rows.begin(), west_steppe_rows.end());
        steppe_anchor_r = static_cast<double>(min_steppe_r + max_steppe_r) * 0.5
            + static_cast<double>(max_steppe_r - min_steppe_r) * 0.55;
    }

    const double vertical_radius = static_cast<double>(args.height) * (0.115 + unit_noise(args.seed, 72002) * 0.025);
    const double center_r = std::max(
        vertical_radius + 1.0,
        std::min(static_cast<double>(args.height) - vertical_radius * 0.35, steppe_anchor_r + signed_noise(args.seed, 72001) * args.height * 0.035)
    );
    const double base_depth = static_cast<double>(args.width) * (0.018 + unit_noise(args.seed, 72003) * 0.009);
    const double max_extra_depth = static_cast<double>(args.width) * (0.035 + unit_noise(args.seed, 72004) * 0.018);
    const double wave_phase = unit_noise(args.seed, 72005) * 2.0 * 3.14159265358979323846;

    for (int r = 1; r <= args.height; ++r) {
        const double normalized_r = (static_cast<double>(r) - center_r) / vertical_radius;
        if (std::abs(normalized_r) > 1.05) {
            continue;
        }

        const double bulge = std::max(0.0, 1.0 - normalized_r * normalized_r);
        const double southern_bias = static_cast<double>(r) / static_cast<double>(args.height);
        const double wave = std::sin(static_cast<double>(r) * 0.34 + wave_phase) * 1.7;
        const double row_noise = signed_noise(args.seed, static_cast<std::uint32_t>(72050 + r * 43)) * 1.6;
        const int depth = std::max(1, static_cast<int>(std::round(
            base_depth + max_extra_depth * bulge * (0.38 + southern_bias * 0.95) + wave + row_noise
        )));
        for (int q = 1; q <= std::min(args.width, depth); ++q) {
            lake_hexes.insert({q, r});
        }
    }

    return lake_hexes;
}

std::set<Coord, decltype(coord_less)*> generate_lake_hexes(const GenerateArgs& args, const RiverNetwork& rivers) {
    std::set<Coord, decltype(coord_less)*> lake_hexes(coord_less);
    if (args.lake_count == 0 || args.lake_size == 0 || rivers.segments.empty()) {
        return lake_hexes;
    }

    std::vector<LakeCandidate> candidates;
    for (const RiverSegment& segment : rivers.segments) {
        if (segment.edge_path.size() < 8) {
            continue;
        }

        const std::size_t start = std::max<std::size_t>(1, segment.edge_path.size() / 4);
        const std::size_t end = std::max(start + 1, segment.edge_path.size() * 9 / 10);
        for (std::size_t index = start; index < end; ++index) {
            const RiverEdge& edge = segment.edge_path[index];
            const double progress = static_cast<double>(index) / static_cast<double>(std::max<std::size_t>(1, segment.edge_path.size() - 1));
            const double center_bias = 1.0 - std::abs(progress - 0.62);
            const double score = center_bias + unit_noise(
                args.seed,
                static_cast<std::uint32_t>(segment.id * 41000 + index * 23 + 11)
            );
            candidates.push_back({edge, score});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const LakeCandidate& first, const LakeCandidate& second) {
        return first.score > second.score;
    });

    std::vector<Coord> seed_coords;
    int built_lakes = 0;
    for (const LakeCandidate& candidate : candidates) {
        if (built_lakes >= args.lake_count) {
            break;
        }

        const Coord seed = unit_noise(args.seed, static_cast<std::uint32_t>(built_lakes * 53000 + candidate.edge.a.q * 97 + candidate.edge.a.r * 31)) < 0.5
            ? candidate.edge.a
            : candidate.edge.b;
        if (!in_bounds(seed, args) || seed.r <= 2 || seed.r >= args.height) {
            continue;
        }

        bool too_close = false;
        for (const Coord& existing : seed_coords) {
            if (rough_coord_distance(seed, existing) < std::max(5, args.lake_size)) {
                too_close = true;
                break;
            }
        }
        if (too_close) {
            continue;
        }

        const int target_size = std::max(2, args.lake_size - 2 + static_cast<int>(
            std::floor(unit_noise(args.seed, static_cast<std::uint32_t>(built_lakes * 53000 + 17)) * 5.0)
        ));
        std::vector<Coord> lake_cells;
        std::vector<Coord> frontier;
        const Coord initial_cells[] = {candidate.edge.a, candidate.edge.b};
        for (const Coord& coord : initial_cells) {
            if (in_bounds(coord, args) && lake_hexes.find(coord) == lake_hexes.end() && !contains_coord(lake_cells, coord)) {
                lake_cells.push_back(coord);
                frontier.push_back(coord);
            }
        }

        while (static_cast<int>(lake_cells.size()) < target_size && !frontier.empty()) {
            std::vector<Coord> neighbor_candidates;
            for (const Coord& coord : frontier) {
                for (int direction = 0; direction < 6; ++direction) {
                    const Coord neighbor = neighbor_in_direction(coord, direction);
                    if (!in_bounds(neighbor, args) || lake_hexes.find(neighbor) != lake_hexes.end() || contains_coord(lake_cells, neighbor) || contains_coord(neighbor_candidates, neighbor)) {
                        continue;
                    }
                    neighbor_candidates.push_back(neighbor);
                }
            }

            if (neighbor_candidates.empty()) {
                break;
            }

            std::sort(neighbor_candidates.begin(), neighbor_candidates.end(), [&](const Coord& first, const Coord& second) {
                const double first_score = unit_noise(args.seed, static_cast<std::uint32_t>(built_lakes * 61000 + first.q * 131 + first.r * 37));
                const double second_score = unit_noise(args.seed, static_cast<std::uint32_t>(built_lakes * 61000 + second.q * 131 + second.r * 37));
                return first_score > second_score;
            });

            const Coord next = neighbor_candidates.front();
            lake_cells.push_back(next);
            frontier.push_back(next);
        }

        for (const Coord& coord : lake_cells) {
            lake_hexes.insert(coord);
        }
        seed_coords.push_back(seed);
        ++built_lakes;
    }

    return lake_hexes;
}

std::vector<LakeNetworkTemplate> chinese_lake_network_templates() {
    return {
        {
            1,
            20,
            10,
            {10, 5},
            {{8, 2}, {7, 3}, {8, 3}, {8, 4}, {9, 4}, {14, 4}, {15, 4}, {15, 5}, {7, 7}, {8, 7}, {7, 8}},
            {{{10, 3}, {10, 4}}, {{10, 4}, {11, 4}}, {{11, 4}, {11, 5}}, {{12, 4}, {11, 5}}, {{12, 4}, {12, 5}}, {{12, 4}, {13, 5}}, {{13, 4}, {13, 5}}, {{8, 5}, {9, 5}}, {{8, 5}, {8, 6}}, {{8, 5}, {9, 6}}, {{7, 6}, {8, 6}}},
        },
        {
            2,
            20,
            10,
            {10, 5},
            {{14, 4}, {7, 5}, {8, 5}, {12, 5}, {13, 5}, {14, 5}, {6, 6}, {7, 6}, {8, 6}, {11, 6}, {18, 6}, {19, 6}, {7, 7}, {17, 7}, {18, 7}, {17, 8}},
            {{{9, 5}, {9, 6}}, {{10, 5}, {9, 6}}, {{10, 5}, {10, 6}}, {{14, 6}, {15, 6}}, {{15, 6}, {15, 7}}, {{16, 6}, {15, 7}}, {{16, 6}, {16, 7}}},
        },
        {
            3,
            20,
            10,
            {10, 5},
            {{10, 3}, {10, 4}, {9, 5}, {11, 5}, {14, 6}, {15, 6}, {16, 6}, {17, 6}, {10, 7}, {15, 7}, {16, 7}, {9, 8}, {10, 8}, {11, 8}},
            {{{12, 5}, {11, 6}}, {{12, 5}, {12, 6}}, {{12, 6}, {13, 6}}, {{12, 6}, {12, 7}}, {{12, 6}, {13, 7}}, {{13, 6}, {13, 7}}, {{11, 7}, {12, 7}}},
        },
        {
            4,
            20,
            10,
            {10, 5},
            {{6, 4}, {6, 5}, {7, 5}, {12, 6}, {5, 7}, {6, 7}, {11, 7}, {12, 7}, {13, 7}, {14, 7}, {5, 8}, {6, 8}, {13, 8}},
            {{{8, 4}, {8, 5}}, {{8, 5}, {9, 5}}, {{9, 5}, {9, 6}}, {{10, 5}, {9, 6}}, {{10, 5}, {10, 6}}, {{6, 6}, {7, 6}}, {{6, 6}, {7, 7}}, {{10, 6}, {11, 6}}},
        },
    };
}

Coord translate_template_coord(const LakeNetworkTemplate& lake_template, const Coord& anchor, const Coord& coord) {
    return {
        anchor.q + coord.q - lake_template.anchor.q,
        anchor.r + coord.r - lake_template.anchor.r,
    };
}

Coord clamp_template_anchor(const GenerateArgs& args, const LakeNetworkTemplate& lake_template, Coord anchor) {
    const int min_q = lake_template.anchor.q;
    const int min_r = lake_template.anchor.r;
    const int max_q = args.width - lake_template.width + lake_template.anchor.q;
    const int max_r = args.height - lake_template.height + lake_template.anchor.r;
    anchor.q = std::max(min_q, std::min(max_q, anchor.q));
    anchor.r = std::max(min_r, std::min(max_r, anchor.r));
    if ((anchor.q - lake_template.anchor.q) % 2 != 0) {
        if (anchor.q + 1 <= max_q) {
            ++anchor.q;
        } else if (anchor.q - 1 >= min_q) {
            --anchor.q;
        }
    }
    return anchor;
}

double score_chinese_lake_network_anchor(
    const GenerateArgs& args,
    const LakeNetworkTemplate& lake_template,
    const std::set<Coord, decltype(coord_less)*>& occupied_lakes,
    const Coord& anchor,
    int candidate_index
) {
    const double target_q = static_cast<double>(args.width) * 0.78;
    const double target_r = static_cast<double>(args.height) * 0.68;
    double score = -std::abs(static_cast<double>(anchor.q) - target_q) * 0.16
        - std::abs(static_cast<double>(anchor.r) - target_r) * 0.22
        + unit_noise(args.seed, static_cast<std::uint32_t>(73000 + candidate_index * 37 + lake_template.id)) * 6.0;

    for (const Coord& coord : lake_template.lake_hexes) {
        const Coord placed = translate_template_coord(lake_template, anchor, coord);
        if (!in_bounds(placed, args)) {
            score -= 1000.0;
            continue;
        }
        score += is_steppe_hex(placed.q, placed.r, args) ? 7.0 : -2.5;
        if (occupied_lakes.find(placed) != occupied_lakes.end()) {
            score -= 35.0;
        }
    }

    for (const RiverEdge& edge : lake_template.edges) {
        const Coord first = translate_template_coord(lake_template, anchor, edge.a);
        const Coord second = translate_template_coord(lake_template, anchor, edge.b);
        if (!in_bounds(first, args) || !in_bounds(second, args)) {
            score -= 1000.0;
            continue;
        }
        if (is_steppe_hex(first.q, first.r, args) || is_steppe_hex(second.q, second.r, args)) {
            score += 1.2;
        }
    }

    return score;
}

LakeNetworkOverlay generate_chinese_lake_network_overlay(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& occupied_lakes
) {
    LakeNetworkOverlay overlay;
    const std::vector<LakeNetworkTemplate> templates = chinese_lake_network_templates();
    if (templates.empty() || args.width < 20 || args.height < 10) {
        return overlay;
    }

    const int template_index = std::min(
        static_cast<int>(templates.size()) - 1,
        static_cast<int>(std::floor(unit_noise(args.seed, 73101) * static_cast<double>(templates.size())))
    );
    const LakeNetworkTemplate& lake_template = templates[template_index];

    Coord best_anchor = clamp_template_anchor(args, lake_template, {
        static_cast<int>(std::round(static_cast<double>(args.width) * 0.78)),
        static_cast<int>(std::round(static_cast<double>(args.height) * 0.68)),
    });
    double best_score = -std::numeric_limits<double>::max();

    for (int candidate = 0; candidate < 48; ++candidate) {
        Coord anchor{
            static_cast<int>(std::round(
                static_cast<double>(args.width) * 0.78
                + signed_noise(args.seed, static_cast<std::uint32_t>(73200 + candidate * 2)) * static_cast<double>(args.width) * 0.12
            )),
            static_cast<int>(std::round(
                static_cast<double>(args.height) * 0.68
                + signed_noise(args.seed, static_cast<std::uint32_t>(73201 + candidate * 2)) * static_cast<double>(args.height) * 0.12
            )),
        };
        anchor = clamp_template_anchor(args, lake_template, anchor);
        const double score = score_chinese_lake_network_anchor(args, lake_template, occupied_lakes, anchor, candidate);
        if (score > best_score) {
            best_score = score;
            best_anchor = anchor;
        }
    }

    overlay.placed = true;
    overlay.template_id = lake_template.id;
    overlay.anchor = best_anchor;
    for (const Coord& coord : lake_template.lake_hexes) {
        const Coord placed = translate_template_coord(lake_template, best_anchor, coord);
        if (in_bounds(placed, args) && !contains_coord(overlay.lake_hexes, placed)) {
            overlay.lake_hexes.push_back(placed);
        }
    }
    for (const RiverEdge& edge : lake_template.edges) {
        const Coord first = translate_template_coord(lake_template, best_anchor, edge.a);
        const Coord second = translate_template_coord(lake_template, best_anchor, edge.b);
        if (in_bounds(first, args) && in_bounds(second, args)) {
            add_unique_edge(overlay.edges, canonical_edge(first, second));
        }
    }

    std::sort(overlay.lake_hexes.begin(), overlay.lake_hexes.end(), coord_less);
    std::sort(overlay.edges.begin(), overlay.edges.end(), edge_less);
    return overlay;
}

void add_valley_candidate(
    std::map<Coord, double, decltype(coord_less)*>& candidates,
    const Coord& coord,
    double strength
) {
    const auto found = candidates.find(coord);
    if (found == candidates.end() || strength > found->second) {
        candidates[coord] = strength;
    }
}

double valley_ring_scale(double thickness, int distance) {
    if (thickness <= 0.0) {
        return 0.0;
    }
    if (distance == 0) {
        return std::min(1.0, thickness);
    }

    const int full_distance = static_cast<int>(std::floor(thickness));
    if (distance <= full_distance) {
        return 1.0;
    }
    if (distance == full_distance + 1) {
        return thickness - static_cast<double>(full_distance);
    }
    return 0.0;
}

double river_valley_strength(int distance) {
    if (distance == 0) {
        return 1.0;
    }
    if (distance == 1) {
        return 0.96;
    }
    if (distance == 2) {
        return 0.82;
    }
    return 0.48 * std::pow(0.62, static_cast<double>(distance - 3));
}

double lake_valley_strength(int distance) {
    if (distance == 1) {
        return 0.98;
    }
    if (distance == 2) {
        return 0.86;
    }
    if (distance == 3) {
        return 0.62;
    }
    return 0.34 * std::pow(0.62, static_cast<double>(distance - 4));
}

std::set<Coord, decltype(coord_less)*> next_neighbor_ring(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& current_ring
) {
    std::set<Coord, decltype(coord_less)*> next_ring(coord_less);
    for (const Coord& coord : current_ring) {
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(coord, direction);
            if (in_bounds(neighbor, args)) {
                next_ring.insert(neighbor);
            }
        }
    }
    return next_ring;
}

std::set<Coord, decltype(coord_less)*> derive_valley_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::vector<RiverEdge>& river_edges
) {
    std::set<Coord, decltype(coord_less)*> valley_hexes(coord_less);
    std::map<Coord, double, decltype(coord_less)*> candidates(coord_less);
    if (args.valley_thickness <= 0.0) {
        return valley_hexes;
    }

    std::set<Coord, decltype(coord_less)*> river_ring(coord_less);
    for (const RiverEdge& edge : river_edges) {
        river_ring.insert(edge.a);
        river_ring.insert(edge.b);
    }
    const int max_river_distance = static_cast<int>(std::ceil(args.valley_thickness));
    for (int distance = 0; distance <= max_river_distance && !river_ring.empty(); ++distance) {
        const double scale = valley_ring_scale(args.valley_thickness, distance);
        if (scale > 0.0) {
            for (const Coord& coord : river_ring) {
                add_valley_candidate(candidates, coord, river_valley_strength(distance) * scale);
            }
        }
        river_ring = next_neighbor_ring(args, river_ring);
    }

    std::set<Coord, decltype(coord_less)*> lake_ring = next_neighbor_ring(args, lake_hexes);
    const double lake_effective_thickness = args.valley_thickness + 1.0;
    const int max_lake_distance = static_cast<int>(std::ceil(lake_effective_thickness));
    for (int distance = 1; distance <= max_lake_distance && !lake_ring.empty(); ++distance) {
        const double scale = valley_ring_scale(lake_effective_thickness, distance);
        if (scale > 0.0) {
            for (const Coord& coord : lake_ring) {
                if (lake_hexes.find(coord) == lake_hexes.end()) {
                    add_valley_candidate(candidates, coord, lake_valley_strength(distance) * scale);
                }
            }
        }
        lake_ring = next_neighbor_ring(args, lake_ring);
    }

    for (const auto& entry : candidates) {
        const Coord coord = entry.first;
        if (lake_hexes.find(coord) != lake_hexes.end() || is_steppe_hex(coord.q, coord.r, args)) {
            continue;
        }

        const double north_south_bias = static_cast<double>(coord.r) / static_cast<double>(std::max(1, args.height));
        const double threshold = std::min(0.985, entry.second + north_south_bias * 0.04);
        const double roll = unit_noise(args.seed, static_cast<std::uint32_t>(74000 + coord.q * 193 + coord.r * 389));
        if (roll < threshold) {
            valley_hexes.insert(coord);
        }
    }

    return valley_hexes;
}

std::vector<LakeRiverConnection> derive_lake_river_connections(
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::vector<RiverEdge>& river_edges
) {
    std::map<VertexKey, std::vector<Coord>> lake_hexes_by_vertex;
    for (const Coord& lake : lake_hexes) {
        for (const VertexKey& vertex : hex_corners(lake)) {
            lake_hexes_by_vertex[vertex].push_back(lake);
        }
    }

    std::map<VertexKey, std::set<VertexKey>> graph;
    std::map<VertexKey, std::vector<RiverEdge>> incident_edges;
    for (const RiverEdge& edge : river_edges) {
        const BoundaryEdge boundary = make_boundary_edge(edge);
        graph[boundary.first].insert(boundary.second);
        graph[boundary.second].insert(boundary.first);
        incident_edges[boundary.first].push_back(edge);
        incident_edges[boundary.second].push_back(edge);
    }

    std::vector<LakeRiverConnection> connections;
    std::set<VertexKey> seen;
    int component_id = 0;

    for (const auto& entry : graph) {
        const VertexKey start = entry.first;
        if (seen.find(start) != seen.end()) {
            continue;
        }

        ++component_id;
        std::vector<VertexKey> stack{start};
        std::vector<VertexKey> component_vertices;
        seen.insert(start);

        while (!stack.empty()) {
            const VertexKey current = stack.back();
            stack.pop_back();
            component_vertices.push_back(current);

            for (const VertexKey& next : graph[current]) {
                if (seen.find(next) == seen.end()) {
                    seen.insert(next);
                    stack.push_back(next);
                }
            }
        }

        std::vector<VertexKey> terminals;
        for (const VertexKey& vertex : component_vertices) {
            if (graph[vertex].size() == 1) {
                terminals.push_back(vertex);
            }
        }
        std::sort(terminals.begin(), terminals.end());

        int terminal_index = 0;
        for (const VertexKey& terminal : terminals) {
            ++terminal_index;
            const auto lake_found = lake_hexes_by_vertex.find(terminal);
            if (lake_found == lake_hexes_by_vertex.end()) {
                continue;
            }
            const auto edge_found = incident_edges.find(terminal);
            if (edge_found == incident_edges.end() || edge_found->second.empty()) {
                continue;
            }

            connections.push_back({
                static_cast<int>(connections.size()) + 1,
                component_id,
                terminal_index,
                terminal,
                lake_found->second,
                edge_found->second.front(),
            });
        }
    }

    return connections;
}

std::map<Coord, int, decltype(coord_less)*> derive_grassland_distances(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    std::map<Coord, int, decltype(coord_less)*> distances(coord_less);
    std::vector<Coord> queue;

    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (is_steppe_hex(q, r, args) || valley_hexes.find(coord) != valley_hexes.end()) {
                distances[coord] = 0;
                queue.push_back(coord);
            }
        }
    }

    for (std::size_t index = 0; index < queue.size(); ++index) {
        const Coord current = queue[index];
        const int current_distance = distances[current];
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(current, direction);
            if (!in_bounds(neighbor, args) || distances.find(neighbor) != distances.end()) {
                continue;
            }
            distances[neighbor] = current_distance + 1;
            queue.push_back(neighbor);
        }
    }

    return distances;
}

std::set<Coord, decltype(coord_less)*> derive_eastern_desert_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::vector<Town>& towns
) {
    std::set<Coord, decltype(coord_less)*> desert_hexes(coord_less);
    std::set<Coord, decltype(coord_less)*> protected_hexes(coord_less);
    for (const Town& town : towns) {
        protected_hexes.insert(town.coord);
    }

    const Coord gate = dzungarian_gate_target(args);
    const double center_q = std::max(1.0, std::min(
        static_cast<double>(args.width),
        static_cast<double>(gate.q) + static_cast<double>(args.width) * (0.08 + unit_noise(args.seed, 81201) * 0.08)
    ));
    const double center_r = std::max(1.0, std::min(
        static_cast<double>(args.height),
        static_cast<double>(gate.r) + static_cast<double>(args.height) * (0.12 + unit_noise(args.seed, 81202) * 0.08)
    ));
    const double radius_q = std::max(4.0, static_cast<double>(args.width) * (0.13 + unit_noise(args.seed, 81203) * 0.045));
    const double radius_r = std::max(4.0, static_cast<double>(args.height) * (0.13 + unit_noise(args.seed, 81204) * 0.045));
    const double angle = (14.0 + signed_noise(args.seed, 81205) * 8.0) * 3.14159265358979323846 / 180.0;
    const double cos_angle = std::cos(angle);
    const double sin_angle = std::sin(angle);

    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (lake_hexes.find(coord) != lake_hexes.end()
                || valley_hexes.find(coord) != valley_hexes.end()) {
                continue;
            }
            if (protected_hexes.find(coord) != protected_hexes.end()) {
                continue;
            }
            const double x = args.width == 1
                ? 0.5
                : static_cast<double>(q - 1) / static_cast<double>(args.width - 1);
            const double steppe_center_r = steppe_center_r_for_x(args, x);
            const double southern_steppe_fringe = static_cast<double>(r) - steppe_center_r;
            const bool base_steppe = is_steppe_hex(q, r, args);
            if (base_steppe && southern_steppe_fringe < steppe_average_half_width(args) * 0.12) {
                continue;
            }

            const double dx = static_cast<double>(q) - center_q;
            const double dy = static_cast<double>(r) - center_r;
            const double rotated_q = dx * cos_angle - dy * sin_angle;
            const double rotated_r = dx * sin_angle + dy * cos_angle;
            const double distance = (rotated_q * rotated_q) / (radius_q * radius_q)
                + (rotated_r * rotated_r) / (radius_r * radius_r);
            const double coarse = signed_noise(
                args.seed,
                static_cast<std::uint32_t>(81250 + (q / 4) * 167 + (r / 4) * 349)
            ) * 0.16;
            const double fine = signed_noise(args.seed, static_cast<std::uint32_t>(81300 + q * 193 + r * 419)) * 0.045;
            const double steppe_penalty = base_steppe ? 0.12 : 0.0;
            if (distance + coarse + fine + steppe_penalty <= 1.0) {
                desert_hexes.insert(coord);
            }
        }
    }

    return desert_hexes;
}

int hex_grid_distance(const Coord& first, const Coord& second);

std::map<Coord, std::string, decltype(coord_less)*> derive_desert_river_corridor_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& desert_hexes,
    const std::vector<RiverEdge>& river_edges
) {
    std::map<Coord, std::string, decltype(coord_less)*> corridor_hexes(coord_less);
    std::map<Coord, int, decltype(coord_less)*> corridor_distances(coord_less);
    const int primary_distance = std::max(1, static_cast<int>(std::ceil(args.valley_thickness)) + 1);
    const int max_distance = primary_distance + 2;

    const auto corridor_terrain = [&](const Coord& coord, int river_distance) {
        const double roll = unit_noise(args.seed, static_cast<std::uint32_t>(81400 + coord.q * 197 + coord.r * 431));
        const bool directly_river_adjacent = river_distance == 0;
        if (river_distance <= primary_distance) {
            if (directly_river_adjacent && roll < 0.035) {
                return std::string("marsh");
            }
            if (roll < 0.047) {
                return std::string("forest");
            }
            return std::string("grassland");
        }

        if (directly_river_adjacent && roll < 0.070) {
            return std::string("marsh");
        }
        if (roll < 0.092) {
            return std::string("forest");
        }
        return std::string("grassland");
    };

    for (const RiverEdge& edge : river_edges) {
        const Coord anchors[] = {edge.a, edge.b};
        for (const Coord& anchor : anchors) {
            for (int r = std::max(1, anchor.r - max_distance); r <= std::min(args.height, anchor.r + max_distance); ++r) {
                for (int q = std::max(1, anchor.q - max_distance); q <= std::min(args.width, anchor.q + max_distance); ++q) {
                    const Coord coord{q, r};
                    if (desert_hexes.find(coord) == desert_hexes.end()) {
                        continue;
                    }

                    const int river_distance = hex_grid_distance(coord, anchor);
                    if (river_distance > max_distance) {
                        continue;
                    }

                    if (river_distance > primary_distance) {
                        const double include_roll = unit_noise(
                            args.seed,
                            static_cast<std::uint32_t>(81500 + coord.q * 211 + coord.r * 389 + river_distance * 997)
                        );
                        const double threshold = river_distance == primary_distance + 1 ? 0.48 : 0.18;
                        if (include_roll >= threshold) {
                            continue;
                        }
                    }

                    const auto existing_distance = corridor_distances.find(coord);
                    if (existing_distance != corridor_distances.end() && existing_distance->second <= river_distance) {
                        continue;
                    }
                    corridor_distances[coord] = river_distance;
                    corridor_hexes[coord] = corridor_terrain(coord, river_distance);
                }
            }
        }
    }

    return corridor_hexes;
}

const char* wild_terrain_for_distance(const GenerateArgs& args, const Coord& coord, int grassland_distance) {
    const double coarse_texture = signed_noise(
        args.seed,
        static_cast<std::uint32_t>(75100 + (coord.q / 5) * 137 + (coord.r / 5) * 257)
    ) * 0.45;
    const double fine_roll = unit_noise(args.seed, static_cast<std::uint32_t>(75200 + coord.q * 197 + coord.r * 431));
    const double adjusted_distance = std::max(1.0, static_cast<double>(grassland_distance) + coarse_texture);

    if (adjusted_distance <= 1.8) {
        return fine_roll < 0.58 ? "hill" : "forest";
    }
    if (adjusted_distance <= 2.6) {
        return fine_roll < 0.76 ? "hill" : "forest";
    }
    if (adjusted_distance <= 4.2) {
        return fine_roll < 0.84 ? "mountain" : "hill";
    }
    return fine_roll < 0.96 ? "mountain" : "hill";
}

bool coord_adjacent_to_lake(
    const Coord& coord,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes
) {
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = neighbor_in_direction(coord, direction);
        if (lake_hexes.find(neighbor) != lake_hexes.end()) {
            return true;
        }
    }
    return false;
}

bool coord_adjacent_to_river(const Coord& coord, const std::vector<RiverEdge>& river_edges) {
    return std::find_if(river_edges.begin(), river_edges.end(), [&coord](const RiverEdge& edge) {
        return coord_equal(edge.a, coord) || coord_equal(edge.b, coord);
    }) != river_edges.end();
}

std::optional<Coord> easternmost_river_destination(const RiverNetwork& rivers) {
    std::optional<Coord> result;
    for (const Coord& destination : rivers.destinations) {
        if (!result.has_value()
            || destination.q > result->q
            || (destination.q == result->q && destination.r > result->r)) {
            result = destination;
        }
    }
    return result;
}

std::map<Coord, std::string, decltype(coord_less)*> derive_steppe_texture_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::set<Coord, decltype(coord_less)*>& forest_blob_hexes,
    const std::vector<RiverEdge>& river_edges
) {
    std::map<Coord, std::string, decltype(coord_less)*> texture(coord_less);
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (!is_steppe_hex(q, r, args)
                || lake_hexes.find(coord) != lake_hexes.end()
                || valley_hexes.find(coord) != valley_hexes.end()
                || forest_blob_hexes.find(coord) != forest_blob_hexes.end()) {
                continue;
            }

            const bool water_adjacent = coord_adjacent_to_lake(coord, lake_hexes) || coord_adjacent_to_river(coord, river_edges);
            const double coarse = signed_noise(
                args.seed,
                static_cast<std::uint32_t>(79200 + (q / 4) * 157 + (r / 4) * 311)
            );
            const double roll = unit_noise(args.seed, static_cast<std::uint32_t>(79300 + q * 197 + r * 431));
            if (water_adjacent && roll < 0.038 + std::max(0.0, coarse) * 0.018) {
                texture[coord] = "steppe_marsh";
            } else if (roll > 0.9895 - std::max(0.0, coarse) * 0.006) {
                texture[coord] = "steppe_hill";
            } else if (roll < 0.004 + std::max(0.0, coarse) * 0.003) {
                texture[coord] = "steppe_forest";
            }
        }
    }
    return texture;
}

bool base_grassland_after_valleys(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const Coord& coord
) {
    return in_bounds(coord, args)
        && lake_hexes.find(coord) == lake_hexes.end()
        && (valley_hexes.find(coord) != valley_hexes.end() || is_steppe_hex(coord.q, coord.r, args));
}

bool adjacent_to_base_grassland(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const Coord& coord
) {
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = neighbor_in_direction(coord, direction);
        if (base_grassland_after_valleys(args, lake_hexes, valley_hexes, neighbor)) {
            return true;
        }
    }
    return false;
}

bool forest_blob_can_grow(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& blocked_hexes,
    const Coord& coord
) {
    return in_bounds(coord, args)
        && lake_hexes.find(coord) == lake_hexes.end()
        && blocked_hexes.find(coord) == blocked_hexes.end();
}

int hex_grid_distance(const Coord& first, const Coord& second) {
    const int first_col = first.q - 1;
    const int first_row = first.r - 1;
    const int second_col = second.q - 1;
    const int second_row = second.r - 1;
    const int first_x = first_col;
    const int first_z = first_row - (first_col - (first_col & 1)) / 2;
    const int first_y = -first_x - first_z;
    const int second_x = second_col;
    const int second_z = second_row - (second_col - (second_col & 1)) / 2;
    const int second_y = -second_x - second_z;
    return std::max({
        std::abs(first_x - second_x),
        std::abs(first_y - second_y),
        std::abs(first_z - second_z),
    });
}

struct CubeCoord {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

CubeCoord coord_to_cube(const Coord& coord) {
    const int col = coord.q - 1;
    const int row = coord.r - 1;
    const int x = col;
    const int z = row - (col - (col & 1)) / 2;
    const int y = -x - z;
    return {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)};
}

Coord cube_to_coord(int x, int z) {
    const int row = z + (x - (x & 1)) / 2;
    return {x + 1, row + 1};
}

Coord cube_lerp_round(const CubeCoord& first, const CubeCoord& second, double t) {
    const double x = first.x + (second.x - first.x) * t;
    const double y = first.y + (second.y - first.y) * t;
    const double z = first.z + (second.z - first.z) * t;

    int rx = static_cast<int>(std::round(x));
    int ry = static_cast<int>(std::round(y));
    int rz = static_cast<int>(std::round(z));
    const double x_diff = std::abs(static_cast<double>(rx) - x);
    const double y_diff = std::abs(static_cast<double>(ry) - y);
    const double z_diff = std::abs(static_cast<double>(rz) - z);

    if (x_diff > y_diff && x_diff > z_diff) {
        rx = -ry - rz;
    } else if (y_diff > z_diff) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    return cube_to_coord(rx, rz);
}

std::vector<Coord> straight_hex_line(const Coord& start, const Coord& end) {
    const int distance = hex_grid_distance(start, end);
    std::vector<Coord> line;
    line.reserve(static_cast<std::size_t>(distance + 1));
    const CubeCoord first = coord_to_cube(start);
    const CubeCoord second = coord_to_cube(end);

    for (int step = 0; step <= distance; ++step) {
        const double t = distance == 0 ? 0.0 : static_cast<double>(step) / static_cast<double>(distance);
        const Coord coord = cube_lerp_round(first, second, t);
        if (line.empty() || !coord_equal(line.back(), coord)) {
            line.push_back(coord);
        }
    }

    return line;
}

std::set<Coord, decltype(coord_less)*> derive_forest_blob_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::set<Coord, decltype(coord_less)*>& blocked_hexes
) {
    std::set<Coord, decltype(coord_less)*> forest_hexes(coord_less);
    std::vector<Coord> seeds;
    if (args.forest_blob_count <= 0 || args.forest_blob_radius <= 0.0) {
        return forest_hexes;
    }

    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (lake_hexes.find(coord) != lake_hexes.end()
                || blocked_hexes.find(coord) != blocked_hexes.end()
                || base_grassland_after_valleys(args, lake_hexes, valley_hexes, coord)
                || !adjacent_to_base_grassland(args, lake_hexes, valley_hexes, coord)) {
                continue;
            }
            seeds.push_back(coord);
        }
    }

    std::sort(seeds.begin(), seeds.end(), [&](const Coord& first, const Coord& second) {
        const double first_score = unit_noise(args.seed, static_cast<std::uint32_t>(78000 + first.q * 173 + first.r * 379));
        const double second_score = unit_noise(args.seed, static_cast<std::uint32_t>(78000 + second.q * 173 + second.r * 379));
        return first_score > second_score;
    });

    const int target_blobs = args.forest_blob_count;
    int blobs_built = 0;
    for (const Coord& seed : seeds) {
        if (blobs_built >= target_blobs) {
            break;
        }
        if (forest_hexes.find(seed) != forest_hexes.end()) {
            continue;
        }

        const int radius = std::max(1, static_cast<int>(std::round(args.forest_blob_radius)));
        const int disk_area = 1 + 3 * radius * (radius + 1);
        const double size_scale = 0.55 + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(78100 + blobs_built * 97 + seed.q * 11 + seed.r * 23)
        ) * 0.25;
        const int target_size = std::max(1, static_cast<int>(std::round(static_cast<double>(disk_area) * size_scale)));
        std::vector<Coord> blob;
        std::vector<Coord> frontier{seed};
        blob.push_back(seed);
        forest_hexes.insert(seed);

        while (static_cast<int>(blob.size()) < target_size && !frontier.empty()) {
            std::vector<Coord> candidates;
            for (const Coord& frontier_coord : frontier) {
                for (int direction = 0; direction < 6; ++direction) {
                    const Coord neighbor = neighbor_in_direction(frontier_coord, direction);
                    if (!forest_blob_can_grow(args, lake_hexes, blocked_hexes, neighbor)
                        || hex_grid_distance(seed, neighbor) > radius
                        || forest_hexes.find(neighbor) != forest_hexes.end()
                        || contains_coord(blob, neighbor)
                        || contains_coord(candidates, neighbor)) {
                        continue;
                    }
                    candidates.push_back(neighbor);
                }
            }
            if (candidates.empty()) {
                break;
            }

            std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
                const double first_grassland_bonus = base_grassland_after_valleys(args, lake_hexes, valley_hexes, first) ? 0.18 : 0.0;
                const double second_grassland_bonus = base_grassland_after_valleys(args, lake_hexes, valley_hexes, second) ? 0.18 : 0.0;
                const double first_score = first_grassland_bonus + unit_noise(
                    args.seed,
                    static_cast<std::uint32_t>(78200 + blobs_built * 997 + first.q * 191 + first.r * 389)
                );
                const double second_score = second_grassland_bonus + unit_noise(
                    args.seed,
                    static_cast<std::uint32_t>(78200 + blobs_built * 997 + second.q * 191 + second.r * 389)
                );
                return first_score > second_score;
            });

            const Coord next = candidates.front();
            blob.push_back(next);
            frontier.push_back(next);
            forest_hexes.insert(next);
        }

        ++blobs_built;
    }

    return forest_hexes;
}

bool final_grassland_before_towns(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const Coord& coord
) {
    return base_grassland_after_valleys(args, lake_hexes, valley_hexes, coord);
}

std::vector<Town> place_generic_lake_feature_towns(
    const GenerateArgs& args,
    const std::string& feature,
    const std::set<Coord, decltype(coord_less)*>& feature_lakes,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::set<Coord, decltype(coord_less)*>& occupied_towns,
    std::uint32_t channel
) {
    std::vector<Town> towns;
    if (feature_lakes.empty()) {
        return towns;
    }

    std::vector<Coord> candidates;
    for (const Coord& lake : feature_lakes) {
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(lake, direction);
            if (!final_grassland_before_towns(args, all_lakes, valley_hexes, neighbor)
                || occupied_towns.find(neighbor) != occupied_towns.end()
                || contains_coord(candidates, neighbor)) {
                continue;
            }
            candidates.push_back(neighbor);
        }
    }

    if (candidates.empty()) {
        return towns;
    }

    std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_valley_bonus = valley_hexes.find(first) != valley_hexes.end() ? 0.18 : 0.0;
        const double second_valley_bonus = valley_hexes.find(second) != valley_hexes.end() ? 0.18 : 0.0;
        const double first_score = first_valley_bonus + unit_noise(
            args.seed,
            channel + static_cast<std::uint32_t>(first.q * 157 + first.r * 313)
        );
        const double second_score = second_valley_bonus + unit_noise(
            args.seed,
            channel + static_cast<std::uint32_t>(second.q * 157 + second.r * 313)
        );
        return first_score > second_score;
    });

    const int target_towns = unit_noise(args.seed, channel + 19U) < 0.5 ? 1 : 2;
    for (const Coord& candidate : candidates) {
        if (static_cast<int>(towns.size()) >= target_towns) {
            break;
        }
        if (occupied_towns.find(candidate) != occupied_towns.end()) {
            continue;
        }
        towns.push_back({candidate, feature});
    }

    return towns;
}

std::vector<Coord> collect_feature_town_candidates(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& feature_lakes,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::set<Coord, decltype(coord_less)*>& occupied_towns
) {
    std::vector<Coord> candidates;
    for (const Coord& lake : feature_lakes) {
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(lake, direction);
            if (!final_grassland_before_towns(args, all_lakes, valley_hexes, neighbor)
                || occupied_towns.find(neighbor) != occupied_towns.end()
                || contains_coord(candidates, neighbor)) {
                continue;
            }
            candidates.push_back(neighbor);
        }
    }
    return candidates;
}

Coord north_east_most_coord(const std::set<Coord, decltype(coord_less)*>& coords) {
    return *std::max_element(coords.begin(), coords.end(), [](const Coord& first, const Coord& second) {
        if (first.q != second.q) {
            return first.q < second.q;
        }
        return first.r > second.r;
    });
}

std::vector<Town> generate_persian_region(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& caspian,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const RiverNetwork& rivers,
    const std::set<Coord, decltype(coord_less)*>& occupied_towns
) {
    std::vector<Town> towns;
    if (caspian.empty()) {
        return towns;
    }

    std::set<Coord, decltype(coord_less)*> local_occupied = occupied_towns;
    const auto add_persian_town = [&](const Coord& coord) {
        if (local_occupied.find(coord) != local_occupied.end()
            || !final_grassland_before_towns(args, all_lakes, valley_hexes, coord)) {
            return false;
        }
        towns.push_back({coord, "persian_town"});
        local_occupied.insert(coord);
        return true;
    };

    const Coord north_east_lake = north_east_most_coord(caspian);
    std::vector<Coord> north_east_candidates;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = neighbor_in_direction(north_east_lake, direction);
        if (final_grassland_before_towns(args, all_lakes, valley_hexes, neighbor)
            && local_occupied.find(neighbor) == local_occupied.end()) {
            north_east_candidates.push_back(neighbor);
        }
    }

    std::sort(north_east_candidates.begin(), north_east_candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_score = static_cast<double>(first.q) * 3.0 - static_cast<double>(first.r)
            + unit_noise(args.seed, static_cast<std::uint32_t>(76100 + first.q * 131 + first.r * 277)) * 0.25;
        const double second_score = static_cast<double>(second.q) * 3.0 - static_cast<double>(second.r)
            + unit_noise(args.seed, static_cast<std::uint32_t>(76100 + second.q * 131 + second.r * 277)) * 0.25;
        return first_score > second_score;
    });

    std::optional<Coord> first_caspian_town;
    if (!north_east_candidates.empty()) {
        if (add_persian_town(north_east_candidates.front())) {
            first_caspian_town = north_east_candidates.front();
        }
    }

    std::vector<Coord> caspian_candidates = collect_feature_town_candidates(args, caspian, all_lakes, valley_hexes, local_occupied);
    if (first_caspian_town.has_value()) {
        caspian_candidates.erase(std::remove_if(
            caspian_candidates.begin(),
            caspian_candidates.end(),
            [&](const Coord& candidate) {
                return hex_grid_distance(first_caspian_town.value(), candidate) <= 3;
            }
        ), caspian_candidates.end());
    }
    std::sort(caspian_candidates.begin(), caspian_candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_valley_bonus = valley_hexes.find(first) != valley_hexes.end() ? 0.18 : 0.0;
        const double second_valley_bonus = valley_hexes.find(second) != valley_hexes.end() ? 0.18 : 0.0;
        const double first_score = first_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(76200 + first.q * 157 + first.r * 313)
        );
        const double second_score = second_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(76200 + second.q * 157 + second.r * 313)
        );
        return first_score > second_score;
    });
    if (!caspian_candidates.empty()) {
        add_persian_town(caspian_candidates.front());
    }

    const RiverSegment* western_river = nullptr;
    for (const RiverSegment& segment : rivers.segments) {
        if (western_river == nullptr
            || segment.from.q < western_river->from.q
            || (segment.from.q == western_river->from.q && segment.from.r < western_river->from.r)) {
            western_river = &segment;
        }
    }
    if (western_river != nullptr) {
        std::vector<Coord> river_candidates;
        for (const RiverEdge& edge : western_river->edge_path) {
            const Coord edge_coords[] = {edge.a, edge.b};
            for (const Coord& coord : edge_coords) {
                if (!final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                    || local_occupied.find(coord) != local_occupied.end()
                    || contains_coord(river_candidates, coord)) {
                    continue;
                }
                river_candidates.push_back(coord);
            }
        }
        const double river_preferred_row = static_cast<double>(args.height + 1) * 0.5
            + (unit_noise(args.seed, 76319) * 2.0 - 1.0) * 4.0;
        std::sort(river_candidates.begin(), river_candidates.end(), [&](const Coord& first, const Coord& second) {
            const double first_score = -std::abs(static_cast<double>(first.r) - river_preferred_row)
                + unit_noise(args.seed, static_cast<std::uint32_t>(76300 + first.q * 149 + first.r * 293)) * 0.08;
            const double second_score = -std::abs(static_cast<double>(second.r) - river_preferred_row)
                + unit_noise(args.seed, static_cast<std::uint32_t>(76300 + second.q * 149 + second.r * 293)) * 0.08;
            return first_score > second_score;
        });
        if (!river_candidates.empty()) {
            add_persian_town(river_candidates.front());
        }
    }

    std::vector<Coord> western_steppe_candidates;
    const int western_band = std::max(1, static_cast<int>(std::ceil(static_cast<double>(args.width) * 0.08)));
    const double preferred_row = static_cast<double>(args.height) * 0.58;
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= western_band; ++q) {
            const Coord coord{q, r};
            if (!is_steppe_hex(q, r, args)
                || !final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                || local_occupied.find(coord) != local_occupied.end()) {
                continue;
            }
            western_steppe_candidates.push_back(coord);
        }
    }
    std::sort(western_steppe_candidates.begin(), western_steppe_candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_score = -static_cast<double>(first.q) * 4.0
            - std::abs(static_cast<double>(first.r) - preferred_row) * 0.08
            + unit_noise(args.seed, static_cast<std::uint32_t>(76400 + first.q * 181 + first.r * 337)) * 0.35;
        const double second_score = -static_cast<double>(second.q) * 4.0
            - std::abs(static_cast<double>(second.r) - preferred_row) * 0.08
            + unit_noise(args.seed, static_cast<std::uint32_t>(76400 + second.q * 181 + second.r * 337)) * 0.35;
        return first_score > second_score;
    });
    if (!western_steppe_candidates.empty()) {
        add_persian_town(western_steppe_candidates.front());
    }

    return towns;
}

std::vector<Town> generate_chinese_region(
    const GenerateArgs& args,
    const LakeNetworkOverlay& chinese_lake_network,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const RiverNetwork& rivers,
    const std::set<Coord, decltype(coord_less)*>& occupied_towns
) {
    std::vector<Town> towns;
    std::set<Coord, decltype(coord_less)*> chinese_lakes(coord_less);
    for (const Coord& coord : chinese_lake_network.lake_hexes) {
        chinese_lakes.insert(coord);
    }
    if (chinese_lakes.empty()) {
        return towns;
    }

    std::set<Coord, decltype(coord_less)*> local_occupied = occupied_towns;
    const auto add_chinese_town = [&](const Coord& coord, std::vector<std::string> labels = {}) {
        if (local_occupied.find(coord) != local_occupied.end()
            || !final_grassland_before_towns(args, all_lakes, valley_hexes, coord)) {
            return false;
        }
        towns.push_back({coord, "chinese_town", labels});
        local_occupied.insert(coord);
        return true;
    };
    const auto force_chinese_town = [&](const Coord& coord, std::vector<std::string> labels = {}) {
        if (!in_bounds(coord, args)
            || all_lakes.find(coord) != all_lakes.end()
            || local_occupied.find(coord) != local_occupied.end()) {
            return false;
        }
        towns.push_back({coord, "chinese_town", labels});
        local_occupied.insert(coord);
        return true;
    };

    std::vector<Coord> lake_candidates = collect_feature_town_candidates(args, chinese_lakes, all_lakes, valley_hexes, local_occupied);
    std::sort(lake_candidates.begin(), lake_candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_valley_bonus = valley_hexes.find(first) != valley_hexes.end() ? 0.18 : 0.0;
        const double second_valley_bonus = valley_hexes.find(second) != valley_hexes.end() ? 0.18 : 0.0;
        const double first_score = first_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(76500 + first.q * 157 + first.r * 313)
        );
        const double second_score = second_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(76500 + second.q * 157 + second.r * 313)
        );
        return first_score > second_score;
    });
    for (const Coord& candidate : lake_candidates) {
        if (static_cast<int>(towns.size()) >= 2) {
            break;
        }
        const bool adjacent_to_existing = std::find_if(towns.begin(), towns.end(), [&](const Town& existing) {
            return hex_grid_distance(existing.coord, candidate) <= 1;
        }) != towns.end();
        if (adjacent_to_existing) {
            continue;
        }
        add_chinese_town(candidate);
    }

    const RiverSegment* eastern_river = nullptr;
    for (const RiverSegment& segment : rivers.segments) {
        if (eastern_river == nullptr
            || segment.from.q > eastern_river->from.q
            || (segment.from.q == eastern_river->from.q && segment.from.r < eastern_river->from.r)) {
            eastern_river = &segment;
        }
    }
    if (eastern_river != nullptr) {
        std::vector<Coord> river_candidates;
        for (const RiverEdge& edge : eastern_river->edge_path) {
            const Coord edge_coords[] = {edge.a, edge.b};
            for (const Coord& coord : edge_coords) {
                if (!final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                    || local_occupied.find(coord) != local_occupied.end()
                    || contains_coord(river_candidates, coord)) {
                    continue;
                }
                river_candidates.push_back(coord);
            }
        }
        const double river_preferred_row = static_cast<double>(args.height + 1) * 0.5
            + (unit_noise(args.seed, 76519) * 2.0 - 1.0) * 4.0;
        std::sort(river_candidates.begin(), river_candidates.end(), [&](const Coord& first, const Coord& second) {
            const double first_score = -std::abs(static_cast<double>(first.r) - river_preferred_row)
                + unit_noise(args.seed, static_cast<std::uint32_t>(76600 + first.q * 149 + first.r * 293)) * 0.08;
            const double second_score = -std::abs(static_cast<double>(second.r) - river_preferred_row)
                + unit_noise(args.seed, static_cast<std::uint32_t>(76600 + second.q * 149 + second.r * 293)) * 0.08;
            return first_score > second_score;
        });
        if (!river_candidates.empty()) {
            add_chinese_town(river_candidates.front());
        }
    }

    if (const std::optional<Coord> access_destination = easternmost_river_destination(rivers); access_destination.has_value()) {
        std::vector<Coord> access_candidates;
        for (int radius = 1; radius <= 6; ++radius) {
            for (int r = std::max(1, access_destination->r - radius); r <= std::min(args.height, access_destination->r + radius); ++r) {
                for (int q = std::max(1, access_destination->q - radius); q <= std::min(args.width, access_destination->q + radius); ++q) {
                    const Coord coord{q, r};
                    if (hex_grid_distance(coord, access_destination.value()) > radius
                        || !in_bounds(coord, args)
                        || all_lakes.find(coord) != all_lakes.end()
                        || local_occupied.find(coord) != local_occupied.end()
                        || !coord_adjacent_to_river(coord, rivers.edges)
                        || contains_coord(access_candidates, coord)) {
                        continue;
                    }
                    access_candidates.push_back(coord);
                }
            }
            if (!access_candidates.empty()) {
                break;
            }
        }

        std::sort(access_candidates.begin(), access_candidates.end(), [&](const Coord& first, const Coord& second) {
            const int first_distance = hex_grid_distance(first, access_destination.value());
            const int second_distance = hex_grid_distance(second, access_destination.value());
            const bool first_grassland = final_grassland_before_towns(args, all_lakes, valley_hexes, first);
            const bool second_grassland = final_grassland_before_towns(args, all_lakes, valley_hexes, second);
            const double first_score = (first_distance == 1 ? 80.0 : 0.0)
                + (first_grassland ? 20.0 : 0.0)
                - static_cast<double>(first_distance) * 3.0
                + unit_noise(args.seed, static_cast<std::uint32_t>(76840 + first.q * 211 + first.r * 397)) * 0.25;
            const double second_score = (second_distance == 1 ? 80.0 : 0.0)
                + (second_grassland ? 20.0 : 0.0)
                - static_cast<double>(second_distance) * 3.0
                + unit_noise(args.seed, static_cast<std::uint32_t>(76840 + second.q * 211 + second.r * 397)) * 0.25;
            if (first_score != second_score) {
                return first_score > second_score;
            }
            return coord_less(first, second);
        });

        if (!access_candidates.empty()) {
            force_chinese_town(access_candidates.front(), {"china_access_town"});
        }
    }

    std::vector<Coord> eastern_steppe_candidates;
    const int eastern_band = std::max(1, static_cast<int>(std::ceil(static_cast<double>(args.width) * 0.08)));
    const int min_eastern_q = std::max(1, args.width - eastern_band + 1);
    const double edge_preferred_row = static_cast<double>(args.height) * 0.68
        + (unit_noise(args.seed, 76719) * 2.0 - 1.0) * 4.0;
    for (int r = 1; r <= args.height; ++r) {
        for (int q = min_eastern_q; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (!is_steppe_hex(q, r, args)
                || !final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                || local_occupied.find(coord) != local_occupied.end()) {
                continue;
            }
            eastern_steppe_candidates.push_back(coord);
        }
    }
    std::sort(eastern_steppe_candidates.begin(), eastern_steppe_candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_score = static_cast<double>(first.q) * 4.0
            - std::abs(static_cast<double>(first.r) - edge_preferred_row) * 0.10
            + unit_noise(args.seed, static_cast<std::uint32_t>(76700 + first.q * 181 + first.r * 337)) * 0.35;
        const double second_score = static_cast<double>(second.q) * 4.0
            - std::abs(static_cast<double>(second.r) - edge_preferred_row) * 0.10
            + unit_noise(args.seed, static_cast<std::uint32_t>(76700 + second.q * 181 + second.r * 337)) * 0.35;
        return first_score > second_score;
    });
    if (!eastern_steppe_candidates.empty()) {
        add_chinese_town(eastern_steppe_candidates.front());
    }

    return towns;
}

std::vector<Town> place_fixed_feature_towns(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& baikal,
    const std::set<Coord, decltype(coord_less)*>& caspian,
    const LakeNetworkOverlay& chinese_lake_network,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const RiverNetwork& rivers
) {
    std::vector<Town> towns;
    std::set<Coord, decltype(coord_less)*> occupied_towns(coord_less);
    const auto add_feature_towns = [&](const std::vector<Town>& feature_towns) {
        for (const Town& town : feature_towns) {
            if (occupied_towns.find(town.coord) != occupied_towns.end()) {
                continue;
            }
            towns.push_back(town);
            occupied_towns.insert(town.coord);
        }
    };

    add_feature_towns(place_generic_lake_feature_towns(args, "baikal", baikal, all_lakes, valley_hexes, occupied_towns, 76001));
    add_feature_towns(generate_persian_region(args, caspian, all_lakes, valley_hexes, rivers, occupied_towns));
    add_feature_towns(generate_chinese_region(args, chinese_lake_network, all_lakes, valley_hexes, rivers, occupied_towns));

    std::sort(towns.begin(), towns.end(), [](const Town& first, const Town& second) {
        return coord_less(first.coord, second.coord);
    });
    return towns;
}

std::optional<Town> place_dzungarian_gate_town(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::vector<Town>& existing_towns
) {
    std::set<Coord, decltype(coord_less)*> occupied_towns(coord_less);
    for (const Town& town : existing_towns) {
        occupied_towns.insert(town.coord);
    }

    const Coord target = dzungarian_gate_target(args);
    std::vector<Coord> candidates;
    for (int radius = 0; radius <= 8 && candidates.empty(); ++radius) {
        for (int r = std::max(1, target.r - radius); r <= std::min(args.height, target.r + radius); ++r) {
            for (int q = std::max(1, target.q - radius); q <= std::min(args.width, target.q + radius); ++q) {
                const Coord coord{q, r};
                if (hex_grid_distance(target, coord) > radius
                    || !final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                    || occupied_towns.find(coord) != occupied_towns.end()) {
                    continue;
                }
                candidates.push_back(coord);
            }
        }
    }
    if (candidates.empty()) {
        return std::nullopt;
    }

    std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
        const int first_distance = hex_grid_distance(target, first);
        const int second_distance = hex_grid_distance(target, second);
        if (first_distance != second_distance) {
            return first_distance < second_distance;
        }
        const double first_noise = unit_noise(args.seed, static_cast<std::uint32_t>(76900 + first.q * 199 + first.r * 421));
        const double second_noise = unit_noise(args.seed, static_cast<std::uint32_t>(76900 + second.q * 199 + second.r * 421));
        if (first_noise != second_noise) {
            return first_noise < second_noise;
        }
        return coord_less(first, second);
    });

    return Town{candidates.front(), "dzungarian_gate"};
}

std::optional<Town> place_oasis_town(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::vector<RiverEdge>& river_edges,
    const std::vector<Town>& existing_towns
) {
    std::set<Coord, decltype(coord_less)*> occupied_towns(coord_less);
    for (const Town& town : existing_towns) {
        occupied_towns.insert(town.coord);
    }

    std::vector<Coord> candidates;
    for (const RiverEdge& edge : river_edges) {
        const Coord edge_coords[] = {edge.a, edge.b};
        for (const Coord& coord : edge_coords) {
            if (!final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                || occupied_towns.find(coord) != occupied_towns.end()
                || contains_coord(candidates, coord)) {
                continue;
            }
            candidates.push_back(coord);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const Coord center{
        std::max(1, static_cast<int>(std::round(static_cast<double>(args.width) * 0.5))),
        std::max(1, static_cast<int>(std::round(static_cast<double>(args.height) * 0.5))),
    };
    std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_valley_bonus = valley_hexes.find(first) != valley_hexes.end() ? 0.65 : 0.0;
        const double second_valley_bonus = valley_hexes.find(second) != valley_hexes.end() ? 0.65 : 0.0;
        const double first_score = static_cast<double>(hex_grid_distance(first, center)) - first_valley_bonus
            + unit_noise(args.seed, static_cast<std::uint32_t>(76800 + first.q * 173 + first.r * 359)) * 0.18;
        const double second_score = static_cast<double>(hex_grid_distance(second, center)) - second_valley_bonus
            + unit_noise(args.seed, static_cast<std::uint32_t>(76800 + second.q * 173 + second.r * 359)) * 0.18;
        if (first_score != second_score) {
            return first_score < second_score;
        }
        return coord_less(first, second);
    });

    return Town{candidates.front(), "oasis"};
}

std::vector<Town> place_extra_grassland_towns(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::vector<RiverEdge>& river_edges,
    const std::vector<Town>& existing_towns
) {
    std::set<Coord, decltype(coord_less)*> occupied_towns(coord_less);
    for (const Town& town : existing_towns) {
        occupied_towns.insert(town.coord);
    }

    std::vector<Coord> candidates;
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            if (!final_grassland_before_towns(args, all_lakes, valley_hexes, coord)
                || occupied_towns.find(coord) != occupied_towns.end()
                || (!coord_adjacent_to_lake(coord, all_lakes) && !coord_adjacent_to_river(coord, river_edges))) {
                continue;
            }
            candidates.push_back(coord);
        }
    }

    std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
        const double first_valley_bonus = valley_hexes.find(first) != valley_hexes.end() ? 0.12 : 0.0;
        const double second_valley_bonus = valley_hexes.find(second) != valley_hexes.end() ? 0.12 : 0.0;
        const double first_score = first_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(77000 + first.q * 181 + first.r * 397)
        );
        const double second_score = second_valley_bonus + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(77000 + second.q * 181 + second.r * 397)
        );
        return first_score > second_score;
    });

    const int target_towns = 2 + static_cast<int>(std::floor(unit_noise(args.seed, 77019) * 3.0));
    std::vector<Town> towns;
    for (const Coord& candidate : candidates) {
        if (static_cast<int>(towns.size()) >= target_towns) {
            break;
        }
        towns.push_back({candidate, "grassland_water"});
    }
    return towns;
}

struct PathQueueNode {
    Coord coord;
    double estimated_total = 0.0;
    double cost_so_far = 0.0;
    int order = 0;
};

struct PathQueueCompare {
    bool operator()(const PathQueueNode& first, const PathQueueNode& second) const {
        if (first.estimated_total != second.estimated_total) {
            return first.estimated_total > second.estimated_total;
        }
        if (first.cost_so_far != second.cost_so_far) {
            return first.cost_so_far > second.cost_so_far;
        }
        return first.order > second.order;
    }
};

template <typename PassableFn, typename StepCostFn, typename HeuristicFn>
std::vector<Coord> find_hex_path(
    const GenerateArgs& args,
    const Coord& start,
    const Coord& end,
    PassableFn passable,
    StepCostFn step_cost,
    HeuristicFn heuristic
) {
    if (!in_bounds(start, args) || !in_bounds(end, args) || !passable(start) || !passable(end)) {
        return {};
    }

    std::priority_queue<PathQueueNode, std::vector<PathQueueNode>, PathQueueCompare> frontier;
    std::map<Coord, double, decltype(coord_less)*> costs(coord_less);
    std::map<Coord, Coord, decltype(coord_less)*> came_from(coord_less);
    int order = 0;

    costs[start] = 0.0;
    frontier.push({start, heuristic(start), 0.0, order++});

    while (!frontier.empty()) {
        const PathQueueNode current_node = frontier.top();
        frontier.pop();
        const auto known_cost = costs.find(current_node.coord);
        if (known_cost == costs.end() || current_node.cost_so_far > known_cost->second + 0.000001) {
            continue;
        }
        if (coord_equal(current_node.coord, end)) {
            std::vector<Coord> path{end};
            Coord current = end;
            while (!coord_equal(current, start)) {
                const auto previous = came_from.find(current);
                if (previous == came_from.end()) {
                    return {};
                }
                current = previous->second;
                path.push_back(current);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(current_node.coord, direction);
            if (!in_bounds(neighbor, args) || !passable(neighbor)) {
                continue;
            }

            const double next_cost = current_node.cost_so_far + step_cost(current_node.coord, neighbor);
            const auto existing_cost = costs.find(neighbor);
            if (existing_cost != costs.end() && next_cost >= existing_cost->second - 0.000001) {
                continue;
            }

            costs[neighbor] = next_cost;
            came_from[neighbor] = current_node.coord;
            frontier.push({neighbor, next_cost + heuristic(neighbor), next_cost, order++});
        }
    }

    return {};
}

std::vector<Coord> route_road_path(
    const GenerateArgs& args,
    const Coord& start,
    const Coord& end,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>* required_grassland = nullptr
) {
    const auto passable = [&](const Coord& coord) {
        return in_bounds(coord, args)
            && lake_hexes.find(coord) == lake_hexes.end()
            && (required_grassland == nullptr
                || coord_equal(coord, start)
                || coord_equal(coord, end)
                || final_grassland_before_towns(args, lake_hexes, *required_grassland, coord));
    };
    const auto step_cost = [&](const Coord& from, const Coord& to) {
        return 1.0 + unit_noise(
            args.seed,
            static_cast<std::uint32_t>(79000 + from.q * 191 + from.r * 389 + to.q * 571 + to.r * 733)
        ) * 0.025;
    };
    const auto heuristic = [&](const Coord& coord) {
        return static_cast<double>(hex_grid_distance(coord, end));
    };

    return find_hex_path(args, start, end, passable, step_cost, heuristic);
}

bool road_cleanup_line_allowed(
    const GenerateArgs& args,
    const std::vector<Coord>& line,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>* required_grassland
) {
    if (line.empty()) {
        return false;
    }

    for (std::size_t i = 0; i < line.size(); ++i) {
        if (!in_bounds(line[i], args) || lake_hexes.find(line[i]) != lake_hexes.end()) {
            return false;
        }
        if (i > 0 && hex_grid_distance(line[i - 1], line[i]) != 1) {
            return false;
        }
        if (required_grassland != nullptr
            && i > 0
            && i + 1 < line.size()
            && !final_grassland_before_towns(args, lake_hexes, *required_grassland, line[i])) {
            return false;
        }
    }

    return true;
}

std::vector<Coord> clean_road_path(
    const GenerateArgs& args,
    const std::vector<Coord>& path,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>* required_grassland = nullptr
) {
    if (path.size() < 3) {
        return path;
    }

    std::vector<Coord> cleaned{path.front()};
    std::size_t current = 0;
    constexpr std::size_t max_cleanup_span = 10;
    while (current + 1 < path.size()) {
        std::size_t best_next = current + 1;
        std::vector<Coord> best_line{path[current], path[current + 1]};
        const std::size_t furthest = std::min(path.size() - 1, current + max_cleanup_span);

        for (std::size_t next = furthest; next > current + 1; --next) {
            std::vector<Coord> line = straight_hex_line(path[current], path[next]);
            if (line.size() < 2 || line.size() - 1 > next - current) {
                continue;
            }
            if (!road_cleanup_line_allowed(args, line, lake_hexes, required_grassland)) {
                continue;
            }
            best_next = next;
            best_line = std::move(line);
            break;
        }

        cleaned.insert(cleaned.end(), best_line.begin() + 1, best_line.end());
        current = best_next;
    }

    return cleaned;
}

std::vector<Road> build_region_roads(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::string& feature,
    int& next_id
) {
    std::vector<Coord> region_towns;
    for (const Town& town : towns) {
        if (town.feature == feature) {
            region_towns.push_back(town.coord);
        }
    }
    if (region_towns.size() < 2) {
        return {};
    }

    const auto q_range = std::minmax_element(region_towns.begin(), region_towns.end(), [](const Coord& first, const Coord& second) {
        return first.q < second.q;
    });
    const auto r_range = std::minmax_element(region_towns.begin(), region_towns.end(), [](const Coord& first, const Coord& second) {
        return first.r < second.r;
    });
    const bool east_west_axis = (q_range.second->q - q_range.first->q) >= (r_range.second->r - r_range.first->r);
    std::sort(region_towns.begin(), region_towns.end(), [&](const Coord& first, const Coord& second) {
        if (east_west_axis && first.q != second.q) {
            return first.q < second.q;
        }
        if (!east_west_axis && first.r != second.r) {
            return first.r < second.r;
        }
        return coord_less(first, second);
    });
    std::vector<Road> roads;
    std::set<RiverEdge, decltype(edge_less)*> selected_pairs(edge_less);

    const auto add_region_road = [&](const Coord& first, const Coord& second) {
        const RiverEdge pair = canonical_edge(first, second);
        if (selected_pairs.find(pair) != selected_pairs.end()) {
            return false;
        }
        const std::vector<Coord> path = route_road_path(args, first, second, lake_hexes);
        if (path.empty()) {
            return false;
        }
        selected_pairs.insert(pair);
        roads.push_back({next_id++, feature, path});
        return true;
    };

    for (std::size_t i = 1; i < region_towns.size(); ++i) {
        add_region_road(region_towns[i - 1], region_towns[i]);
    }

    const auto selected_graph_distance = [&](const Coord& start, const Coord& end) {
        std::queue<Coord> frontier;
        std::map<Coord, int, decltype(coord_less)*> distances(coord_less);
        frontier.push(start);
        distances[start] = 0;

        while (!frontier.empty()) {
            const Coord current = frontier.front();
            frontier.pop();
            const int current_distance = distances[current];
            if (coord_equal(current, end)) {
                return current_distance;
            }

            for (const RiverEdge& pair : selected_pairs) {
                std::optional<Coord> next;
                if (coord_equal(pair.a, current)) {
                    next = pair.b;
                } else if (coord_equal(pair.b, current)) {
                    next = pair.a;
                }
                if (!next.has_value() || distances.find(next.value()) != distances.end()) {
                    continue;
                }
                distances[next.value()] = current_distance + 1;
                frontier.push(next.value());
            }
        }

        return std::numeric_limits<int>::max();
    };

    struct RoadCandidate {
        Coord first;
        Coord second;
        int distance = 0;
        double score = 0.0;
    };

    std::vector<RoadCandidate> candidates;
    for (std::size_t i = 0; i < region_towns.size(); ++i) {
        for (std::size_t j = i + 1; j < region_towns.size(); ++j) {
            const RiverEdge pair = canonical_edge(region_towns[i], region_towns[j]);
            if (selected_pairs.find(pair) != selected_pairs.end()) {
                continue;
            }
            const int distance = hex_grid_distance(region_towns[i], region_towns[j]);
            const double axis_span = east_west_axis
                ? std::abs(region_towns[j].q - region_towns[i].q)
                : std::abs(region_towns[j].r - region_towns[i].r);
            candidates.push_back({
                region_towns[i],
                region_towns[j],
                distance,
                static_cast<double>(distance)
                    - axis_span * 0.2
                    + unit_noise(args.seed, static_cast<std::uint32_t>(
                        82600
                        + region_towns[i].q * 193
                        + region_towns[i].r * 389
                        + region_towns[j].q * 577
                        + region_towns[j].r * 761
                    )) * 0.35
            });
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const RoadCandidate& first, const RoadCandidate& second) {
        if (first.score != second.score) {
            return first.score < second.score;
        }
        if (first.distance != second.distance) {
            return first.distance < second.distance;
        }
        if (!coord_equal(first.first, second.first)) {
            return coord_less(first.first, second.first);
        }
        return coord_less(first.second, second.second);
    });

    const int maximum_pairs = static_cast<int>(region_towns.size() * (region_towns.size() - 1) / 2);
    const int target_pairs = std::min(
        maximum_pairs,
        static_cast<int>(region_towns.size()) + (region_towns.size() >= 5 ? 1 : 0)
    );
    for (const RoadCandidate& candidate : candidates) {
        if (static_cast<int>(roads.size()) >= target_pairs) {
            break;
        }
        if (selected_graph_distance(candidate.first, candidate.second) < 3) {
            continue;
        }
        add_region_road(candidate.first, candidate.second);
    }

    return roads;
}

std::optional<Coord> westernmost_town_for_feature(const std::vector<Town>& towns, const std::string& feature) {
    std::optional<Coord> result;
    for (const Town& town : towns) {
        if (town.feature != feature) {
            continue;
        }
        if (!result.has_value()
            || town.coord.q < result->q
            || (town.coord.q == result->q && town.coord.r < result->r)) {
            result = town.coord;
        }
    }
    return result;
}

std::optional<Coord> easternmost_town_for_feature(const std::vector<Town>& towns, const std::string& feature) {
    std::optional<Coord> result;
    for (const Town& town : towns) {
        if (town.feature != feature) {
            continue;
        }
        if (!result.has_value()
            || town.coord.q > result->q
            || (town.coord.q == result->q && town.coord.r > result->r)) {
            result = town.coord;
        }
    }
    return result;
}

std::optional<Road> build_silk_road(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    int& next_id
) {
    const std::optional<Coord> chinese_anchor = westernmost_town_for_feature(towns, "chinese_town");
    const std::optional<Coord> persian_anchor = easternmost_town_for_feature(towns, "persian_town");
    const auto oasis = std::find_if(towns.begin(), towns.end(), [](const Town& town) {
        return town.feature == "oasis";
    });
    const auto dzungarian_gate = std::find_if(towns.begin(), towns.end(), [](const Town& town) {
        return town.feature == "dzungarian_gate";
    });
    if (!chinese_anchor.has_value() || !persian_anchor.has_value() || oasis == towns.end() || dzungarian_gate == towns.end()) {
        return std::nullopt;
    }

    std::vector<Coord> persian_path = route_road_path(args, persian_anchor.value(), dzungarian_gate->coord, lake_hexes, &valley_hexes);
    const std::vector<Coord> oasis_path = route_road_path(args, dzungarian_gate->coord, oasis->coord, lake_hexes, &valley_hexes);
    const std::vector<Coord> chinese_path = route_road_path(args, oasis->coord, chinese_anchor.value(), lake_hexes, &valley_hexes);
    if (persian_path.empty() || oasis_path.empty() || chinese_path.empty()) {
        return std::nullopt;
    }
    persian_path.insert(persian_path.end(), oasis_path.begin() + 1, oasis_path.end());
    persian_path.insert(persian_path.end(), chinese_path.begin() + 1, chinese_path.end());
    return Road{next_id++, "silk_road", persian_path};
}

std::vector<Coord> clean_path_through_waypoints(
    const GenerateArgs& args,
    const std::vector<Coord>& path,
    const std::vector<Coord>& waypoints,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    if (waypoints.empty()) {
        return clean_road_path(args, path, lake_hexes, &valley_hexes);
    }

    std::vector<std::size_t> waypoint_indices;
    std::size_t search_start = 0;
    for (const Coord& waypoint : waypoints) {
        const auto iter = std::find_if(path.begin() + static_cast<std::ptrdiff_t>(search_start), path.end(), [&waypoint](const Coord& coord) {
            return coord_equal(coord, waypoint);
        });
        if (iter == path.end()) {
            return clean_road_path(args, path, lake_hexes, &valley_hexes);
        }
        const std::size_t index = static_cast<std::size_t>(std::distance(path.begin(), iter));
        waypoint_indices.push_back(index);
        search_start = index + 1;
    }

    std::vector<Coord> cleaned;
    std::size_t segment_start = 0;
    for (const std::size_t waypoint_index : waypoint_indices) {
        const std::vector<Coord> segment(path.begin() + static_cast<std::ptrdiff_t>(segment_start), path.begin() + static_cast<std::ptrdiff_t>(waypoint_index) + 1);
        const std::vector<Coord> segment_cleaned = clean_road_path(args, segment, lake_hexes, &valley_hexes);
        if (cleaned.empty()) {
            cleaned = segment_cleaned;
        } else if (!segment_cleaned.empty()) {
            cleaned.insert(cleaned.end(), segment_cleaned.begin() + 1, segment_cleaned.end());
        }
        segment_start = waypoint_index;
    }

    const std::vector<Coord> final_segment(path.begin() + static_cast<std::ptrdiff_t>(segment_start), path.end());
    const std::vector<Coord> final_cleaned = clean_road_path(args, final_segment, lake_hexes, &valley_hexes);
    if (cleaned.empty()) {
        cleaned = final_cleaned;
    } else if (!final_cleaned.empty()) {
        cleaned.insert(cleaned.end(), final_cleaned.begin() + 1, final_cleaned.end());
    }
    return cleaned;
}

std::vector<Road> clean_roads(
    const GenerateArgs& args,
    const std::vector<Road>& roads,
    const std::vector<Town>& towns,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    std::vector<Road> cleaned_roads = roads;
    const auto oasis = std::find_if(towns.begin(), towns.end(), [](const Town& town) {
        return town.feature == "oasis";
    });
    const auto dzungarian_gate = std::find_if(towns.begin(), towns.end(), [](const Town& town) {
        return town.feature == "dzungarian_gate";
    });

    for (Road& road : cleaned_roads) {
        if (road.feature == "silk_road" && oasis != towns.end() && dzungarian_gate != towns.end()) {
            road.path = clean_path_through_waypoints(args, road.path, {dzungarian_gate->coord, oasis->coord}, lake_hexes, valley_hexes);
        } else {
            road.path = clean_road_path(args, road.path, lake_hexes);
        }
    }

    return cleaned_roads;
}

std::vector<Road> generate_roads(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    std::vector<Road> roads;
    int next_id = 1;
    const std::vector<Road> persian_roads = build_region_roads(args, towns, lake_hexes, "persian_town", next_id);
    roads.insert(roads.end(), persian_roads.begin(), persian_roads.end());
    const std::vector<Road> chinese_roads = build_region_roads(args, towns, lake_hexes, "chinese_town", next_id);
    roads.insert(roads.end(), chinese_roads.begin(), chinese_roads.end());
    const std::optional<Road> silk_road = build_silk_road(args, towns, lake_hexes, valley_hexes, next_id);
    if (silk_road.has_value()) {
        roads.push_back(silk_road.value());
    }
    return clean_roads(args, roads, towns, lake_hexes, valley_hexes);
}

bool wall_hex_allowed(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::vector<Town>& towns,
    const Coord& coord
) {
    if (!in_bounds(coord, args) || lake_hexes.find(coord) != lake_hexes.end()) {
        return false;
    }
    return std::find_if(towns.begin(), towns.end(), [&coord](const Town& town) {
        return coord_equal(town.coord, coord);
    }) == towns.end();
}

std::vector<Coord> wall_anchor_candidates_near(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::vector<Town>& towns,
    const Coord& target,
    int radius
) {
    std::vector<Coord> candidates;
    for (int r = std::max(1, target.r - radius); r <= std::min(args.height, target.r + radius); ++r) {
        for (int q = std::max(1, target.q - radius); q <= std::min(args.width, target.q + radius); ++q) {
            const Coord coord{q, r};
            if (hex_grid_distance(coord, target) <= radius
                && wall_hex_allowed(args, lake_hexes, towns, coord)) {
                candidates.push_back(coord);
            }
        }
    }
    return candidates;
}

std::optional<Coord> find_wall_sw_anchor(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::vector<RiverEdge>& river_edges,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    std::vector<Town> chinese_towns;
    std::copy_if(towns.begin(), towns.end(), std::back_inserter(chinese_towns), [](const Town& town) {
        return town.feature == "chinese_town";
    });
    if (chinese_towns.empty()) {
        return std::nullopt;
    }
    const Town western_town = *std::min_element(chinese_towns.begin(), chinese_towns.end(), [](const Town& first, const Town& second) {
        if (first.coord.q != second.coord.q) {
            return first.coord.q < second.coord.q;
        }
        return first.coord.r < second.coord.r;
    });

    std::optional<RiverEdge> best_edge;
    double best_score = -std::numeric_limits<double>::infinity();
    const double preferred_row = std::max(1.0, static_cast<double>(western_town.coord.r) - static_cast<double>(args.height) * 0.12);
    for (const RiverEdge& edge : river_edges) {
        const double mid_q = (static_cast<double>(edge.a.q) + static_cast<double>(edge.b.q)) * 0.5;
        const double mid_r = (static_cast<double>(edge.a.r) + static_cast<double>(edge.b.r)) * 0.5;
        if (mid_q > static_cast<double>(western_town.coord.q) + 1.5) {
            continue;
        }
        const int distance = std::min(hex_grid_distance(edge.a, western_town.coord), hex_grid_distance(edge.b, western_town.coord));
        if (distance > std::max(8, args.width / 7)) {
            continue;
        }
        const bool valley_touch = valley_hexes.find(edge.a) != valley_hexes.end() || valley_hexes.find(edge.b) != valley_hexes.end();
        const double west_bonus = std::max(0.0, static_cast<double>(western_town.coord.q) - mid_q) * 0.55;
        const double score = (valley_touch ? 18.0 : 0.0)
            + west_bonus
            - static_cast<double>(distance) * 2.0
            - std::abs(mid_r - preferred_row) * 0.35
            + unit_noise(args.seed, static_cast<std::uint32_t>(81200 + edge.a.q * 197 + edge.a.r * 389 + edge.b.q * 587 + edge.b.r * 733)) * 0.2;
        if (score > best_score) {
            best_score = score;
            best_edge = edge;
        }
    }

    Coord target = best_edge.has_value()
        ? Coord{std::min(best_edge->a.q, best_edge->b.q) - 2, std::min(best_edge->a.r, best_edge->b.r) - 1}
        : Coord{western_town.coord.q - 5, western_town.coord.r - 5};
    target.q = std::max(1, std::min(args.width, target.q));
    target.r = std::max(1, std::min(args.height, target.r));

    for (int radius = 0; radius <= 8; ++radius) {
        std::vector<Coord> candidates = wall_anchor_candidates_near(args, lake_hexes, towns, target, radius);
        if (candidates.empty()) {
            continue;
        }
        std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
            const double first_score = -hex_grid_distance(first, target)
                - static_cast<double>(first.q) * 0.05
                - static_cast<double>(first.r) * 0.03;
            const double second_score = -hex_grid_distance(second, target)
                - static_cast<double>(second.q) * 0.05
                - static_cast<double>(second.r) * 0.03;
            if (first_score != second_score) {
                return first_score > second_score;
            }
            return coord_less(first, second);
        });
        return candidates.front();
    }

    return std::nullopt;
}

std::optional<Coord> find_wall_ne_anchor(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes
) {
    std::vector<Town> chinese_towns;
    std::copy_if(towns.begin(), towns.end(), std::back_inserter(chinese_towns), [](const Town& town) {
        return town.feature == "chinese_town";
    });
    if (chinese_towns.empty()) {
        return std::nullopt;
    }
    int easternmost_q = 1;
    int northernmost_r = args.height;
    for (const Town& town : chinese_towns) {
        easternmost_q = std::max(easternmost_q, town.coord.q);
        northernmost_r = std::min(northernmost_r, town.coord.r);
    }
    Coord target{
        std::min(args.width, easternmost_q + std::max(3, args.width / 18)),
        std::max(1, northernmost_r - std::max(3, args.height / 14))
    };
    for (int radius = 0; radius <= 10; ++radius) {
        std::vector<Coord> candidates = wall_anchor_candidates_near(args, lake_hexes, towns, target, radius);
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const Coord& candidate) {
            return candidate.q < easternmost_q - 2 || candidate.r > northernmost_r + 1;
        }), candidates.end());
        if (candidates.empty()) {
            continue;
        }
        std::sort(candidates.begin(), candidates.end(), [&](const Coord& first, const Coord& second) {
            const double first_score = -hex_grid_distance(first, target)
                + static_cast<double>(first.q) * 0.04
                - static_cast<double>(first.r) * 0.04;
            const double second_score = -hex_grid_distance(second, target)
                + static_cast<double>(second.q) * 0.04
                - static_cast<double>(second.r) * 0.04;
            if (first_score != second_score) {
                return first_score > second_score;
            }
            return coord_less(first, second);
        });
        return candidates.front();
    }

    return std::nullopt;
}

struct WallPathState {
    int edge = -1;
    VertexKey vertex;
};

bool operator<(const WallPathState& first, const WallPathState& second) {
    if (first.edge != second.edge) {
        return first.edge < second.edge;
    }
    return first.vertex < second.vertex;
}

struct WallPathQueueNode {
    WallPathState state;
    double estimated_total = 0.0;
    double cost_so_far = 0.0;
    int order = 0;
};

struct WallPathQueueCompare {
    bool operator()(const WallPathQueueNode& first, const WallPathQueueNode& second) const {
        if (first.estimated_total != second.estimated_total) {
            return first.estimated_total > second.estimated_total;
        }
        if (first.cost_so_far != second.cost_so_far) {
            return first.cost_so_far > second.cost_so_far;
        }
        return first.order > second.order;
    }
};

struct WallRouteOptions {
    double bow_multiplier = 1.0;
    double terminal_projection_min = 0.75;
    double mountain_terminal_bonus = -7.0;
    double map_edge_terminal_bonus = 2.0;
};

double wall_edge_mid_q(const BoundaryEdge& edge) {
    return (static_cast<double>(edge.edge.a.q) + static_cast<double>(edge.edge.b.q)) * 0.5;
}

double wall_edge_mid_r(const BoundaryEdge& edge) {
    return (static_cast<double>(edge.edge.a.r) + static_cast<double>(edge.edge.b.r)) * 0.5;
}

double wall_edge_anchor_distance(const BoundaryEdge& edge, const Coord& anchor) {
    return static_cast<double>(std::min(hex_grid_distance(edge.edge.a, anchor), hex_grid_distance(edge.edge.b, anchor)));
}

std::optional<int> nearest_wall_edge_to_anchor(const std::vector<BoundaryEdge>& boundaries, const Coord& anchor) {
    std::optional<int> best;
    double best_score = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < boundaries.size(); ++i) {
        const double midpoint_q = wall_edge_mid_q(boundaries[i]);
        const double midpoint_r = wall_edge_mid_r(boundaries[i]);
        const double dq = midpoint_q - static_cast<double>(anchor.q);
        const double dr = midpoint_r - static_cast<double>(anchor.r);
        const double score = dq * dq + dr * dr + wall_edge_anchor_distance(boundaries[i], anchor) * 0.25;
        if (score < best_score) {
            best_score = score;
            best = static_cast<int>(i);
        }
    }
    return best;
}

Coord wall_edge_mid_coord(const GenerateArgs& args, const BoundaryEdge& edge) {
    return {
        std::max(1, std::min(args.width, static_cast<int>(std::round(wall_edge_mid_q(edge))))),
        std::max(1, std::min(args.height, static_cast<int>(std::round(wall_edge_mid_r(edge))))),
    };
}

bool wall_blocks_outside_access_to_chinese_towns(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::vector<RiverEdge>& wall_edges,
    const std::map<Coord, Terrain, decltype(coord_less)*>& terrain_by_coord
) {
    if (wall_edges.empty()) {
        return false;
    }

    std::vector<Coord> chinese_towns;
    for (const Town& town : towns) {
        if (town.feature == "chinese_town") {
            chinese_towns.push_back(town.coord);
        }
    }
    if (chinese_towns.empty()) {
        return true;
    }

    std::set<RiverEdge, decltype(edge_less)*> wall_edge_set(edge_less);
    for (const RiverEdge& edge : wall_edges) {
        wall_edge_set.insert(canonical_edge(edge.a, edge.b));
    }

    const auto terrain_at = [&](const Coord& coord) {
        const auto found = terrain_by_coord.find(coord);
        return found == terrain_by_coord.end() ? Terrain::None : found->second;
    };
    const auto blocks_hex = [&](const Coord& coord) {
        const Terrain terrain = terrain_at(coord);
        return terrain == Terrain::Mountain || terrain == Terrain::Lake;
    };
    const auto reached_chinese_town = [&](const Coord& coord) {
        return std::find_if(chinese_towns.begin(), chinese_towns.end(), [&coord](const Coord& town) {
            return coord_equal(coord, town);
        }) != chinese_towns.end();
    };

    std::queue<Coord> frontier;
    std::set<Coord, decltype(coord_less)*> visited(coord_less);
    const auto add_outside_seed = [&](const Coord& coord) {
        if (!in_bounds(coord, args) || blocks_hex(coord) || visited.find(coord) != visited.end()) {
            return;
        }
        visited.insert(coord);
        frontier.push(coord);
    };
    for (int q = 1; q <= args.width; ++q) {
        add_outside_seed({q, 1});
    }
    for (int r = 1; r <= args.height; ++r) {
        add_outside_seed({1, r});
    }

    while (!frontier.empty()) {
        const Coord current = frontier.front();
        frontier.pop();
        if (reached_chinese_town(current)) {
            return false;
        }

        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(current, direction);
            if (!in_bounds(neighbor, args) || blocks_hex(neighbor) || visited.find(neighbor) != visited.end()) {
                continue;
            }
            if (wall_edge_set.find(canonical_edge(current, neighbor)) != wall_edge_set.end()) {
                continue;
            }
            visited.insert(neighbor);
            frontier.push(neighbor);
        }
    }

    return true;
}

std::vector<RiverEdge> route_bowed_wall_edge_path(
    const GenerateArgs& args,
    const Coord& start,
    const Coord& end,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::vector<Town>& towns,
    const std::map<Coord, Terrain, decltype(coord_less)*>& terrain_by_coord,
    const WallRouteOptions& options
) {
    std::vector<BoundaryEdge> boundaries;
    for (const RiverEdge& edge : all_map_edges(args)) {
        if (wall_hex_allowed(args, lake_hexes, towns, edge.a)
            && wall_hex_allowed(args, lake_hexes, towns, edge.b)) {
            boundaries.push_back(make_boundary_edge(edge));
        }
    }
    if (boundaries.empty()) {
        return {};
    }

    std::map<VertexKey, std::vector<int>> edges_by_vertex;
    for (std::size_t i = 0; i < boundaries.size(); ++i) {
        edges_by_vertex[boundaries[i].first].push_back(static_cast<int>(i));
        edges_by_vertex[boundaries[i].second].push_back(static_cast<int>(i));
    }

    const std::optional<int> start_edge = nearest_wall_edge_to_anchor(boundaries, start);
    const std::optional<int> end_edge = nearest_wall_edge_to_anchor(boundaries, end);
    if (!start_edge.has_value() || !end_edge.has_value()) {
        return {};
    }

    const auto terrain_at = [&](const Coord& coord) {
        const auto found = terrain_by_coord.find(coord);
        return found == terrain_by_coord.end() ? Terrain::None : found->second;
    };
    const auto touches_map_edge = [&](const BoundaryEdge& edge) {
        const Coord coords[] = {edge.edge.a, edge.edge.b};
        for (const Coord& coord : coords) {
            if (coord.q == 1 || coord.q == args.width || coord.r == 1 || coord.r == args.height) {
                return true;
            }
        }
        return false;
    };
    const auto touches_mountain = [&](const BoundaryEdge& edge) {
        return terrain_at(edge.edge.a) == Terrain::Mountain || terrain_at(edge.edge.b) == Terrain::Mountain;
    };
    const auto is_terminal_edge = [&](const BoundaryEdge& edge) {
        return touches_mountain(edge) || touches_map_edge(edge);
    };

    const auto terminal_edge_outward_from = [&](const Coord& anchor, const Coord& inward) {
        const double outward_q = static_cast<double>(anchor.q - inward.q);
        const double outward_r = static_cast<double>(anchor.r - inward.r);
        const double outward_length = std::max(1.0, std::sqrt(outward_q * outward_q + outward_r * outward_r));
        std::optional<int> best;
        double best_score = std::numeric_limits<double>::max();
        std::optional<int> fallback;
        double fallback_score = std::numeric_limits<double>::max();

        for (std::size_t i = 0; i < boundaries.size(); ++i) {
            const BoundaryEdge& edge = boundaries[i];
            if (!is_terminal_edge(edge)) {
                continue;
            }
            const double dq = wall_edge_mid_q(edge) - static_cast<double>(anchor.q);
            const double dr = wall_edge_mid_r(edge) - static_cast<double>(anchor.r);
            const double projection = (dq * outward_q + dr * outward_r) / outward_length;
            const double lateral = std::abs(dq * outward_r - dr * outward_q) / outward_length;
            const double distance = std::sqrt(dq * dq + dr * dr);
            const double terminal_bonus = touches_mountain(edge)
                ? options.mountain_terminal_bonus
                : options.map_edge_terminal_bonus;
            const double score = distance
                + lateral * 1.6
                - projection * 0.18
                + terminal_bonus
                + unit_noise(args.seed, static_cast<std::uint32_t>(81800 + i * 193 + anchor.q * 389 + anchor.r * 733)) * 0.1;
            if (score < fallback_score) {
                fallback_score = score;
                fallback = static_cast<int>(i);
            }
            if (projection < options.terminal_projection_min) {
                continue;
            }
            if (score < best_score) {
                best_score = score;
                best = static_cast<int>(i);
            }
        }

        return best.has_value() ? best : fallback;
    };

    const auto route_between_edges = [&](int route_start_edge, int route_end_edge, const Coord& route_start, const Coord& route_end, double bow_multiplier) {
        if (route_start_edge == route_end_edge) {
            return std::vector<int>{route_start_edge};
        }

        const double dx = static_cast<double>(route_end.q - route_start.q);
        const double dy = static_cast<double>(route_end.r - route_start.r);
        const double length_sq = std::max(1.0, dx * dx + dy * dy);
        const double bow = std::max(3.0, static_cast<double>(args.height) * 0.08) * bow_multiplier * options.bow_multiplier;
        const auto progress_for = [&](double q, double r) {
            const double raw_t = ((q - static_cast<double>(route_start.q)) * dx + (r - static_cast<double>(route_start.r)) * dy) / length_sq;
            return std::max(0.0, std::min(1.0, raw_t));
        };
        const auto expected_row = [&](double q, double r) {
            const double t = progress_for(q, r);
            const double linear_r = static_cast<double>(route_start.r) + dy * t;
            return linear_r - std::sin(t * 3.14159265358979323846) * bow;
        };
        const auto curve_error = [&](const BoundaryEdge& edge) {
            return std::abs(wall_edge_mid_r(edge) - expected_row(wall_edge_mid_q(edge), wall_edge_mid_r(edge)));
        };
        const auto edge_heuristic = [&](int edge_index) {
            const BoundaryEdge& boundary = boundaries[edge_index];
            return wall_edge_anchor_distance(boundary, route_end) * 0.85 + curve_error(boundary) * 0.12;
        };
        const auto step_cost = [&](int from_edge, int to_edge) {
            const BoundaryEdge& from = boundaries[from_edge];
            const BoundaryEdge& to = boundaries[to_edge];
            const double from_t = progress_for(wall_edge_mid_q(from), wall_edge_mid_r(from));
            const double to_t = progress_for(wall_edge_mid_q(to), wall_edge_mid_r(to));
            const double backtrack_penalty = to_t + 0.015 < from_t ? 1.6 : 0.0;
            const double south_penalty = wall_edge_mid_r(to) > expected_row(wall_edge_mid_q(to), wall_edge_mid_r(to)) + 2.5 ? 0.8 : 0.0;
            return 1.0
                + curve_error(to) * 0.32
                + backtrack_penalty
                + south_penalty
                + unit_noise(args.seed, static_cast<std::uint32_t>(81600 + from_edge * 131 + to_edge * 197)) * 0.04;
        };

        std::priority_queue<WallPathQueueNode, std::vector<WallPathQueueNode>, WallPathQueueCompare> frontier;
        std::map<WallPathState, double> costs;
        std::map<WallPathState, WallPathState> came_from;
        int order = 0;

        const BoundaryEdge& start_boundary = boundaries[route_start_edge];
        const WallPathState starts[] = {
            {route_start_edge, start_boundary.first},
            {route_start_edge, start_boundary.second},
        };
        for (const WallPathState& state : starts) {
            costs[state] = 0.0;
            frontier.push({state, edge_heuristic(state.edge), 0.0, order++});
        }

        while (!frontier.empty()) {
            const WallPathQueueNode current_node = frontier.top();
            frontier.pop();
            const auto known_cost = costs.find(current_node.state);
            if (known_cost == costs.end() || current_node.cost_so_far > known_cost->second + 0.000001) {
                continue;
            }
            if (current_node.state.edge == route_end_edge) {
                std::vector<int> edge_indices{current_node.state.edge};
                WallPathState current = current_node.state;
                while (current.edge != route_start_edge) {
                    const auto previous = came_from.find(current);
                    if (previous == came_from.end()) {
                        return std::vector<int>{};
                    }
                    current = previous->second;
                    edge_indices.push_back(current.edge);
                }
                std::reverse(edge_indices.begin(), edge_indices.end());
                return edge_indices;
            }

            const auto adjacent = edges_by_vertex.find(current_node.state.vertex);
            if (adjacent == edges_by_vertex.end()) {
                continue;
            }

            for (const int candidate : adjacent->second) {
                if (candidate == current_node.state.edge) {
                    continue;
                }
                const BoundaryEdge& boundary = boundaries[candidate];
                const VertexKey next_vertex = other_vertex(boundary, current_node.state.vertex);
                const WallPathState next{candidate, next_vertex};
                const double next_cost = current_node.cost_so_far + step_cost(current_node.state.edge, candidate);
                const auto existing_cost = costs.find(next);
                if (existing_cost != costs.end() && next_cost >= existing_cost->second - 0.000001) {
                    continue;
                }
                costs[next] = next_cost;
                came_from[next] = current_node.state;
                frontier.push({next, next_cost + edge_heuristic(candidate), next_cost, order++});
            }
        }

        return std::vector<int>{};
    };

    const int sw_terminal_edge = terminal_edge_outward_from(start, end).value_or(start_edge.value());
    const int ne_terminal_edge = terminal_edge_outward_from(end, start).value_or(end_edge.value());
    const Coord sw_terminal = wall_edge_mid_coord(args, boundaries[sw_terminal_edge]);
    const Coord ne_terminal = wall_edge_mid_coord(args, boundaries[ne_terminal_edge]);

    std::vector<int> combined;
    const auto append_path = [&](const std::vector<int>& edge_indices) {
        for (const int edge_index : edge_indices) {
            if (combined.empty() || combined.back() != edge_index) {
                combined.push_back(edge_index);
            }
        }
    };

    append_path(route_between_edges(sw_terminal_edge, start_edge.value(), sw_terminal, start, 0.0));
    append_path(route_between_edges(start_edge.value(), end_edge.value(), start, end, 1.0));
    append_path(route_between_edges(end_edge.value(), ne_terminal_edge, end, ne_terminal, 0.0));

    std::vector<RiverEdge> path;
    path.reserve(combined.size());
    for (const int edge_index : combined) {
        path.push_back(boundaries[edge_index].edge);
    }
    return path;
}

std::vector<Wall> generate_walls(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::vector<RiverEdge>& river_edges,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes,
    const std::vector<Hex>& hexes
) {
    const std::optional<Coord> start = find_wall_sw_anchor(args, towns, river_edges, lake_hexes, valley_hexes);
    const std::optional<Coord> end = find_wall_ne_anchor(args, towns, lake_hexes);
    if (!start.has_value() || !end.has_value() || coord_equal(start.value(), end.value())) {
        return {};
    }

    std::map<Coord, Terrain, decltype(coord_less)*> terrain_by_coord(coord_less);
    for (const Hex& hex : hexes) {
        terrain_by_coord[hex.coord] = hex.terrain;
    }

    const WallRouteOptions attempts[] = {
        {},
        {1.2, 0.55, 1.0, -8.0},
        {1.45, 0.35, 2.5, -11.0},
        {1.75, 0.15, 4.0, -13.0},
        {2.1, 0.0, 6.0, -16.0},
    };
    std::vector<RiverEdge> fallback_path;
    for (const WallRouteOptions& options : attempts) {
        const std::vector<RiverEdge> edge_path = route_bowed_wall_edge_path(
            args,
            start.value(),
            end.value(),
            lake_hexes,
            towns,
            terrain_by_coord,
            options
        );
        if (edge_path.empty()) {
            continue;
        }
        if (fallback_path.empty()) {
            fallback_path = edge_path;
        }
        if (wall_blocks_outside_access_to_chinese_towns(args, towns, edge_path, terrain_by_coord)) {
            Wall wall;
            wall.id = 1;
            wall.feature = "great_wall";
            wall.edge_path = edge_path;
            return {wall};
        }
    }

    if (fallback_path.empty()) {
        return {};
    }

    Wall wall;
    wall.id = 1;
    wall.feature = "great_wall";
    wall.edge_path = fallback_path;
    return {wall};
}

std::vector<WallGate> generate_wall_gates(
    const GenerateArgs& args,
    const std::vector<Wall>& walls,
    const std::vector<Road>& roads,
    const std::vector<Hex>& hexes
) {
    std::vector<RiverEdge> wall_edges;
    std::map<RiverEdge, int, decltype(edge_less)*> wall_edge_indices(edge_less);
    for (const Wall& wall : walls) {
        for (const RiverEdge& edge : wall.edge_path) {
            const RiverEdge canonical = canonical_edge(edge.a, edge.b);
            if (wall_edge_indices.find(canonical) == wall_edge_indices.end()) {
                wall_edge_indices[canonical] = static_cast<int>(wall_edges.size());
                wall_edges.push_back(canonical);
            }
        }
    }
    if (wall_edges.empty()) {
        return {};
    }

    std::map<Coord, Terrain, decltype(coord_less)*> terrain_by_coord(coord_less);
    for (const Hex& hex : hexes) {
        terrain_by_coord[hex.coord] = hex.terrain;
    }
    const auto terrain_at = [&](const Coord& coord) {
        const auto found = terrain_by_coord.find(coord);
        return found == terrain_by_coord.end() ? Terrain::None : found->second;
    };
    const auto gate_terrain_allowed = [&](const RiverEdge& edge) {
        const Terrain first = terrain_at(edge.a);
        const Terrain second = terrain_at(edge.b);
        return first != Terrain::Lake
            && second != Terrain::Lake
            && first != Terrain::Mountain
            && second != Terrain::Mountain;
    };

    std::vector<WallGate> gates;
    std::set<RiverEdge, decltype(edge_less)*> gate_edges(edge_less);
    const auto add_gate = [&](const RiverEdge& edge, const std::string& kind) {
        const RiverEdge canonical = canonical_edge(edge.a, edge.b);
        if (wall_edge_indices.find(canonical) == wall_edge_indices.end()
            || gate_edges.find(canonical) != gate_edges.end()
            || !gate_terrain_allowed(canonical)) {
            return false;
        }
        gates.push_back({static_cast<int>(gates.size()) + 1, kind, canonical});
        gate_edges.insert(canonical);
        return true;
    };

    for (const Road& road : roads) {
        for (std::size_t i = 1; i < road.path.size(); ++i) {
            const RiverEdge edge = canonical_edge(road.path[i - 1], road.path[i]);
            const std::string kind = road.feature == "silk_road" ? "silk_gate" : "road_gate";
            add_gate(edge, kind);
        }
    }

    const int target_gate_count = std::max(
        5,
        std::min(9, static_cast<int>(std::round(static_cast<double>(wall_edges.size()) / 18.0)))
    );
    const int minimum_spacing = std::max(8, static_cast<int>(std::round(static_cast<double>(wall_edges.size()) / static_cast<double>(target_gate_count + 3))));
    const auto far_from_existing_gates = [&](int index) {
        for (const WallGate& gate : gates) {
            const auto found = wall_edge_indices.find(canonical_edge(gate.edge.a, gate.edge.b));
            if (found == wall_edge_indices.end()) {
                continue;
            }
            if (std::abs(found->second - index) < minimum_spacing) {
                return false;
            }
        }
        return true;
    };
    const auto best_supplemental_index_near = [&](int target_index) {
        int best_index = -1;
        double best_score = std::numeric_limits<double>::max();
        const int margin = std::min(6, std::max(2, static_cast<int>(wall_edges.size()) / 20));
        const int window = std::max(minimum_spacing, 10);
        for (int offset = -window; offset <= window; ++offset) {
            const int index = target_index + offset;
            if (index < margin || index >= static_cast<int>(wall_edges.size()) - margin) {
                continue;
            }
            const RiverEdge& edge = wall_edges[index];
            if (gate_edges.find(edge) != gate_edges.end()
                || !gate_terrain_allowed(edge)
                || !far_from_existing_gates(index)) {
                continue;
            }
            const double score = std::abs(offset)
                + unit_noise(args.seed, static_cast<std::uint32_t>(82100 + index * 977 + target_index * 37)) * 0.4;
            if (score < best_score) {
                best_score = score;
                best_index = index;
            }
        }
        return best_index;
    };

    int supplemental_slots = std::max(0, target_gate_count - static_cast<int>(gates.size()));
    for (int slot = 0; slot < supplemental_slots; ++slot) {
        const int target_index = static_cast<int>(std::round(
            (static_cast<double>(slot) + 1.0)
            / (static_cast<double>(supplemental_slots) + 1.0)
            * static_cast<double>(std::max(0, static_cast<int>(wall_edges.size()) - 1))
        ));
        const int index = best_supplemental_index_near(target_index);
        if (index >= 0) {
            add_gate(wall_edges[index], "gate");
        }
    }

    std::sort(gates.begin(), gates.end(), [&](const WallGate& first, const WallGate& second) {
        const int first_index = wall_edge_indices[canonical_edge(first.edge.a, first.edge.b)];
        const int second_index = wall_edge_indices[canonical_edge(second.edge.a, second.edge.b)];
        if (first_index != second_index) {
            return first_index < second_index;
        }
        return first.id < second.id;
    });
    for (std::size_t i = 0; i < gates.size(); ++i) {
        gates[i].id = static_cast<int>(i) + 1;
    }
    return gates;
}

int distance_between_edges(const RiverEdge& first, const RiverEdge& second) {
    return std::min({
        hex_grid_distance(first.a, second.a),
        hex_grid_distance(first.a, second.b),
        hex_grid_distance(first.b, second.a),
        hex_grid_distance(first.b, second.b),
    });
}

bool edge_has_town_endpoint(const RiverEdge& edge, const std::vector<Town>& towns) {
    return std::find_if(towns.begin(), towns.end(), [&edge](const Town& town) {
        return edge_touches_coord(edge, town.coord);
    }) != towns.end();
}

bool edge_near_town(const RiverEdge& edge, const std::vector<Town>& towns, int radius) {
    return std::find_if(towns.begin(), towns.end(), [&edge, radius](const Town& town) {
        return hex_grid_distance(edge.a, town.coord) <= radius
            || hex_grid_distance(edge.b, town.coord) <= radius;
    }) != towns.end();
}

bool ford_too_close(const RiverEdge& edge, const std::vector<Crossing>& crossings, int minimum_spacing) {
    return std::find_if(crossings.begin(), crossings.end(), [&edge, minimum_spacing](const Crossing& crossing) {
        return crossing.kind == "ford" && distance_between_edges(edge, crossing.edge) <= minimum_spacing;
    }) != crossings.end();
}

bool crossing_too_close(const RiverEdge& edge, const std::vector<Crossing>& crossings, int minimum_spacing) {
    return std::find_if(crossings.begin(), crossings.end(), [&edge, minimum_spacing](const Crossing& crossing) {
        return distance_between_edges(edge, crossing.edge) <= minimum_spacing;
    }) != crossings.end();
}

void promote_nearby_fords_to_bridges(std::vector<Crossing>& crossings, const RiverEdge& edge, int minimum_spacing) {
    for (Crossing& crossing : crossings) {
        if (crossing.kind == "ford" && distance_between_edges(edge, crossing.edge) <= minimum_spacing) {
            crossing.kind = "bridge";
        }
    }
}

bool add_crossing(
    std::vector<Crossing>& crossings,
    const RiverEdge& edge,
    const std::string& kind,
    int& next_id,
    const std::vector<Town>& towns,
    bool enforce_ford_spacing
) {
    const RiverEdge canonical = canonical_edge(edge.a, edge.b);
    for (Crossing& crossing : crossings) {
        if (!edge_equal(crossing.edge, canonical)) {
            continue;
        }
        if (kind == "bridge" && crossing.kind != "bridge") {
            crossing.kind = "bridge";
        }
        return false;
    }

    if (kind == "ford") {
        if (edge_near_town(canonical, towns, 1)) {
            return false;
        }
        if (enforce_ford_spacing && crossing_too_close(canonical, crossings, 6)) {
            return false;
        }
    }

    crossings.push_back({next_id++, kind, canonical});
    return true;
}

bool crossing_exists_on_edge(const std::vector<Crossing>& crossings, const RiverEdge& edge) {
    const RiverEdge canonical = canonical_edge(edge.a, edge.b);
    return std::find_if(crossings.begin(), crossings.end(), [&canonical](const Crossing& crossing) {
        return edge_equal(crossing.edge, canonical);
    }) != crossings.end();
}

bool steppe_ford_edge(
    const GenerateArgs& args,
    const RiverEdge& edge,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    return final_grassland_before_towns(args, all_lakes, valley_hexes, edge.a)
        && final_grassland_before_towns(args, all_lakes, valley_hexes, edge.b);
}

std::vector<Crossing> generate_crossings(
    const GenerateArgs& args,
    const std::vector<Town>& towns,
    const std::vector<Road>& roads,
    const std::vector<RiverEdge>& river_edges,
    const std::vector<RiverSegment>& river_segments,
    const std::set<Coord, decltype(coord_less)*>& all_lakes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
) {
    std::vector<Crossing> crossings;
    int next_id = 1;
    std::set<RiverEdge, decltype(edge_less)*> river_edge_set(edge_less);
    for (const RiverEdge& edge : river_edges) {
        river_edge_set.insert(edge);
    }

    std::vector<std::pair<RiverEdge, bool>> road_crossings;
    for (const Road& road : roads) {
        for (std::size_t i = 1; i < road.path.size(); ++i) {
            const RiverEdge edge = canonical_edge(road.path[i - 1], road.path[i]);
            if (river_edge_set.find(edge) == river_edge_set.end()) {
                continue;
            }
            const bool bridge = edge_has_town_endpoint(edge, towns) || edge_near_town(edge, towns, 1);
            const auto existing = std::find_if(road_crossings.begin(), road_crossings.end(), [&edge](const auto& crossing) {
                return edge_equal(crossing.first, edge);
            });
            if (existing != road_crossings.end()) {
                existing->second = existing->second || bridge;
            } else {
                road_crossings.push_back({edge, bridge});
            }
        }
    }

    for (const auto& crossing : road_crossings) {
        if (crossing.second) {
            add_crossing(crossings, crossing.first, "bridge", next_id, towns, false);
        }
    }
    for (const auto& crossing : road_crossings) {
        if (crossing.second || crossing_exists_on_edge(crossings, crossing.first)) {
            continue;
        }
        if (crossing_too_close(crossing.first, crossings, 6)) {
            promote_nearby_fords_to_bridges(crossings, crossing.first, 6);
            add_crossing(crossings, crossing.first, "bridge", next_id, towns, false);
        } else {
            add_crossing(crossings, crossing.first, "ford", next_id, towns, true);
        }
    }

    for (const RiverSegment& segment : river_segments) {
        std::vector<std::pair<int, RiverEdge>> ford_candidates;
        for (std::size_t i = 0; i < segment.edge_path.size(); ++i) {
            const RiverEdge canonical = canonical_edge(segment.edge_path[i].a, segment.edge_path[i].b);
            if (!steppe_ford_edge(args, canonical, all_lakes, valley_hexes)
                || edge_near_town(canonical, towns, 1)
                || crossing_exists_on_edge(crossings, canonical)) {
                continue;
            }
            ford_candidates.push_back({static_cast<int>(i), canonical});
        }
        if (ford_candidates.empty()) {
            continue;
        }

        const int extra_ford_target = 2 + static_cast<int>(std::floor(unit_noise(args.seed, static_cast<std::uint32_t>(79519 + segment.id * 97)) * 3.0));
        for (int slot = 0; slot < extra_ford_target; ++slot) {
            const double base_position = (static_cast<double>(slot) + 1.0) / (static_cast<double>(extra_ford_target) + 1.0);
            const double jitter = signed_noise(args.seed, static_cast<std::uint32_t>(79600 + segment.id * 311 + slot * 37))
                * (0.34 / static_cast<double>(extra_ford_target));
            const double target_index = std::clamp(base_position + jitter, 0.04, 0.96)
                * static_cast<double>(std::max(1, static_cast<int>(segment.edge_path.size()) - 1));

            std::sort(ford_candidates.begin(), ford_candidates.end(), [&](const auto& first, const auto& second) {
                const double first_score = std::abs(static_cast<double>(first.first) - target_index)
                    + unit_noise(args.seed, static_cast<std::uint32_t>(80500 + segment.id * 997 + slot * 101 + first.second.a.q * 131 + first.second.a.r * 193 + first.second.b.q * 307 + first.second.b.r * 389)) * 0.4;
                const double second_score = std::abs(static_cast<double>(second.first) - target_index)
                    + unit_noise(args.seed, static_cast<std::uint32_t>(80500 + segment.id * 997 + slot * 101 + second.second.a.q * 131 + second.second.a.r * 193 + second.second.b.q * 307 + second.second.b.r * 389)) * 0.4;
                if (first_score != second_score) {
                    return first_score < second_score;
                }
                return edge_less(first.second, second.second);
            });

            for (const auto& candidate : ford_candidates) {
                if (add_crossing(crossings, candidate.second, "ford", next_id, towns, true)) {
                    break;
                }
            }
        }
    }

    std::sort(crossings.begin(), crossings.end(), [](const Crossing& first, const Crossing& second) {
        return first.id < second.id;
    });
    return crossings;
}

void print_coord(const Coord& coord) {
    std::cout << "{\"q\":" << coord.q << ",\"r\":" << coord.r << "}";
}

void print_string_array(const std::vector<std::string>& values) {
    std::cout << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "\"" << values[i] << "\"";
    }
    std::cout << "]";
}

void print_vertex(const VertexKey& vertex) {
    std::cout << "{\"x\":" << vertex.x << ",\"y\":" << vertex.y << "}";
}

void print_edge(const RiverEdge& edge) {
    std::cout << "{\"a\":";
    print_coord(edge.a);
    std::cout << ",\"b\":";
    print_coord(edge.b);
    std::cout << ",\"river\":true}";
}

void print_plain_edge(const RiverEdge& edge) {
    std::cout << "{\"a\":";
    print_coord(edge.a);
    std::cout << ",\"b\":";
    print_coord(edge.b);
    std::cout << "}";
}

void print_town(const Town& town) {
    std::cout << "{\"q\":" << town.coord.q << ",\"r\":" << town.coord.r << ",\"feature\":\"" << town.feature << "\"";
    if (!town.labels.empty()) {
        std::cout << ",\"labels\":";
        print_string_array(town.labels);
    }
    std::cout << "}";
}

void print_road(const Road& road) {
    std::cout << "{\"id\":" << road.id << ",\"feature\":\"" << road.feature << "\",\"path\":[";
    for (std::size_t i = 0; i < road.path.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(road.path[i]);
    }
    std::cout << "]}";
}

void print_crossing(const Crossing& crossing) {
    std::cout << "{\"id\":" << crossing.id << ",\"kind\":\"" << crossing.kind << "\",\"edge\":";
    print_edge(crossing.edge);
    std::cout << "}";
}

void print_wall(const Wall& wall) {
    std::cout << "{\"id\":" << wall.id << ",\"feature\":\"" << wall.feature << "\",\"edge_path\":[";
    for (std::size_t i = 0; i < wall.edge_path.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_plain_edge(wall.edge_path[i]);
    }
    std::cout << "]}";
}

void print_wall_gate(const WallGate& gate) {
    std::cout << "{\"id\":" << gate.id << ",\"kind\":\"" << gate.kind << "\",\"edge\":";
    print_plain_edge(gate.edge);
    std::cout << "}";
}

bool coord_in_vector(const std::vector<Coord>& coords, const Coord& target) {
    return std::find_if(coords.begin(), coords.end(), [&target](const Coord& coord) {
        return coord_equal(coord, target);
    }) != coords.end();
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

void print_hex(const Hex& hex) {
    std::cout << "{\"q\":" << hex.coord.q << ",\"r\":" << hex.coord.r
        << ",\"terrain\":\"" << terrain_to_string(hex.terrain) << "\",\"labels\":";
    print_string_array(hex.labels);
    std::cout << "}";
}

GeneratedMap generate_map(const GenerateArgs& args) {
    const RiverNetwork rivers = generate_river_network(args);
    const std::set<Coord, decltype(coord_less)*> lakes = generate_lake_hexes(args, rivers);
    const std::set<Coord, decltype(coord_less)*> baikal = generate_baikal_hexes(args);
    const std::set<Coord, decltype(coord_less)*> caspian = generate_caspian_hexes(args);
    std::set<Coord, decltype(coord_less)*> all_lakes(coord_less);
    all_lakes.insert(lakes.begin(), lakes.end());
    all_lakes.insert(baikal.begin(), baikal.end());
    all_lakes.insert(caspian.begin(), caspian.end());
    const LakeNetworkOverlay chinese_lake_network = generate_chinese_lake_network_overlay(args, all_lakes);
    for (const Coord& lake : chinese_lake_network.lake_hexes) {
        all_lakes.insert(lake);
    }
    std::vector<RiverEdge> all_river_edges = rivers.edges;
    for (const RiverEdge& edge : chinese_lake_network.edges) {
        add_unique_edge(all_river_edges, edge);
    }
    std::sort(all_river_edges.begin(), all_river_edges.end(), edge_less);
    const std::set<Coord, decltype(coord_less)*> valley_hexes = derive_valley_hexes(args, all_lakes, all_river_edges);
    std::vector<Town> towns = place_fixed_feature_towns(args, baikal, caspian, chinese_lake_network, all_lakes, valley_hexes, rivers);
    const std::optional<Town> dzungarian_gate = place_dzungarian_gate_town(args, all_lakes, valley_hexes, towns);
    if (dzungarian_gate.has_value()) {
        towns.push_back(dzungarian_gate.value());
    }
    const std::optional<Town> oasis = place_oasis_town(args, all_lakes, valley_hexes, all_river_edges, towns);
    if (oasis.has_value()) {
        towns.push_back(oasis.value());
    }
    const std::vector<Town> extra_towns = place_extra_grassland_towns(args, all_lakes, valley_hexes, all_river_edges, towns);
    towns.insert(towns.end(), extra_towns.begin(), extra_towns.end());
    std::sort(towns.begin(), towns.end(), [](const Town& first, const Town& second) {
        return coord_less(first.coord, second.coord);
    });
    const std::vector<Road> roads = generate_roads(args, towns, all_lakes, valley_hexes);
    const std::set<Coord, decltype(coord_less)*> eastern_desert_hexes = derive_eastern_desert_hexes(args, all_lakes, valley_hexes, towns);
    const std::map<Coord, std::string, decltype(coord_less)*> desert_river_corridor_hexes = derive_desert_river_corridor_hexes(args, eastern_desert_hexes, all_river_edges);
    std::set<Coord, decltype(coord_less)*> forest_blocked_hexes = eastern_desert_hexes;
    for (const Town& town : towns) {
        forest_blocked_hexes.insert(town.coord);
    }
    for (const Road& road : roads) {
        for (const Coord& coord : road.path) {
            forest_blocked_hexes.insert(coord);
        }
    }
    const std::set<Coord, decltype(coord_less)*> forest_blob_hexes = derive_forest_blob_hexes(args, all_lakes, valley_hexes, forest_blocked_hexes);
    const std::map<Coord, std::string, decltype(coord_less)*> steppe_texture_hexes = derive_steppe_texture_hexes(args, all_lakes, valley_hexes, forest_blob_hexes, all_river_edges);
    const std::vector<Crossing> crossings = generate_crossings(args, towns, roads, all_river_edges, rivers.segments, all_lakes, valley_hexes);
    const std::map<Coord, int, decltype(coord_less)*> grassland_distances = derive_grassland_distances(args, valley_hexes);
    const std::vector<LakeRiverConnection> lake_river_connections = derive_lake_river_connections(all_lakes, all_river_edges);

    GeneratedMap map;
    map.seed = args.seed;
    map.width = args.width;
    map.height = args.height;
    map.hexes.reserve(static_cast<std::size_t>(args.width * args.height));

    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord coord{q, r};
            const bool base_steppe = is_steppe_hex(q, r, args);
            const bool lake = all_lakes.find(coord) != all_lakes.end();
            const bool random_lake = lakes.find(coord) != lakes.end();
            const bool baikal_lake = baikal.find(coord) != baikal.end();
            const bool caspian_lake = caspian.find(coord) != caspian.end();
            const bool chinese_lake = coord_in_vector(chinese_lake_network.lake_hexes, coord);
            const bool valley = valley_hexes.find(coord) != valley_hexes.end();
            const auto desert_river_corridor = desert_river_corridor_hexes.find(coord);
            const bool eastern_desert = eastern_desert_hexes.find(coord) != eastern_desert_hexes.end()
                && desert_river_corridor == desert_river_corridor_hexes.end();
            const bool forest_blob = forest_blob_hexes.find(coord) != forest_blob_hexes.end();
            const auto steppe_texture = steppe_texture_hexes.find(coord);
            const auto town = std::find_if(towns.begin(), towns.end(), [&coord](const Town& candidate) {
                return coord_equal(candidate.coord, coord);
            });
            const bool urban = town != towns.end();
            std::vector<std::string> labels{
                base_steppe ? "base_steppe" : "wild_terrain",
            };
            const auto remove_label = [&](const std::string& label) {
                labels.erase(std::remove(labels.begin(), labels.end(), label), labels.end());
            };
            Terrain terrain = Terrain::None;
            if (urban) {
                terrain = Terrain::Urban;
            } else if (lake) {
                terrain = Terrain::Lake;
            } else if (forest_blob) {
                terrain = Terrain::Forest;
            } else if (desert_river_corridor != desert_river_corridor_hexes.end()) {
                if (desert_river_corridor->second == "marsh") {
                    terrain = Terrain::Marsh;
                } else if (desert_river_corridor->second == "forest") {
                    terrain = Terrain::Forest;
                } else {
                    terrain = Terrain::Grassland;
                }
            } else if (eastern_desert) {
                terrain = Terrain::Desert;
            } else if (steppe_texture != steppe_texture_hexes.end()) {
                if (steppe_texture->second == "steppe_hill") {
                    terrain = Terrain::Hill;
                } else if (steppe_texture->second == "steppe_forest") {
                    terrain = Terrain::Forest;
                } else if (steppe_texture->second == "steppe_marsh") {
                    terrain = Terrain::Marsh;
                } else {
                    terrain = Terrain::Grassland;
                }
            } else if (valley || base_steppe) {
                terrain = Terrain::Grassland;
            } else {
                const auto distance = grassland_distances.find(coord);
                const std::string wild_terrain = wild_terrain_for_distance(
                    args,
                    coord,
                    distance == grassland_distances.end() ? std::max(args.width, args.height) : distance->second
                );
                if (wild_terrain == "hill") {
                    terrain = Terrain::Hill;
                } else if (wild_terrain == "forest") {
                    terrain = Terrain::Forest;
                } else if (wild_terrain == "mountain") {
                    terrain = Terrain::Mountain;
                } else {
                    terrain = Terrain::None;
                }
            }

            if ((forest_blob || eastern_desert) && base_steppe) {
                remove_label("base_steppe");
            }
            if ((lake || valley || urban || desert_river_corridor != desert_river_corridor_hexes.end()) && !base_steppe) {
                remove_label("wild_terrain");
            }
            if (valley) {
                labels.push_back("valley");
            }
            if (forest_blob) {
                labels.push_back("forest_blob");
            }
            if (eastern_desert) {
                labels.push_back("eastern_desert");
            }
            if (desert_river_corridor != desert_river_corridor_hexes.end()) {
                labels.push_back("desert_river_corridor");
                if (desert_river_corridor->second == "marsh") {
                    labels.push_back("desert_river_marsh");
                } else if (desert_river_corridor->second == "forest") {
                    labels.push_back("desert_river_forest");
                }
            }
            if (steppe_texture != steppe_texture_hexes.end()) {
                labels.push_back(steppe_texture->second);
            }
            if (lake) {
                if (random_lake) {
                    labels.push_back("random_lakes");
                }
                if (baikal_lake) {
                    labels.push_back("lake_baikal");
                }
                if (caspian_lake) {
                    labels.push_back("caspian_sea");
                }
                if (chinese_lake) {
                    labels.push_back("chinese_lakes");
                }
            }
            if (urban) {
                labels.push_back("urban");
                labels.push_back(town->feature == "grassland_water" ? "water_adjacent_town" : "fixed_feature_town");
                labels.push_back(town->feature);
                labels.insert(labels.end(), town->labels.begin(), town->labels.end());
            }
            map.hexes.push_back({coord, terrain, labels});
        }
    }

    map.towns = towns;
    map.river_sources = rivers.sources;
    map.river_destinations = rivers.destinations;
    map.merge_points = rivers.merge_points;
    map.river_segments = rivers.segments;
    map.edges = all_river_edges;
    map.lake_river_connections = lake_river_connections;
    map.roads = roads;
    map.walls = generate_walls(args, towns, all_river_edges, all_lakes, valley_hexes, map.hexes);
    map.wall_gates = generate_wall_gates(args, map.walls, roads, map.hexes);
    map.crossings = crossings;
    map.metadata.terrain_types = {
        Terrain::None,
        Terrain::Grassland,
        Terrain::Lake,
        Terrain::Hill,
        Terrain::Mountain,
        Terrain::Forest,
        Terrain::Marsh,
        Terrain::Desert,
        Terrain::Urban,
    };
    map.metadata.chinese_lake_network = chinese_lake_network;
    return map;
}

void print_generated_map_json(const GeneratedMap& map) {
    std::cout << "{";
    std::cout << "\"schema\":\"" << map.schema << "\",";
    std::cout << "\"seed\":" << map.seed << ",";
    std::cout << "\"width\":" << map.width << ",";
    std::cout << "\"height\":" << map.height << ",";
    std::cout << "\"hexes\":[";
    for (std::size_t i = 0; i < map.hexes.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_hex(map.hexes[i]);
    }
    std::cout << "],";
    std::cout << "\"towns\":[";
    for (std::size_t i = 0; i < map.towns.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_town(map.towns[i]);
    }
    std::cout << "],";
    std::cout << "\"river_sources\":[";
    for (std::size_t i = 0; i < map.river_sources.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(map.river_sources[i]);
    }
    std::cout << "],";
    std::cout << "\"river_destinations\":[";
    for (std::size_t i = 0; i < map.river_destinations.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(map.river_destinations[i]);
    }
    std::cout << "],";
    std::cout << "\"merge_points\":[";
    for (std::size_t i = 0; i < map.merge_points.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(map.merge_points[i]);
    }
    std::cout << "],";
    std::cout << "\"river_segments\":[";
    for (std::size_t i = 0; i < map.river_segments.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        const RiverSegment& segment = map.river_segments[i];
        std::cout << "{\"id\":" << segment.id << ",\"kind\":\"" << segment.kind << "\",\"from\":";
        print_coord(segment.from);
        std::cout << ",\"to\":";
        print_coord(segment.to);
        std::cout << ",\"edge_path\":[";
        for (std::size_t edge_index = 0; edge_index < segment.edge_path.size(); ++edge_index) {
            if (edge_index > 0) {
                std::cout << ",";
            }
            print_edge(segment.edge_path[edge_index]);
        }
        std::cout << "]}";
    }
    std::cout << "],";
    std::cout << "\"edges\":[";
    for (std::size_t i = 0; i < map.edges.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_edge(map.edges[i]);
    }
    std::cout << "],";
    std::cout << "\"lake_river_connections\":[";
    for (std::size_t i = 0; i < map.lake_river_connections.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        const LakeRiverConnection& connection = map.lake_river_connections[i];
        std::cout << "{\"id\":" << connection.id
            << ",\"kind\":\"river_terminal_to_lake_vertex\""
            << ",\"river_component\":" << connection.river_component
            << ",\"terminal_index\":" << connection.terminal_index
            << ",\"vertex\":";
        print_vertex(connection.vertex);
        std::cout << ",\"lake_hexes\":[";
        for (std::size_t hex_index = 0; hex_index < connection.lake_hexes.size(); ++hex_index) {
            if (hex_index > 0) {
                std::cout << ",";
            }
            print_coord(connection.lake_hexes[hex_index]);
        }
        std::cout << "],\"river_edge\":";
        print_edge(connection.river_edge);
        std::cout << "}";
    }
    std::cout << "],";
    std::cout << "\"roads\":[";
    for (std::size_t i = 0; i < map.roads.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_road(map.roads[i]);
    }
    std::cout << "],";
    std::cout << "\"walls\":[";
    for (std::size_t i = 0; i < map.walls.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_wall(map.walls[i]);
    }
    std::cout << "],";
    std::cout << "\"wall_gates\":[";
    for (std::size_t i = 0; i < map.wall_gates.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_wall_gate(map.wall_gates[i]);
    }
    std::cout << "],";
    std::cout << "\"crossings\":[";
    for (std::size_t i = 0; i < map.crossings.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_crossing(map.crossings[i]);
    }
    std::cout << "],";
    std::cout << "\"metadata\":{";
    std::cout << "\"generator\":\"" << map.metadata.generator << "\",";
    std::cout << "\"terrain_types\":[";
    for (std::size_t i = 0; i < map.metadata.terrain_types.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "\"" << terrain_to_string(map.metadata.terrain_types[i]) << "\"";
    }
    std::cout << "],";
    std::cout << "\"hex_label_model\":\"" << map.metadata.hex_label_model << "\",";
    std::cout << "\"lake_river_connection_model\":\"" << map.metadata.lake_river_connection_model << "\",";
    std::cout << "\"crossing_model\":\"" << map.metadata.crossing_model << "\",";
    std::cout << "\"chinese_lake_network\":";
    if (map.metadata.chinese_lake_network.placed) {
        std::cout << "{\"template_id\":" << map.metadata.chinese_lake_network.template_id << ",\"anchor\":";
        print_coord(map.metadata.chinese_lake_network.anchor);
        std::cout << "}";
    } else {
        std::cout << "null";
    }
    std::cout << "}";
    std::cout << "}\n";
}

void print_generated_map(const GenerateArgs& args) {
    print_generated_map_json(generate_map(args));
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--rivers <n>] [--lakes <n>] [--lake-size <n>] [--meander-forward <n>] [--meander-forward-jitter <n>] [--meander-lateral <n>] [--meander-lateral-jitter <n>] [--meander-strength <n>] [--meander-reach <n>] [--river-slant-strength <n>] [--valley-thickness <n>] [--forest-blobs <n>] [--forest-blob-radius <n>] [--meander-timeout <n>]\n";
    std::cerr << "  steppe_engine game-new [--width <n>] [--height <n>] [--factions <n>]\n";
    std::cerr << "  steppe_engine game-select --unit <id> < game-state.json\n";
    std::cerr << "  steppe_engine game-reachable --unit <id> < game-state.json\n";
    std::cerr << "  steppe_engine game-attackable --unit <id> < game-state.json\n";
    std::cerr << "  steppe_engine game-move --unit <id> --q <q> --r <r> < game-state.json\n";
    std::cerr << "  steppe_engine game-attack --attacker <id> --defender <id> < game-state.json\n";
    std::cerr << "  steppe_engine game-end-turn < game-state.json\n";
}

} // namespace steppe
