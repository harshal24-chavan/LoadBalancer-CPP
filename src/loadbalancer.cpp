#include<iostream>
#include<functional>
#include<vector>
#include<string>
#include<memory>
#include<mutex>

#include "loadbalancer.h"
#include "RouteStrategy.h"
#include "Server.h"



LoadBalancer &LoadBalancer::getInstance() {
  static LoadBalancer lb;
  return lb;
}
void LoadBalancer::addServer(std::string url) {
  std::lock_guard<std::mutex> lock(serverListMutex);
  serverList.push_back(std::make_unique<Server>(url));
}

void LoadBalancer::removeServer(std::string url) {
  std::lock_guard<std::mutex> lock(serverListMutex);
  std::erase_if(serverList, [&](const std::unique_ptr<Server> &server) {
    return server->getUrl() == url;
  });
}
void LoadBalancer::listServers() {
  std::cout << "------------Registered Servers------------" << std::endl;

  for (const auto &server : serverList) {
    std::cout << server->getUrl() << std::endl;
  }

  std::cout << "------------------------------------------" << std::endl;
}


void LoadBalancer::setStrategy(std::unique_ptr<IRouteStrategy> strategy) {
  routeStrategy = std::move(strategy);
}

Server &LoadBalancer::getServer() {
  Server &selectedServer = routeStrategy->selectServer(serverList);

  // make http request using crp:
  return selectedServer;
}

size_t LoadBalancer::getHealthyCount() const {
  size_t count = 0;
  for(auto& server: serverList){
    count += server->checkHealth() == true;
  }
  return count;
}

void LoadBalancer::accept(std::function<void(Server&)> visitorFunc){
  std::lock_guard<std::mutex> lock(serverListMutex);
  for(const auto& server: serverList){
    visitorFunc(*server);
  }
}
