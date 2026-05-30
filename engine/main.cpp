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
    double meander_forward = 14.0;
    double meander_forward_jitter = 4.0;
    double meander_lateral = 10.0;
    double meander_lateral_jitter = 4.0;
    double meander_strength = 1.6;
    double meander_reach = 2.0;
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
    if (args.meander_forward > 40.0 || args.meander_forward_jitter > 40.0 || args.meander_lateral > 40.0 || args.meander_lateral_jitter > 40.0) {
        throw std::runtime_error("meander distances are capped at 40 for the prototype");
    }
    if (args.meander_strength > 10.0 || args.meander_reach > 40.0 || args.meander_timeout > 200) {
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
        throw std::runtime_error("could not resolve shared edge vertices");
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
        head.current_edge = *start_edge;
        head.current_vertex = downstream;
        head.edge_path.push_back(edges[*start_edge]);
        head.visited_vertices.insert(upstream);
        head.visited_vertices.insert(downstream);
        occupied_edges.insert(edges[*start_edge]);
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

                const bool occupied_by_other_edge = occupied_edges.find(edges[candidate]) != occupied_edges.end();
                const auto occupied_vertex = occupied_vertices.find(next_vertex);
                const bool occupied_by_other_vertex = occupied_vertex != occupied_vertices.end() && occupied_vertex->second != head.id;
                if ((occupied_by_other_edge || occupied_by_other_vertex) && !can_merge) {
                    continue;
                }

                const double dy = vertex_y(next_vertex) - vertex_y(head.current_vertex);
                const double side_distance = std::min(vertex_x(next_vertex), max_x - vertex_x(next_vertex));
                const double south_score = dy >= 0.0 ? -dy * 2.0 : 18.0 + std::abs(dy) * 8.0;
                const double side_score = vertex_y(next_vertex) < bottom_y * 0.9
                    ? (side_distance < 1.0 ? 90.0 : (side_distance < 2.0 ? 35.0 : 0.0))
                    : 0.0;
                const double meander_dx = vertex_x(next_vertex) - head.meander_target_x;
                const double meander_dy = vertex_y(next_vertex) - head.meander_target_y;
                const double meander_score = meander_active && head.has_meander_target
                    ? std::sqrt(meander_dx * meander_dx + meander_dy * meander_dy) * args.meander_strength
                    : 0.0;
                const double merge_score = (occupied_by_other_edge || occupied_by_other_vertex) ? -30.0 : 0.0;
                const double noise = unit_noise(
                    args.seed,
                    static_cast<std::uint32_t>(head.id * 20000 + step * 131 + candidate * 17)
                ) * 14.0;
                const double score = south_score + side_score + meander_score + merge_score + noise;

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
                occupied_vertices[best_next] = head.id;
                add_unique_edge(network.edges, edges[best_edge]);
                continue;
            }

            head.edge_path.push_back(edges[best_edge]);
            head.current_edge = best_edge;
            head.current_vertex = best_next;
            head.endpoint = representative_coord_for_edge_vertex(best_next, boundaries[best_edge]);
            head.active = false;
            head.merged = true;
            network.merge_points.push_back(head.endpoint);
            add_unique_edge(network.edges, edges[best_edge]);
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

void print_coord(const Coord& coord) {
    std::cout << "{\"q\":" << coord.q << ",\"r\":" << coord.r << "}";
}

void print_edge(const RiverEdge& edge) {
    std::cout << "{\"a\":";
    print_coord(edge.a);
    std::cout << ",\"b\":";
    print_coord(edge.b);
    std::cout << ",\"river\":true}";
}

void print_generated_map(const GenerateArgs& args) {
    const RiverNetwork rivers = generate_river_network(args);

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
            const char* terrain = is_steppe_hex(q, r, args) ? "grassland" : "none";
            std::cout << "{\"q\":" << q << ",\"r\":" << r << ",\"terrain\":\"" << terrain << "\"}";
        }
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
    for (std::size_t i = 0; i < rivers.edges.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        print_edge(rivers.edges[i]);
    }
    std::cout << "],";
    std::cout << "\"roads\":[],";
    std::cout << "\"metadata\":{";
    std::cout << "\"generator\":\"prototype-steppe-blob\",";
    std::cout << "\"terrain_types\":[\"none\",\"grassland\",\"light_forest\",\"heavy_forest\",\"hills\",\"mountains\",\"urban\"]";
    std::cout << "}";
    std::cout << "}\n";
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--rivers <n>] [--meander-forward <n>] [--meander-forward-jitter <n>] [--meander-lateral <n>] [--meander-lateral-jitter <n>] [--meander-strength <n>] [--meander-reach <n>] [--meander-timeout <n>]\n";
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
