#pragma once

#include "RouteStrategy.h"
#include "Server.h"
#include<vector>
#include<string>


class LoadBalancer {
private:
  std::vector<std::unique_ptr<Server>> serverList;
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
  std::vector<std::unique_ptr<Server>> getServerList();
  void setStrategy(std::unique_ptr<IRouteStrategy> strategy);
  Server &getServer();
  size_t getHealthyCount() const;
};

