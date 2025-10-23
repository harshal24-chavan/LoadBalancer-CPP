#include "HotReloader.h"

// Include full definitions here, in the .cpp file
#include "loadbalancer.h"
#include "tomlParser.h"
#include <efsw/efsw.hpp>
#include <iostream>
#include <utility> // For std::move

// --- Define the Listener Class ---
/**
 * @brief Internal listener class, hidden from the rest of the program.
 * This is an implementation detail of the HotReloader.
 */
class UpdateListener : public efsw::FileWatchListener {
private:
  HotReloader &reloader; // Reference back to the owner

public:
  UpdateListener(HotReloader &hr) : reloader(hr) {}

  /**
   * @brief This is the callback function that efsw will execute.
   */
  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        std::string oldFilename) override {

    // We only care if "config.toml" is modified
    if (filename == "config.toml" && action == efsw::Actions::Modified) {
      reloader.onConfigChanged();
    }
  }
};

// --- HotReloader Implementation ---

HotReloader::HotReloader(LoadBalancer &lb)
    : loadbalancer(lb), fileWatcher(std::make_unique<efsw::FileWatcher>()),
      listener(std::make_unique<UpdateListener>(
          *this)) // Pass 'this' to the listener
{
  // Constructor body is empty
}

// The destructor must be defined in the .cpp file,
// where UpdateListener and FileWatcher are complete types.
HotReloader::~HotReloader() = default;

void HotReloader::start() {
  // Watch the current directory "."
  efsw::WatchID configWatchId = fileWatcher->addWatch(
      ".", listener.get(), false); // false = not recursive

  // This call starts the monitoring in the background.
  fileWatcher->watch();
  std::cout << "[HotReloader] Watching for changes to config.toml..."
            << std::endl;
}

void HotReloader::onConfigChanged() {
  std::cout << "[HotReloader] config.toml modified. Reloading configuration..."
            << std::endl;
  try {
    // 1. Parse the new configuration
    AppConfig config = parseTomlFile("config.toml");

    // 2. Safely update the load balancer
    loadbalancer.updateConfig(config); // Assumes LoadBalancer has this method

  } catch (const std::exception &e) {
    std::cerr << "[HotReloader] Error during reload: " << e.what() << std::endl;
  }
}
