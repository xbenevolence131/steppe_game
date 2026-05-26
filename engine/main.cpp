#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GenerateArgs {
    int width = 100;
    int height = 40;
    std::uint32_t seed = 1;
    int river_count = 3;
    int source_variance = 8;
    int horizontal_band = 8;
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

struct SourceMerge {
    Coord first_source;
    Coord second_source;
    Coord merge_point;
};

struct RiverOutlet {
    Coord point;
    std::string kind;
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

struct WorldPoint {
    double x = 0.0;
    double y = 0.0;
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
        } else if ((key == "--source-variance" || key == "--source-exclusion-radius") && i + 1 < argc) {
            args.source_variance = parse_non_negative_int(argv[++i], "source variance");
        } else if (key == "--horizontal-band" && i + 1 < argc) {
            args.horizontal_band = parse_non_negative_int(argv[++i], "horizontal band");
        } else {
            throw std::runtime_error("unknown or incomplete argument: " + key);
        }
    }

    if (args.width > 160 || args.height > 80) {
        throw std::runtime_error("width and height are capped at 160x80 for the prototype");
    }
    if (args.river_count > 100) {
        throw std::runtime_error("rivers are capped at 100 for the prototype");
    }
    if (args.source_variance > 100) {
        throw std::runtime_error("source variance is capped at 100 for the prototype");
    }
    if (args.horizontal_band > 100) {
        throw std::runtime_error("horizontal band is capped at 100 for the prototype");
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

std::vector<Coord> collect_northern_candidates(
    const GenerateArgs& args,
    bool allow_top_edge,
    int min_q,
    int max_q
) {
    std::vector<Coord> candidates;
    const int north_limit = std::max(1, static_cast<int>(std::ceil(args.height * 0.35)));
    const int min_row = allow_top_edge || args.height <= 2 ? 1 : 2;
    const int clamped_min_q = std::max(1, min_q);
    const int clamped_max_q = std::min(args.width, max_q);

    for (int r = min_row; r <= north_limit; ++r) {
        for (int q = clamped_min_q; q <= clamped_max_q; ++q) {
            if (!is_steppe_hex(q, r, args)) {
                candidates.push_back({q, r});
            }
        }
    }

    return candidates;
}

std::vector<Coord> collect_southern_candidates(
    const GenerateArgs& args,
    bool allow_bottom_edge,
    int min_q,
    int max_q
) {
    std::vector<Coord> candidates;
    const int south_start = std::min(args.height, std::max(1, static_cast<int>(std::floor(args.height * 0.65))));
    const int max_row = allow_bottom_edge || args.height <= 2 ? args.height : std::max(1, args.height - 1);
    const int clamped_min_q = std::max(1, min_q);
    const int clamped_max_q = std::min(args.width, max_q);

    for (int r = south_start; r <= max_row; ++r) {
        for (int q = clamped_min_q; q <= clamped_max_q; ++q) {
            if (!is_steppe_hex(q, r, args)) {
                candidates.push_back({q, r});
            }
        }
    }

    return candidates;
}

std::vector<Coord> collect_merge_point_candidates(
    const GenerateArgs& args,
    int min_q,
    int max_q
) {
    std::vector<Coord> candidates;
    const int min_row = std::max(1, static_cast<int>(std::floor(args.height * 0.35)));
    const int max_row = std::min(args.height, static_cast<int>(std::ceil(args.height * 0.72)));
    const int clamped_min_q = std::max(2, min_q);
    const int clamped_max_q = std::min(std::max(1, args.width - 1), max_q);

    for (int r = min_row; r <= max_row; ++r) {
        for (int q = clamped_min_q; q <= clamped_max_q; ++q) {
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
    int height,
    int spacing
) {
    std::mt19937 rng(seed);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<Coord> selected;
    selected.reserve(std::min(count, static_cast<int>(candidates.size())));

    const int starting_distance = std::max(1, std::min({width, height, spacing}));
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
    std::vector<Coord> sources;
    const int source_count = std::min(count, args.width);
    sources.reserve(source_count);
    if (count <= 0) {
        return sources;
    }

    std::vector<bool> occupied(static_cast<std::size_t>(args.width + 1), false);
    const auto nearest_open_q = [&](int preferred_q, int min_q, int max_q) {
        const auto scan = [&](int scan_min_q, int scan_max_q) -> std::optional<int> {
            for (int radius = 0; radius <= args.width; ++radius) {
                const int left_q = preferred_q - radius;
                if (left_q >= scan_min_q && left_q <= scan_max_q && !occupied[static_cast<std::size_t>(left_q)]) {
                    return left_q;
                }

                const int right_q = preferred_q + radius;
                if (right_q != left_q && right_q >= scan_min_q && right_q <= scan_max_q && !occupied[static_cast<std::size_t>(right_q)]) {
                    return right_q;
                }
            }
            return std::nullopt;
        };

        if (const std::optional<int> local_q = scan(min_q, max_q); local_q.has_value()) {
            return *local_q;
        }
        if (const std::optional<int> global_q = scan(1, args.width); global_q.has_value()) {
            return *global_q;
        }
        return std::max(1, std::min(args.width, preferred_q));
    };

    const double spacing = static_cast<double>(args.width) / static_cast<double>(source_count + 1);
    for (int index = 1; index <= source_count; ++index) {
        const int ideal_q = std::max(1, std::min(args.width, static_cast<int>(std::llround(spacing * index))));
        const int variance = args.source_variance;
        const int min_q = std::max(1, ideal_q - variance);
        const int max_q = std::min(args.width, ideal_q + variance);
        const double roll = unit_noise(args.seed, static_cast<std::uint32_t>(0x5f3759dfU + index * 7919));
        int q = min_q + static_cast<int>(std::floor(roll * static_cast<double>(max_q - min_q + 1)));
        q = std::max(1, std::min(args.width, q));

        q = nearest_open_q(q, min_q, max_q);
        occupied[static_cast<std::size_t>(q)] = true;
        sources.push_back({q, 1});
    }

    std::sort(sources.begin(), sources.end(), coord_less);
    return sources;
}

std::vector<Coord> generate_river_destinations(const GenerateArgs& args, int count) {
    std::vector<Coord> candidates = collect_southern_candidates(args, false, 1, args.width);
    if (candidates.empty()) {
        candidates = collect_southern_candidates(args, true, 1, args.width);
    }

    return select_spaced_points(candidates, count, args.seed ^ 0x85ebca6bU, args.width, args.height, 2);
}

std::vector<Coord> generate_merge_points(const GenerateArgs& args, int count) {
    return select_spaced_points(
        collect_merge_point_candidates(args, 1, args.width),
        count,
        args.seed ^ 0xc2b2ae35U,
        args.width,
        args.height,
        2
    );
}

std::optional<Coord> choose_merge_point_for_sources(
    const GenerateArgs& args,
    const Coord& first_source,
    const Coord& second_source,
    int merge_index
) {
    const int min_q = std::min(first_source.q, second_source.q) - args.horizontal_band;
    const int max_q = std::max(first_source.q, second_source.q) + args.horizontal_band;
    std::vector<Coord> candidates = collect_merge_point_candidates(args, min_q, max_q);
    if (candidates.empty()) {
        candidates = collect_merge_point_candidates(args, 1, args.width);
    }

    std::vector<Coord> selected = select_spaced_points(
        candidates,
        1,
        args.seed ^ static_cast<std::uint32_t>(0xc2b2ae35U + merge_index * 4099),
        args.width,
        args.height,
        1
    );

    if (selected.empty()) {
        return std::nullopt;
    }
    return selected[0];
}

std::optional<Coord> choose_destination_for_outlet(
    const GenerateArgs& args,
    const Coord& outlet,
    int destination_index
) {
    const int min_q = std::max(1, outlet.q - args.horizontal_band);
    const int max_q = std::min(args.width, outlet.q + args.horizontal_band);
    const double roll = unit_noise(args.seed, static_cast<std::uint32_t>(0x85ebca6bU + destination_index * 6151));
    const int q = min_q + static_cast<int>(std::floor(roll * static_cast<double>(max_q - min_q + 1)));
    return Coord{std::max(1, std::min(args.width, q)), args.height};
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

WorldPoint hex_center_world(const Coord& hex) {
    const int col = hex.q - 1;
    const int row = hex.r - 1;
    return {
        static_cast<double>(col) * 1.5,
        1.7320508075688772935 * (static_cast<double>(row) + ((col & 1) ? 0.5 : 0.0)),
    };
}

Coord nearest_hex_to_world_point(const GenerateArgs& args, const WorldPoint& point) {
    Coord best{1, 1};
    double best_distance = std::numeric_limits<double>::max();

    for (int r = 1; r <= args.height; ++r) {
        for (int q = 1; q <= args.width; ++q) {
            const Coord candidate{q, r};
            const WorldPoint center = hex_center_world(candidate);
            const double dx = center.x - point.x;
            const double dy = center.y - point.y;
            const double distance = dx * dx + dy * dy;
            if (distance < best_distance) {
                best = candidate;
                best_distance = distance;
            }
        }
    }

    return best;
}

std::vector<Coord> generate_meander_control_points(
    const GenerateArgs& args,
    const Coord& source,
    const Coord& destination,
    int segment_id
) {
    const WorldPoint start = hex_center_world(source);
    const WorldPoint end = hex_center_world(destination);
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length < 4.0) {
        return {};
    }

    const int control_count = std::max(1, std::min(5, static_cast<int>(std::round(length / 10.0))));
    const double normal_x = -dy / length;
    const double normal_y = dx / length;
    const double amplitude = std::max(1.5, std::min(args.width, args.height) * 0.12);

    std::vector<Coord> controls;
    controls.reserve(control_count);
    for (int i = 1; i <= control_count; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(control_count + 1);
        const double wave = (i % 2 == 0 ? -1.0 : 1.0);
        const double jitter = signed_noise(args.seed, static_cast<std::uint32_t>(segment_id * 30000 + i * 97));
        const double offset = wave * amplitude * (0.65 + std::abs(jitter) * 0.7);
        const double along = signed_noise(args.seed, static_cast<std::uint32_t>(segment_id * 30000 + i * 97 + 31)) * 1.4;
        const WorldPoint target{
            start.x + dx * t + normal_x * offset + (dx / length) * along,
            start.y + dy * t + normal_y * offset + (dy / length) * along,
        };
        const Coord snapped = nearest_hex_to_world_point(args, target);
        if (!coord_equal(snapped, source) && !coord_equal(snapped, destination) && !path_contains(controls, snapped)) {
            controls.push_back(snapped);
        }
    }

    return controls;
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

std::vector<Coord> route_meandering_river_path(
    const GenerateArgs& args,
    const Coord& source,
    const Coord& destination,
    int segment_id
) {
    std::vector<Coord> waypoints;
    waypoints.push_back(source);
    for (const Coord& control : generate_meander_control_points(args, source, destination, segment_id)) {
        waypoints.push_back(control);
    }
    waypoints.push_back(destination);

    std::vector<Coord> full_path;
    for (std::size_t i = 1; i < waypoints.size(); ++i) {
        std::vector<Coord> subpath = route_river_path(
            args,
            waypoints[i - 1],
            waypoints[i],
            segment_id * 100 + static_cast<int>(i)
        );
        if (subpath.empty()) {
            continue;
        }

        if (full_path.empty()) {
            full_path = subpath;
        } else {
            full_path.insert(full_path.end(), subpath.begin() + 1, subpath.end());
        }
    }

    if (full_path.empty() || !coord_equal(full_path.back(), destination)) {
        return route_river_path(args, source, destination, segment_id);
    }

    return full_path;
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

    std::vector<RiverSegment> segments;
    int segment_id = 1;
    std::vector<RiverOutlet> outlets;
    int merge_index = 0;

    for (std::size_t i = 0; i < sources.size();) {
        const bool can_merge = i + 1 < sources.size();
        const double merge_roll = can_merge
            ? unit_noise(args.seed, static_cast<std::uint32_t>(0x6d2b79f5U + i * 97))
            : 1.0;

        if (can_merge && merge_roll < 0.28) {
            const std::optional<Coord> merge_point = choose_merge_point_for_sources(
                args,
                sources[i],
                sources[i + 1],
                merge_index
            );

            if (merge_point.has_value()) {
                merge_points.push_back(*merge_point);

                const std::vector<Coord> first_hex_path = route_meandering_river_path(
                    args,
                    sources[i],
                    *merge_point,
                    segment_id
                );
                segments.push_back({
                    segment_id,
                    sources[i],
                    *merge_point,
                    "source_to_merge",
                    trace_outside_edge_path(args, first_hex_path, segment_id),
                });
                ++segment_id;

                const std::vector<Coord> second_hex_path = route_meandering_river_path(
                    args,
                    sources[i + 1],
                    *merge_point,
                    segment_id
                );
                segments.push_back({
                    segment_id,
                    sources[i + 1],
                    *merge_point,
                    "source_to_merge",
                    trace_outside_edge_path(args, second_hex_path, segment_id),
                });
                ++segment_id;

                outlets.push_back({*merge_point, "merge_to_destination"});
                ++merge_index;
                i += 2;
                continue;
            }
        }

        outlets.push_back({sources[i], "source_to_destination"});
        ++i;
    }

    std::sort(merge_points.begin(), merge_points.end(), coord_less);

    for (std::size_t i = 0; i < outlets.size(); ++i) {
        const std::optional<Coord> destination = choose_destination_for_outlet(
            args,
            outlets[i].point,
            static_cast<int>(i)
        );
        if (!destination.has_value()) {
            continue;
        }

        destinations.push_back(*destination);
        const std::vector<Coord> hex_path = route_meandering_river_path(
            args,
            outlets[i].point,
            *destination,
            segment_id
        );
        segments.push_back({
            segment_id,
            outlets[i].point,
            *destination,
            outlets[i].kind,
            trace_outside_edge_path(args, hex_path, segment_id),
        });
        ++segment_id;
    }

    std::sort(destinations.begin(), destinations.end(), coord_less);

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
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--rivers <n>] [--source-variance <n>] [--horizontal-band <n>]\n";
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
