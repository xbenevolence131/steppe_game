#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

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

struct BoundaryEdge {
    RiverEdge edge;
    VertexKey first;
    VertexKey second;
};

struct RiverSegment {
    int id = 0;
    Coord from;
    Coord to;
    std::string kind;
    std::vector<RiverEdge> edge_path;
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

struct LakeRiverConnection {
    int id = 0;
    int river_component = 0;
    int terminal_index = 0;
    VertexKey vertex;
    std::vector<Coord> lake_hexes;
    RiverEdge river_edge;
};

struct LakeNetworkTemplate {
    int id = 0;
    int width = 0;
    int height = 0;
    Coord anchor;
    std::vector<Coord> lake_hexes;
    std::vector<RiverEdge> edges;
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

bool is_steppe_hex(int q, int r, const GenerateArgs& args) {
    if (args.height <= 2) {
        return true;
    }

    const double x = args.width == 1
        ? 0.5
        : static_cast<double>(q - 1) / static_cast<double>(args.width - 1);
    const double map_midline = (static_cast<double>(args.height) + 1.0) * 0.5;
    const double angle_degrees = signed_noise(args.seed, 1) * 15.0;
    const double angle_radians = angle_degrees * 3.14159265358979323846 / 180.0;
    const double total_row_rise = std::tan(angle_radians) * 0.8660254037844386 * static_cast<double>(args.width - 1);
    const double wobble_phase = unit_noise(args.seed, 2) * 2.0 * 3.14159265358979323846;
    const double wobble_frequency = 1.0 + unit_noise(args.seed, 3) * 1.5;
    const double wobble = std::sin(x * wobble_frequency * 2.0 * 3.14159265358979323846 + wobble_phase)
        * std::max(0.35, args.height * 0.07);
    const double center_offset = signed_noise(args.seed, 4) * std::max(0.0, args.height * 0.08);
    const double center_r = map_midline + center_offset + (x - 0.5) * total_row_rise + wobble;
    const double average_half_width = std::max(1.0, args.height * (0.18 + unit_noise(args.seed, 5) * 0.1));
    const double width_phase = unit_noise(args.seed, 6) * 2.0 * 3.14159265358979323846;
    const double width_variation = std::sin((x * 2.0 + 0.25) * 3.14159265358979323846 + width_phase)
        * average_half_width * 0.22;
    const double half_width = average_half_width + width_variation;

    return std::abs(static_cast<double>(r) - center_r) <= half_width;
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

    const double slot_width = static_cast<double>(args.width) / static_cast<double>(args.river_count);
    const int jitter_limit = std::max(0, static_cast<int>(std::floor(slot_width * 0.25)));
    for (int i = 0; i < args.river_count; ++i) {
        const double slot_center = (static_cast<double>(i) + 0.5) * slot_width + 1.0;
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
    const Coord& coord
) {
    return in_bounds(coord, args) && lake_hexes.find(coord) == lake_hexes.end();
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

std::set<Coord, decltype(coord_less)*> derive_forest_blob_hexes(
    const GenerateArgs& args,
    const std::set<Coord, decltype(coord_less)*>& lake_hexes,
    const std::set<Coord, decltype(coord_less)*>& valley_hexes
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
                    if (!forest_blob_can_grow(args, lake_hexes, neighbor)
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
    const std::set<Coord, decltype(coord_less)*>& occupied_towns
) {
    std::set<Coord, decltype(coord_less)*> chinese_lakes(coord_less);
    for (const Coord& coord : chinese_lake_network.lake_hexes) {
        chinese_lakes.insert(coord);
    }
    return place_generic_lake_feature_towns(
        args,
        "chinese_town",
        chinese_lakes,
        all_lakes,
        valley_hexes,
        occupied_towns,
        76003
    );
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
    add_feature_towns(generate_chinese_region(args, chinese_lake_network, all_lakes, valley_hexes, occupied_towns));

    std::sort(towns.begin(), towns.end(), [](const Town& first, const Town& second) {
        return coord_less(first.coord, second.coord);
    });
    return towns;
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

void print_town(const Town& town) {
    std::cout << "{\"q\":" << town.coord.q << ",\"r\":" << town.coord.r << ",\"feature\":\"" << town.feature << "\"}";
}

bool coord_in_vector(const std::vector<Coord>& coords, const Coord& target) {
    return std::find_if(coords.begin(), coords.end(), [&target](const Coord& coord) {
        return coord_equal(coord, target);
    }) != coords.end();
}

void print_generated_map(const GenerateArgs& args) {
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
    const std::set<Coord, decltype(coord_less)*> forest_blob_hexes = derive_forest_blob_hexes(args, all_lakes, valley_hexes);
    std::vector<Town> towns = place_fixed_feature_towns(args, baikal, caspian, chinese_lake_network, all_lakes, valley_hexes, rivers);
    const std::vector<Town> extra_towns = place_extra_grassland_towns(args, all_lakes, valley_hexes, all_river_edges, towns);
    towns.insert(towns.end(), extra_towns.begin(), extra_towns.end());
    std::sort(towns.begin(), towns.end(), [](const Town& first, const Town& second) {
        return coord_less(first.coord, second.coord);
    });
    const std::map<Coord, int, decltype(coord_less)*> grassland_distances = derive_grassland_distances(args, valley_hexes);
    const std::vector<LakeRiverConnection> lake_river_connections = derive_lake_river_connections(all_lakes, all_river_edges);

    std::cout << "{";
    std::cout << "\"schema\":\"steppe-terrain.v1\",";
    std::cout << "\"seed\":" << args.seed << ",";
    std::cout << "\"width\":" << args.width << ",";
    std::cout << "\"height\":" << args.height << ",";
    std::cout << "\"hexes\":[";

    bool first = true;
    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            if (!first) {
                std::cout << ",";
            }
            first = false;
            const Coord coord{q, r};
            const bool base_steppe = is_steppe_hex(q, r, args);
            const bool lake = all_lakes.find(coord) != all_lakes.end();
            const bool random_lake = lakes.find(coord) != lakes.end();
            const bool baikal_lake = baikal.find(coord) != baikal.end();
            const bool caspian_lake = caspian.find(coord) != caspian.end();
            const bool chinese_lake = coord_in_vector(chinese_lake_network.lake_hexes, coord);
            const bool valley = valley_hexes.find(coord) != valley_hexes.end();
            const bool forest_blob = forest_blob_hexes.find(coord) != forest_blob_hexes.end();
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
            const char* terrain = "none";
            if (urban) {
                terrain = "urban";
            } else if (lake) {
                terrain = "lake";
            } else if (forest_blob) {
                terrain = "forest";
            } else if (valley || base_steppe) {
                terrain = "grassland";
            } else {
                const auto distance = grassland_distances.find(coord);
                terrain = wild_terrain_for_distance(
                    args,
                    coord,
                    distance == grassland_distances.end() ? std::max(args.width, args.height) : distance->second
                );
            }

            if (forest_blob && base_steppe) {
                remove_label("base_steppe");
            }
            if ((lake || valley || urban) && !base_steppe) {
                remove_label("wild_terrain");
            }
            if (valley) {
                labels.push_back("valley");
            }
            if (forest_blob) {
                labels.push_back("forest_blob");
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
            }
            std::cout << "{\"q\":" << q << ",\"r\":" << r << ",\"terrain\":\"" << terrain << "\",\"labels\":";
            print_string_array(labels);
            std::cout << "}";
        }
    }

    std::cout << "],";
    std::cout << "\"towns\":[";
    for (std::size_t i = 0; i < towns.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_town(towns[i]);
    }
    std::cout << "],";
    std::cout << "\"river_sources\":[";
    for (std::size_t i = 0; i < rivers.sources.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(rivers.sources[i]);
    }
    std::cout << "],";
    std::cout << "\"river_destinations\":[";
    for (std::size_t i = 0; i < rivers.destinations.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(rivers.destinations[i]);
    }
    std::cout << "],";
    std::cout << "\"merge_points\":[";
    for (std::size_t i = 0; i < rivers.merge_points.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_coord(rivers.merge_points[i]);
    }
    std::cout << "],";
    std::cout << "\"river_segments\":[";
    for (std::size_t i = 0; i < rivers.segments.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        const RiverSegment& segment = rivers.segments[i];
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
    for (std::size_t i = 0; i < all_river_edges.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_edge(all_river_edges[i]);
    }
    std::cout << "],";
    std::cout << "\"lake_river_connections\":[";
    for (std::size_t i = 0; i < lake_river_connections.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        const LakeRiverConnection& connection = lake_river_connections[i];
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
    std::cout << "\"roads\":[],";
    std::cout << "\"metadata\":{";
    std::cout << "\"generator\":\"prototype-steppe-blob\",";
    std::cout << "\"terrain_types\":[\"none\",\"grassland\",\"lake\",\"hill\",\"mountain\",\"forest\",\"marsh\",\"urban\"],";
    std::cout << "\"hex_label_model\":\"final-semantic-labels.v2\",";
    std::cout << "\"lake_river_connection_model\":\"river-terminal-lake-vertex.v1\",";
    std::cout << "\"chinese_lake_network\":";
    if (chinese_lake_network.placed) {
        std::cout << "{\"template_id\":" << chinese_lake_network.template_id << ",\"anchor\":";
        print_coord(chinese_lake_network.anchor);
        std::cout << "}";
    } else {
        std::cout << "null";
    }
    std::cout << "}";
    std::cout << "}\n";
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--rivers <n>] [--lakes <n>] [--lake-size <n>] [--meander-forward <n>] [--meander-forward-jitter <n>] [--meander-lateral <n>] [--meander-lateral-jitter <n>] [--meander-strength <n>] [--meander-reach <n>] [--river-slant-strength <n>] [--valley-thickness <n>] [--forest-blobs <n>] [--forest-blob-radius <n>] [--meander-timeout <n>]\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    try {
        const std::string command = argv[1];
        if (command == "generate") {
            print_generated_map(parse_generate_args(argc, argv));
            return EXIT_SUCCESS;
        }

        throw std::runtime_error("unknown command: " + command);
    } catch (const std::exception& ex) {
        std::cerr << "steppe_engine: " << ex.what() << "\n";
        print_usage();
        return EXIT_FAILURE;
    }
}
