#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
#include <windows.h>

using json = nlohmann::json;


class DiscordRPC {
private:
    std::string clientId;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    bool connected = false;
    int64_t startTime;

    bool connectToDiscord() {
        // Try to connect to Discord IPC pipes
        for (int i = 0; i < 10; i++) {
            std::string pipeName = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
            pipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

            if (pipe != INVALID_HANDLE_VALUE) {
                // Send handshake
                json handshake = {
                    {"v", 1},
                    {"client_id", clientId}
                };

                if (sendMessage(0, handshake.dump())) {
                    connected = true;
                    std::cout << "✅ Connected to Discord via pipe: " << pipeName << std::endl;
                    return true;
                }
                CloseHandle(pipe);
                pipe = INVALID_HANDLE_VALUE;
            }
        }
        std::cout << "❌ Could not connect to Discord. Make sure Discord is running!" << std::endl;
        return false;
    }

    bool sendMessage(int opcode, const std::string& data) {
        if (pipe == INVALID_HANDLE_VALUE) return false;

        uint32_t length = static_cast<uint32_t>(data.length());
        DWORD written;

        // Write opcode (4 bytes)
        if (!WriteFile(pipe, &opcode, sizeof(opcode), &written, NULL) || written != sizeof(opcode)) {
            return false;
        }

        // Write length (4 bytes)
        if (!WriteFile(pipe, &length, sizeof(length), &written, NULL) || written != sizeof(length)) {
            return false;
        }

        // Write data
        if (!WriteFile(pipe, data.c_str(), length, &written, NULL) || written != length) {
            return false;
        }

        FlushFileBuffers(pipe);
        return true;
    }

public:
    DiscordRPC(const std::string& cid) : clientId(cid) {
        startTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    bool connect() {
        std::cout << "🔗 Attempting to connect to Discord..." << std::endl;
        return connectToDiscord();
    }

    bool isConnected() const {
        return connected;
    }

    void updatePresence(const std::string& state, const std::string& details,
                       const std::string& largeImageKey = "lol_logo",
                       const std::string& largeImageText = "League of Legends") {
        if (!connected) {
            std::cout << "Discord not connected. Status: " << details << " | " << state << std::endl;
            return;
        }

        json activity = {
            {"details", details},
            {"state", state},
            {"timestamps", {
                {"start", startTime}
            }},
            {"assets", {
                {"large_image", largeImageKey},
                {"large_text", largeImageText}
            }},
            {"buttons", {
                {
                    {"label", "Get it"},
                    {"url", "https://github.com/MBKarrigan/lol-discord-rpc"}
                },
                {
                    {"label", "My website"},
                    {"url", "https://dev.karrigan.me"}
                }
            }}
        };

        json payload = {
            {"cmd", "SET_ACTIVITY"},
            {"nonce", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count())},
            {"args", {
                {"pid", GetCurrentProcessId()},
                {"activity", activity}
            }}
        };

        if (sendMessage(1, payload.dump())) {
            std::cout << "🎮 Discord Updated: " << details << " | " << state << std::endl;
        } else {
            std::cout << "❌ Failed to update Discord presence" << std::endl;
            connected = false; // Mark as disconnected so we can retry
        }
    }

    void clearPresence() {
        if (!connected) return;

        json payload = {
            {"cmd", "SET_ACTIVITY"},
            {"nonce", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count())},
            {"args", {
                {"pid", GetCurrentProcessId()},
                {"activity", nullptr}
            }}
        };

        sendMessage(1, payload.dump());
    }

    ~DiscordRPC() {
        if (pipe != INVALID_HANDLE_VALUE) {
            clearPresence();
            CloseHandle(pipe);
        }
    }
};

int main() {
    std::cout << "=== League of Legends Discord RPC ===" << std::endl;
    std::cout << "Make sure Discord is running before continuing!" << std::endl;

    std::string clientId;
    std::cout << "\nDiscord Bot Client ID: ";
    std::getline(std::cin, clientId);

    if (clientId.empty()) {
        std::cerr << "❌ Client ID cannot be empty!" << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    DiscordRPC rpc(clientId);
    bool discordConnected = rpc.connect();

    if (!discordConnected) {
        std::cout << "⚠️  Continuing without Discord RPC..." << std::endl;
    }

    std::cout << "\n🎮 Starting League of Legends monitoring..." << std::endl;
    std::cout << "Start a League of Legends game to see your status!" << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;

    const std::string url = "https://127.0.0.1:2999/liveclientdata/allgamedata";
    bool wasInGame = false;

    while (true) {
        try {
            cpr::Response response = cpr::Get(
                cpr::Url{url},
                cpr::VerifySsl{false},
                cpr::Timeout{3000}
            );

            if (response.status_code == 200) {
                json data = json::parse(response.text);

                // Extract game data
                std::string match = data["gameData"]["gameMode"];
                int level = data["activePlayer"]["level"];
                int gold = std::round(data["activePlayer"]["currentGold"].get<double>());
                int kills = data["allPlayers"][0]["scores"]["kills"];
                int deaths = data["allPlayers"][0]["scores"]["deaths"];
                int assists = data["allPlayers"][0]["scores"]["assists"];
                std::string champ = data["allPlayers"][0]["championName"];

                // Create Discord status
                std::string state = std::to_string(kills) + "/" + std::to_string(deaths) + "/" +
                                  std::to_string(assists) + " Gold: " + std::to_string(gold);
                std::string details = "Playing " + match + " on " + champ;

                // Try to reconnect if Discord was disconnected
                if (!rpc.isConnected()) {
                    discordConnected = rpc.connect();
                }

                rpc.updatePresence(state, details);
                wasInGame = true;
            }
            else {
                if (wasInGame) {
                    std::cout << "🔴 Game ended or API unavailable" << std::endl;
                    wasInGame = false;
                }

                if (!rpc.isConnected()) {
                    discordConnected = rpc.connect();
                }

                rpc.updatePresence("Offline", " ");
            }
        }
        catch (const json::exception& e) {
            std::cerr << "❌ JSON Error: " << e.what() << std::endl;
            rpc.updatePresence("Unable to parse game data", " ");
        }
        catch (const std::exception& e) {
            if (wasInGame) {
                std::cerr << "❌ Request Error: " << e.what() << std::endl;
                wasInGame = false;
            }
            rpc.updatePresence("Offline", " ");
        }

        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}