#include "game_state.h"
#include "steppe_generator.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

std::string read_stdin() {
    return {std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
}

std::string arg_value(int argc, char** argv, const std::string& name, const std::string& fallback) {
    for (int i = 2; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return argv[i + 1];
        }
    }
    return fallback;
}

int int_arg(int argc, char** argv, const std::string& name, int fallback) {
    try {
        return std::stoi(arg_value(argc, argv, name, std::to_string(fallback)));
    } catch (...) {
        return fallback;
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        steppe::print_usage();
        return EXIT_FAILURE;
    }

    try {
        const std::string command = argv[1];
        if (command == "generate") {
            steppe::print_generated_map(steppe::parse_generate_args(argc, argv));
            return EXIT_SUCCESS;
        }
        if (command == "game-new") {
            steppe::game::print_game_state_json(
                steppe::game::create_default_play_sandbox(
                    int_arg(argc, argv, "--width", 10),
                    int_arg(argc, argv, "--height", 10),
                    int_arg(argc, argv, "--factions", 2)
                ),
                std::cout
            );
            return EXIT_SUCCESS;
        }
        if (command == "game-reachable") {
            const steppe::game::GameState state = steppe::game::parse_game_state_json(read_stdin());
            steppe::game::print_reachable_json(
                steppe::game::reachable_hexes(state, int_arg(argc, argv, "--unit", 0)),
                std::cout
            );
            return EXIT_SUCCESS;
        }
        if (command == "game-move") {
            steppe::game::GameState state = steppe::game::parse_game_state_json(read_stdin());
            const bool ok = steppe::game::move_unit(
                state,
                int_arg(argc, argv, "--unit", 0),
                {int_arg(argc, argv, "--q", 0), int_arg(argc, argv, "--r", 0)}
            );
            steppe::game::print_game_patch_json(state, ok, std::cout);
            return ok ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        if (command == "game-end-turn") {
            steppe::game::GameState state = steppe::game::parse_game_state_json(read_stdin());
            steppe::game::end_turn(state);
            steppe::game::print_game_patch_json(state, true, std::cout);
            return EXIT_SUCCESS;
        }

        throw std::runtime_error("unknown command: " + command);
    } catch (const std::exception& ex) {
        std::cerr << "steppe_engine: " << ex.what() << "\n";
        steppe::print_usage();
        return EXIT_FAILURE;
    }
}
