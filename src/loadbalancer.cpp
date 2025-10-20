#include "loadbalancer.h"
#include "RouteStrategy.h"
#include "Server.h"

#include<iostream>
#include<vector>
#include<string>
#include<memory>


LoadBalancer &LoadBalancer::getInstance() {
    static LoadBalancer lb;
    return lb;
}
void LoadBalancer::addServer(std::string url) {
    serverList.push_back(std::make_unique<Server>(url));
}

void LoadBalancer::removeServer(std::string url) {
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


