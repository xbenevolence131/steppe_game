#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GenerateArgs {
    int width = 16;
    int height = 10;
    std::uint32_t seed = 1;
    int river_sources = 3;
};

struct Coord {
    int q = 1;
    int r = 1;
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
        } else if (key == "--river-sources" && i + 1 < argc) {
            args.river_sources = parse_non_negative_int(argv[++i], "river sources");
        } else {
            throw std::runtime_error("unknown or incomplete argument: " + key);
        }
    }

    if (args.width > 80 || args.height > 60) {
        throw std::runtime_error("width and height are capped at 80x60 for the prototype");
    }
    if (args.river_sources > 100) {
        throw std::runtime_error("river sources are capped at 100 for the prototype");
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
    for (const Coord& source : selected) {
        if (hex_distance(candidate, source) < minimum_distance) {
            return false;
        }
    }
    return true;
}

std::vector<Coord> collect_river_source_candidates(const GenerateArgs& args, bool allow_top_edge) {
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

std::vector<Coord> generate_river_sources(const GenerateArgs& args) {
    std::vector<Coord> candidates = collect_river_source_candidates(args, false);
    if (candidates.empty()) {
        candidates = collect_river_source_candidates(args, true);
    }

    std::mt19937 rng(args.seed ^ 0x5f3759dfU);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::vector<Coord> selected;
    selected.reserve(std::min(args.river_sources, static_cast<int>(candidates.size())));

    const int starting_distance = std::max(2, std::min(args.width, args.height) / 4 + 1);
    for (int minimum_distance = starting_distance; minimum_distance >= 1; --minimum_distance) {
        for (const Coord& candidate : candidates) {
            if (static_cast<int>(selected.size()) >= args.river_sources) {
                return selected;
            }
            if (is_far_enough(candidate, selected, minimum_distance)) {
                selected.push_back(candidate);
            }
        }
    }

    return selected;
}

void print_generated_map(const GenerateArgs& args) {
    const std::vector<Coord> river_sources = generate_river_sources(args);

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
    std::cout << "\"edges\":[],";
    std::cout << "\"roads\":[],";
    std::cout << "\"metadata\":{";
    std::cout << "\"generator\":\"prototype-steppe-blob\",";
    std::cout << "\"terrain_types\":[\"none\",\"grassland\",\"light_forest\",\"heavy_forest\",\"hills\",\"mountains\",\"urban\"]";
    std::cout << "}";
    std::cout << "}\n";
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  steppe_engine generate --width <n> --height <n> [--seed <n>] [--river-sources <n>]\n";
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
