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
#include "MetricsCalc.h"
#include "RequestHandler.h"
#include "ResponseDecorator.h"
#include "RouteStrategy.h"
#include "Server.h"
#include "healthChecker.h"
#include "loadbalancer.h"
#include "tomlParser.h"

int main() {
  AppConfig config = parseTomlFile("config.toml");
  MetricsCalc &metrics = MetricsCalc::getInstance();

  crow::SimpleApp app;

  LoadBalancer &lb = LoadBalancer::getInstance();
  // // add servers to load balancer
  // for (std::string &url : config.serverList) {
  //   lb.addServer(url);
  // }
  lb.updateConfig(config);
  lb.listServers();

  // RequestForwarder forwarder(lb, metricsCalc);

  std::unique_ptr<IRequestHandler> proxyHandler =
      std::make_unique<ProxyHandler>(lb);

  proxyHandler =
      std::move(std::make_unique<MetricsDecorator>(std::move(proxyHandler)));
  proxyHandler =
      std::move(std::make_unique<LoggingDecorator>(std::move(proxyHandler)));

  HealthChecker healthChecker(lb, config);
  healthChecker.startMonitoring();

  HotReloader hotReloader(lb);
  hotReloader.start();

  CROW_ROUTE(app, "/metrics")([&metrics]() {
    crow::json::wvalue response;
    response["activeConnections"] = metrics.getActiveConnections();
    response["totalRequests"] = metrics.getTotalRequests();
    response["totalResponses"] = metrics.getTotalResponses();
    response["totalResponseTimeMS"] = metrics.getTotalResponseTimeMS();
    response["AvgResponseTime"] = metrics.getAvgResponseTime();
    return crow::response(200, response);
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
  ([&proxyHandler](const crow::request &req) {
    return proxyHandler->handle(req);
  });

  CROW_CATCHALL_ROUTE(app)
  ([&proxyHandler](const crow::request &req) {
    return proxyHandler->handle(req);
  });

  // 4. ▶️ Start the server
  std::cout << "🔥 Load Balancer server starting on port " << config.port
            << std::endl;

  app.port(config.port).multithreaded().run();

  return 0;
}
