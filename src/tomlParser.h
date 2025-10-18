// tomlParser.h

#pragma once // Prevents this file from being included multiple times

#include <string>
#include <vector>

/**
 * @brief Holds the application's configuration, loaded from the TOML file.
 *
 * Provides default values in case parsing fails or keys are missing.
 */
struct AppConfig {
    std::string host = "0.0.0.0"; // Default host
    std::string strategy = "RoundRobin"; // Default strategy
    int port = 18080;             // Default port
    std::vector<std::string> serverList;
};

/**
 * @brief Parses a TOML configuration file.
 *
 * @param filename The path to the .toml file.
 * @return An AppConfig struct populated with values from the file.
 * If parsing fails, it returns an AppConfig with default values.
 */
AppConfig parseTomlFile(const std::string& filename);
