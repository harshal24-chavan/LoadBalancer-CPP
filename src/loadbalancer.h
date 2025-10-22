#pragma once

#include "RouteStrategy.h"
#include "Server.h"
#include<vector>
#include<string>
#include<mutex>


class LoadBalancer {
private:
  std::vector<std::unique_ptr<Server>> serverList;
  std::mutex serverListMutex;
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
  size_t getHealthyCount() const;

  //visitor pattern accept function
  void accept(std::function<void(Server&)> visitorFunc);
};

