#include <iostream>

#include <atomic>
#include <memory>
#include <string_view>
#include <unordered_map>
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

  std::vector<std::pair<std::string, int>> parsedServers;

  for (const std::string &serverInfo : config.serverList) {
    std::string_view view = serverInfo;

    size_t schemePos = view.find("://");
    if (schemePos == std::string_view::npos) {
      std::cerr << "Invalid URL format (missing ://): " << serverInfo << '\n';
      exit(1);
    }

    view.remove_prefix(schemePos + 3);

    size_t colonPos = view.find(':');
    if (colonPos == std::string_view::npos) {
      std::cerr << "Invalid URL format (missing port): " << serverInfo << '\n';
      continue;
    }

    std::string serverHost(view.substr(0, colonPos));
    std::string_view portView = view.substr(colonPos + 1);

    int serverPort = std::stoi(std::string(portView));

    parsedServers.push_back({serverHost, serverPort});

    std::cout << "Registered Backend -> Host: " << serverHost
              << " | Port: " << serverPort << '\n';
  }

  // 4. ▶️ Start the server
  std::cout << "Load Balancer server starting on port " << config.port
            << std::endl;

  auto routing_function = [&lb]() -> int { return lb.getServer(); };

  IoUringEngine engine(routing_function, parsedServers, config.port,
                       config.queue_depth);

  engine.run();

  return 0;
}
