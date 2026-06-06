#include "game_state.h"
#include "steppe_generator.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error "steppe_daemon currently supports Windows Winsock only."
#endif

namespace {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

class WinsockSession {
public:
    WinsockSession() {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~WinsockSession() {
        WSACleanup();
    }
};

std::map<std::string, steppe::game::GameState> games;
std::map<std::string, std::vector<steppe::game::GameState>> undo_stacks;
constexpr std::size_t max_undo_depth = 64;

std::size_t find_matching(const std::string& json, std::size_t open_index, char open, char close) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t i = open_index; i < json.size(); ++i) {
        const char ch = json[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
        } else if (ch == open) {
            ++depth;
        } else if (ch == close) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    throw std::runtime_error("invalid JSON structure");
}

std::string object_field(const std::string& json, const std::string& key) {
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) {
        return {};
    }
    std::size_t value_start = start + marker.size();
    while (value_start < json.size() && std::isspace(static_cast<unsigned char>(json[value_start]))) {
        ++value_start;
    }
    if (value_start >= json.size()) {
        return {};
    }
    if (json[value_start] == '[') {
        return json.substr(value_start, find_matching(json, value_start, '[', ']') - value_start + 1);
    }
    if (json[value_start] == '{') {
        return json.substr(value_start, find_matching(json, value_start, '{', '}') - value_start + 1);
    }
    if (json[value_start] == '"') {
        std::size_t end = value_start + 1;
        bool escaped = false;
        for (; end < json.size(); ++end) {
            if (escaped) {
                escaped = false;
            } else if (json[end] == '\\') {
                escaped = true;
            } else if (json[end] == '"') {
                break;
            }
        }
        return json.substr(value_start, end - value_start + 1);
    }
    std::size_t end = value_start;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') {
        ++end;
    }
    return json.substr(value_start, end - value_start);
}

std::string string_field(const std::string& json, const std::string& key, const std::string& fallback = "") {
    const std::string field = object_field(json, key);
    if (field.size() < 2 || field.front() != '"' || field.back() != '"') {
        return fallback;
    }
    return field.substr(1, field.size() - 2);
}

int int_field(const std::string& json, const std::string& key, int fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return std::stoi(field);
    } catch (...) {
        return fallback;
    }
}

double number_field(const std::string& json, const std::string& key, double fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return std::stod(field);
    } catch (...) {
        return fallback;
    }
}

std::uint32_t seed_field(const std::string& json, const std::string& key, std::uint32_t fallback) {
    const std::string field = object_field(json, key);
    if (field.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::uint32_t>(std::stoul(field));
    } catch (...) {
        return fallback;
    }
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

std::string game_patch_json(const steppe::game::GameState& state, bool ok) {
    std::ostringstream out;
    steppe::game::print_game_patch_json(state, ok, out);
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

steppe::GenerateArgs generate_args_from_command(const std::string& command) {
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
    const std::string game_id = string_field(body, "gameId", "local-dev");
    const std::string command = object_field(body, "command");
    const std::string type = string_field(command, "type", "");
    if (command.empty() || type.empty()) {
        return error_response("command envelope must include command.type");
    }

    if (type == "generate_map") {
        return ok_response(game_id, generated_map_json(steppe::generate_map(generate_args_from_command(command))));
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
    if (type == "move_unit") {
        const std::string to = object_field(command, "to");
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::move_unit(
            state,
            int_field(command, "unitId", 0),
            {int_field(to, "q", 0), int_field(to, "r", 0)}
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
        const std::string to = object_field(command, "to");
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::detach_herd(
            state,
            int_field(command, "unitId", 0),
            std::max(0, int_field(command, "horses", 0)),
            {int_field(to, "q", 0), int_field(to, "r", 0)}
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
        const std::string to = object_field(command, "to");
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::create_horse_archers(
            state,
            int_field(command, "unitId", 0),
            {int_field(to, "q", 0), int_field(to, "r", 0)}
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
        const std::string to = object_field(command, "to");
        const steppe::game::GameState before = state;
        const bool ok = steppe::game::create_mongol_lancers(
            state,
            int_field(command, "unitId", 0),
            {int_field(to, "q", 0), int_field(to, "r", 0)}
        );
        if (ok) {
            push_undo_state(game_id, before);
        }
        return command_response(game_id, ok, game_patch_json(state, ok));
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

void send_all(SOCKET socket, const std::string& text) {
    const char* data = text.data();
    int remaining = static_cast<int>(text.size());
    while (remaining > 0) {
        const int sent = send(socket, data, remaining, 0);
        if (sent <= 0) {
            return;
        }
        data += sent;
        remaining -= sent;
    }
}

std::string http_response(int status, const std::string& body) {
    const char* reason = status == 200 ? "OK" : status == 404 ? "Not Found" : status == 405 ? "Method Not Allowed" : "Bad Request";
    std::ostringstream out;
    out << "HTTP/1.1 " << status << " " << reason << "\r\n"
        << "Content-Type: application/json; charset=utf-8\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return out.str();
}

HttpRequest read_http_request(SOCKET socket) {
    std::string raw;
    char buffer[4096];
    std::size_t header_end = std::string::npos;
    while (header_end == std::string::npos) {
        const int received = recv(socket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            throw std::runtime_error("client disconnected");
        }
        raw.append(buffer, buffer + received);
        header_end = raw.find("\r\n\r\n");
        if (raw.size() > 9 * 1024 * 1024) {
            throw std::runtime_error("request too large");
        }
    }

    const std::string header = raw.substr(0, header_end);
    const std::size_t request_line_end = header.find("\r\n");
    const std::string request_line = request_line_end == std::string::npos ? header : header.substr(0, request_line_end);
    std::istringstream request_line_stream(request_line);
    HttpRequest request;
    request_line_stream >> request.method >> request.path;

    std::size_t content_length = 0;
    std::istringstream header_stream(header);
    std::string line;
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string lower_prefix = "content-length:";
        if (line.size() >= lower_prefix.size()) {
            std::string prefix = line.substr(0, lower_prefix.size());
            std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            if (prefix == lower_prefix) {
                content_length = static_cast<std::size_t>(std::stoul(line.substr(lower_prefix.size())));
            }
        }
    }

    request.body = raw.substr(header_end + 4);
    while (request.body.size() < content_length) {
        const int received = recv(socket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            throw std::runtime_error("client disconnected");
        }
        request.body.append(buffer, buffer + received);
    }
    if (request.body.size() > content_length) {
        request.body.resize(content_length);
    }
    return request;
}

void serve_client(SOCKET client) {
    try {
        const HttpRequest request = read_http_request(client);
        if (request.method == "GET" && request.path == "/health") {
            send_all(client, http_response(200, "{\"ok\":true}"));
        } else if (request.method == "POST" && request.path == "/command") {
            send_all(client, http_response(200, handle_command(request.body)));
        } else if (request.method != "GET" && request.method != "POST") {
            send_all(client, http_response(405, error_response("method not allowed")));
        } else {
            send_all(client, http_response(404, error_response("not found")));
        }
    } catch (const std::exception& ex) {
        send_all(client, http_response(400, error_response(ex.what())));
    }
    closesocket(client);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const WinsockSession winsock;
        const int port = parse_port(argc, argv);
        SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server == INVALID_SOCKET) {
            throw std::runtime_error("socket creation failed");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

        if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
            closesocket(server);
            throw std::runtime_error("bind failed on 127.0.0.1:" + std::to_string(port));
        }
        if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(server);
            throw std::runtime_error("listen failed");
        }

        std::cout << "steppe_daemon listening at http://127.0.0.1:" << port << "\n";
        while (true) {
            SOCKET client = accept(server, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                continue;
            }
            serve_client(client);
        }
    } catch (const std::exception& ex) {
        std::cerr << "steppe_daemon: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
