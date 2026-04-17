#pragma once

#include "ConnectionPool.hpp"
#include <functional>
#include <memory>

class IoUringEngine {
public:
  IoUringEngine(int port, int queue_depth = 1024,
                std::function<int()> routing_callback);
  ~IoUringEngine();

  // Delete copy constructors - engines are unique resources
  IoUringEngine(const IoUringEngine &) = delete;
  IoUringEngine &operator=(const IoUringEngine &) = delete;

  void run();

private:
  std::unique_ptr<ConnectionPool> pool;
  struct Impl; // Forward declaration of the implementation
  std::unique_ptr<Impl> pimpl;
  std::function<int()> get_next_server_callback;
};
