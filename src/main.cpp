#include "crow.h"

#include <cpr/body.h>
#include <cpr/cpr.h>

#include <iostream>

#include <atomic>
#include <memory>
#include <utility>

#include <string>
#include <vector>

#include "HotReloader.h"
#include "RequestForwarder.h"
#include "RouteStrategy.h"
#include "Server.h"
#include "healthChecker.h"
#include "loadbalancer.h"
#include "tomlParser.h"

int main() {
  AppConfig config = parseTomlFile("config.toml");

  crow::SimpleApp app;

  LoadBalancer &lb = LoadBalancer::getInstance();
  // // add servers to load balancer
  // for (std::string &url : config.serverList) {
  //   lb.addServer(url);
  // }
  lb.updateConfig(config);
  lb.listServers();

  RequestForwarder forwarder(lb);

  HealthChecker healthChecker(lb, config);
  healthChecker.startMonitoring();

  HotReloader hotReloader(lb);
  hotReloader.start();

  CROW_ROUTE(app, "/home")
  ([&forwarder](const crow::request &req) {
    return crow::response(200, "Load Balancer");
  });
  CROW_ROUTE(app, "/health")
  ([&lb]() {
    size_t healthy = lb.getHealthyCount();
    if (healthy > 0) {
      crow::json::wvalue response;
      response["status"] = "healthy";
      response["healthy_servers"] = healthy;
      return crow::response(200, response);
    } else {
      crow::json::wvalue response;
      response["status"] = "unhealthy";
      response["healthy_servers"] = healthy;
      return crow::response(503, response);
    }
  });
  CROW_ROUTE(app, "/api")
  ([&forwarder](const crow::request &req) { return forwarder.forward(req); });

  CROW_CATCHALL_ROUTE(app)
  ([&forwarder](const crow::request &req) { return forwarder.forward(req); });

  // 4. ▶️ Start the server
  std::cout << "🔥 Load Balancer server starting on port " << config.port
            << std::endl;

  app.port(config.port).multithreaded().run();

  return 0;
}
