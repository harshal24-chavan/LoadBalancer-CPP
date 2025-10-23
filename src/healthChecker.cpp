#include <chrono>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "healthChecker.h"
#include "loadbalancer.h"
#include "tomlParser.h"

void HealthChecker::monitoringLoop() {
  while (true) {
    std::cout << "[HealthCheck] Running health checks..." << std::endl;

    std::vector<std::shared_ptr<Server>> serverList =
        loadbalancer.getAllServers();

    for (const auto &server : serverList) {
      auto timeout = cpr::Timeout{5000}; // 5 seconds timeout
      std::string url = server->getUrl();
      cpr::Response cprRes = cpr::Get(cpr::Url{url}, timeout);

      if (cprRes.status_code >= 200 && cprRes.status_code < 400) {
        server->setHealthy(true);
      } else {
        server->setHealthy(false);
      }
    }
    loadbalancer.rebuildActiveServer();

    std::this_thread::sleep_for(
        std::chrono::seconds(config.healthCheckInterval));
  }
}

HealthChecker::HealthChecker(LoadBalancer &lb, AppConfig conf)
    : loadbalancer(lb), config(conf) {}

void HealthChecker::startMonitoring() {

  std::thread([this]() { this->monitoringLoop(); }).detach();
}
