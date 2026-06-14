#include "game_state.h"
#include "steppe_generator.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::map<std::string, steppe::game::GameState> games;
std::map<std::string, std::vector<steppe::game::GameState>> undo_stacks;
constexpr std::size_t max_undo_depth = 64;

using Json = nlohmann::json;

int int_field(const Json& json, const char* key, int fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number_integer()) {
        return fallback;
    }
    return found->get<int>();
}

double number_field(const Json& json, const char* key, double fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number()) {
        return fallback;
    }
    return found->get<double>();
}

std::uint32_t seed_field(const Json& json, const char* key, std::uint32_t fallback) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_number_unsigned()) {
        return fallback;
    }
    return found->get<std::uint32_t>();
}

std::string string_field(const Json& json, const char* key, const std::string& fallback = "") {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_string()) {
        return fallback;
    }
    return found->get<std::string>();
}

steppe::Coord coord_field(const Json& json, const char* key) {
    const auto found = json.find(key);
    if (found == json.end() || !found->is_object()) {
        return {0, 0};
    }
    return {int_field(*found, "q", 0), int_field(*found, "r", 0)};
}

std::string escape_json(const std::string& text) {
    std::string escaped;
    for (const char ch : text) {
        if (ch == '"' || ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

std::string game_state_json(const steppe::game::GameState& state) {
    std::ostringstream out;
    steppe::game::print_game_state_json(state, out);
    return out.str();
}

std::string game_patch_json(
    const steppe::game::GameState& state,
    bool ok,
    const std::vector<steppe::game::AiAnimationStep>* animation = nullptr
) {
    std::ostringstream out;
    steppe::game::print_game_patch_json(state, ok, out, animation);
    return out.str();
}

std::string detach_herd_options_json(const steppe::game::DetachHerdOptions& options) {
    std::ostringstream out;
    steppe::game::print_detach_herd_options_json(options, out);
    return out.str();
}

std::string create_horse_archers_options_json(const steppe::game::CreateHorseArchersOptions& options) {
    std::ostringstream out;
    steppe::game::print_create_horse_archers_options_json(options, out);
    return out.str();
}

std::string create_mongol_lancers_options_json(const steppe::game::CreateUnitOptions& options) {
    std::ostringstream out;
    steppe::game::print_create_mongol_lancers_options_json(options, out);
    return out.str();
}

std::string combat_preview_json(const steppe::game::CombatPreview& preview) {
    std::ostringstream out;
    steppe::game::print_combat_preview_json(preview, out);
    return out.str();
}

std::string unit_defaults_json() {
    std::ostringstream out;
    out << "{\"units\":{";
    const std::vector<steppe::game::UnitKind> kinds = steppe::game::unit_kinds();
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        const steppe::game::UnitDefaults defaults = steppe::game::unit_defaults(kinds[i]);
        if (i > 0) {
            out << ",";
        }
        out << "\"" << steppe::game::unit_kind_key(defaults.kind) << "\":{"
            << "\"stance\":\"" << steppe::game::unit_stance_key(defaults.stance) << "\""
            << ",\"legalStances\":[";
        for (std::size_t stance_index = 0; stance_index < defaults.legal_stances.size(); ++stance_index) {
            if (stance_index > 0) {
                out << ",";
            }
            out << "\"" << steppe::game::unit_stance_key(defaults.legal_stances[stance_index]) << "\"";
        }
        out << "],"
            << "\"hp\":" << defaults.hp
            << ",\"attack\":" << defaults.attack
            << ",\"defense\":" << defaults.defense
            << ",\"readinessDamage\":" << defaults.readiness_damage
            << ",\"readiness\":" << defaults.readiness
            << ",\"move\":" << defaults.move
            << ",\"projectsZoc\":" << (defaults.projects_zoc ? "true" : "false")
            << ",\"respectsZoc\":" << (defaults.respects_zoc ? "true" : "false")
            << ",\"population\":" << defaults.population
            << ",\"horses\":" << defaults.horses
            << ",\"allowedFactions\":[";
        bool wrote_faction = false;
        const auto write_faction = [&](const char* key, steppe::game::OwnerId owner) {
            if (!steppe::game::unit_kind_available_to_owner(defaults.kind, owner)) {
                return;
            }
            if (wrote_faction) {
                out << ",";
            }
            out << "\"" << key << "\"";
            wrote_faction = true;
        };
        write_faction("mongol", steppe::game::mongol_owner);
        write_faction("chinese", steppe::game::chinese_owner);
        write_faction("persian", steppe::game::persian_owner);
        write_faction("jurchen", steppe::game::jurchen_owner);
        write_faction("forest_nomad", steppe::game::forest_nomad_owner);
        out << "],\"allowedArchetypes\":[";
        bool wrote_archetype = false;
        const auto write_archetype = [&](const char* key) {
            if (!steppe::game::unit_kind_available_to_archetype(defaults.kind, key)) {
                return;
            }
            if (wrote_archetype) {
                out << ",";
            }
            out << "\"" << key << "\"";
            wrote_archetype = true;
        };
        write_archetype("steppe_nomad");
        write_archetype("chinese");
        write_archetype("persian");
        write_archetype("jurchen");
        write_archetype("forest_nomad");
        out << "]"
            << "}";
    }
    out << "}}";
    return out.str();
}

std::string generated_map_json(const steppe::GeneratedMap& map) {
    std::ostringstream out;
    std::streambuf* previous = std::cout.rdbuf(out.rdbuf());
    steppe::print_generated_map_json(map);
    std::cout.rdbuf(previous);
    return out.str();
}

std::string ok_response(const std::string& game_id, const std::string& view_json) {
    return "{\"ok\":true,\"gameId\":\"" + escape_json(game_id) + "\",\"view\":" + view_json + "}";
}

std::string command_response(const std::string& game_id, bool ok, const std::string& view_json) {
    return std::string("{\"ok\":") + (ok ? "true" : "false")
        + ",\"gameId\":\"" + escape_json(game_id) + "\",\"view\":" + view_json + "}";
}

void push_undo_state(const std::string& game_id, const steppe::game::GameState& state) {
    std::vector<steppe::game::GameState>& stack = undo_stacks[game_id];
    stack.push_back(state);
    if (stack.size() > max_undo_depth) {
        stack.erase(stack.begin());
    }
}

std::string error_response(const std::string& message) {
    return "{\"ok\":false,\"error\":\"" + escape_json(message) + "\"}";
}

steppe::GenerateArgs generate_args_from_command(const Json& command) {
    steppe::GenerateArgs args;
    args.width = std::max(1, std::min(120, int_field(command, "width", args.width)));
    args.height = std::max(1, std::min(80, int_field(command, "height", args.height)));
    args.river_count = std::max(0, std::min(20, int_field(command, "rivers", args.river_count)));
    args.lake_count = std::max(0, std::min(20, int_field(command, "lakes", args.lake_count)));
    args.lake_size = std::max(1, std::min(40, int_field(command, "lakeSize", args.lake_size)));
    args.meander_forward = std::max(0.0, std::min(40.0, number_field(command, "meanderForward", args.meander_forward)));
    args.meander_forward_jitter = std::max(0.0, std::min(40.0, number_field(command, "meanderForwardJitter", args.meander_forward_jitter)));
    args.meander_lateral = std::max(0.0, std::min(40.0, number_field(command, "meanderLateral", args.meander_lateral)));
    args.meander_lateral_jitter = std::max(0.0, std::min(40.0, number_field(command, "meanderLateralJitter", args.meander_lateral_jitter)));
    args.meander_strength = std::max(0.0, std::min(10.0, number_field(command, "meanderStrength", args.meander_strength)));
    args.meander_reach = std::max(0.0, std::min(40.0, number_field(command, "meanderReach", args.meander_reach)));
    args.river_slant_strength = std::max(0.0, std::min(10.0, number_field(command, "riverSlantStrength", args.river_slant_strength)));
    args.valley_thickness = std::max(0.0, std::min(5.0, number_field(command, "valleyThickness", args.valley_thickness)));
    args.forest_blob_count = std::max(0, std::min(10, int_field(command, "forestBlobs", args.forest_blob_count)));
    args.forest_blob_radius = std::max(0.0, std::min(20.0, number_field(command, "forestBlobRadius", args.forest_blob_radius)));
    args.meander_timeout = std::max(1, std::min(200, int_field(command, "meanderTimeout", args.meander_timeout)));
    args.seed = seed_field(command, "seed", args.seed);
    return args;
}

std::string handle_command(const std::string& body) {
    Json envelope;
    try {
        envelope = Json::parse(body);
    } catch (const Json::parse_error& error) {
        return error_response(std::string("invalid JSON command envelope: ") + error.what());
    }
    if (!envelope.is_object()) {
        return error_response("command envelope must be a JSON object");
    }

    const std::string game_id = string_field(envelope, "gameId", "local-dev");
    const auto command_it = envelope.find("command");
    if (command_it == envelope.end() || !command_it->is_object()) {
        return error_response("command envelope must include command.type");
    }
    const Json& command = *command_it;
    const std::string type = string_field(command, "type", "");
    if (type.empty()) {
        return error_response("command envelope must include command.type");
    }

    if (type == "generate_map") {
        return ok_response(game_id, generated_map_json(steppe::generate_map(generate_args_from_command(command))));
    }
    if (type == "unit_defaults") {
        return ok_response(game_id, unit_defaults_json());
    }
    if (type == "new_game") {
        steppe::game::GameState state = steppe::game::create_default_play_sandbox(
            std::max(1, std::min(120, int_field(command, "width", 10))),
            std::max(1, std::min(80, int_field(command, "height", 10))),
            std::max(1, std::min(3, int_field(command, "factions", 2)))
        );
        games[game_id] = state;
        undo_stacks[game_id].clear();
        return ok_response(game_id, game_state_json(state));
    }
    if (type == "load_game") {
        const auto state_it = command.find("state");
        if (state_it == command.end() || !state_it->is_object()) {
            return error_response("load_game requires command.state");
        }
        steppe::game::GameState state = steppe::game::parse_game_state_json(state_it->dump());
        games[game_id] = state;
        undo_stacks[game_id].clear();
        return ok_response(game_id, game_state_json(state));
    }

    auto found = games.find(game_id);
    if (found == games.end()) {
        return error_response("unknown gameId: " + game_id);
    }

    steppe::game::GameState& state = found->second;
    if (type == "undo") {
        std::vector<steppe::game::GameState>& stack = undo_stacks[game_id];
        if (stack.empty()) {
            return command_response(game_id, false, game_patch_json(state, false));
        }
        state = stack.back();
        stack.pop_back();
        return command_response(game_id, true, game_patch_json(state, true));
    }
    if (type == "select_unit") {
        const bool ok = steppe::game::select_unit(state, int_field(command, "unitId", 0));
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "set_unit_stance") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::set_unit_stance(
            state,
            int_field(command, "unitId", 0),
            steppe::game::unit_stance_from_key(string_field(command, "stance", "default"))
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "move_unit") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::move_unit(
            state,
            int_field(command, "unitId", 0),
            coord_field(command, "to")
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "attack_unit") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::attack_unit(
            state,
            int_field(command, "attackerId", 0),
            int_field(command, "defenderId", 0)
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "combat_preview") {
        const steppe::game::CombatPreview preview = steppe::game::combat_preview(
            state,
            int_field(command, "attackerId", 0),
            int_field(command, "defenderId", 0)
        );
        return command_response(game_id, preview.valid, combat_preview_json(preview));
    }
    if (type == "detach_herd_options") {
        const steppe::game::DetachHerdOptions options = steppe::game::detach_herd_options(
            state,
            int_field(command, "unitId", 0),
            std::max(0, int_field(command, "horses", 0))
        );
        return command_response(game_id, true, detach_herd_options_json(options));
    }
    if (type == "detach_herd") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::detach_herd(
            state,
            int_field(command, "unitId", 0),
            std::max(0, int_field(command, "horses", 0)),
            coord_field(command, "to")
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "create_horse_archers_options") {
        const steppe::game::CreateHorseArchersOptions options = steppe::game::create_horse_archers_options(
            state,
            int_field(command, "unitId", 0)
        );
        return command_response(game_id, true, create_horse_archers_options_json(options));
    }
    if (type == "create_horse_archers") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::create_horse_archers(
            state,
            int_field(command, "unitId", 0),
            coord_field(command, "to")
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "create_mongol_lancers_options") {
        const steppe::game::CreateUnitOptions options = steppe::game::create_mongol_lancers_options(
            state,
            int_field(command, "unitId", 0)
        );
        return command_response(game_id, true, create_mongol_lancers_options_json(options));
    }
    if (type == "create_mongol_lancers") {
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::create_mongol_lancers(
            state,
            int_field(command, "unitId", 0),
            coord_field(command, "to")
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
    }
    if (type == "execute_ai_group") {
        const steppe::game::GameState before = state;
        std::vector<steppe::game::AiAnimationStep> animation;
        const bool ok = steppe::game::execute_ai_group_turn(
            state,
            int_field(command, "groupId", 0),
            &animation
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok, &animation));
    }
    if (type == "step_ai_turn") {
        const steppe::game::GameState before = state;
        steppe::game::AiAnimationStep step;
        const bool ok = steppe::game::step_ai_turn(state, &step);
        std::vector<steppe::game::AiAnimationStep> animation;
        if (ok) {
            animation.push_back(std::move(step));
        } else {
            steppe::game::end_turn(state);
        }
        push_undo_state(game_id, before);
        return command_response(game_id, true, game_patch_json(state, true, &animation));
    }
    if (type == "end_turn") {
        const steppe::game::GameState before = state;
        steppe::game::end_turn(state);
        push_undo_state(game_id, before);
        return command_response(game_id, true, game_patch_json(state, true));
    }

    return error_response("unknown command type: " + type);
}

int parse_port(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--port") {
            return std::max(1, std::min(65535, std::atoi(argv[i + 1])));
        }
    }
    return 4001;
}

} // namespace

int main(int argc, char** argv) {
    try {
        steppe::game::load_unit_types();
        const int port = parse_port(argc, argv);
        httplib::Server server;
        server.set_payload_max_length(9 * 1024 * 1024);
        server.Get("/health", [](const httplib::Request&, httplib::Response& response) {
            response.set_content("{\"ok\":true}", "application/json; charset=utf-8");
        });
        server.Post("/command", [](const httplib::Request& request, httplib::Response& response) {
            response.set_content(handle_command(request.body), "application/json; charset=utf-8");
        });

        std::cout << "steppe_daemon listening at http://127.0.0.1:" << port << "\n";
        if (!server.listen("127.0.0.1", port)) {
            throw std::runtime_error("bind/listen failed on 127.0.0.1:" + std::to_string(port));
        }
    } catch (const std::exception& ex) {
        std::cerr << "steppe_daemon: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
