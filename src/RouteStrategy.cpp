
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "BackendPool.hpp"
#include "RouteStrategy.h"
#include "Server.h"

int RoundRobin::selectServer() {
  return currentServer.fetch_add(1, std::memory_order_relaxed) % serverCount;
}

int LeastConnection::selectServer() {

  return 0;

  // std::lock_guard<std::mutex> lock(mtx);
  //
  // if (serverList.empty()) {
  //   throw std::runtime_error("No available servers to route the request.");
  // }
  //
  // Server *selectedServer = serverList[0].get();
  // int minConnections = selectedServer->getConnections();
  // for (const auto &server : serverList) {
  //   int connectionCount = server->getConnections();
  //   if (connectionCount < minConnections) {
  //     minConnections = connectionCount;
  //     selectedServer = server.get();
  //   }
  // }
  // selectedServer->incrementActiveConnection();
  // // return *selectedServer;
  // return 0;
}

std::unique_ptr<IRouteStrategy>
StrategyFactory::getStrategy(AppConfig &config) {

  const std::string &strategy = config.strategy;

  if (strategy == "RoundRobin") {
    uint64_t serverListSize = config.serverList.size();
    return std::make_unique<RoundRobin>(serverListSize);
  } else if (strategy == "LeastConnection") {
    return std::make_unique<LeastConnection>();
  } else {
    throw std::invalid_argument("Unknown Strategy: " + strategy);
  }
}
