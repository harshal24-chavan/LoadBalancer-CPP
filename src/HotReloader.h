#pragma once

#include <memory> // For std::unique_ptr

// Forward declarations to avoid including heavy headers
class LoadBalancer;
namespace efsw {
class FileWatcher;
}
class UpdateListener; // This is an "incomplete type"

/**
 * @brief Manages the file watcher and triggers the load balancer to reload.
 */
class HotReloader {
private:
  LoadBalancer &loadbalancer;

  // std::unique_ptr can hold an incomplete type (forward-declared)
  // as long as the destructor is defined in the .cpp file.
  std::unique_ptr<efsw::FileWatcher> fileWatcher;
  std::unique_ptr<UpdateListener> listener;

public:
  /**
   * @brief Constructor: Takes the LoadBalancer dependency.
   */
  HotReloader(LoadBalancer &lb);

  /**
   * @brief Destructor: Required for std::unique_ptr with incomplete types.
   * The compiler needs this to find the full definition of the
   * classes when destroying the unique_ptrs.
   */
  ~HotReloader();

  /**
   * @brief Starts the file watching process (non-blocking).
   */
  void start();

  /**
   * @brief Public callback method to be triggered by the listener.
   */
  void onConfigChanged();
};
