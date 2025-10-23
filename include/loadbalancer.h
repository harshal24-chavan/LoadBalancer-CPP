#pragma once

#include "RouteStrategy.h"
#include "Server.h"
#include "tomlParser.h"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class LoadBalancer {
private:
  std::vector<std::shared_ptr<Server>> serverList;
  std::vector<std::shared_ptr<Server>> activeServerList;

  std::mutex activeServerListMutex;
  std::mutex serverListMutex;
  std::mutex loadBalancerConfigMutex;

  std::unique_ptr<IRouteStrategy> routeStrategy;

  LoadBalancer() = default;

  // deleting the copy constructor
  LoadBalancer(const LoadBalancer &) = delete;
  LoadBalancer &operator=(const LoadBalancer &) = delete;

public:
  static LoadBalancer &getInstance();
  void addServer(std::string url);
  void removeServer(std::string url);

  void listServers();
  void setStrategy(std::unique_ptr<IRouteStrategy> strategy);

  Server &getServer();
  std::vector<std::shared_ptr<Server>> getAllServers();
  int getHealthyCount() const;

  void rebuildActiveServer();

  // visitor pattern accept function
  void accept(std::function<void(Server &)> visitorFunc);
  // observer pattern with hotrealoader.cpp where loadbalancer is subscribed to
  // any modifications to config file
  void updateConfig(AppConfig config);
};
