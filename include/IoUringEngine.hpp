#pragma once

#include "BackendPool.hpp"
#include "PipePool.hpp"
#include <functional>
#include <memory>

class IoUringEngine {
public:
  IoUringEngine(std::function<int()> routing_callback, int port,
                int queue_depth = 1024);
  ~IoUringEngine();

  // Delete copy constructors - engines are unique resources
  IoUringEngine(const IoUringEngine &) = delete;
  IoUringEngine &operator=(const IoUringEngine &) = delete;

  void run();

private:
  std::unique_ptr<BackendPool> pool;
  std::unique_ptr<PipePool> pipePool;
  struct Impl; // Forward declaration of the implementation
  std::unique_ptr<Impl> pimpl;
  std::function<int()> get_next_server_callback;
};
