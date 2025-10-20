#include "crow.h"

#include <cpr/cpr.h>
#include <cpr/body.h>

#include <iostream>

#include <atomic>
#include <memory>
#include <utility>

#include <string>
#include <vector>

#include "loadbalancer.h"
#include "RouteStrategy.h"
#include "Server.h"
#include "tomlParser.h"

int main() {
    AppConfig config = parseTomlFile("config.toml");

    crow::SimpleApp app;

    LoadBalancer &lb = LoadBalancer::getInstance();
    // add servers to load balancer
    for(std::string& url: config.serverList){
        lb.addServer(url);
    }
    lb.listServers();

    lb.setStrategy(StrategyFactory::getStrategy(config.strategy));


    // 4. ▶️ Start the server
    std::cout << "🔥 Load Balancer server starting on port " << config.port << std::endl;

    app.port(config.port)
        .multithreaded()
        .run();

    return 0;
}
