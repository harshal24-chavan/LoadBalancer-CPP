#pragma once

#include "BackendPool.hpp"
#include "tomlParser.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Server;

// The abstract interface (the "contract")
class IRouteStrategy {

public:
  virtual ~IRouteStrategy() = default;
  virtual int selectServer() = 0;
};

class RoundRobin : public IRouteStrategy {
private:
  std::atomic<uint64_t> serverCount{0};
  std::atomic<uint64_t> currentServer{0};

public:
  RoundRobin(int sc) : serverCount(sc) {}

  int selectServer() override;
};

class LeastConnection : public IRouteStrategy {
private:
  std::shared_ptr<BackendPool> pool;

public:
  LeastConnection() = default;
  mutable std::mutex mtx;

  int selectServer() override;
};

class StrategyFactory {
public:
  static std::unique_ptr<IRouteStrategy> getStrategy(AppConfig &config);
};
