#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GenerateArgs {
    int width = 50;
    int height = 20;
    std::uint32_t seed = 1;
    int river_count = 3;
    int merge_point_count = 1;
};

struct Coord {
    int q = 1;
    int r = 1;
};

struct River {
    int id = 1;
    Coord source;
    Coord destination;
    std::vector<Coord> path;
};

struct RiverEdge {
    Coord a;
    Coord b;
};

struct RiverSegment {
    int id = 1;
    Coord from;
    Coord to;
    std::string kind;
    std::vector<RiverEdge> edge_path;
};

struct VertexKey {
    long long x = 0;
    long long y = 0;
};

bool operator<(const VertexKey& a, const VertexKey& b) {
    if (a.x != b.x) {
        return a.x < b.x;
    }
    return a.y < b.y;
}

struct BoundaryEdge {
    RiverEdge edge;
    VertexKey first;
    VertexKey second;
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
        } else if (key == "--merge-points" && i + 1 < argc) {
            args.merge_point_count = parse_non_negative_int(argv[++i], "merge points");
        } else {
            throw std::runtime_error("unknown or incomplete argument: " + key);
        }
    }

    if (args.width > 80 || args.height > 60) {
        throw std::runtime_error("width and height are capped at 80x60 for the prototype");
    }
    if (args.river_count > 100) {
        throw std::runtime_error("rivers are capped at 100 for the prototype");
    }
    if (args.merge_point_count > 100) {
        throw std::runtime_error("merge points are capped at 100 for the prototype");
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

int hex_distance(const Coord& a, const Coord& b) {
    const int a_col = a.q - 1;
    const int a_row = a.r - 1;
    const int b_col = b.q - 1;
    const int b_row = b.r - 1;

    const int a_x = a_col;
    const int a_z = a_row - (a_col - (a_col & 1)) / 2;
    const int a_y = -a_x - a_z;
    const int b_x = b_col;
    const int b_z = b_row - (b_col - (b_col & 1)) / 2;
    const int b_y = -b_x - b_z;

    return std::max({std::abs(a_x - b_x), std::abs(a_y - b_y), std::abs(a_z - b_z)});
}

bool is_far_enough(const Coord& candidate, const std::vector<Coord>& selected, int minimum_distance) {
    for (const Coord& existing : selected) {
        if (hex_distance(candidate, existing) < minimum_distance) {
            return false;
        }
    }
    return true;
}

bool coord_less(const Coord& a, const Coord& b) {
    if (a.q != b.q) {
        return a.q < b.q;
    }
    return a.r < b.r;
}

bool coord_equal(const Coord& a, const Coord& b) {
    return a.q == b.q && a.r == b.r;
}

RiverEdge canonical_edge(Coord a, Coord b) {
    if (coord_less(b, a)) {
        std::swap(a, b);
    }
    return {a, b};
}

bool edge_less(const RiverEdge& a, const RiverEdge& b) {
    if (!coord_equal(a.a, b.a)) {
        return coord_less(a.a, b.a);
    }
    return coord_less(a.b, b.b);
}

bool edge_equal(const RiverEdge& a, const RiverEdge& b) {
    return coord_equal(a.a, b.a) && coord_equal(a.b, b.b);
}

bool vertex_less(const VertexKey& a, const VertexKey& b) {
    if (a.x != b.x) {
        return a.x < b.x;
    }
    return a.y < b.y;
}

bool vertex_equal(const VertexKey& a, const VertexKey& b) {
    return a.x == b.x && a.y == b.y;
}

std::vector<Coord> collect_northern_candidates(const GenerateArgs& args, bool allow_top_edge) {
    std::vector<Coord> candidates;
    const int north_limit = std::max(1, static_cast<int>(std::ceil(args.height * 0.35)));
    const int min_row = allow_top_edge || args.height <= 2 ? 1 : 2;

    for (int r = min_row; r <= north_limit; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            if (!is_steppe_hex(q, r, args)) {
                candidates.push_back({q, r});
            }
        }
    }

    return candidates;
}

std::vector<Coord> collect_southern_candidates(const GenerateArgs& args, bool allow_bottom_edge) {
    std::vector<Coord> candidates;
    const int south_start = std::min(args.height, std::max(1, static_cast<int>(std::floor(args.height * 0.65))));
    const int max_row = allow_bottom_edge || args.height <= 2 ? args.height : std::max(1, args.height - 1);

    for (int r = south_start; r <= max_row; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            if (!is_steppe_hex(q, r, args)) {
                candidates.push_back({q, r});
            }
        }
    }

    return candidates;
}

std::vector<Coord> collect_merge_point_candidates(const GenerateArgs& args) {
    std::vector<Coord> candidates;
    const int min_row = std::max(1, static_cast<int>(std::floor(args.height * 0.35)));
    const int max_row = std::min(args.height, static_cast<int>(std::ceil(args.height * 0.72)));

    for (int r = min_row; r <= max_row; ++r) {
        for (int q = 2; q <= std::max(1, args.width - 1); ++q) {
            if (is_steppe_hex(q, r, args)) {
                candidates.push_back({q, r});
            }
        }
    }

    return candidates;
}

std::vector<Coord> select_spaced_points(
    std::vector<Coord> candidates,
    int count,
    std::uint32_t seed,
    int width,
    int height
) {
    std::mt19937 rng(seed);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<Coord> selected;
    selected.reserve(std::min(count, static_cast<int>(candidates.size())));

    const int starting_distance = std::max(2, std::min(width, height) / 4 + 1);
    for (int minimum_distance = starting_distance; minimum_distance >= 1; --minimum_distance) {
        for (const Coord& candidate : candidates) {
            if (static_cast<int>(selected.size()) >= count) {
                return selected;
            }
            if (is_far_enough(candidate, selected, minimum_distance)) {
                selected.push_back(candidate);
            }
        }
    }

    return selected;
}

std::vector<Coord> generate_river_sources(const GenerateArgs& args, int count) {
    std::vector<Coord> candidates = collect_northern_candidates(args, false);
    if (candidates.empty()) {
        candidates = collect_northern_candidates(args, true);
    }

    return select_spaced_points(candidates, count, args.seed ^ 0x5f3759dfU, args.width, args.height);
}

std::vector<Coord> generate_river_destinations(const GenerateArgs& args, int count) {
    std::vector<Coord> candidates = collect_southern_candidates(args, false);
    if (candidates.empty()) {
        candidates = collect_southern_candidates(args, true);
    }

    return select_spaced_points(candidates, count, args.seed ^ 0x85ebca6bU, args.width, args.height);
}

std::vector<Coord> generate_merge_points(const GenerateArgs& args, int count) {
    return select_spaced_points(
        collect_merge_point_candidates(args),
        count,
        args.seed ^ 0xc2b2ae35U,
        args.width,
        args.height
    );
}

bool in_bounds(const Coord& coord, const GenerateArgs& args) {
    return coord.q >= 1 && coord.q <= args.width && coord.r >= 1 && coord.r <= args.height;
}

std::vector<Coord> neighbors(const Coord& coord, const GenerateArgs& args) {
    static const int directions[2][6][2] = {
        {{1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {0, 1}},
        {{1, 1}, {1, 0}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}},
    };

    std::vector<Coord> result;
    const int parity = (coord.q - 1) & 1;
    for (const auto& direction : directions[parity]) {
        Coord next{coord.q + direction[0], coord.r + direction[1]};
        if (in_bounds(next, args)) {
            result.push_back(next);
        }
    }
    return result;
}

Coord neighbor_in_direction(const Coord& coord, const GenerateArgs& args, int direction) {
    static const int directions[2][6][2] = {
        {{1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {0, 1}},
        {{1, 1}, {1, 0}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}},
    };

    const int parity = (coord.q - 1) & 1;
    Coord next{coord.q + directions[parity][direction][0], coord.r + directions[parity][direction][1]};
    if (in_bounds(next, args)) {
        return next;
    }
    return coord;
}

int direction_between(const Coord& from, const Coord& to, const GenerateArgs& args) {
    static const int directions[2][6][2] = {
        {{1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {0, 1}},
        {{1, 1}, {1, 0}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}},
    };

    const int parity = (from.q - 1) & 1;
    for (int direction = 0; direction < 6; ++direction) {
        Coord next{from.q + directions[parity][direction][0], from.r + directions[parity][direction][1]};
        if (coord_equal(next, to)) {
            return direction;
        }
    }
    return -1;
}

int side_direction(int movement_direction, bool trace_left_side) {
    return (movement_direction + (trace_left_side ? 1 : 5)) % 6;
}

double line_distance(const Coord& point, const Coord& start, const Coord& end) {
    const double px = static_cast<double>(point.q - 1) * 1.5;
    const double py = static_cast<double>(point.r - 1) + (((point.q - 1) & 1) ? 0.5 : 0.0);
    const double ax = static_cast<double>(start.q - 1) * 1.5;
    const double ay = static_cast<double>(start.r - 1) + (((start.q - 1) & 1) ? 0.5 : 0.0);
    const double bx = static_cast<double>(end.q - 1) * 1.5;
    const double by = static_cast<double>(end.r - 1) + (((end.q - 1) & 1) ? 0.5 : 0.0);
    const double dx = bx - ax;
    const double dy = by - ay;
    const double length_squared = dx * dx + dy * dy;

    if (length_squared == 0.0) {
        const double point_dx = px - ax;
        const double point_dy = py - ay;
        return std::sqrt(point_dx * point_dx + point_dy * point_dy);
    }

    const double t = std::max(0.0, std::min(1.0, ((px - ax) * dx + (py - ay) * dy) / length_squared));
    const double projected_x = ax + t * dx;
    const double projected_y = ay + t * dy;
    const double point_dx = px - projected_x;
    const double point_dy = py - projected_y;
    return std::sqrt(point_dx * point_dx + point_dy * point_dy);
}

bool path_contains(const std::vector<Coord>& path, const Coord& coord) {
    for (const Coord& existing : path) {
        if (coord_equal(existing, coord)) {
            return true;
        }
    }
    return false;
}

std::vector<Coord> route_river_path(const GenerateArgs& args, const Coord& source, const Coord& destination, int river_id) {
    std::vector<Coord> path;
    path.push_back(source);

    Coord current = source;
    const int max_steps = std::max(1, args.width * args.height * 3);
    for (int step = 0; step < max_steps && !coord_equal(current, destination); ++step) {
        const int current_distance = hex_distance(current, destination);
        Coord best = current;
        double best_score = std::numeric_limits<double>::max();

        for (const Coord& next : neighbors(current, args)) {
            const int next_distance = hex_distance(next, destination);
            if (next_distance >= current_distance) {
                continue;
            }

            const double noise = unit_noise(args.seed, static_cast<std::uint32_t>(river_id * 10000 + step * 17 + next.q * 131 + next.r));
            const double southward_progress = static_cast<double>(next.r - current.r);
            const double score =
                static_cast<double>(next_distance) * 10.0
                + line_distance(next, source, destination) * 2.25
                + noise * 3.0
                - southward_progress * 0.75
                + (path_contains(path, next) ? 1000.0 : 0.0);

            if (score < best_score) {
                best_score = score;
                best = next;
            }
        }

        if (coord_equal(best, current)) {
            break;
        }

        path.push_back(best);
        current = best;
    }

    return path;
}

bool contains_edge(const std::vector<RiverEdge>& edges, const RiverEdge& edge) {
    for (const RiverEdge& existing : edges) {
        if (edge_equal(existing, edge)) {
            return true;
        }
    }
    return false;
}

void add_boundary_edge(std::vector<RiverEdge>& edges, const GenerateArgs& args, const Coord& hex, int direction) {
    const Coord across_edge = neighbor_in_direction(hex, args, direction);
    if (coord_equal(hex, across_edge)) {
        return;
    }

    const RiverEdge edge = canonical_edge(hex, across_edge);
    if (!contains_edge(edges, edge)) {
        edges.push_back(edge);
    }
}

void add_edge_arc(
    std::vector<RiverEdge>& edges,
    const GenerateArgs& args,
    const Coord& hex,
    int start_direction,
    int end_direction,
    bool trace_left_side
) {
    int step = trace_left_side ? 1 : 5;
    int edge_count = 1;
    for (int direction = start_direction; direction != end_direction && edge_count < 6; direction = (direction + step) % 6) {
        ++edge_count;
    }

    if (edge_count > 3) {
        step = step == 1 ? 5 : 1;
    }

    int direction = start_direction;

    for (int guard = 0; guard < 6; ++guard) {
        add_boundary_edge(edges, args, hex, direction);
        if (direction == end_direction) {
            return;
        }
        direction = (direction + step) % 6;
    }
}

std::vector<VertexKey> hex_corners(const Coord& hex) {
    constexpr double pi = 3.14159265358979323846;
    constexpr double sqrt_three = 1.7320508075688772935;
    constexpr double scale = 1000000.0;

    const int col = hex.q - 1;
    const int row = hex.r - 1;
    const double cx = static_cast<double>(col) * 1.5;
    const double cy = sqrt_three * (static_cast<double>(row) + ((col & 1) ? 0.5 : 0.0));

    std::vector<VertexKey> corners;
    corners.reserve(6);
    for (int i = 0; i < 6; ++i) {
        const double angle = pi / 180.0 * static_cast<double>(60 * i);
        corners.push_back({
            static_cast<long long>(std::llround((cx + std::cos(angle)) * scale)),
            static_cast<long long>(std::llround((cy + std::sin(angle)) * scale)),
        });
    }
    return corners;
}

BoundaryEdge make_boundary_edge(const RiverEdge& edge) {
    const std::vector<VertexKey> a_corners = hex_corners(edge.a);
    const std::vector<VertexKey> b_corners = hex_corners(edge.b);
    std::vector<VertexKey> shared;

    for (const VertexKey& a_corner : a_corners) {
        for (const VertexKey& b_corner : b_corners) {
            if (vertex_equal(a_corner, b_corner)) {
                shared.push_back(a_corner);
            }
        }
    }

    if (shared.size() >= 2) {
        return {edge, shared[0], shared[1]};
    }

    return {edge, a_corners[0], a_corners[1]};
}

int find_edge_index(const std::vector<BoundaryEdge>& boundary_edges, const RiverEdge& edge) {
    for (std::size_t i = 0; i < boundary_edges.size(); ++i) {
        if (edge_equal(boundary_edges[i].edge, edge)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int find_boundary_edge_for_direction(
    const std::vector<BoundaryEdge>& boundary_edges,
    const std::vector<Coord>& hex_path,
    const GenerateArgs& args,
    const Coord& hex,
    int preferred_direction
) {
    const int offsets[] = {0, 1, -1, 2, -2, 3};
    for (const int offset : offsets) {
        const int direction = (preferred_direction + offset + 6) % 6;
        const Coord neighbor = neighbor_in_direction(hex, args, direction);
        if (coord_equal(hex, neighbor) || path_contains(hex_path, neighbor)) {
            continue;
        }

        const int index = find_edge_index(boundary_edges, canonical_edge(hex, neighbor));
        if (index >= 0) {
            return index;
        }
    }
    return -1;
}

VertexKey other_vertex(const BoundaryEdge& edge, const VertexKey& vertex) {
    return vertex_equal(edge.first, vertex) ? edge.second : edge.first;
}

std::vector<int> walk_boundary_to_edge(
    int start_edge,
    const VertexKey& start_vertex,
    int end_edge,
    const std::vector<BoundaryEdge>& boundary_edges,
    const std::map<VertexKey, std::vector<int>>& edges_by_vertex
) {
    std::vector<int> result;
    result.push_back(start_edge);

    if (start_edge == end_edge) {
        return result;
    }

    int previous_edge = start_edge;
    VertexKey current_vertex = start_vertex;

    for (std::size_t guard = 0; guard <= boundary_edges.size() + 1; ++guard) {
        const auto found = edges_by_vertex.find(current_vertex);
        if (found == edges_by_vertex.end()) {
            return {};
        }

        int next_edge = -1;
        for (const int candidate : found->second) {
            if (candidate != previous_edge) {
                next_edge = candidate;
                break;
            }
        }

        if (next_edge < 0) {
            return {};
        }

        result.push_back(next_edge);
        if (next_edge == end_edge) {
            return result;
        }

        current_vertex = other_vertex(boundary_edges[next_edge], current_vertex);
        previous_edge = next_edge;
    }

    return {};
}

std::vector<RiverEdge> trace_outside_edge_path(
    const GenerateArgs& args,
    const std::vector<Coord>& hex_path,
    int segment_id
) {
    std::vector<RiverEdge> edge_path;
    if (hex_path.size() < 2) {
        return edge_path;
    }

    const bool trace_left_side = unit_noise(args.seed, static_cast<std::uint32_t>(segment_id * 7919)) < 0.5;
    std::vector<BoundaryEdge> boundary_edges;

    for (const Coord& hex : hex_path) {
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = neighbor_in_direction(hex, args, direction);
            if (coord_equal(hex, neighbor) || path_contains(hex_path, neighbor)) {
                continue;
            }

            const RiverEdge edge = canonical_edge(hex, neighbor);
            bool exists = false;
            for (const BoundaryEdge& boundary_edge : boundary_edges) {
                if (edge_equal(boundary_edge.edge, edge)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                boundary_edges.push_back(make_boundary_edge(edge));
            }
        }
    }

    if (boundary_edges.empty()) {
        return edge_path;
    }

    std::map<VertexKey, std::vector<int>> edges_by_vertex;
    for (std::size_t i = 0; i < boundary_edges.size(); ++i) {
        edges_by_vertex[boundary_edges[i].first].push_back(static_cast<int>(i));
        edges_by_vertex[boundary_edges[i].second].push_back(static_cast<int>(i));
    }

    const int outgoing = direction_between(hex_path[0], hex_path[1], args);
    const int incoming = direction_between(hex_path[hex_path.size() - 2], hex_path.back(), args);
    if (outgoing < 0 || incoming < 0) {
        return edge_path;
    }

    const int start_edge = find_boundary_edge_for_direction(
        boundary_edges,
        hex_path,
        args,
        hex_path[0],
        side_direction(outgoing, trace_left_side)
    );
    const int end_edge = find_boundary_edge_for_direction(
        boundary_edges,
        hex_path,
        args,
        hex_path.back(),
        side_direction(incoming, trace_left_side)
    );

    if (start_edge < 0 || end_edge < 0) {
        return edge_path;
    }

    std::vector<int> first_candidate = walk_boundary_to_edge(
        start_edge,
        boundary_edges[start_edge].first,
        end_edge,
        boundary_edges,
        edges_by_vertex
    );
    std::vector<int> second_candidate = walk_boundary_to_edge(
        start_edge,
        boundary_edges[start_edge].second,
        end_edge,
        boundary_edges,
        edges_by_vertex
    );

    const std::vector<int>* chosen = nullptr;
    if (first_candidate.empty()) {
        chosen = &second_candidate;
    } else if (second_candidate.empty()) {
        chosen = &first_candidate;
    } else {
        chosen = first_candidate.size() <= second_candidate.size() ? &first_candidate : &second_candidate;
    }

    if (!chosen) {
        return edge_path;
    }

    for (const int edge_index : *chosen) {
        edge_path.push_back(boundary_edges[edge_index].edge);
    }
    return edge_path;
}

std::vector<RiverSegment> generate_river_segments(
    const GenerateArgs& args,
    std::vector<Coord>& sources,
    std::vector<Coord>& destinations,
    std::vector<Coord>& merge_points
) {
    sources = generate_river_sources(args, args.river_count);
    std::sort(sources.begin(), sources.end(), coord_less);

    const int desired_merge_points = std::min(args.merge_point_count, static_cast<int>(sources.size()));
    merge_points = generate_merge_points(args, desired_merge_points);
    std::sort(merge_points.begin(), merge_points.end(), coord_less);

    const int destination_count = merge_points.empty()
        ? static_cast<int>(sources.size())
        : static_cast<int>(merge_points.size());
    destinations = generate_river_destinations(args, destination_count);
    std::sort(sources.begin(), sources.end(), coord_less);
    std::sort(destinations.begin(), destinations.end(), coord_less);

    std::vector<RiverSegment> segments;
    int segment_id = 1;

    if (merge_points.empty()) {
        const int count = std::min(static_cast<int>(sources.size()), static_cast<int>(destinations.size()));
        for (int i = 0; i < count; ++i) {
            const std::vector<Coord> hex_path = route_river_path(args, sources[i], destinations[i], segment_id);
            segments.push_back({
                segment_id,
                sources[i],
                destinations[i],
                "source_to_destination",
                trace_outside_edge_path(args, hex_path, segment_id),
            });
            ++segment_id;
        }
        return segments;
    }

    for (std::size_t i = 0; i < sources.size(); ++i) {
        const Coord& merge_point = merge_points[i % merge_points.size()];
        const std::vector<Coord> hex_path = route_river_path(args, sources[i], merge_point, segment_id);
        segments.push_back({
            segment_id,
            sources[i],
            merge_point,
            "source_to_merge",
            trace_outside_edge_path(args, hex_path, segment_id),
        });
        ++segment_id;
    }

    const int downstream_count = std::min(static_cast<int>(merge_points.size()), static_cast<int>(destinations.size()));
    for (int i = 0; i < downstream_count; ++i) {
        const std::vector<Coord> hex_path = route_river_path(args, merge_points[i], destinations[i], segment_id);
        segments.push_back({
            segment_id,
            merge_points[i],
            destinations[i],
            "merge_to_destination",
            trace_outside_edge_path(args, hex_path, segment_id),
        });
        ++segment_id;
    }

    return segments;
}

std::vector<RiverEdge> collect_river_edges(const std::vector<RiverSegment>& segments) {
    std::vector<RiverEdge> edges;
    for (const RiverSegment& segment : segments) {
        for (const RiverEdge& edge : segment.edge_path) {
            edges.push_back(edge);
        }
    }

    std::sort(edges.begin(), edges.end(), edge_less);
    edges.erase(std::unique(edges.begin(), edges.end(), edge_equal), edges.end());
    return edges;
}

void print_generated_map(const GenerateArgs& args) {
    std::vector<Coord> river_sources;
    std::vector<Coord> river_destinations;
    std::vector<Coord> merge_points;
    const std::vector<RiverSegment> river_segments = generate_river_segments(
        args,
        river_sources,
        river_destinations,
        merge_points
    );
    const std::vector<RiverEdge> river_edges = collect_river_edges(river_segments);

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
    for (std::size_t i = 0; i < river_sources.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "{\"q\":" << river_sources[i].q << ",\"r\":" << river_sources[i].r << "}";
    }
    std::cout << "],";
    std::cout << "\"river_destinations\":[";
    for (std::size_t i = 0; i < river_destinations.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "{\"q\":" << river_destinations[i].q << ",\"r\":" << river_destinations[i].r << "}";
    }
    std::cout << "],";
    std::cout << "\"merge_points\":[";
    for (std::size_t i = 0; i < merge_points.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "{\"q\":" << merge_points[i].q << ",\"r\":" << merge_points[i].r << "}";
    }
    std::cout << "],";
    std::cout << "\"river_segments\":[";
    for (std::size_t i = 0; i < river_segments.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        const RiverSegment& segment = river_segments[i];
        std::cout << "{";
        std::cout << "\"id\":" << segment.id << ",";
        std::cout << "\"kind\":\"" << segment.kind << "\",";
        std::cout << "\"from\":{\"q\":" << segment.from.q << ",\"r\":" << segment.from.r << "},";
        std::cout << "\"to\":{\"q\":" << segment.to.q << ",\"r\":" << segment.to.r << "},";
        std::cout << "\"edge_path\":[";
        for (std::size_t edge_index = 0; edge_index < segment.edge_path.size(); ++edge_index) {
            if (edge_index > 0) {
                std::cout << ",";
            }
            std::cout << "{\"a\":{\"q\":" << segment.edge_path[edge_index].a.q << ",\"r\":" << segment.edge_path[edge_index].a.r << "},";
            std::cout << "\"b\":{\"q\":" << segment.edge_path[edge_index].b.q << ",\"r\":" << segment.edge_path[edge_index].b.r << "}}";
        }
        std::cout << "]}";
    }
    std::cout << "],";
    std::cout << "\"edges\":[";
    for (std::size_t i = 0; i < river_edges.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "{\"a\":{\"q\":" << river_edges[i].a.q << ",\"r\":" << river_edges[i].a.r << "},";
        std::cout << "\"b\":{\"q\":" << river_edges[i].b.q << ",\"r\":" << river_edges[i].b.r << "},";
        std::cout << "\"river\":true}";
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
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--rivers <n>] [--merge-points <n>]\n";
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
