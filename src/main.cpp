#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "botcraft/Game/ConnectionClient.hpp"
#include "botcraft/Network/NetworkManager.hpp"
#include "botcraft/Utilities/Logger.hpp"
#include "protocolCraft/Utilities/Json.hpp"
#include "protocolCraft/enums.hpp"

using namespace ProtocolCraft::Json;

// Global client pointer for signal handler
Botcraft::ConnectionClient* g_client = nullptr;

// Global flag for skipping wait cycle
std::atomic<bool> g_skip_wait(false);
std::mutex        g_skip_mutex;

void              SignalHandler(int signal) {
    if (g_client) {
        LOG_WARNING("Received signal " << signal << ", disconnecting...");
        g_client->Disconnect();
    }
    std::exit(1);
}

// Set up terminate handler to catch uncaught exceptions
void TerminateHandler() {
    // Use std::cerr instead of LOG_FATAL to avoid potential exceptions in
    // terminate handler
    std::cerr << "[FATAL] Uncaught exception in background thread. Attempting "
                 "graceful shutdown..."
              << std::endl;
    try {
        if (g_client) {
            g_client->Disconnect();
        }
    } catch (...) {
        // Ignore any exceptions during shutdown
    }
    std::abort();
}

struct LocationConfig {
    std::string name;
    std::string type; // "warp" or "home"
    int         wait_minutes;
};

struct Config {
    std::string                 address;
    std::string                 username;
    std::string                 password;
    std::vector<LocationConfig> locations;
};

const std::string CONFIG_FILE = "config.json";

std::string       GetInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

std::string GetPasswordInput(const std::string& prompt) {
    std::string password;
    std::cout << prompt;
    // On Unix systems, we can hide password input, but for simplicity, just read
    // normally
    std::getline(std::cin, password);
    return password;
}

bool SaveConfig(const Config& config) {
    try {
        Value json_config;
        json_config["server"]["address"]  = config.address;
        json_config["server"]["username"] = config.username;
        json_config["server"]["password"] = config.password;

        Array locations_array;
        for (const auto& location : config.locations) {
            Object location_obj;
            location_obj["name"]         = location.name;
            location_obj["type"]         = location.type;
            location_obj["wait_minutes"] = location.wait_minutes;
            locations_array.push_back(location_obj);
        }
        json_config["locations"] = locations_array;

        std::ofstream file(CONFIG_FILE);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open config file for writing");
            return false;
        }

        file << json_config.Dump(2, ' ');
        file.close();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving config: " << e.what());
        return false;
    }
}

bool LoadConfig(Config& config) {
    try {
        if (!std::filesystem::exists(CONFIG_FILE)) {
            return false;
        }

        std::ifstream file(CONFIG_FILE);
        if (!file.is_open()) {
            return false;
        }

        Value json_config;
        file >> json_config;
        file.close();

        if (!json_config.is<Object>() || json_config.get<Object>().find("server") == json_config.get<Object>().end()) {
            LOG_ERROR("Invalid config file: missing 'server' section");
            return false;
        }

        const auto& server = json_config["server"].get<Object>();
        if (server.find("address") == server.end() || server.find("username") == server.end() || server.find("password") == server.end()) {
            LOG_ERROR("Invalid config file: missing required server fields");
            return false;
        }

        config.address  = server.at("address").get_string();
        config.username = server.at("username").get_string();
        config.password = server.at("password").get_string();

        config.locations.clear();
        const auto& json_obj = json_config.get<Object>();

        // Try new "locations" key first
        if (json_obj.find("locations") != json_obj.end() && json_config["locations"].is<Array>()) {
            const auto& locations_array = json_config["locations"].get<Array>();
            for (const auto& location_val : locations_array) {
                if (location_val.is<Object>()) {
                    const auto& location_obj = location_val.get<Object>();
                    if (location_obj.find("name") != location_obj.end() && location_obj.find("wait_minutes") != location_obj.end()) {
                        LocationConfig location;
                        location.name         = location_obj.at("name").get_string();
                        location.wait_minutes = location_obj.at("wait_minutes").get_number<int>();

                        // Get type, default to "warp" for backward compatibility
                        if (location_obj.find("type") != location_obj.end() && location_obj.at("type").is_string()) {
                            location.type = location_obj.at("type").get_string();
                        } else {
                            // Check for old location_type at root level for backward
                            // compatibility
                            if (json_obj.find("location_type") != json_obj.end() && json_config["location_type"].is_string()) {
                                std::string old_type = json_config["location_type"].get_string();
                                location.type        = (old_type == "homes") ? "home" : "warp";
                            } else {
                                location.type = "warp"; // Default
                            }
                        }
                        config.locations.push_back(location);
                    }
                }
            }
        }
        // Backward compatibility: also check for "warps" key
        else if (json_obj.find("warps") != json_obj.end() && json_config["warps"].is<Array>()) {
            const auto& warps_array = json_config["warps"].get<Array>();
            for (const auto& warp_val : warps_array) {
                if (warp_val.is<Object>()) {
                    const auto& warp_obj = warp_val.get<Object>();
                    if (warp_obj.find("name") != warp_obj.end() && warp_obj.find("wait_minutes") != warp_obj.end()) {
                        LocationConfig location;
                        location.name         = warp_obj.at("name").get_string();
                        location.wait_minutes = warp_obj.at("wait_minutes").get_number<int>();
                        location.type         = "warp"; // Old format assumed warps
                        config.locations.push_back(location);
                    }
                }
            }
        }

        // If no locations configured, use defaults
        if (config.locations.empty()) {
            config.locations.push_back({"creeper_farm", "warp", 30});
            config.locations.push_back({"slime_farm", "warp", 30});
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading config: " << e.what());
        return false;
    }
}

int RunSetup() {
    std::cout << "\n=== MC-AFKer Setup ===\n" << std::endl;

    Config config;

    std::cout << "Enter server configuration:\n" << std::endl;

    std::string address = GetInput("Server address (e.g., mc.example.com:25565): ");
    if (address.empty()) {
        address = "127.0.0.1:25565";
        std::cout << "Using default: " << address << std::endl;
    }

    std::string username = GetInput("Username (for offline mode): ");
    if (username.empty()) {
        username = "HarroldAFKBot";
        std::cout << "Using default: " << username << std::endl;
    }

    std::string password = GetPasswordInput("Password (for /register and /login): ");
    if (password.empty()) {
        std::cerr << "Error: Password cannot be empty!" << std::endl;
        return 1;
    }

    config.address  = address;
    config.username = username;
    config.password = password;

    std::cout << "\nConfigure locations (press Enter with empty name to finish):\n" << std::endl;
    std::cout << "For each location, you can choose whether it's a 'warp' or 'home'.\n" << std::endl;
    config.locations.clear();

    int location_num = 1;
    while (true) {
        std::string location_name = GetInput("Location " + std::to_string(location_num) + " name (or press Enter to finish): ");
        if (location_name.empty()) {
            if (config.locations.empty()) {
                std::cout << "No locations configured. Using defaults: creeper_farm "
                             "(warp, 30 min), slime_farm (warp, 30 min)"
                          << std::endl;
                config.locations.push_back({"creeper_farm", "warp", 30});
                config.locations.push_back({"slime_farm", "warp", 30});
            }
            break;
        }

        // Ask for type (warp or home) for each location
        std::string location_type = GetInput("Type for " + location_name + " ('warp' or 'home', default: warp): ");
        if (location_type.empty() || (location_type != "home" && location_type != "warp")) {
            location_type = "warp";
            std::cout << "Using default: warp" << std::endl;
        }

        std::string wait_str     = GetInput("Wait time in minutes for " + location_name + " (default 30): ");
        int         wait_minutes = 30;
        if (!wait_str.empty()) {
            try {
                wait_minutes = std::stoi(wait_str);
                if (wait_minutes < 1) {
                    wait_minutes = 30;
                    std::cout << "Invalid wait time, using default: 30 minutes" << std::endl;
                }
            } catch (...) { std::cout << "Invalid wait time, using default: 30 minutes" << std::endl; }
        }

        config.locations.push_back({location_name, location_type, wait_minutes});
        std::cout << "Added " << location_type << ": " << location_name << " (wait " << wait_minutes << " minutes)" << std::endl;
        location_num++;
    }

    if (SaveConfig(config)) {
        std::cout << "\nConfiguration saved to " << CONFIG_FILE << std::endl;
        std::cout << "You can now run the bot without arguments!" << std::endl;
        return 0;
    } else {
        std::cerr << "\nFailed to save configuration!" << std::endl;
        return 1;
    }
}

void ShowHelp(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [options]\n"
              << "Options:\n"
              << "\t-h, --help\tShow this help message\n"
              << "\t--setup\t\tRun setup wizard to configure the bot\n"
              << "\t--first-time\tForce registration (even if already registered)\n"
              << "\n"
              << "If no options are provided and config.json exists, the bot will use\n"
              << "the saved configuration. Otherwise, run with --setup first.\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        // Init logging, log everything >= Info, only to console, no file
        Botcraft::Logger::GetInstance().SetLogLevel(Botcraft::LogLevel::Info);
        Botcraft::Logger::GetInstance().SetFilename("");
        Botcraft::Logger::GetInstance().RegisterThread("main");

        // Check for setup mode
        bool setup_mode = false;
        bool first_time = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                ShowHelp(argv[0]);
                return 0;
            } else if (arg == "--setup") {
                setup_mode = true;
            } else if (arg == "--first-time") {
                first_time = true;
            }
        }

        if (setup_mode) {
            return RunSetup();
        }

        // Load configuration
        Config config;
        if (!LoadConfig(config)) {
            std::cerr << "\nNo configuration found!" << std::endl;
            std::cerr << "Please run: " << argv[0] << " --setup" << std::endl;
            return 1;
        }

        LOG_INFO("Loaded configuration from " << CONFIG_FILE);
        LOG_INFO("Server: " << config.address);
        LOG_INFO("Username: " << config.username);
        LOG_INFO("Locations configured: " << config.locations.size());

        // Set up signal handlers
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        std::set_terminate(TerminateHandler);

        Botcraft::ConnectionClient client;
        g_client = &client;

        LOG_INFO("Starting connection process in OFFLINE mode");
        LOG_INFO("Connecting as: " << config.username);
        LOG_INFO("Server address: " << config.address);
        client.Connect(config.address, config.username);

        // Wait for connection to be fully established and in Play state
        LOG_INFO("Waiting for connection to be fully established...");
        auto      network_manager   = client.GetNetworkManager();
        int       wait_attempts     = 0;
        const int max_wait_attempts = 30;

        while (network_manager && network_manager->GetConnectionState() != ProtocolCraft::ConnectionState::Play && !client.GetShouldBeClosed() &&
               wait_attempts < max_wait_attempts) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            wait_attempts++;
        }

        if (client.GetShouldBeClosed()) {
            LOG_FATAL("Connection closed unexpectedly during initial wait");
            return 1;
        }

        if (wait_attempts >= max_wait_attempts) {
            LOG_WARNING("Connection did not reach Play state after 30 seconds, "
                        "proceeding anyway...");
        } else {
            LOG_INFO("Connection established, now in Play state");
        }

        // Additional short wait to let any remaining initial packets process
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Determine if we should register or login
        std::string registration_file = "registered_" + config.username + ".txt";
        bool        should_register   = first_time || !std::filesystem::exists(registration_file);

        if (should_register) {
            LOG_INFO("Registering for the first time...");
            client.SendChatCommand("register " + config.password + " " + config.password);
            std::ofstream file(registration_file);
            file << "registered";
            file.close();
            LOG_INFO("Registration command sent. Waiting 2 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } else {
            LOG_INFO("Logging in...");
            client.SendChatCommand("login " + config.password);
            LOG_INFO("Login command sent. Waiting 2 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        std::string cs_msg = "cc";
        client.SendChatMessage(cs_msg);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Main location loop using configuration
        LOG_INFO("Starting location loop with " << config.locations.size() << " locations...");
        LOG_INFO("Press Enter during wait cycles to skip to the next location");
        size_t current_location_index = 0;

        while (!client.GetShouldBeClosed()) {
            try {
                if (config.locations.empty()) {
                    LOG_ERROR("No locations configured!");
                    break;
                }

                const auto& location       = config.locations[current_location_index];
                std::string command_prefix = (location.type == "home") ? "home" : "warp";
                LOG_INFO("Going to " << location.type << " " << location.name << "...");

                // Send chat message indicating where we're teleporting
                std::string teleport_msg = "Teleporting to " + location.name + " for " + std::to_string(location.wait_minutes) + "min.";

                client.SendChatMessage(teleport_msg);

                // Small delay before sending the command
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                client.SendChatCommand(command_prefix + " " + location.name);

                // Reset skip flag
                g_skip_wait = false;

                LOG_INFO("Waiting " << location.wait_minutes << " minutes at " << location.name << " (press Enter to skip)...");

                // Start a thread to monitor stdin for skip input
                std::thread input_thread([&]() {
                    std::string line;
                    std::getline(std::cin, line);
                    g_skip_wait = true;
                    LOG_INFO("Skip requested! Moving to next location...");
                });
                input_thread.detach();

                // Sleep in smaller chunks to check connection status and skip flag
                bool skipped = false;
                for (int i = 0; i < location.wait_minutes * 60 && !client.GetShouldBeClosed(); ++i) {
                    if (g_skip_wait) {
                        skipped = true;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (skipped) {
                    LOG_INFO("Wait cycle skipped by user");
                }

                if (client.GetShouldBeClosed()) {
                    LOG_WARNING("Connection closed, exiting loop");
                    break;
                }

                // Move to next location (cycle)
                current_location_index = (current_location_index + 1) % config.locations.size();
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in location loop: " << e.what());
                LOG_INFO("Waiting 5 seconds before retrying...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

        g_client = nullptr;
        client.Disconnect();

        return 0;
    } catch (std::exception& e) {
        LOG_FATAL("Exception: " << e.what());
        return 1;
    } catch (...) {
        LOG_FATAL("Unknown exception");
        return 2;
    }
}
