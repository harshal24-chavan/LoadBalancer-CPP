#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <toml++/toml.hpp>
#include "tomlParser.h"

AppConfig parseTomlFile(const std::string& filename) {
  AppConfig config; // Create a config object with default values

  toml::table tbl;
  try {
    tbl = toml::parse_file(filename);

    // --- Safely get the LoadBalancer settings ---
    // Use .value_or() for safety. It returns the default if the key isn't found.
    config.host = tbl["LoadBalancer"]["host"].value_or("0.0.0.0");
    config.port = tbl["LoadBalancer"]["port"].value_or(18080);
    config.strategy = tbl["LoadBalancer"]["strategy"].value_or("RoundRobin");

    // --- Safely parse the 'array of tables' for Servers ---
    // Note the capital 'S' in "Servers"
    if (toml::array* server_array = tbl["Servers"].as_array()) {

      server_array->for_each([&](auto&& el) {
        if (toml::table* server_table = el.as_table()) {

          // Safely get the "url" string from within the table
          if (auto url_node = server_table->get("url")) {
            if (auto url_str = url_node->value<std::string>()) {
              config.serverList.push_back(*url_str);
            }
          }
        }
      });
    }
  }
  catch (const toml::parse_error& err) {
    std::cerr << "TOML parsing failed:\n" << err << "\n";
    std::cerr << "Using default configuration.\n";
  }
  catch (const std::exception& e) {
    std::cerr << "An error occurred during config parsing: " << e.what() << std::endl;
    std::cerr << "Using default configuration.\n";
  }

  return config;
}
