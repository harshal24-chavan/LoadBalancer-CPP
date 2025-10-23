#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "RouteStrategy.h"
#include "Server.h"
#include "loadbalancer.h"
#include "tomlParser.h"

LoadBalancer &LoadBalancer::getInstance() {
  static LoadBalancer lb;
  return lb;
}
void LoadBalancer::addServer(std::string url) {
  std::lock_guard<std::mutex> lock(serverListMutex);
  serverList.push_back(std::make_unique<Server>(url));
  activeServerList.push_back(std::make_unique<Server>(url));
}

void LoadBalancer::removeServer(std::string url) {
  std::lock_guard<std::mutex> lock(serverListMutex);
  std::erase_if(serverList, [&](const std::unique_ptr<Server> &server) {
    return server->getUrl() == url;
  });
  std::erase_if(activeServerList, [&](const std::unique_ptr<Server> &server) {
    return server->getUrl() == url;
  });
}
void LoadBalancer::addActiveServer(std::string url) {
  std::lock_guard<std::mutex> lock(activeServerListMutex);
  activeServerList.push_back(std::make_unique<Server>(url));
}

void LoadBalancer::removeActiveServer(std::string url) {
  std::lock_guard<std::mutex> lock(activeServerListMutex);
  std::erase_if(activeServerList, [&](const std::unique_ptr<Server> &server) {
    return server->getUrl() == url;
  });
}

void LoadBalancer::listServers() {
  std::cout << "------------Registered Servers------------" << std::endl;

  for (const auto &server : serverList) {
    std::cout << server->getUrl() << std::endl;
  }

  std::cout << "------------------------------------------" << std::endl;
  std::cout << "------------Active Servers------------" << std::endl;

  for (const auto &server : activeServerList) {
    std::cout << server->getUrl() << std::endl;
  }

  std::cout << "------------------------------------------" << std::endl;
}

void LoadBalancer::setStrategy(std::unique_ptr<IRouteStrategy> strategy) {
  routeStrategy = std::move(strategy);
}

Server &LoadBalancer::getServer() {
  // select servers from the active list
  Server &selectedServer = routeStrategy->selectServer(activeServerList);

  // make http request using crp:
  return selectedServer;
}

int LoadBalancer::getHealthyCount() const { return activeServerList.size(); }

/**
 * this Function is an implementation part of Visitor pattern for healthChecker
 * it ccepts healthchecker visiting and checks health of a server and chanes the
 * activeStateList it modifies the active State List by removing unhealthy
 * servers the unhealthy servers are still there in the original ServerList
 * array so that if they come back online again we can check and add them to
 * active server list
 */
void LoadBalancer::accept(std::function<void(Server &)> visitorFunc) {
  std::lock_guard<std::mutex> lock(serverListMutex);
  for (const auto &server : serverList) {
    visitorFunc(*server);
  }
}

/**
 * @brief
 * this Function is am implementation of Oberser patter for hot reloader
 * it is notified when there are any changes to the config.toml file and updates
 * accordingly
 */
void LoadBalancer::updateConfig(AppConfig config) {
  std::lock_guard<std::mutex> lock(loadBalancerConfigMutex);
  std::set<std::string> newServerList;

  for (const auto &it : config.serverList) {
    newServerList.insert(it);
  }

  for (const auto &server : serverList) {
    if (!newServerList.count(server.getUrl())) {
      removeServer(server.getUrl());
    }
  }

  for (const auto &it : newServerList) {
    // adding any new server to the list
    addServer(it);
  }

  setStrategy(StrategyFactory::getStrategy(config.strategy));
}
