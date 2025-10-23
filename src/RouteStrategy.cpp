
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "RouteStrategy.h"
#include "Server.h"

Server &RoundRobin::selectServer(
    const std::vector<std::shared_ptr<Server>> &serverList) {
  // lock is automatically removed when function ends
  std::lock_guard<std::mutex> lock(mtx);

  if (serverList.empty()) {
    throw std::runtime_error("No available servers to route the request.");
  }

  if (serverCount >= serverList.size()) {
    serverCount = 0;
  }

  Server *selectedServer = serverList[serverCount].get();

  serverCount = (serverCount + 1) % serverList.size();

  return *selectedServer;
}

Server &LeastConnection::selectServer(
    const std::vector<std::shared_ptr<Server>> &serverList) {

  std::lock_guard<std::mutex> lock(mtx);

  if (serverList.empty()) {
    throw std::runtime_error("No available servers to route the request.");
  }

  Server *selectedServer = serverList[0].get();
  int minConnections = selectedServer->getConnections();
  for (const auto &server : serverList) {
    int connectionCount = server->getConnections();
    if (connectionCount < minConnections) {
      minConnections = connectionCount;
      selectedServer = server.get();
    }
  }
  selectedServer->incrementActiveConnection();
  return *selectedServer;
}

std::unique_ptr<IRouteStrategy>
StrategyFactory::getStrategy(const std::string &strategy) {
  if (strategy == "RoundRobin") {
    return std::make_unique<RoundRobin>();
  } else if (strategy == "LeastConnection") {
    return std::make_unique<LeastConnection>();
  } else {
    throw std::invalid_argument("Unknown Strategy: " + strategy);
  }
}
