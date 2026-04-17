#include <iostream>

#include <atomic>
#include <memory>
#include <utility>

#include <string>
#include <vector>

#include "HotReloader.h"
#include "IoUringEngine.hpp"
#include "MetricsCalc.h"
#include "RouteStrategy.h"
#include "Server.h"
#include "healthChecker.h"
#include "loadbalancer.h"
#include "tomlParser.h"

int main() {
  AppConfig config = parseTomlFile("config.toml");
  MetricsCalc &metrics = MetricsCalc::getInstance();

  LoadBalancer &lb = LoadBalancer::getInstance();
  // // add servers to load balancer
  // for (std::string &url : config.serverList) {
  //   lb.addServer(url);
  // }
  lb.updateConfig(config);
  lb.listServers();

  // RequestForwarder forwarder(lb, metricsCalc);

  // std::unique_ptr<IRequestHandler> proxyHandler =
  //     std::make_unique<ProxyHandler>(lb);
  //
  // proxyHandler =
  //     std::move(std::make_unique<MetricsDecorator>(std::move(proxyHandler)));
  // proxyHandler =
  //     std::move(std::make_unique<LoggingDecorator>(std::move(proxyHandler)));
  //

  // 4. ▶️ Start the server
  std::cout << "Load Balancer server starting on port " << config.port
            << std::endl;

  auto routing_function = []() -> int { return 0; };

  IoUringEngine engine(routing_function, 8080, 8192);

  engine.run();

  return 0;
}
